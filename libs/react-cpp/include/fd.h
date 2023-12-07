/**
 *  Base.h
 *
 *  Base interface/class for all classes that are wrapped around a filedescriptor
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
class Fd
{
protected:
    /**
     *  The event loop to which it is connected
     *  @var    Loop
     */
    Loop *_loop;

    /**
     *  The filedescriptor
     *  @var    int
     */
    int _fd;

private:
    /**
     *  Watcher for readability
     *  @var    ReadWatcher
     */
    std::weak_ptr<ReadWatcher> _reader;

    /**
     *  Watcher for writability
     *  @var    WriteWatcher
     */
    std::weak_ptr<WriteWatcher> _writer;

public:
    /**
     *  Constructor
     *  @param  loop        event loop
     *  @param  fd          the file descriptor
     *  @param  flags       extra flags to set on the filedescriptor
     */
    Fd(Loop *loop, int fd, int flags) : _loop(loop), _fd(fd) 
    {
        // add flags
        fcntl(fd, F_SETFL, flags);
    }

    /**
     *  Constructor
     *  @param  loop        event loop
     *  @param  fd          the file descriptor
     */
    Fd(Loop *loop, int fd) : _loop(loop), _fd(fd) {}

    /**
     *  Deleted copy constructor
     *  @param  fd
     */
    Fd(const Fd &fd) = delete;

    /**
     *  Move constructor
     *  @param  fd
     */
    Fd(Fd &&fd) : _loop(fd._loop), _fd(fd._fd),
        _reader(std::move(fd._reader)), _writer(std::move(fd._writer))
    {
        // other fd is invalid
        fd._fd = -1;
    }

    /**
     *  Destructor
     */
    virtual ~Fd()
    {
        // no longer interested in read or write events
        if (!_reader.expired()) _reader.lock()->cancel();
        if (!_writer.expired()) _writer.lock()->cancel();
    }

    /**
     *  Retrieve the internal filedescriptor
     *  @return int
     */
    int fd() const
    {
        return _fd;
    }

    /**
     *  Register a handler for readability
     *
     *  Note that if you had already registered a handler before, then that one
     *  will be reset. Only your new handler will be called when the filedescriptor
     *  becomes readable
     *
     *  @param  callback
     *  @return ReadWatcher
     */
    template <typename CALLBACK>
    std::shared_ptr<ReadWatcher> onReadable(const CALLBACK &callback)
    {
        // cancel the current watcher (if we have one)
        if (!_reader.expired()) _reader.lock()->cancel();

        // and set up a new one
        auto watcher = _loop->onReadable(_fd, callback);

        // we keep a weak reference to it
        _reader = watcher;

        // done
        return watcher;
    }

    /**
     *  Register a handler for writability
     *
     *  Note that if you had already registered a handler before, then that one
     *  will be reset. Only your new handler will be called when the filedescriptor
     *  becomes writable
     *
     *  @param  callback
     *  @return WriteWatcher
     */
    template <typename CALLBACK>
    std::shared_ptr<WriteWatcher> onWritable(const CALLBACK &callback)
    {
        // cancel the current watcher (if we have one)
        if (!_writer.expired()) _writer.lock()->cancel();

        // and set up a new one
        auto watcher = _loop->onWritable(_fd, callback);

        // we keep a weak reference to it
        _writer = watcher;

        // done
        return watcher;
    }
};

/**
 *  End namespace
 */
}
