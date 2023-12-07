/**
 *  Timeout.h
 *
 *  Timer that fires once
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
    class TimeoutWatcher : private Watcher {
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
     *  @var    TimeoutCallback
     */
        TimeoutCallback _callback;

        /**
     *  Is the timer active?
     *  @var    bool
     */
        bool _active = false;

        /**
     *  Initialize the object
     *  @param  timeout time until invocation
     */
        void initialize(Timestamp timeout);

    protected:
        /**
     *  Invoke the callback
     */
        virtual void invoke() override
        {
            // timer is no longer active
            cancel();

            // notify parent (return value is not important, a timer is always
            // cancelled after it expired)
            _callback();
        }

    public:
       /**
     *  When should it expired?
     *  @var    Timeout
     */
        Timestamp current_timeout;

        /**
     *  Constructor
     *  @param  loop        Event loop
     *  @param  timeout     Timeout period
     *  @param  callback    Function that is called when timer is expired
     */
        template <typename CALLBACK>
        TimeoutWatcher(Loop *loop, Timestamp timeout, const CALLBACK &callback) : _loop(loop), _callback(callback)
        {
            // store pointer to current object
            _watcher.data = this;

            // initialize the watcher
            initialize(timeout);

            // start the timer
            start();
        }

        /**
     *  Constructor
     *
     *  This constructor is used to create a timeout that is not
     *  initially started. After construction, the set() member
     *  function can be used to set a timeout and start
     */
        TimeoutWatcher(Loop *loop, const TimeoutCallback &callback) : _loop(loop), _callback(callback)
        {
            // store pointer to current object
            _watcher.data = this;

            // initialize the watcher
            initialize(0.0);
        }

        /**
     *  No copying or moving allowed
     *  @param  that
     */
        TimeoutWatcher(const TimeoutWatcher &that) = delete;
        TimeoutWatcher(TimeoutWatcher &&that) = delete;

        /**
     *  Destructor
     */
        virtual ~TimeoutWatcher()
        {
            // cancel the timer
            cancel();
        }

        /**
     *  No copying or moving
     *  @param  that
     */
        TimeoutWatcher &operator=(const TimeoutWatcher &that) = delete;
        TimeoutWatcher &operator=(TimeoutWatcher &&that) = delete;

        /**
     *  Start the timer
     *  @return bool
     */
        virtual bool start()
        {
            // skip if already running
            if (_active)
                return false;

            // start now
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
            // skip if not running
            if (!_active)
                return false;

            // stop now
            ev_timer_stop(*_loop, &_watcher);

            // remember that it no longer is active
            _active = false;

            // done
            return true;
        }

        /**
     *  Set the timer to a new time
     *  @param  timeout new time to invocation
     *  @return bool
     */
        bool set(Timestamp timeout)
        {
                // cancel the current time
                cancel();

                // set a new timer
                ev_timer_set(&_watcher, timeout, 0.0);

                // start the timer
                return start();
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
         return set(timeout);
      }
    };

    /**
 *  End namespace
 */
}
