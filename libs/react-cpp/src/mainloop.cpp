/**
 *  MainLoop.cpp
 *
 *  Mainloop implementation
 *
 *  @copyright 2014 Copernica BV
 */
#include "includes.h"

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Register a function that is called the moment a signal is fired.
 *  @param  signum      The signal
 *  @param  callback    Function that is called the moment the signal is caught
 *  @return             Object that can be used to stop checking for signals
 */
std::shared_ptr<SignalWatcher> MainLoop::onSignal(int signum, const SignalCallback &callback)
{
    // create self-destructing implementation object
    auto *signal = new SharedSignalWatcher(this, signum, callback);
    
    // done
    return signal->pointer();
}

/**
 *  Register a function that is called the moment the status of a child changes
 *  @param  pid         The child PID
 *  @param  trace       Monitor for all status changes (true) or only for child exits (false)
 *  @param  callback    Function that is called the moment the child changes status
 *  @return
 */
std::shared_ptr<StatusWatcher> MainLoop::onStatusChange(pid_t pid, bool trace, const StatusCallback &callback)
{
    // create self-destructing implementation object
    auto *status = new SharedStatusWatcher(this, pid, trace, callback);
    
    // done
    return status->pointer();
}

/**
 *  End namespace
 */
}

