/**
 *  Cleanup.h
 *
 *  Cleanup watcherthat is managed by the loop that
 *  can be shared with the outside world.
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
class SharedCleanupWatcher : public Shared<CleanupWatcher>, public CleanupWatcher
{
private:
    /**
     *  Called before loop cleans up
     */
    virtual void invoke() override
    {
        // keep a shared pointer for as long as the callback is called, this
        // ensures that the object is not destructed if the user calls
        // the cancel() method, and immediately afterwards a different method
        auto ptr = pointer();

        // now we call the base invoke method
        CleanupWatcher::invoke();
    }

public:
    /**
     *  Constructor
     *  @param  loop        Event loop
     *  @param  callback    Function that is called before loop cleans up
     */
    SharedCleanupWatcher(Loop *loop, const CleanupCallback &callback) : Shared(this), CleanupWatcher(loop, callback) {}

    /**
     *  Destructor
     */
    virtual ~SharedCleanupWatcher() {}

    /**
     *  Cancel the watcher
     */
    virtual bool cancel() override
    {
        // call base
        if (!CleanupWatcher::cancel()) return false;

        // because we are no longer interested in the event loop, we no longer need the pointer
        reset();

        // done
        return true;
    }
};

/**
 *  End namespace
 */
}
