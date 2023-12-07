/**
 *  Base.h
 *
 *  Inaccessible base class of the resolver that is exclusively used internally
 *
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React { namespace Dns {

/**
 *  Class definition
 */
class Base
{
protected:
    /**
     *  Constructor
     *  @var    Loop
     */
    Base(Loop *loop);

    /**
     *  Pointer to the loop
     *  @var    Loop
     */
    Loop *_loop;

    /**
     *  The ares channel
     */
    std::shared_ptr<Channel> _channel;

public:
    /**
     *  Destructor
     */
    virtual ~Base() {};

    /**
     *  Get access to the loop
     */
    Loop *loop()
    {
        return _loop;
    }

    /**
     *  Get access to the channel
     */
    std::shared_ptr<Channel> channel()
    {
        return _channel;
    }
};

/**
 *  End namespace
 */
}}

