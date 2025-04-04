#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PROXY_PORT 8080
#define NUM_CLIENTS 10

void *simulate_client(void *arg) {
    int client_no = *((int *)arg);
    free(arg);
    
    int sockfd;
    struct sockaddr_in proxy_addr;
    char buffer[4096];
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Client socket error");
        pthread_exit(NULL);
    }
    
    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(PROXY_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &proxy_addr.sin_addr) <= 0) {
        perror("Client inet_pton error");
        close(sockfd);
        pthread_exit(NULL);
    }
    
    if (connect(sockfd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
        perror("Client connect error");
        close(sockfd);
        pthread_exit(NULL);
    }
    
    // Send a simple message to the proxy
    char message[100];
    sprintf(message, "Hello from client %d", client_no);
    write(sockfd, message, strlen(message));
    printf("[Client %d] Sent: %s\n", client_no, message);
    
    // Read response from proxy
    ssize_t n = read(sockfd, buffer, sizeof(buffer)-1);
    if (n > 0) {
        buffer[n] = '\0';
        printf("[Client %d] Received: %s\n", client_no, buffer);
    } else {
        printf("[Client %d] No response received.\n", client_no);
    }
    
    close(sockfd);
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_CLIENTS];
    for (int i = 0; i < NUM_CLIENTS; i++) {
        int *client_no = malloc(sizeof(int));
        *client_no = i;
        pthread_create(&threads[i], NULL, simulate_client, client_no);
        // delay between requests
        usleep(100000); // 0.1 seconds
    }
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}
