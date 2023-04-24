#ifndef __CACHE_WR_H__
#define __CACHE_WR_H__

#include "cache.h"

typedef struct {
  cache_t *cache;
  sem_t mutex;
  sem_t w;
  int readcnt;
} cache_wr_t;

cache_wr_t *init_cache_wr(void);
void free_cache_wr(cache_wr_t *cache);
int get_object_wr(char *uri, cache_wr_t *cache, char *objectp, int *size);
void put_object_wr(char *uri, char *object, int size, cache_wr_t *cache);

#endif