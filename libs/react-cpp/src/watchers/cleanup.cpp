/**
 *  CleanupWatcher.cpp
 *
 *  @copyright 2014 Copernica BV
 */

#include "includes.h"

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Callback method that is called before the event loop is destroyed
 *  @param  loop        The loop shutting down
 *  @param  watcher     Internal watcher object
 *  @param  revents     Events triggered
 */
static void onCleanup(struct ev_loop *loop, ev_cleanup *watcher, int revents)
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
void CleanupWatcher::initialize()
{
    // initialize the watcher
    ev_cleanup_init(&_watcher, onCleanup);
}


/**
 *  End namespace
 */
}
