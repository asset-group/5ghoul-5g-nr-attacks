/**
 *  SynchronizeWatcher.cpp
 * 
 *  @copyright 2014 Copernica BV
 */
#include "includes.h"

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Callback method that is called when the synchronizer is triggered
 *  @param  loop        The loop in which the event was triggered
 *  @param  watcher     Internal watcher object
 *  @param  revents     Events triggered
 */
static void onSynchronize(struct ev_loop *loop, ev_async *watcher, int revents)
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
void SynchronizeWatcher::initialize()
{
    // initialize the async
    ev_async_init(&_watcher, onSynchronize);
}


/**
 *  End namespace
 */
}
