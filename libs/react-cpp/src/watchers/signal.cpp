/**
 *  Signal.cpp
 * 
 *  @copyright 2014 Copernica BV
 */
#include "includes.h"

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Callback method that is called when the signal is caught
 *  @param  loop        The loop in which the event was triggered
 *  @param  watcher     Internal watcher object
 *  @param  revents     Events triggered
 */
static void onSignal(struct ev_loop *loop, ev_signal *watcher, int revents)
{
    // retrieve the reader
    Watcher *object = (Watcher *)watcher->data;

    // call it
    object->invoke();
}
    
/**
 *  Initialize the object
 *  @param  signum
 */
void SignalWatcher::initialize(int signum)
{
    // initialize the signal watcher
    ev_signal_init(&_watcher, onSignal, signum);
}

/**
 *  End namespace
 */
}
