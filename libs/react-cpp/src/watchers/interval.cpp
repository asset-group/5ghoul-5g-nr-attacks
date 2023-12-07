/**
 *  Interval.cpp
 * 
 *  @copyright 2014 Copernica BV
 */
#include "includes.h"

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Callback method that is called when the timer expires
 *  @param  loop        The loop in which the event was triggered
 *  @param  watcher     Internal watcher object
 *  @param  revents     Events triggered
 */
static void onExpired(struct ev_loop *loop, ev_timer *watcher, int revents)
{
    // retrieve the reader
    Watcher *object = (Watcher *)watcher->data;

    // call it
    object->invoke();
}
    
/**
 *  Initialize the object
 *  @param  initial
 *  @param  timeout
 */
void IntervalWatcher::initialize(Timestamp initial, Timestamp timeout)
{
    current_interval = timeout;
    // initialize the timer
    ev_timer_init(&_watcher, onExpired, initial, timeout);
}

/**
 *  End namespace
 */
}
