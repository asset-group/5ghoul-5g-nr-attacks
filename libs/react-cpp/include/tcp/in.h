/**
 *  In.h
 *
 *  Input buffer for socket connections that will retrieve
 *  data from a socket.
 *
 *  @copyright 2014 Copernica BV
 */

/**
  * Set up namespace
  */
namespace React { namespace Tcp {

/**
 *  Class definition
 */
template <size_t SIZE = 1536>
class In
{
    /**
     *  Buffer for incoming data
     */
    char _buffer[SIZE];

    /**
     *  The amount of bytes currently in the buffer
     */
    size_t _size = 0;

    /**
     *  The underlying TCP connectino
     */
    Connection *_connection;

    /**
     *  Callback to execute when the connection drops
     */
    LostCallback _lostCallback;

    /**
     *  The callback to inform once data arrives
     */
    DataCallback _readCallback;

    /**
     *  We have lost the connection
     *
     *  Callbacks may have captured variables
     *  which need to be destroyed.
     */
    void lost()
    {
        // there might be some data left in the buffer
        if (_size && _readCallback) _readCallback(_buffer, _size);

        // unregister any callbacks we've had
        _connection->onReadable(nullptr);

        // remove the data callback
        _readCallback = nullptr;

        // copy the lost callback (if it is there)
        if (_lostCallback)
        {
            // copy and remove
            auto callback = _lostCallback;
            _lostCallback = nullptr;

            // execute the callback
            callback();
        }
    }

public:
    /**
     *  Constructor
     *
     *  Wraps the class around the connection
     *
     *  @param  connection  the connection to wrap around
     */
    In(Connection *connection) : _connection(connection) {}

    /**
     *  Check if the connection is lost
     *
     *  @param  callback
     */
    void onLost(const LostCallback &callback)
    {
        // install the callback
        _lostCallback = callback;
    }

    /**
     *  Check for data to come in
     *
     *  This method is called for all data that comes in via the connection.
     *  If you set a data hander, the onReadable handler that you've set before
     *  will be overridden.
     *
     *  @param  callback
     */
    void onData(const DataCallback &callback)
    {
        // store the data callback
        _readCallback = callback;

        // install a readability handler
        _connection->onReadable([this]() -> bool {

            // is there any data remaining in the buffer
            if (_size)
            {
                // send to the callback immediately
                _readCallback(_buffer, _size);
                _size = 0;
            }

            // receive the data
            ssize_t bytes = _connection->recv(_buffer, SIZE);

            // check for error
            if (bytes == 0 || (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
            {
                // connection was lost
                lost();

                // we know enough
                return false;
            }
            else
            {
                // should be zero or more
                if (bytes <= 0) return true;

                // report
                return _readCallback(_buffer, bytes);
            }
        });
    }

    /**
     *  Check for lines to come in
     *
     *  This method is called for every line that comes in via the connection.
     *  If you set a data hander, the onReadable handler that you've set before
     *  will be overridden.
     *
     *  @param  callback
     */
    void onLine(const DataCallback &callback)
    {
        // store the data callback
        _readCallback = callback;

        // install a readability handler
        _connection->onReadable([this]() -> bool {

            // receive the data
            ssize_t bytes = _connection->recv(_buffer + _size, SIZE - _size);

            // check for error
            if (bytes == 0 || (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
            {
                // connection was lost
                lost();

                // we know enough
                return false;
            }
            else
            {
                // should be zero or more
                if (bytes <= 0) return true;

                // increment number of bytes in the buffer
                _size += bytes;

                // are we overflowing with data
                if (_size == SIZE)
                {
                    // give all data to the callback
                    _readCallback(_buffer, SIZE);

                    // no more data remains
                    _size = 0;

                    // but we want to keep listening
                    return true;
                }

                // if the callback returns false just once we want to stop listening
                // but we do want to get all data out before the listener is stopped
                bool listen = true;

                // the starting point of each chunk
                char *start;

                // process all lines in the data
                for (start = _buffer; auto *newline = (char*)std::memchr(start, '\r', _size + _buffer - start); start = newline + 2)
                {
                    // execute the callback
                    listen &= _readCallback(start, newline - start);
                }

                // any data left?
                if (start < _buffer + _size)
                {
                    // update number of bytes in buffer
                    _size += _buffer - start;

                    //  move the remaining bytes to the beginning
                    std::memmove(_buffer, start, _size);
                }

                // do we want to keep listening?
                return listen;
            }
        });
    }
};

/**
 *  End namespace
 */
}}
