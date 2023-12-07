/**
 *  SharedInterval.h
 *
 *  Interval that is managed by the loop, and that can be shared with the outside
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
class SharedIntervalWatcher : public Shared<IntervalWatcher>, public IntervalWatcher
{
private:
    /**
     *  Called when timer expires
     */
    virtual void invoke() override
    {
        // keep a shared pointer for as long as the callback is called, this
        // ensures that the object is not destructed if the user calls 
        // the cancel() method, and immediately afterwards a different method
        auto ptr = pointer();
        
        // now we call the base invoke method
        IntervalWatcher::invoke();
    }

public:
    /**
     *  Constructor
     *  @param  loop        Event loop
     *  @param  initial     Initial timeout
     *  @param  interval    Timeout interval period
     *  @param  callback    Function that is called when timer is expired
     */
    SharedIntervalWatcher(Loop *loop, Timestamp initial, Timestamp interval, const IntervalCallback &callback) : Shared(this), IntervalWatcher(loop, initial, interval, callback) {}
    
    /**
     *  Destructor
     */
    virtual ~SharedIntervalWatcher() {}
    
    /**
     *  Start the timer
     *  @return bool
     */
    virtual bool start() override
    {
        // call base
        if (!IntervalWatcher::start()) return false;
        
        // make sure the shared pointer is valid, so that we have a reference to ourselves
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
        if (!IntervalWatcher::cancel()) return false;
        
        // because the interval is no longer running, we no longer have to keep a pointer to ourselves
        reset();
        
        // done
        return true;
    }
};

/**
 *  End namespace
 */
}
