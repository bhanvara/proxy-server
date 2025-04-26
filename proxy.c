#include "proxy.h"
#include "backend_servers.h"
#include "config.h"
#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_SERVERS 10
#define BUFFER_SIZE 4096
#define MAX_EVENTS 1000
#define LISTEN_PORT 8080

// Note: Backend type, backend_pool and backend_count are defined in backend_servers.h / backend_servers.c

// Global state for backend active counts
static int backend_active[MAX_SERVERS] = {0};
pthread_mutex_t backend_mutex = PTHREAD_MUTEX_INITIALIZER;

int get_least_connection_index() {
    pthread_mutex_lock(&backend_mutex);
    int minIndex = 0;
    for (int i = 1; i < backend_count; i++) {
        if (backend_active[i] < backend_active[minIndex])
            minIndex = i;
    }
    backend_active[minIndex]++; // Increment active count for chosen backend
    pthread_mutex_unlock(&backend_mutex);
    return minIndex;
}

typedef enum {
    STATE_BACKEND_CONNECT,
    STATE_WAIT_CLIENT,
    STATE_WAIT_BACKEND,
    STATE_DONE
} conn_state_t;

typedef struct {
    int client_fd;
    int backend_fd;
    int backend_index;
    conn_state_t state;
    char buffer[BUFFER_SIZE];
    ssize_t buflen;
    char req_key[BUFFER_SIZE];  // copy of the GET request (if applicable) to use as cache key
} connection_t;

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        exit(EXIT_FAILURE);
    }
}

void cleanup_connection(int epoll_fd, connection_t *conn) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->client_fd, NULL);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->backend_fd, NULL);
    close(conn->client_fd);
    close(conn->backend_fd);
    pthread_mutex_lock(&backend_mutex);
    backend_active[conn->backend_index]--;
    pthread_mutex_unlock(&backend_mutex);
    free(conn);
    printf("[Proxy] Cleaned up connection.\n");
}

