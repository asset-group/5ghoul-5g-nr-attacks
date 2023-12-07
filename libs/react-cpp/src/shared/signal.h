/**
 *  SharedSignalWatcher.h
 *
 *  SignalWatcher watcher that is managed by the loop, and that can be shared with the
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
class SharedSignalWatcher : public Shared<SignalWatcher>, public SignalWatcher
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
        SignalWatcher::invoke();
    }

public:
    /**
     *  Constructor
     *  @param  loop        Event loop
     *  @param  signum      The signal to watch
     *  @param  callback    Function that is called when timer is expired
     */
    SharedSignalWatcher(MainLoop *loop, int signum, const SignalCallback &callback) : Shared(this), SignalWatcher(loop, signum, callback) {}

    /**
     *  Destructor
     */
    virtual ~SharedSignalWatcher() {}

    /**
     *  Start the signal watcher
     *  @return bool
     */
    virtual bool start() override
    {
        // call base
        if (!SignalWatcher::start()) return false;

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
        if (!SignalWatcher::cancel()) return false;

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
