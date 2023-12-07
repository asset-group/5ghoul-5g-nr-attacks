/**
 *  WritePipe.h
 *
 *  A pipe that can be used from an event loop for non-blocking writing
 *  to the pipe. The other end of the pipe (the readable part)
 *  will be blocking.
 *
 *  @copyright 2015 Copernica B.V.
 */

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Class definition
 */
class WritePipe : public Pipe
{
protected:
    /**
     *  The filedescriptor for writability
     *  @var Fd
     */
    Fd _write;
    
public:
    /**
     *  Constructor
     *  @param  loop        The event loop
     */
    WritePipe(Loop *loop, int flags = 0) : 
        Pipe(flags), 
        _write(loop, _fds[1], O_NONBLOCK) {}
    
    /**
     *  Destructor
     */
    virtual ~WritePipe() {}
    
    /**
     *  Retrieve the file descriptor for writing
     *
     *  @return The file descriptor
     */
    Fd &write()
    {
        // return the Fd object
        return _write;
    }
    
    /**
     *  Register a handler for writability
     *
     *  Note that if you had already registered a handler before, then that one
     *  will be reset. Only your new handler will be called when the filedescriptor
     *  becomes writable
     *
     *  @param  callback    The callback to invoke when data can be sent
     *  @return Watcher object to cancel writing
     */
    std::shared_ptr<WriteWatcher> onWritable(const WriteCallback &callback)
    {
        // pass on to the filedescriptor
        return _write.onWritable(callback);
    }
};

/**
 *  End of namespace
 */
}

