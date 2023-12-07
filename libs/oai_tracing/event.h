#ifndef _EVENT_H_
#define _EVENT_H_

#include "utils.h"
#include "T_defs.h"
#ifdef T_SEND_TIME
#include <time.h>
#endif

enum event_arg_type {
  EVENT_INT,
  EVENT_ULONG,
  EVENT_STRING,
  EVENT_BUFFER
};

typedef struct {
  enum event_arg_type type;
  //int offset;
  union {
    int i;
    unsigned long ul;
    char *s;
    struct {
      int bsize;
      void *b;
    };
  };
} event_arg;

typedef struct {
#ifdef T_SEND_TIME
  struct timespec sending_time;
#endif
  int type;
  char *buffer;
  event_arg e[T_MAX_ARGS];
  int ecount;
} event;

event get_event(int s, OBUF *v, void *d);

#ifdef T_SEND_TIME
event new_event(struct timespec sending_time, int type,
    int length, char *buffer, void *database);
#else
event new_event(int type, int length, char *buffer, void *database);
#endif

#endif /* _EVENT_H_ */
