/**
 *  MainLoop.h
 * 
 *  The main loop is an extended loop class that can also be used to watch
 *  for signals. In normal circumstances you only need to create one
 *  main loop, in the master thread. If you start additional threads, you can
 *  use the regular loop class.
 * 
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Class definition
 */
class MainLoop : public Loop
{
public:
    /**
     *  Constructor
     */
    MainLoop() : Loop(ev_default_loop(EVFLAG_AUTO)) {}
    
    /**
     *  Destructor
     */
    virtual ~MainLoop() {}
    
    /**
     *  Register a function that is called the moment a signal is fired.
     *
     *  This method takes two arguments: the signal number to be checked and
     *  the function that is going to be called the moment the signal is 
     *  triggered.
     *
     *  The method returns a shared pointer to a watcher object. This watcher
     *  object can be used to later stop watching for the signal. It is 
     *  legal to ignore the return value if you don't need it. In that case
     *  the signal is watched for the entire runtime of the loop.
     * 
     *  @param  signum      The signal
     *  @param  callback    Function that is called the moment the signal is caught
     *  @return             Object that can be used to stop checking for signals
     */
    std::shared_ptr<SignalWatcher> onSignal(int signum, const SignalCallback &callback);

    /**
     *  Alternative onSignal() method that accepts other callbacks
     *  @param  signum      The signal
     *  @param  callback    Function that is called the moment the signal is caught
     *  @return             Object that can be used to stop checking for signals
     */
    template <typename CALLBACK>
    std::shared_ptr<SignalWatcher> onSignal(int signum, const CALLBACK &callback) { return onSignal(signum, SignalCallback(callback)); }

    /**
     *  Register a function that is called the moment the status of a child changes
     * 
     *  This method takes two arguments: the PID of the child to check, and
     *  a callback that is going to be called every time the status of the
     *  child changes.
     * 
     *  The method returns a shared pointer to a watcher object. This watcher
     *  object can be used to later stop watching for the signal. It is 
     *  legal to ignore the return value if you don't need it. In that case
     *  the signal is watched for the entire runtime of the loop.
     * 
     *  @param  pid         The child PID
     *  @param  trace       If true, the watcher is notified for all status changes, otherwise only exits
     *  @param  callback    Function that is called the moment the child changes status
     *  @return             Object that can be used to stop checking for status changes
     */
    std::shared_ptr<StatusWatcher> onStatusChange(pid_t pid, bool trace, const StatusCallback &callback);
    
    /**
     *  Alternative onChild() method that accepts other callbacks
     *  @param  pid         The child PID
     *  @param  trace       If true, the watcher is notified for all status changes, otherwise only exits
     *  @param  callback    Function that is called the moment the child changes status
     *  @return             Object that can be used to stop checking for status changes
     */
    template <typename CALLBACK>
    std::shared_ptr<StatusWatcher> onStatusChange(pid_t pid, bool trace, const CALLBACK &callback) { return onStatusChange(pid, trace, StatusCallback(callback)); }
    
    /**
     *  Alternative onChild() method that accepts other callbacks, and for which trace is always off
     *  @param  pid         The child PID
     *  @param  callback    Function that is called the moment the child changes status
     *  @return             Object that can be used to stop checking for status changes
     */
    template <typename CALLBACK>
    std::shared_ptr<StatusWatcher> onStatusChange(pid_t pid, const CALLBACK &callback) { return onStatusChange(pid, false, StatusCallback(callback)); }

    /**
     *  Alternative onChild() method that monitors all children for exits
     *  @param  callback    Function that is called the moment the child changes status
     *  @return             Object that can be used to stop checking for status changes
     */
    template <typename CALLBACK>
    std::shared_ptr<StatusWatcher> onStatusChange(const CALLBACK &callback) { return onStatusChange(0, false, StatusCallback(callback)); }

};
    
/**
 *  End of namespace
 */
}
