#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

void new_thread(void *(*f)(void *), void *data)
{
  pthread_t t;
  pthread_attr_t att;

  if (pthread_attr_init(&att))
  {
    fprintf(stderr, "pthread_attr_init err\n");
    exit(1);
  }

  if (pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED))
  {
    fprintf(stderr, "pthread_attr_setdetachstate err\n");
    exit(1);
  }

  if (pthread_attr_setstacksize(&att, 10000000))
  {
    fprintf(stderr, "pthread_attr_setstacksize err\n");
    exit(1);
  }

  if (pthread_create(&t, &att, f, data))
  {
    fprintf(stderr, "pthread_create err\n");
    exit(1);
  }

  if (pthread_attr_destroy(&att))
  {
    fprintf(stderr, "pthread_attr_destroy err\n");
    exit(1);
  }
}

void sleepms(int ms)
{
  struct timespec t;
  t.tv_sec = ms / 1000;
  t.tv_nsec = (ms % 1000) * 1000000L;

  /* TODO: deal with EINTR */
  if (nanosleep(&t, NULL))
    abort();
}

void bps(char *out, float v, char *suffix)
{
  static char *bps_unit[4] = {"", "k", "M", "G"};
  int flog;

  if (v < 1000)
    flog = 0;
  else
    flog = floor(floor(log10(v)) / 3);

  if (flog > 3)
    flog = 3;

  v /= pow(10, flog * 3);
  sprintf(out, "%g%s%s", round(v * 100) / 100, bps_unit[flog], suffix);
}

/****************************************************************************/
/* list                                                                     */
/****************************************************************************/

list *list_remove_head(list *l)
{
  list *ret;

  if (l == NULL)
    return NULL;

  ret = l->next;

  if (ret != NULL)
    ret->last = l->last;

  free(l);
  return ret;
}

list *list_append(list *l, void *data)
{
  list *new = calloc(1, sizeof(list));

  if (new == NULL)
    abort();

  new->data = data;

  if (l == NULL)
  {
    new->last = new;
    return new;
  }

  l->last->next = new;
  l->last = new;
  return l;
}

/****************************************************************************/
/* socket                                                                   */
/****************************************************************************/

int create_listen_socket(char *addr, int port)
{
  struct sockaddr_in a;
  int s;
  int v;
  s = socket(AF_INET, SOCK_STREAM, 0);

  if (s == -1)
  {
    perror("socket");
    exit(1);
  }

  v = 1;

  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(int)))
  {
    perror("setsockopt");
    exit(1);
  }

  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = inet_addr(addr);

  if (bind(s, (struct sockaddr *)&a, sizeof(a)))
  {
    perror("bind");
    exit(1);
  }

  if (listen(s, 5))
  {
    perror("listen");
    exit(1);
  }

  return s;
}

int socket_accept(int s)
{
  struct sockaddr_in a;
  socklen_t alen;
  alen = sizeof(a);
  return accept(s, (struct sockaddr *)&a, &alen);
}

int socket_send(int socket, void *buffer, int size)
{
  char *x = buffer;
  int ret;

  while (size)
  {
    ret = write(socket, x, size);

    if (ret <= 0)
      return -1;

    size -= ret;
    x += ret;
  }

  return 0;
}

int get_connection(char *addr, int port)
{
  int s, t;
  printf("waiting for connection on %s:%d\n", addr, port);
  s = create_listen_socket(addr, port);
  t = socket_accept(s);

  if (t == -1)
  {
    perror("accept");
    exit(1);
  }

  close(s);
  printf("connected\n");
  return t;
}

int fullread(int fd, void *_buf, int count)
{
  char *buf = _buf;
  int ret = 0;
  int l;

  while (count)
  {
    l = read(fd, buf, count);

    if (l <= 0)
      return -1;

    count -= l;
    buf += l;
    ret += l;
  }

  return ret;
}

int try_connect_to(char *addr, int port)
{
  int s;
  struct sockaddr_in a;
  s = socket(AF_INET, SOCK_STREAM, 0);

  if (s == -1)
  {
    perror("socket");
    exit(1);
  }

  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = inet_addr(addr);

  if (connect(s, (struct sockaddr *)&a, sizeof(a)) == -1)
  {
    perror("connect");
    close(s);
    return -1;
  }

  return s;
}

int connect_to(char *addr, int port)
{
  int s;
  // printf("connecting to %s:%d\n", addr, port);
again:
  s = try_connect_to(addr, port);

  if (s == -1)
  {
    // printf("trying again in 1s\n");
    sleep(1);
    goto again;
  }

  return s;
}

/****************************************************************************/
/* buffer                                                                   */
/****************************************************************************/

void PUTC(OBUF *o, char c)
{
  if (o->osize == o->omaxsize)
  {
    o->omaxsize += 512;
    o->obuf = realloc(o->obuf, o->omaxsize);

    if (o->obuf == NULL)
      abort();
  }

  o->obuf[o->osize] = c;
  o->osize++;
}

void PUTS(OBUF *o, char *s)
{
  while (*s)
    PUTC(o, *s++);
}

static int clean(char c)
{
  if (!isprint(c))
    c = ' ';

  return c;
}

void PUTS_CLEAN(OBUF *o, char *s)
{
  while (*s)
    PUTC(o, clean(*s++));
}

void PUTI(OBUF *o, int i)
{
  char s[64];
  sprintf(s, "%d", i);
  PUTS(o, s);
}

void PUTX2(OBUF *o, int i)
{
  char s[64];
  sprintf(s, "%2.2x", i);
  PUTS(o, s);
}

void PUTUL(OBUF *o, unsigned long l)
{
  char s[128];
  sprintf(s, "%lu", l);
  PUTS(o, s);
}
