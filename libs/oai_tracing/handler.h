#ifndef _HANDLER_H_
#define _HANDLER_H_

typedef void event_handler;

#include "event.h"

event_handler *new_handler(void *database);
void handle_event(event_handler *h, event e);

unsigned long register_handler_function(event_handler *_h, int event_id,
    void (*f)(void *, event), void *p);
void remove_handler_function(event_handler *h, int event_id,
    unsigned long handler_id);

#endif /* _HANDLER_H_ */
