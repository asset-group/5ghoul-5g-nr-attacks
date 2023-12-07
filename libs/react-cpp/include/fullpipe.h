/**
 *  FullPipe.h
 *
 *  A pipe that can be used from an event loop for both non-blocking writing
 *  as well as non-blocking reading from the pipe.
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
class FullPipe : public Pipe
{
protected:
    /**
     *  The filedescriptor for readability
     *  @var Fd
     */
    Fd _read;

    /**
     *  The filedescriptor for writability
     *  @var Fd
     */
    Fd _write;
    
public:
    /**
     *  Constructor
     *  @param  readloop    The event loop for reading
     *  @param  writeloop   The event loop for writing
     *  @param  flags       Optional flags to pass to the pipe2() system call
     */
    FullPipe(Loop *readloop, Loop *writeloop, int flags = 0) : 
        Pipe(O_NONBLOCK | flags),
        _read(readloop, _fds[0]),
        _write(writeloop, _fds[1]) {}

    /**
     *  Constructor
     *  @param  loop        The event loop for reading and writing
     *  @param  flags       Optional flags to pass to the pipe2() system call
     */
    FullPipe(Loop *loop, int flags = 0) : 
        Pipe(O_NONBLOCK | flags),
        _read(loop, _fds[0]),
        _write(loop, _fds[1]) {}

    /**
     *  Destructor
     */
    virtual ~FullPipe() {}

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

