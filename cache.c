#include "cache.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

static cache_entry_t *cache_head = NULL;

void cache_init() {
    cache_head = NULL;
}

void cache_cleanup() {
    cache_entry_t *entry = cache_head;
    while (entry) {
        cache_entry_t *tmp = entry;
        entry = entry->next;
        free(tmp);
    }
    cache_head = NULL;
}

void cache_expire() {
    time_t now = time(NULL);
    cache_entry_t **ptr = &cache_head;
    while (*ptr) {
        if ((*ptr)->expire_time != 0 && (*ptr)->expire_time <= now) {
            cache_entry_t *expired = *ptr;
            *ptr = expired->next;
            free(expired);
        } else {
            ptr = &(*ptr)->next;
        }
    }
}

char *cache_lookup(const char *key) {
    cache_expire();  // clean up expired entries
    cache_entry_t *entry = cache_head;
    while (entry) {
        if (strncmp(entry->key, key, CACHE_KEY_SIZE) == 0)
            return entry->value;
        entry = entry->next;
    }
    return NULL;
}

void cache_insert(const char *key, const char *value, int ttl_seconds) {
    cache_expire();  // expire old entries first
    cache_entry_t *entry = malloc(sizeof(cache_entry_t));
    if (!entry) return;
    strncpy(entry->key, key, CACHE_KEY_SIZE - 1);
    entry->key[CACHE_KEY_SIZE - 1] = '\0';
    strncpy(entry->value, value, CACHE_VALUE_SIZE - 1);
    entry->value[CACHE_VALUE_SIZE - 1] = '\0';
    if (ttl_seconds > 0)
        entry->expire_time = time(NULL) + ttl_seconds;
    else
        entry->expire_time = 0; // never expire
    entry->next = cache_head;
    cache_head = entry;
}