/**
 *  Out.h
 *
 *  Smart output buffer that can be wrapped around a connection, and that
 *  offers alternative send() and close() methods that will buffer any
 *  data, so that you no longer have to check the return value of the send
 *  method yourself
 *
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React { namespace Tcp {

/**
 *  Class definition
 */
class Out
{
private:
    /**
     *  The underlying TCP connection
     *  @var    Connection
     */
    Connection *_connection;

    /**
     *  Buffer with data that still needs to be sent
     *  @var    Buffer
     */
    Buffer _buffer;

    /**
     *  Is the connection active?
     *  @var    bool
     */
    enum {
        status_active,
        status_closing,
        status_closed
    } _status = status_active;

    /**
     *  Callback that is called when connection is closed
     *  @var    CloseCallback
     */
    CloseCallback _closeCallback;

    /**
     *  Callback that is called when connection is really writable (and no longer buffered)
     *  @var    WriteCallback
     */
    WriteCallback _writeCallback;

    /**
     *  Install a handler
     */
    void checkWritable()
    {
        // wait for the connection to become writable once again
        _connection->onWritable([this]() -> bool {

            // do we still have a buffer?
            if (_buffer.size() > 0)
            {
                // send more data to the buffer
                ssize_t result = _connection->writev(_buffer.iovec(), _buffer.count());

                // forget errors
                if (result < 0) result = 0;

                // shrink the buffer
                _buffer.shrink(result);

                // if there still is a buffer, we want more notifications, also if
                // we want to close the connection, or when the user is interested
                // in write-callbacks
                return _buffer.size() > 0 || _status == status_closing || _writeCallback;
            }
            else if (_status == status_closing)
            {
                // close the tcp connection
                _connection->close();

                // copy the close callback (because it might destruct the object)
                auto callback = _closeCallback;

                // reset the connection
                reset();

                // notify the callback
                if (callback) callback();

                // no further write events
                return false;
            }
            else if (_writeCallback)
            {
                // remember the callback, because calling it might destruct the object
                auto callback = _writeCallback;

                // reset callback
                _writeCallback = nullptr;

                // inform the user that the connection is writable
                if (!callback()) return false;

                // the user wants more write events, register the callback again
                _writeCallback = callback;

                // we want more
                return true;
            }
            else
            {
                // no further write events are necessary
                return false;
            }
        });
    }

    /**
     *  Reset the connection
     */
    void reset()
    {
        // unregister any callbacks we've had
        _connection->onWritable(nullptr);

        // empty buffer
        _buffer.clear();

        // forget the callbacks
        _closeCallback = nullptr;
        _writeCallback = nullptr;

        // change status
        _status = status_closed;
    }

public:
    /**
     *  Constructor
     *
     *  This wraps the class around a connection object
     *
     *  @param  connection
     */
    Out(Connection *connection) : _connection(connection) {}

    /**
     *  Destructor
     */
    virtual ~Out()
    {
        // forget the onWritable handler
        _connection->onWritable(nullptr);
    }

    /**
     *  Install a handler for writability
     *
     *  Although the send() method automatically ensures that all data is
     *  delivered, and will do its own buffering, you may still want to be
     *  updated when the connection is really writable, and no buffering
     *  will be done. For that you can install a write handler. This will
     *  only be called when there is absolutely no buffer
     *
     *  @param  callback
     */
    void onWritable(const WriteCallback &callback)
    {
        // not possible when not active
        if (_status != status_active) return;

        // copy the callback
        _writeCallback = callback;

        // check for writability
        checkWritable();
    }

    /**
     *  Send data to the buffer
     *
     *  This will send out the data, or buffer it internally if it could not
     *  immediately be sent. It returns the number of bytes sent / or that are
     *  going to be sent. If it returns zero, it means that the connection is
     *  going to be closed, or is already closed, and that sending data to it
     *  is meaningless.
     *
     *  @param  buffer      Data to send
     *  @param  size        Size of the data
     *  @return             Number of bytes virtually sent
     */
    size_t send(const void *data, size_t size)
    {
        // impossible when no longer active
        if (_status != status_active) return 0;

        // do we already have a buffer?
        if (_buffer.size() > 0) return _buffer.add(data, size);

        // try sending it to the connection
        ssize_t result = _connection->send(data, size);
        if (result >= 0 && (size_t)result >= size) return result;

        // check if socket is in an error sate
        if (result < 0 && errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
        {
            // remember that the connection is no longer valid
            reset();

            // nothing was sent
            return 0;
        }
        else
        {
            // update number of bytes sent, to forget the error
            if (result < 0) result = 0;

            // add remaining bytes to buffer
            _buffer.add((const char*)data + result, size - result);

            // check for writability
            checkWritable();

            // done
            return size;
        }
    }

    /**
     *  Close the connection
     *
     *  This will either immediately close the connection, or mark the
     *  connection as being closed and close it after all buffers were sent.
     *
     *  The callback is called when the connection is fully closed. Returns false
     *  on failure (in which the callback will not be called) and true on success
     *  (and the callback will be called when the operation is finished)
     *
     *  @return bool
     */
    bool close(const CloseCallback &callback = nullptr)
    {
        // skip if not active
        if (_status != status_active) return false;

        // remember the callback and that we're closing
        _closeCallback = callback;

        // remember that we're closing
        _status = status_closing;

        // check for writability
        checkWritable();

        // done
        return true;
    }
};

/**
 *  End namespace
 */
}}

