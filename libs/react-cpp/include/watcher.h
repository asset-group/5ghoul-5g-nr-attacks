/**
 *  Watcher.h
 *
 *  Internal base class for the watcher objects (timers, readers, writers, synchronizers)
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
class Watcher
{
protected:
    /**
     *  Internal class to monitor if a watcher still exists
     */
    class Monitor
    {
    private:
        /**
         *  The watcher being monitored
         *  @var    Watcher
         */
        Watcher *_watcher;

    public:
        /**
         *  Constructor
         *  @param  watcher the watcher to monitor for destruction
         */
        Monitor(Watcher *watcher) : _watcher(watcher)
        {
            // store monitor in watcher
            _watcher->_monitor = this;
        }

        /**
         *  Destructor
         */
        virtual ~Monitor()
        {
            if (_watcher) _watcher->_monitor = nullptr;
        }

        /**
         *  Check if the monitor is valid
         *  @return bool
         */
        bool valid()
        {
            return _watcher != nullptr;
        }

        /**
         *  Invalidate the object
         */
        void invalidate()
        {
            _watcher = nullptr;
        }
    };

    /**
     *  Pointer to the monitor now active
     *  @var    Monitor
     */
    Monitor *_monitor = nullptr;

public:
    /**
     *  Destructor
     */
    virtual ~Watcher()
    {
        if (_monitor) _monitor->invalidate();
    }

    /**
     *  Invoke the watcher
     */
    virtual void invoke() = 0;
};

/**
 *  End namespace
 */
}
