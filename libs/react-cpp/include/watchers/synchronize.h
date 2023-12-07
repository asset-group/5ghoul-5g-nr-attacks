/**
 *  Synchronize.h
 *
 *  Object that can be used to synchroninze an event loop.
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
class SynchronizeWatcher : private Watcher
{
private:
    /**
     *  Pointer to the loop
     *  @var    Loop
     */
    Loop *_loop;

    /**
     *  IO resource
     *  @var    ev_async
     */
    struct ev_async _watcher;

    /**
     *  Callback function
     *  @var    SynchroninzeCallback
     */
    SynchronizeCallback _callback;

    /**
     *  Is it active?
     *  @var    bool
     */
    bool _active = false;

    /**
     *  Initialize the object
     */
    void initialize();

protected:
    /**
     *  Invoke the callback
     */
    virtual void invoke() override
    {
        // call the callback
        _callback();
    }

public:
    /**
     *  Constructor
     *  @param  loop        Event loop
     *  @param  callback    Function that is called when synchronizer is activated
     */
    template <typename CALLBACK>
    SynchronizeWatcher(Loop *loop, const CALLBACK &callback) :
        _loop(loop), _callback(callback)
    {
        // store pointer to current object
        _watcher.data = this;

        // initialize the watcher
        initialize();
        
        // start right away
        start();
    }
    
    /**
     *  No copying or moving allowed
     *  @param  that
     */
    SynchronizeWatcher(const SynchronizeWatcher &that) = delete;
    SynchronizeWatcher(SynchronizeWatcher &&that) = delete;
    
    /**
     *  Destructor
     */
    virtual ~SynchronizeWatcher() 
    {       
        // destructor
        cancel();
    }

    /**
     *  No copying or moving
     *  @param  that
     */
    SynchronizeWatcher &operator=(const SynchronizeWatcher &that) = delete;
    SynchronizeWatcher &operator=(SynchronizeWatcher &&that) = delete;

    /**
     *  Synchronize with the event loop
     * 
     *  This is a thread safe method, that is normally called from an other thread.
     *  
     *  After you've called synchronize, a call to the registered callback will
     *  be soon executed in the event loop in which the synchronizer was created.
     * 
     *  @return bool 
     */
    bool synchronize()
    {
        if (_active)
            return false;

        ev_async_send(*_loop, &_watcher);
        return true;
    }

    /**
     *  Start the synchronizer
     *  @return bool
     */
        virtual bool start()
        {
            // skip if already running
            if (_active)
                return false;

            ev_async_start(*_loop, &_watcher);

            synchronize();

            // remember that it is active
            return _active = true;
        }
    
    /**
     *  Cancel the synchronizer
     * 
     *  The owner loop will no longer be notified, even not when the synchronize()
     *  method is called. Calling this method makes the object effectively useless
     * 
     *  @return bool
     */
    virtual bool cancel()
    {
        // skip if already stopped
        if (!_active) return false;
        
        // stop now
        ev_async_stop(*_loop, &_watcher);
        
        // remember that it is no longer active
        _active = false;
        
        // done
        return true;
    }
}; 
 
/**
 *  End namespace
 */
}
