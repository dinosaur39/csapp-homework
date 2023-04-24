#include "fdbuf.h"

void fdbuf_init(fdbuf_t *p, int n)
{
  p->buf = Calloc(n, sizeof(fd_t));
  p->n = n;
  p->front = p->rear = 0;
  Sem_init(&p->mutex, 0, 1);
  Sem_init(&p->slots, 0, n);
  Sem_init(&p->fds, 0, 0);
}

void fdbuf_deinit(fdbuf_t *p)
{
  Free(p->buf);
}

void fdbuf_push(fdbuf_t *p, fd_t fd)
{
  P(&p->slots);
  P(&p->mutex);
  p->buf[(++p->rear) % (p->n)] = fd;
  V(&p->mutex);
  V(&p->fds);
}

int fdbuf_pop(fdbuf_t *p)
{
  int fd;
  P(&p->fds);
  P(&p->mutex);
  fd = p->buf[(++p->front) % (p->n)];
  V(&p->mutex);
  V(&p->slots);
  return fd;
}
