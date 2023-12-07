/**
 *  Cleanup.h
 *
 *  Object that can be used to monitor event loop cleanup
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
class CleanupWatcher : private Watcher
{
private:
    /**
     *  Pointer to the loop
     */
    Loop *_loop;

    /**
     *  Watcher resource
     */
    struct ev_cleanup _watcher;

    /**
     *  Callback function
     */
    CleanupCallback _callback;

    /**
     *  Is it active?
     *  @var    bool
     */
    bool _active = true;

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
     *  @param  callback    Function that is called before the loop stops
     */
    CleanupWatcher(Loop *loop, const CleanupCallback &callback) :
        _loop(loop), _callback(callback)
    {
        // store pointer to current object
        _watcher.data = this;

        // initialize the watcher
        initialize();

        // start right away
        ev_cleanup_start(*_loop, &_watcher);
    }

    /**
     *  No copying or moving allowed
     *  @param  that
     */
    CleanupWatcher(const CleanupWatcher &that) = delete;
    CleanupWatcher(CleanupWatcher &&that) = delete;

    /**
     *  Destructor
     */
    virtual ~CleanupWatcher()
    {
        // destructor
        cancel();
    }

    /**
     *  No copying or moving
     *  @param  that
     */
    CleanupWatcher &operator=(const CleanupWatcher &that) = delete;
    CleanupWatcher &operator=(CleanupWatcher &&that) = delete;

    /**
     *  Cancel the watcher
     *
     *  The callback will no longer be called, not even when the loop gets
     *  destroyed. Calling this method makes the object effectively useless
     *
     *  @return bool
     */
    virtual bool cancel()
    {
        // skip if already stopped
        if (!_active) return false;

        // stop now
        ev_cleanup_stop(*_loop, &_watcher);

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
