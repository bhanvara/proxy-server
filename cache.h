#ifndef CACHE_H
#define CACHE_H

#include <time.h>

#define CACHE_KEY_SIZE 256
#define CACHE_VALUE_SIZE 4096

typedef struct cache_entry {
    char key[CACHE_KEY_SIZE];         // e.g., the GET request line/URI
    char value[CACHE_VALUE_SIZE];     // the backend response
    time_t expire_time;               // absolute expiration time (0 for never)
    struct cache_entry *next;
} cache_entry_t;

/* Initialize the cache (call once at startup) */
void cache_init();

/* Free all cache entries */
void cache_cleanup();

/* Look up a cached value by key (returns NULL if not found or expired) */
char *cache_lookup(const char *key);

/* Insert a cache entry with TTL in seconds (ttl <= 0 for never expire) */
void cache_insert(const char *key, const char *value, int ttl_seconds);

/* Remove expired entries (optional, can be called periodically) */
void cache_expire();

#endif // CACHE_H