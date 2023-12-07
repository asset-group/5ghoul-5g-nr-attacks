/**
 *  ReadPipe.h
 *
 *  A pipe that can be used from an event loop for non-blocking reading
 *  from the pipe. The other end of the pipe (the writable part)
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
class ReadPipe : public Pipe
{
protected:
    /**
     *  The filedescriptor for readability
     *  @var Fd
     */
    Fd _read;
    
public:
    /**
     *  Constructor
     *  @param  loop        The event loop
     *  @param  flags       Optional fcntl() flags
     */
    ReadPipe(Loop *loop, int flags = 0) : 
        Pipe(flags),
        _read(loop, _fds[0], O_NONBLOCK) {}
    
    /**
     *  Destructor
     */
    virtual ~ReadPipe() {}
    
    /**
     *  Retrieve the file descriptor for reading
     *
     *  @return The file descriptor
     */
    Fd &read()
    {
        // return the Fd object
        return _read;
    }
    
    /**
     *  Register a handler for readability
     *
     *  Note that if you had already registered a handler before, then that one
     *  will be reset. Only your new handler will be called when the filedescriptor
     *  becomes readable
     *
     *  @param  callback    The callback to invoke when data is available to be read
     *  @return Watcher object to cancel reading
     */
    std::shared_ptr<ReadWatcher> onReadable(const ReadCallback &callback)
    {
        // pass on to the filedescriptor
        return _read.onReadable(callback);
    }
};

/**
 *  End of namespace
 */
}

