/**
 *  SignalWatcher.h
 *
 *  Object that watches for signals to be fired.
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
class SignalWatcher : private Watcher
{
private:
    /**
     *  Pointer to the loop
     *  @var    MainLoop
     */
    MainLoop *_loop;

    /**
     *  IO resource
     *  @var    ev_signal
     */
    struct ev_signal _watcher;

    /**
     *  Callback function
     *  @var    SignalCallback
     */
    SignalCallback _callback;

    /**
     *  Is it active?
     *  @var    bool
     */
    bool _active = false;

    /**
     *  Initialize the object
     *  @param  signum  signal number
     */
    void initialize(int signum);

protected:
    /**
     *  Invoke the callback
     */
    virtual void invoke() override
    {
        // check if object is still valid
        Monitor monitor(this);
        
        // call the callback
        if (_callback() || !monitor.valid()) return;
        
        // cancel watcher
        cancel();
    }

public:
    /**
     *  Constructor
     *  @param  loop        Event loop
     *  @param  signum      The signal to watch
     *  @param  callback    Function that is called when timer is expired
     */
    template <typename CALLBACK>
    SignalWatcher(MainLoop *loop, int signum, const CALLBACK &callback) : _loop(loop), _callback(callback)
    {
        // store pointer to current object
        _watcher.data = this;

        // initialize the watcher
        initialize(signum);

        // start the timer
        start();
    }

    /**
     *  No copying or moving allowed
     *  @param  that
     */
    SignalWatcher(const SignalWatcher &that) = delete;
    SignalWatcher(SignalWatcher &&that) = delete;

    /**
     *  Destructor
     */
    virtual ~SignalWatcher() 
    {
        // cancel the timer
        cancel();
    }

    /**
     *  No copying or moving
     *  @param  that
     */
    SignalWatcher &operator=(const SignalWatcher &that) = delete;
    SignalWatcher &operator=(SignalWatcher &&that) = delete;
    
    /**
     *  Start the signal watcher
     *  @return bool
     */
    virtual bool start()
    {
        // skip if already running
        if (_active) return false;
        
        // start now
        ev_signal_start(*_loop, &_watcher);
        
        // remember that it is active
        return _active = true;
    }
    
    /**
     *  Cancel the signal watcher
     *  @return bool
     */
    virtual bool cancel()
    {
        // skip if not running
        if (!_active) return false;
        
        // stop now
        ev_signal_stop(*_loop, &_watcher);
        
        // remember that it no longer is active
        _active = false;
        
        // done
        return true;
    }
}; 
 
/**
 *  End namespace
 */
}
