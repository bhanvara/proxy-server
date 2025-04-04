// backend_servers.h
#define MAX_SERVERS 10

typedef struct {
    char *ip;
    int port;
} Backend;

extern Backend backend_pool[MAX_SERVERS];
extern int backend_count;

