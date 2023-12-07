/**
 *  SharedTimeoutWatcher.h
 *
 *  TimeoutWatcher that is managed by the loop, and that can be shared with the outside
 *  world.
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
class SharedTimeoutWatcher : public Shared<TimeoutWatcher>, public TimeoutWatcher
{
private:
    /**
     *  Called when timer expires
     */
    virtual void invoke() override
    {
        // keep a shared pointer for as long as the callback is called
        auto ptr = pointer();
        
        // now we call the base invoke method
        TimeoutWatcher::invoke();

        if (ptr.use_count() <= 2)
            cancel();
    }

public:
    /**
     *  Constructor
     *  @param  loop        Event loop
     *  @param  timeout     Timeout period
     *  @param  callback    Function that is called when timer is expired
     */
    SharedTimeoutWatcher(Loop *loop, Timestamp timeout, const TimeoutCallback &callback) : Shared(this), TimeoutWatcher(loop, timeout, callback) {}
    
    /**
     *  Destructor
     */
    virtual ~SharedTimeoutWatcher() {
        // std::cout << "~SharedTimeoutWatcher" << std::endl;
    }
    
    /**
     *  Start the timer
     *  @return bool
     */
    virtual bool start() override
    {
        // call base
        if (!TimeoutWatcher::start()) return false;
        
        // object lives again, fix the shared pointer
        restore();
        
        // done
        return true;
    }
    
    /**
     *  Cancel the timer
     *  @return bool
     */
    virtual bool cancel() override
    {
        // call base
        if (!TimeoutWatcher::cancel()) return false;
        
        // object is cancelled, forget shared pointer
        reset();
        
        // done
        return true;
    }
};

/**
 *  End namespace
 */
}
