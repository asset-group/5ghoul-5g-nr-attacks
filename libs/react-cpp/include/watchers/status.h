/**
 *  Status.h
 *
 *  Object that watches for a child process status change.
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
class StatusWatcher : private Watcher
{
private:
    /**
     *  Pointer to the loop
     *  @var    MainLoop
     */
    MainLoop *_loop;

    /**
     *  IO resource
     *  @var    ev_child
     */
    struct ev_child _watcher;

    /**
     *  Callback function
     *  @var    StatusCallback
     */
    StatusCallback _callback;

    /**
     *  Is it active?
     *  @var    bool
     */
    bool _active = false;

    /**
     *  Initialize the object
     *  @param  pid     The PID to watch
     *  @param  trace   Should the watcher be called for all status changes (true) or only for exits (false)
     */
    void initialize(pid_t pid, bool trace);

protected:
    /**
     *  Invoke the callback
     */
    virtual void invoke() override
    {
        // check if object is still valid
        Monitor monitor(this);
        
        // call the callback
        if (_callback(_watcher.rpid, _watcher.rstatus) || !monitor.valid()) return;
        
        // cancel watcher
        cancel();
    }

public:
    /**
     *  Constructor
     *  @param  loop        Event loop
     *  @param  pid         The PID to watch
     *  @param  trace       Should the watcher be called for all status changes (true) or only for exits (false)
     *  @param  callback    Function that is called when status changes
     */
    template <typename CALLBACK>
    StatusWatcher(MainLoop *loop, pid_t pid, bool trace, const CALLBACK &callback) : _loop(loop), _callback(callback)
    {
        // store pointer to current object
        _watcher.data = this;

        // initialize the watcher
        initialize(pid, trace);

        // start the timer
        start();
    }

    /**
     *  Alternative constructor to watch a specific PID only for exits
     *  @param  loop        Event loop
     *  @param  pid         The PID to watch
     *  @param  callback    Function that is called when status changes
     */
    template <typename CALLBACK>
    StatusWatcher(MainLoop *loop, pid_t pid, const CALLBACK &callback) : StatusWatcher(loop, pid, false, callback) {}

    /**
     *  Alternative constructor to watch ALL child processes for exits
     *  @param  loop        Event loop
     *  @param  callback    Function that is called when status changes
     */
    template <typename CALLBACK>
    StatusWatcher(MainLoop *loop, const CALLBACK &callback) : StatusWatcher(loop, 0, false, callback) {}

    /**
     *  No copying or moving allowed
     *  @param  that
     */
    StatusWatcher(const StatusWatcher &that) = delete;
    StatusWatcher(StatusWatcher &&that) = delete;

    /**
     *  Destructor
     */
    virtual ~StatusWatcher() 
    {
        // cancel the timer
        cancel();
    }

    /**
     *  No copying or moving
     *  @param  that
     */
    StatusWatcher &operator=(const StatusWatcher &that) = delete;
    StatusWatcher &operator=(StatusWatcher &&that) = delete;
    
    /**
     *  Start the signal watcher
     *  @return bool
     */
    virtual bool start()
    {
        // skip if already running
        if (_active) return false;
        
        // start now
        ev_child_start(*_loop, &_watcher);
        
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
        ev_child_stop(*_loop, &_watcher);
        
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
