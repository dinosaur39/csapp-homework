#ifndef __MYCACHE_H__
#define __MYCACHE_H__

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define CACHE_MAXLINE 8192

typedef struct {
  cachenode_t *prev;
  cachenode_t *next;
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
char *get_object(char *uri, cache_t *cache);
void put_object(char *uri, char *object, int size, cache_t *cache);

static cachenode_t *create_cachenode(char *uri, char *object, int size);
static void remove_cachenode(cachenode_t *node);
static void insert_cachenode(cachenode_t *node, cache_t *cache);
static void free_cachenode(cachenode_t *node);

#endif