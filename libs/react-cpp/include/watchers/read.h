/**
 *  ReadWatcher.h
 *
 *  Class that represents an object that is busy watching a filedescriptor  
 *  for readability
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
class ReadWatcher : private Watcher
{
private:
    /**
     *  Pointer to the loop
     *  @var    Loop
     */
    Loop *_loop;

    /**
     *  IO resource
     *  @var    ev_io
     */
    struct ev_io _watcher;
    
    /**
     *  Callback function
     *  @var    ReadCallback
     */
    ReadCallback _callback;
    
    /**
     *  Is it active?
     *  @var    bool
     */
    bool _active = false;

    /**
     *  Function to initialize the io watcher
     *  @param  fd  file descriptor to watch
     */
    void initialize(int fd);

protected:
    /**
     *  Call the reader (which in turn will call the handler)
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
     *  Constructor
     *  @param  loop        The event loop
     *  @param  fd          File descriptor
     *  @param  callback    Function called when filedescriptor becomes readable
     */
    template <typename CALLBACK>
    ReadWatcher(Loop *loop, int fd, const CALLBACK &callback) : 
        _loop(loop), _callback(callback)
    {
        // store pointer to current object
        _watcher.data = this;
        
        // initialize the watcher
        initialize(fd);
        
        // start (resume) the watcher
        resume();
    }

    /**
     *  No copying or moving allowed
     *  @param  that
     */
    ReadWatcher(const ReadWatcher &that) = delete;
    ReadWatcher(ReadWatcher &&that) = delete;

    /**
     *  Destructor
     */
    virtual ~ReadWatcher()
    {
        cancel();
    }

    /**
     *  No copying or moving
     *  @param  that
     */
    ReadWatcher &operator=(const ReadWatcher &that) = delete;
    ReadWatcher &operator=(ReadWatcher &&that) = delete;
    
    /**
     *  Cancel the watcher
     *  @return bool
     */
    virtual bool cancel()
    {
        // must be active
        if (!_active) return false;
        
        // stop watcher
        ev_io_stop(*_loop, &_watcher);
        
        // no longer active
        _active = false;
        
        // done
        return true;
    }
    
    /**
     *  Start/resume the watcher
     *  @return bool
     */
    virtual bool resume()
    {
        // should not be active
        if (_active) return false;
        
        // start it
        ev_io_start(*_loop, &_watcher);
        
        // done
        return _active = true;
    }
};

/**
 *  End namespace
 */
}

