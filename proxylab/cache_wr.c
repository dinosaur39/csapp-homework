#include "cache_wr.h"

cache_wr_t *init_cache_wr(void)
{
  cache_wr_t *cache_wr =(cache_wr_t *) Malloc(sizeof(cache_wr_t));
  cache_wr->cache = init_cache();
  Sem_init(&cache_wr->mutex, 0, 1);
  Sem_init(&cache_wr->w, 0, 1);
  cache_wr->readcnt = 0;
  return cache_wr;
}

void free_cache_wr(cache_wr_t *cache_wr)
{
  Free(cache_wr->cache);
  Free(cache_wr);
}

int get_object_wr(char *uri, cache_wr_t *cache_wr, char* objp, int *size)
{
  sem_t *mutexp = &cache_wr->mutex, *wp = &cache_wr->w;
  int has_proxy;

  P(mutexp);
  cache_wr->readcnt++;
  if (cache_wr->readcnt == 1)
    P(wp);
  V(mutexp);

  has_proxy = get_object(uri, cache_wr->cache, objp, size);

  P(mutexp);
  cache_wr->readcnt--;
  if (cache_wr->readcnt == 0)
    V(wp);
  V(mutexp);
  return has_proxy;
}

void put_object_wr(char *uri, char *object, int size, cache_wr_t *cache_wr)
{
  sem_t *wp = &cache_wr->w;
  P(wp);
  put_object(uri, object, size, cache_wr->cache);
  V(wp);
}