/**
 *  Status.cpp
 * 
 *  @copyright 2014 Copernica BV
 */
#include "includes.h"

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Callback method that is called when child process changes status
 *  @param  loop        The loop in which the event was triggered
 *  @param  watcher     Internal watcher object
 *  @param  revents     Events triggered
 */
static void onStatusChange(struct ev_loop *loop, ev_child *watcher, int revents)
{
    // retrieve the reader
    Watcher *object = (Watcher *)watcher->data;

    // call it
    object->invoke();
}
    
/**
 *  Initialize the object
 *  @param  pid
 *  @param  trace
 */
void StatusWatcher::initialize(pid_t pid, bool trace)
{
    // initialize the signal watcher
    ev_child_init(&_watcher, onStatusChange, pid, trace ? 1 : 0);
}

/**
 *  End namespace
 */
}
