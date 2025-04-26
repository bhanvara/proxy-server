#include "backend_servers.h"

// Define the backend pool and number of backends
Backend backend_pool[MAX_SERVERS] = {
    {"127.0.0.1", 9090},
    {"127.0.0.1", 9091},
    {"127.0.0.1", 9092},
    {"127.0.0.1", 9093},
    {"127.0.0.1", 9094},
    {"127.0.0.1", 9095},
    {"127.0.0.1", 9096},
    {"127.0.0.1", 9097},
    {"127.0.0.1", 9098},
    {"127.0.0.1", 9099}
};

int backend_count = 10;
