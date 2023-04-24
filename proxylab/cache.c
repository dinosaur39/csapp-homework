#include "cache.h"

static cachenode_t *create_cachenode(char *uri, char *object, int size);
static void remove_cachenode(cachenode_t *node);
static void insert_cachenode(cachenode_t *node, cache_t *cache);
static void free_cachenode(cachenode_t *node);

cache_t *init_cache(void)
{
  cache_t *cache = (cache_t*) Malloc(sizeof(cache_t));
  cachenode_t *start = (cachenode_t*) Malloc(sizeof(cachenode_t));
  cache->cache_used = 0;
  cache->start = start;
  start->prev = start->next = start;
  *start->uri = '\0';
  start->size = 0;
  return cache;
}

void free_cache(cache_t *cache)
{
  cachenode_t *node = cache->start, *next_node;
  int object_stored = node->size;
  node = node->next;
  for (int i = 0; i < object_stored; i++) {
    next_node = node->next;
    free_cachenode(node);
    node = next_node;
  }
  Free(node);
  Free(cache);
}

int get_object(char *uri, cache_t *cache, char *objectp, int *sizep)
{
  cachenode_t *node = cache->start;
  int object_stored = node->size;
  printf("uri of target: %s\n", uri);
  for (;object_stored > 0; object_stored--) {
    node = node->next;
    printf("uri of object: %s\n", node->uri);
    if (!strncmp(uri, node->uri, CACHE_MAXLINE))
      break;
  }
  if (object_stored) {
    //remove_cachenode(node);
    //insert_cachenode(node, cache);
    printf("find cache of uri:%s\n", uri);
    *sizep = node->size;
    memcpy(objectp, node->object, *sizep);
    return 1;
  }
  else {
    *sizep = -1;
    return 0;
  }
}

void put_object(char *uri, char *object, int size, cache_t *cache)
{
  if (size > MAX_OBJECT_SIZE) return;

  cachenode_t *start = cache->start, *tail, *newnode;
  int cache_used = cache->cache_used, node_count = start->size;
  
  cache_used += size;
  while (cache_used > MAX_CACHE_SIZE) {
    tail = start->prev;
    cache_used -= tail->size;
    remove_cachenode(tail);
    free_cachenode(tail);
    node_count--;
  }

  newnode = create_cachenode(uri, object, size);
  insert_cachenode(newnode, cache);
  node_count++;
  cache->cache_used = cache_used;
  start->size = node_count;
}

static cachenode_t *create_cachenode(char *uri, char *object, int size)
{
  cachenode_t *newnode = (cachenode_t*) Malloc(sizeof(cachenode_t));
  newnode->object = (char *) Calloc((size_t)size, sizeof(char));
  strncpy(newnode->uri, uri, CACHE_MAXLINE);
  memcpy(newnode->object, object, size);
  newnode->size = size;
  return newnode;
}

/* 
 * remove_cachenode - this function only remove cachenode from the 
 * linked list, you need to free the node manually.
 */
static void remove_cachenode(cachenode_t *node)
{
  cachenode_t *prev = node->prev, *next = node->next;
  prev->next = next;
  next->prev = prev;
}

/* 
 * insert_cachenode - This cache use LRU eviction policy, so it will
 * insert the new node at the head of the linked list.
 */
static void insert_cachenode(cachenode_t *node, cache_t *cache)
{
  cachenode_t *start = cache->start, *oldhead = start->next;
  start->next = node;
  oldhead->prev = node;
  node->prev = start;
  node->next = oldhead;
}

static void free_cachenode(cachenode_t *node)
{
  Free(node->object);
  Free(node);
}