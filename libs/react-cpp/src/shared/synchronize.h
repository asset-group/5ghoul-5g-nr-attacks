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
class SharedSynchronizeWatcher : public Shared<SynchronizeWatcher>, public SynchronizeWatcher
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
        SynchronizeWatcher::invoke();

        // // Check if no more than 2 internal instances are alive,udestroy object
        if (ptr.use_count() <= 2)
            cancel();
    }

public:
    /**
     *  Constructor
     *  @param  loop        Event loop
     *  @param  initial     Initial timeout
     *  @param  interval    Timeout interval period
     *  @param  callback    Function that is called when timer is expired
     */
    SharedSynchronizeWatcher(Loop *loop, const SynchronizeCallback &callback) : Shared(this), SynchronizeWatcher(loop, callback) {}
    
    /**
     *  Destructor
     */
    virtual ~SharedSynchronizeWatcher() {
        // std::cout << "~SharedSynchronizeWatcher" << std::endl;
    }
    
    /**
     *  Start the timer
     *  @return bool
     */
    virtual bool start() override
    {
        // call base
        if (!SynchronizeWatcher::start()) return false;
        
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
        if (!SynchronizeWatcher::cancel()) return false;
        
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
