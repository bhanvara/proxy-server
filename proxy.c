#include "proxy.h"
#include "backend_servers.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// backend pool with example servers and set backend_count.
Backend backend_pool[MAX_SERVERS] = {
    {"127.0.0.1", 9090},
    {"127.0.0.1", 9091}
    // ... can add up to MAX_SERVERS entries...
};
int backend_count = 2;

// Array to track active connections for each backend
static int backend_active[MAX_SERVERS] = {0};
pthread_mutex_t backend_mutex = PTHREAD_MUTEX_INITIALIZER;

/* round-robin load balancing:
int get_next_backend_index() {
    int index;
    pthread_mutex_lock(&backend_mutex);
    index = current_backend_index;
    current_backend_index = (current_backend_index + 1) % backend_count;
    pthread_mutex_unlock(&backend_mutex);
    return index;
}
*/

// Least Connections load balancing algorithm
int get_least_connection_index() {
    pthread_mutex_lock(&backend_mutex);
    int minIndex = 0;
    for (int i = 1; i < backend_count; i++) {
        if (backend_active[i] < backend_active[minIndex])
            minIndex = i;
    }
    // Increment active count for chosen backend
    backend_active[minIndex]++;
    pthread_mutex_unlock(&backend_mutex);
    return minIndex;
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);
    
    printf("[Proxy] Incoming client connection: FD %d\n", client_fd);
    
    // Select backend using least connections method
    int index = get_least_connection_index();
    Backend target = backend_pool[index];
    printf("[Proxy] Selected backend %s:%d for client FD %d (active connections: %d)\n", 
           target.ip, target.port, client_fd, backend_active[index]);
    
    // Connect to the selected backend server
    int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (backend_fd < 0) {
        perror("[Proxy] Backend socket error");
        // Decrement active count on failure
        pthread_mutex_lock(&backend_mutex);
        backend_active[index]--;
        pthread_mutex_unlock(&backend_mutex);
        close(client_fd);
        return NULL;
    }
    
    struct sockaddr_in backend_addr;
    memset(&backend_addr, 0, sizeof(backend_addr));
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(target.port);
    if (inet_pton(AF_INET, target.ip, &backend_addr.sin_addr) <= 0) {
        perror("[Proxy] inet_pton error");
        close(client_fd);
        close(backend_fd);
        pthread_mutex_lock(&backend_mutex);
        backend_active[index]--;
        pthread_mutex_unlock(&backend_mutex);
        return NULL;
    }
    
    if (connect(backend_fd, (struct sockaddr *)&backend_addr, sizeof(backend_addr)) < 0) {
        perror("[Proxy] Connection to backend failed");
        close(client_fd);
        close(backend_fd);
        pthread_mutex_lock(&backend_mutex);
        backend_active[index]--;
        pthread_mutex_unlock(&backend_mutex);
        return NULL;
    }
    
    printf("[Proxy] Connected to backend %s:%d for client FD %d\n", target.ip, target.port, client_fd);
    
    char buffer[4096];
    ssize_t n;
    
    // Read request from client and forward to backend
    n = read(client_fd, buffer, sizeof(buffer));
    if (n > 0) {
        printf("[Proxy] Relaying %zd bytes from client FD %d to backend %s:%d\n", n, client_fd, target.ip, target.port);
        write(backend_fd, buffer, n);
    } else {
        printf("[Proxy] No data from client FD %d\n", client_fd);
    }
    
    // Read response from backend and forward to client
    n = read(backend_fd, buffer, sizeof(buffer));
    if (n > 0) {
        printf("[Proxy] Forwarding %zd bytes from backend %s:%d to client FD %d\n", n, target.ip, target.port, client_fd);
        write(client_fd, buffer, n);
    } else {
        printf("[Proxy] No response from backend %s:%d\n", target.ip, target.port);
    }
    
    close(backend_fd);
    close(client_fd);
    printf("[Proxy] Closed connection for client FD %d\n", client_fd);
    
    // Decrement active connections count for chosen backend
    pthread_mutex_lock(&backend_mutex);
    backend_active[index]--;
    pthread_mutex_unlock(&backend_mutex);
    
    return NULL;
}
