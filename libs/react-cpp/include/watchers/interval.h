/**
 *  Interval.h
 *
 *  Timer that fires periodically
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
class IntervalWatcher : private Watcher
{
private:
    /**
     *  Pointer to the loop
     *  @var    Loop
     */
    Loop *_loop;

    /**
     *  IO resource
     *  @var    ev_timer
     */
    struct ev_timer _watcher;

    /**
     *  Callback function
     *  @var    IntervalCallback
     */
    IntervalCallback _callback;

    /**
     *  Is the timer active?
     *  @var    bool
     */
    bool _active = false;

    /**
     *  Initialize the object
     *  @param  initial     time to first invocation
     *  @param  interval    time between invocations
     */
    void initialize(Timestamp initial, Timestamp interval);

protected:
    /**
     *  Invoke the callback
     */
    virtual void invoke() override
    {
        // monitor ourselves
        Monitor monitor(this);

        // call the callback
        if (_callback() || !monitor.valid()) return;

        // cancel the watcher
        cancel();
    }

public:
    /**
     *  Current Interval
     *  @var    Timestamp
     */
    Timestamp current_interval;

    /**
     *  Constructor
     *  @param  loop        Event loop
     *  @param  initial     Initial timeout
     *  @param  interval    Timeout interval period
     *  @param  callback    Function that is called when timer is expired
     */
    template <typename CALLBACK>
    IntervalWatcher(Loop *loop, Timestamp initial, Timestamp interval, const CALLBACK &callback) :
        _loop(loop), _callback(callback)
    {
        // store pointer to current object
        _watcher.data = this;

        // initialize the watcher
        initialize(initial, interval);

        // start the timer
        start();
    }

    /**
     *  Simpler constructor
     *  @param  loop        Event loop
     *  @param  interval    Timeout interval period
     *  @param  callback    Function that is called when timer is expired
     */
    template <typename CALLBACK>
    IntervalWatcher(Loop *loop, Timestamp interval, const CALLBACK &callback) : IntervalWatcher(loop, interval, interval, callback) {}

    /**
     *  Constructor
     *
     *  This constructor is used to create an interval that is not
     *  initially started. After construction, the set() member
     *  function can be used to set a timeout and start
     */
    IntervalWatcher(Loop *loop, const IntervalCallback &callback) :
        _loop(loop), _callback(callback)
    {
        // store pointer to current object
        _watcher.data = this;

        // initialize the watcher
        initialize(0.0, 0.0);
    }

    /**
     *  No copying or moving allowed
     *  @param  that
     */
    IntervalWatcher(const IntervalWatcher &that) = delete;
    IntervalWatcher(IntervalWatcher &&that) = delete;

    /**
     *  Destructor
     */
    virtual ~IntervalWatcher()
    {
        // cancel the timer
        cancel();
    }

    /**
     *  No copying or moving
     *  @param  that
     */
    IntervalWatcher &operator=(const IntervalWatcher &that) = delete;
    IntervalWatcher &operator=(IntervalWatcher &&that) = delete;

    /**
     *  Start the timer
     *  @return bool
     */
    virtual bool start()
    {
        // not necessary if already running
        if (_active) return false;

        // start the timer
        ev_now_update(*_loop);
        ev_timer_start(*_loop, &_watcher);
        ev_async_send(*_loop, &_loop->async_wakeup);

        // remember that it is active
        return _active = true;
    }

    /**
     *  Cancel the timer
     *  @return bool
     */
    virtual bool cancel()
    {
        // not necessary if not already running
        if (!_active) return false;

        // stop the timer
        ev_timer_stop(*_loop, &_watcher);

        // remember that it is no longer active
        _active = false;

        // done
        return true;
    }

    /**
     *  Set the timer to a new time
     *  @param  initial     Initial timeout for the first timeout
     *  @param  interval    Interval for subsequent timeouts
     *  @return bool
     */
    bool set(Timestamp initial, Timestamp interval)
    {
        // cancel current timer
        cancel();

        // set the new timer
        ev_timer_set(&_watcher, initial, interval);

        // and start the timer
        return start();
    }

    /**
     *  Set the timer to a new time
     *  @param  timeout new time before invocation
     *  @return bool
     */
    bool set(Timestamp timeout)
    {
        // call other implementation
        return set(timeout, timeout);
    }

    /**
     *  Set the timer to a new time in milliseconds
     *  @param  timeout_ms new time before invocation
     *  @return bool
     */
    bool setMS(int timeout_ms)
    {
        Timestamp timeout = static_cast<Timestamp>(timeout_ms) / 1000.0;
        // call other implementation
        return set(timeout, timeout);
    }

};

/**
 *  End namespace
 */
}
