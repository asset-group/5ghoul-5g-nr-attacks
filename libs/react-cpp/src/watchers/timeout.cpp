/**
 *  Timeout.cpp
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
 *  @param  timeout
 */
void TimeoutWatcher::initialize(Timestamp timeout)
{
    // set the expiration time
    current_timeout = timeout;
    
    // initialize the timer
    ev_timer_init(&_watcher, onExpired, timeout, 0.0);
}


/**
 *  End namespace
 */
}
