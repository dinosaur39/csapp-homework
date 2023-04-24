/*
 * fdbuf.h - definitions for tools to manipulate file descriptor buffer pool 
 */
#ifndef __FDBUF_H__
#define __FDBUF_H__

#include "csapp.h"

typedef int fd_t;

typedef struct {
  fd_t *buf; // buffer array
  int n; // number of slots
  int front; // index of first item in this buffer
  int rear; //index of last item in this buffer
  sem_t mutex; // protection when accessing buffer
  sem_t slots; // number of available slots
  sem_t fds; // number of available items
} fdbuf_t;

void fdbuf_init(fdbuf_t *p, int n);
void fdbuf_deinit(fdbuf_t *p);
void fdbuf_push(fdbuf_t *p, fd_t fd);
fd_t fdbuf_pop(fdbuf_t *p);


#endif