void add_fd(int epoll_fd, int fd, connection_t *conn, uint32_t events) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.ptr = conn;  // store only the connection pointer
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl ADD");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int listen_fd, epoll_fd;
    struct sockaddr_in listen_addr;

    // Initialize cache before starting (cache_init defined in cache.c)
    cache_init();

    // Create listening socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(LISTEN_PORT);
    if (bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    set_nonblocking(listen_fd);
    printf("[Proxy] Listening on port %d...\n", LISTEN_PORT);

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;  // listening fd stored in 'fd' field
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        perror("epoll_ctl: listen_fd");
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }
        for (int i = 0; i < nfds; i++) {
            // Check if the event is for the listening socket
            if (events[i].data.fd == listen_fd) {
                // Accept all pending connections
                while (1) {
                    int client_fd = accept(listen_fd, NULL, NULL);
                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        perror("accept");
                        break;
                    }
                    set_nonblocking(client_fd);
                    printf("[Proxy] Accepted client FD %d\n", client_fd);
                    
                    // Choose backend using least connections load balancing
                    int index = get_least_connection_index();
                    Backend target = backend_pool[index];
                    printf("[Proxy] Selected backend %s:%d for client FD %d\n", 
                           target.ip, target.port, client_fd);
                    
                    connection_t *conn = malloc(sizeof(connection_t));
                    if (!conn) {
                        perror("malloc");
                        close(client_fd);
                        continue;
                    }
                    conn->client_fd = client_fd;
                    conn->backend_index = index;
                    conn->state = STATE_BACKEND_CONNECT;
                    conn->buflen = 0;
                    memset(conn->req_key, 0, sizeof(conn->req_key));
                    
                    // Create backend socket
                    if ((conn->backend_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        perror("backend socket");
                        close(client_fd);
                        free(conn);
                        pthread_mutex_lock(&backend_mutex);
                        backend_active[index]--;
                        pthread_mutex_unlock(&backend_mutex);
                        continue;
                    }
                    set_nonblocking(conn->backend_fd);
                    struct sockaddr_in backend_addr;
                    memset(&backend_addr, 0, sizeof(backend_addr));
                    backend_addr.sin_family = AF_INET;
                    backend_addr.sin_port = htons(target.port);
                    if (inet_pton(AF_INET, target.ip, &backend_addr.sin_addr) <= 0) {
                        perror("inet_pton");
                        close(client_fd);
                        close(conn->backend_fd);
                        free(conn);
                        pthread_mutex_lock(&backend_mutex);
                        backend_active[index]--;
                        pthread_mutex_unlock(&backend_mutex);
                        continue;
                    }
                    int ret = connect(conn->backend_fd, (struct sockaddr *)&backend_addr, sizeof(backend_addr));
                    if (ret < 0 && errno != EINPROGRESS) {
                        perror("connect to backend");
                        close(client_fd);
                        close(conn->backend_fd);
                        free(conn);
                        pthread_mutex_lock(&backend_mutex);
                        backend_active[index]--;
                        pthread_mutex_unlock(&backend_mutex);
                        continue;
                    }
                    // Register the backend FD for writability to detect connect completion.
                    add_fd(epoll_fd, conn->backend_fd, conn, EPOLLOUT);
                    // Register the client FD for reading the client request.
                    add_fd(epoll_fd, conn->client_fd, conn, EPOLLIN);
                }
            } else {
                // The event is for one of our connection fds.
                connection_t *conn = events[i].data.ptr;
                if (conn->state == STATE_BACKEND_CONNECT) {
                    if (events[i].events & EPOLLOUT) {
                        int err = 0;
                        socklen_t len = sizeof(err);
                        if (getsockopt(conn->backend_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
                            fprintf(stderr, "[Proxy] Backend connect failed: %s\n", strerror(err));
                            cleanup_connection(epoll_fd, conn);
                            continue;
                        }
                        conn->state = STATE_WAIT_CLIENT;
                        // Modify the backend registration to monitor EPOLLIN for the response.
                        struct epoll_event ev_mod;
                        memset(&ev_mod, 0, sizeof(ev_mod));
                        ev_mod.events = EPOLLIN;
                        ev_mod.data.ptr = conn;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn->backend_fd, &ev_mod) < 0) {
                            perror("epoll_ctl MOD backend_fd");
                            cleanup_connection(epoll_fd, conn);
                            continue;
                        }
                        printf("[Proxy] Connected to backend FD %d for client FD %d\n", conn->backend_fd, conn->client_fd);
                    }
                } else if (conn->state == STATE_WAIT_CLIENT) {
                    if (events[i].events & EPOLLIN) {
                        ssize_t n = read(conn->client_fd, conn->buffer, sizeof(conn->buffer) - 1);
                        if (n <= 0) {
                            if (n < 0)
                                perror("read from client");
                            cleanup_connection(epoll_fd, conn);
                            continue;
                        }
                        conn->buffer[n] = '\0';  // ensure null-termination
                        conn->buflen = n;
                        
                        // If the request is a GET, check the cache.
                        if (strncmp(conn->buffer, "GET", 3) == 0) {
                            char *cached_response = cache_lookup(conn->buffer);
                            if (cached_response) {
                                printf("[Proxy] Found cached response for client FD %d\n", conn->client_fd);
                                write(conn->client_fd, cached_response, strlen(cached_response));
                                cleanup_connection(epoll_fd, conn);
                                continue;
                            }
                            // Save GET request as cache key for later caching.
                            strncpy(conn->req_key, conn->buffer, BUFFER_SIZE);
                            conn->req_key[BUFFER_SIZE - 1] = '\0';
                        } else {
                            conn->req_key[0] = '\0';
                        }
                        
                        printf("[Proxy] Read %zd bytes from client FD %d, sending to backend FD %d\n", 
                               n, conn->client_fd, conn->backend_fd);
                        ssize_t wn = write(conn->backend_fd, conn->buffer, conn->buflen);
                        if (wn < 0) {
                            perror("write to backend");
                            cleanup_connection(epoll_fd, conn);
                            continue;
                        }
                        conn->state = STATE_WAIT_BACKEND;
                        struct epoll_event ev_mod;
                        memset(&ev_mod, 0, sizeof(ev_mod));
                        ev_mod.events = EPOLLIN;
                        ev_mod.data.ptr = conn;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn->backend_fd, &ev_mod) < 0) {
                            perror("epoll_ctl MOD for backend response");
                            cleanup_connection(epoll_fd, conn);
                            continue;
                        }
                    }
                } else if (conn->state == STATE_WAIT_BACKEND) {
                    if (events[i].events & EPOLLIN) {
                        ssize_t n = read(conn->backend_fd, conn->buffer, sizeof(conn->buffer) - 1);
                        if (n <= 0) {
                            if (n < 0)
                                perror("read from backend");
                            cleanup_connection(epoll_fd, conn);
                            continue;
                        }
                        conn->buffer[n] = '\0';
                        printf("[Proxy] Read %zd bytes from backend FD %d, sending to client FD %d\n",
                               n, conn->backend_fd, conn->client_fd);
                        ssize_t wn = write(conn->client_fd, conn->buffer, n);
                        if (wn < 0) {
                            perror("write to client");
                            cleanup_connection(epoll_fd, conn);
                            continue;
                        }
                        // If the original request was GET, cache the backend response with a TTL of 60 seconds.
                        if (conn->req_key[0] != '\0') {
                            cache_insert(conn->req_key, conn->buffer, 60);
                        }
                        conn->state = STATE_DONE;
                        cleanup_connection(epoll_fd, conn);
                    }
                }
            }
        }
    }
    close(listen_fd);
    close(epoll_fd);
    return 0;
}