/**
 *  Write.cpp
 *
 *  @copyright 2014 Copernica BV
 */
#include "includes.h"

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Function that gets called when the filedescriptor is active
 *  @param  loop    underlying libev loop
 *  @param  watcher libev io structure
 *  @param  events  events happening on the file descriptor
 */
static void onActive(struct ev_loop *loop, ev_io *watcher, int events)
{
    // retrieve the reader
    Watcher *object = (Watcher *)watcher->data;

    // call it
    object->invoke();
}

/**
 *  Initialize
 *  @param  fd  file descriptor to monitor for writability
 */
void WriteWatcher::initialize(int fd)
{
    // initialize the watcher
    ev_io_init(&_watcher, onActive, fd, EV_WRITE);
}

/**
 *  End namespace
 */    
}
