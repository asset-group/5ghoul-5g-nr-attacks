#include "handler.h"
#include "event.h"
#include "database.h"
#include <stdlib.h>
#include <stdio.h>

typedef void (*handler_function)(void *p, event e);

struct handler_data {
  handler_function f;
  void *p;
  unsigned long id;
};

struct handler_list {
  struct handler_data *f;
  int size;
  int maxsize;
};

/* internal definition of an event handler */
struct _event_handler {
  void *database;
  struct handler_list *events;
  unsigned long next_id;
};

void handle_event(event_handler *_h, event e)
{
  struct _event_handler *h = _h;
  int i;
  for (i = 0; i < h->events[e.type].size; i++)
    h->events[e.type].f[i].f(h->events[e.type].f[i].p, e);
}

event_handler *new_handler(void *database)
{
  struct _event_handler *ret = calloc(1, sizeof(struct _event_handler));
  if (ret == NULL) abort();

  ret->database = database;

  ret->events = calloc(number_of_ids(database), sizeof(struct handler_list));
  if (ret->events == NULL) abort();

  ret->next_id = 1;

  return ret;
}

unsigned long register_handler_function(event_handler *_h, int event_id,
    void (*f)(void *, event), void *p)
{
  struct _event_handler *h = _h;
  unsigned long ret = h->next_id;
  struct handler_list *l;

  h->next_id++;
  if (h->next_id == 2UL * 1024 * 1024 * 1024)
    { printf("%s:%d: this is bad...\n", __FILE__, __LINE__); abort(); }

  l = &h->events[event_id];
  if (l->size == l->maxsize) {
    l->maxsize += 16;
    l->f = realloc(l->f, l->maxsize * sizeof(struct handler_data));
    if (l->f == NULL) abort();
  }
  l->f[l->size].f = f;
  l->f[l->size].p = p;
  l->f[l->size].id = ret;

  l->size++;

  return ret;
}

void remove_handler_function(event_handler *h, int event_id,
    unsigned long handler_id)
{
  printf("%s:%d: TODO\n", __FILE__, __LINE__);
  abort();
}
