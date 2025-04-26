#ifndef BACKEND_SERVERS_H
#define BACKEND_SERVERS_H

#define MAX_SERVERS 10

typedef struct {
    char ip[16];
    int port;
} Backend;

extern Backend backend_pool[MAX_SERVERS];
extern int backend_count;

#endif // BACKEND_SERVERS_H