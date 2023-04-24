#ifndef __MYCACHE_H__
#define __MYCACHE_H__

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define CACHE_MAXLINE 8192

typedef struct cache_node {
  struct cache_node *prev;
  struct cache_node *next;
  char uri[CACHE_MAXLINE];
  char *object;
  int size;
} cachenode_t;

typedef struct {
  cachenode_t *start;
  int cache_used;
} cache_t;

cache_t *init_cache(void);
void free_cache(cache_t *cache);
int get_object(char *uri, cache_t *cache, char *objectp, int *sizep);
void put_object(char *uri, char *object, int size, cache_t *cache);

#endif