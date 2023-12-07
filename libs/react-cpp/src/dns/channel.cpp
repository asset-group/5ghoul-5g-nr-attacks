/**
 *  Channel.cpp
 *
 *  Wrapper class around the ares channel.
 *
 *  @copyright 2014 Copernica BV
 */
#include "includes.h"

/**
 *  Set up namespace
 */
namespace React { namespace Dns {

/**
 *  Static function that is called on expiration of a timer
 *  @param  channel the ares channel that the timer expired on
 */
static void timer_expires(ares_channel channel)
{
    // sets of readable and writable filedescriptors
    fd_set readable;
    fd_set writable;

    // both sets are empty
    FD_ZERO(&readable);
    FD_ZERO(&writable);

    // process everything
    ares_process(channel, &readable, &writable);
}

/**
 *  Callback method that is called when a socket changes state
 *  @param  data    Used-supplied-data (pointer to the resolver)
 *  @param  fd      The filedescriptor
 *  @param  read    Does the socket want to be notified when it is readable
 *  @param  write   Does the socket want to be notified when it is writable
 */
static void socket_state_callback(void *data, int fd, int read, int write)
{
    // retrieve a pointer to the channel
    auto *channel = static_cast<Channel*>(data);

    // change the state
    channel->check(fd, read, write);
}

/**
 *  Constructor
 */
Channel::Channel(Loop *loop) : _loop(loop), _timer(_loop, 0.0, [this]() { timer_expires(*this); setTimeout(); })
{
    // initialisation options
    struct ares_options options;

    // set the callback function
    options.sock_state_cb = socket_state_callback;
    options.sock_state_cb_data = this;

    // initialize the channel
    ares_init_options(&_channel, &options, ARES_OPT_SOCK_STATE_CB);
}

/**
 *  Destructor
 */
Channel::~Channel()
{
    // cancel the timer
    _timer.cancel();

    // if there is no channel we have nothing to cleanup
    if (!_channel) return;

    // cancel all current requests
    ares_cancel(_channel);

    // destroy the channel
    ares_destroy(_channel);
}

/**
 *  Set the timeout for the next iteration
 */
void Channel::setTimeout()
{
    // do we need a timer in the first place?
    if (!_channel)
    {
        // no calls are pending, stop the timer
        _timer.cancel();
    }
    else
    {
        // max timeout is 10 seconds
        Timeval timeval(10.0);

        // calculate new timestamp
        ares_timeout(*this, &timeval, &timeval);

        // set the timeout
        _timer.set(timeval);
    }
}

/**
 *  Check a certain filedescriptor for readability or writability
 *  @param  fd      Filedescriptor
 *  @param  read    Check for readability
 *  @param  write   Check for writability
 */
void Channel::check(int fd, bool read, bool write)
{
    // remove from the appriate sets
    if (!read) _readers.erase(fd);
    if (!write) _writers.erase(fd);

    // insert new entry for readability
    if (read && _readers.find(fd) == _readers.end())
    {
        // add a new reader
        auto *watcher = new ReadWatcher(_loop, fd, [this,fd]() -> bool {
            ares_process_fd(*this, fd, 0);
            return true;
        });

        // add to the set of watcher
        _readers[fd] = std::unique_ptr<ReadWatcher>(watcher);
    }

    // insert new entry for writability
    if (write && _writers.find(fd) == _writers.end())
    {
        // add a watcher
        auto *watcher = new WriteWatcher(_loop, fd, [this,fd]() -> bool {
            ares_process_fd(*this, 0, fd);
            return true;
        });

        // add to the set of watchers
        _writers[fd] = std::unique_ptr<WriteWatcher>(watcher);
    }
}

/**
 *  End namespace
 */
}}
