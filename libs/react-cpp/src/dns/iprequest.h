/**
 *  IpRequest.h
 *
 *  Class that contains all information for an IPv4 request
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
class IpRequest : public Request
{
private:
    /**
     *  The callback to be called
     *  @var    IpsCallback
     */
    IpCallback _callback;

public:
    /**
     *  Constructor
     *  @param  resolver    the resolver object
     *  @param  callback    the callback to invoke on completion
     */
    IpRequest(Base *resolver, const IpCallback &callback) : 
        Request(resolver), _callback(callback) {}
    
    /**
     *  Destructor
     */
    virtual ~IpRequest() {}
    
    /**
     *  Invoke the callback
     *  @param  result          The result to report
     *  @param  error           Optional error message
     */
    void invoke(IpResult &&result, const char *error)
    {
        // execute the callback
        _callback(std::move(result), error);
    }
};
    
/**
 *  End namespace
 */
}}
