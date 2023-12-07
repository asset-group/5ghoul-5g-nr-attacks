/**
 *  Channel.h
 *
 *  Wrapper class around the ares channel.
 *
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React { namespace Dns {

/**
 *  Class implementation
 */
class Channel
{
private:
    /**
     *  Pointer to the loop
     *  @var    Loop
     */
    Loop *_loop;

    /**
     *  Set of readers
     *  @var    std::map
     */
    std::map<int,std::unique_ptr<ReadWatcher>> _readers;

    /**
     *  Set of writers
     *  @var    std::map
     */
    std::map<int,std::unique_ptr<WriteWatcher>> _writers;

    /**
     *  The actual underlying channel
     */
    ares_channel _channel = nullptr;

    /**
     *  Timer
     *  @var    TimeoutWatcher
     */
    TimeoutWatcher _timer;
public:
    /**
     *  Constructor
     */
    Channel(Loop *loop);

    /**
     *  Destructor
     */
    virtual ~Channel();

    /**
     *  Set the timeout for the next iteration
     */
    void setTimeout();

    /**
     *  Check a certain filedescriptor for readability or writability
     *  @param  fd      Filedescriptor
     *  @param  read    Check for readability
     *  @param  write   Check for writability
     */
    void check(int fd, bool read, bool write);

    /**
     *  Check whether the channel is valid
     */
    operator bool ()
    {
        // valid when not a null pointer
        return _channel;
    }

    /**
     *  Cast to an ares channel
     */
    operator ares_channel ()
    {
        // return the channel
        return _channel;
    }
};

/**
 *  End namespace
 */
}}
