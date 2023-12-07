/**
 *  MxRequest.h
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
class MxRequest : public Request
{
private:
    /**
     *  The callback to be called
     *  @var    IpsCallback
     */
    MxCallback _callback;

public:
    /**
     *  Constructor
     *  @param  resolver    the resolver object
     *  @param  callback    the callback to invoke on completion
     */
    MxRequest(Base *resolver, const MxCallback &callback) : 
        Request(resolver), _callback(callback) {}
    
    /**
     *  Destructor
     */
    virtual ~MxRequest() {}
    
    /**
     *  Invoke the callback
     *  @param  result          The result to report
     *  @param  error           Optional error message
     */
    void invoke(MxResult &&result, const char *error)
    {
        _callback(std::move(result), error);
    }
};
    
/**
 *  End namespace
 */
}}
