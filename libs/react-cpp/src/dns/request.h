/**
 *  Request.h
 *
 *  Base class for requests
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
class Request
{
protected:
    /**
     *  Pointer to the resolver
     *  @var Base
     */
    Base *_resolver;

    /**
     *  Keep a copy of the channel pointer
     *  so that it stays in scope as long
     *  as the request is around
     */
    std::shared_ptr<Channel> _channel;

    /**
     *  Constructor
     *  @param  resolver    the resolver object
     */
    Request(Base *resolver) : _resolver(resolver), _channel(resolver->channel()) {}

public:
    /**
     *  Destructor
     */
    virtual ~Request() {}

    /**
     *  Schedule a delete of the request
     *
     *  This will delete the request in
     *  a later iteration of the event
     *  loop.
     */
    void free()
    {
        // schedule a timeout on the loop
        _resolver->loop()->onTimeout(0, [this]() { delete this; });
    }
};

/**
 *  End namespace
 */
}}

