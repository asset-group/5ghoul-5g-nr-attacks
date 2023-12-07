/**
 *  SharedStatusWatcher.h
 *
 *  StatisWatcher watcher that is managed by the loop, and that can be shared with the 
 *  outside world.
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
class SharedStatusWatcher : public Shared<StatusWatcher>, public StatusWatcher
{
private:
    /**
     *  Called when signal is caught
     */
    virtual void invoke() override
    {
        // keep a shared pointer for as long as the callback is called, this
        // ensures that the object is not destructed if the user calls 
        // the cancel() method, and immediately afterwards a different method
        auto ptr = pointer();
        
        // now we call the base invoke method
        StatusWatcher::invoke();
    }

public:
    /**
     *  Constructor
     *  @param  loop        Event loop
     *  @param  pid         The PID to watch
     *  @param  trace       Should the watcher be called for all status changes (true) or only for exits (false)
     *  @param  callback    Function that is called when status changes
     */
    SharedStatusWatcher(MainLoop *loop, pid_t pid, bool trace, const StatusCallback &callback) : Shared(this), StatusWatcher(loop, pid, trace, callback) {}
    
    /**
     *  Destructor
     */
    virtual ~SharedStatusWatcher() {}
    
    /**
     *  Start the signal watcher
     *  @return bool
     */
    virtual bool start() override
    {
        // call base
        if (!StatusWatcher::start()) return false;
        
        // make sure the shared pointer is valid, so that we have a reference to ourselves
        restore();
        
        // done
        return true;
    }
    
    /**
     *  Cancel the signal watcher
     *  @return bool
     */
    virtual bool cancel() override
    {
        // call base
        if (!StatusWatcher::cancel()) return false;
        
        // because the signal watcher is no longer running, we no longer have to keep a pointer to ourselves
        reset();
        
        // done
        return true;
    }
};

/**
 *  End namespace
 */
}
