#include "event.h"
#include "database.h"
#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

event get_event(int socket, OBUF *event_buffer, void *database)
{
#ifdef T_SEND_TIME
  struct timespec t;
#endif
  int type;
  int32_t length;

  /* Events type -1 and -2 are special: the tracee sends its version
   * of T_messages.txt using those events.
   * We have to check that the local version of T_messages.txt is identical
   * to the one the tracee uses. We don't report those events to the
   * application.
   */

again:
  if (fullread(socket, &length, 4) == -1)
    goto read_error;
#ifdef T_SEND_TIME
  if (fullread(socket, &t, sizeof(struct timespec)) == -1)
    goto read_error;
  length -= sizeof(struct timespec);
#endif
  if (fullread(socket, &type, sizeof(int)) == -1)
    goto read_error;
  length -= sizeof(int);
  if (event_buffer->omaxsize < length)
  {
    event_buffer->omaxsize = (length + 65535) & ~65535;
    event_buffer->obuf = realloc(event_buffer->obuf, event_buffer->omaxsize);
    if (event_buffer->obuf == NULL)
    {
      printf("out of memory\n");
      exit(1);
    }
  }
  if (fullread(socket, event_buffer->obuf, length) == -1)
    goto read_error;
  event_buffer->osize = length;

  if (type == -1)
    append_received_config_chunk(event_buffer->obuf, length);
  if (type == -2)
    verify_config();

  if (type == -1 || type == -2)
    goto again;

#ifdef T_SEND_TIME
  return new_event(t, type, length, event_buffer->obuf, database);
#else
  return new_event(type, length, event_buffer->obuf, database);
#endif

read_error:
  return (event){type : -1};
}

#ifdef T_SEND_TIME
event new_event(struct timespec sending_time, int type,
                int length, char *buffer, void *database)
#else
event new_event(int type, int length, char *buffer, void *database)
#endif
{
  database_event_format f;
  event e;
  int i;
  int offset;

#ifdef T_SEND_TIME
  e.sending_time = sending_time;
#endif
  e.type = type;
  e.buffer = buffer;

  f = get_format(database, type);

  e.ecount = f.count;

  offset = 0;

  /* setup offsets */
  /* TODO: speedup (no strcmp, string event to include length at head) */
  for (i = 0; i < f.count; i++)
  {
    //e.e[i].offset = offset;
    if (!strcmp(f.type[i], "int"))
    {
      e.e[i].type = EVENT_INT;
      e.e[i].i = *(int *)(&buffer[offset]);
      offset += 4;
    }
    else if (!strcmp(f.type[i], "ulong"))
    {
      e.e[i].type = EVENT_ULONG;
      e.e[i].ul = *(unsigned long *)(&buffer[offset]);
      offset += sizeof(unsigned long);
    }
    else if (!strcmp(f.type[i], "string"))
    {
      e.e[i].type = EVENT_STRING;
      e.e[i].s = &buffer[offset];
      while (buffer[offset])
        offset++;
      offset++;
    }
    else if (!strcmp(f.type[i], "buffer"))
    {
      int len;
      e.e[i].type = EVENT_BUFFER;
      len = *(int *)(&buffer[offset]);
      e.e[i].bsize = len;
      e.e[i].b = &buffer[offset + sizeof(int)];
      offset += len + sizeof(int);
    }
    else
    {
      printf("unhandled type '%s'\n", f.type[i]);
      abort();
    }
  }

  if (e.ecount == 0)
  {
    printf("FORMAT not set in event %d\n", type);
    abort();
  }

  return e;
}
