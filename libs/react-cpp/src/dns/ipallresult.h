/**
 *  IpAllResult.h
 *
 *  Implementation-only class that processes the results of a call to fetch
 *  all Ips.
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
class IpAllResult : public IpResult
{
private:
    /**
     *  Number of pending calls
     *  @var    int
     */
    int _pending = 0;
    
    /**
     *  The error status code that we have received
     *  @var    int
     */
    const char *_error;
    
public:
    /**
     *  Update pending count, and return the new value
     *  @param  change  number to add to existing pending operations
     *  @return value
     */
    int pending(int change = 0)
    {
        return _pending += change;
    }
    
    /**
     *  Set the error code
     *  @var    error
     */
    void setError(const char *error)
    {
        _error = error;
    }
    
    /**
     *  Retrieve an error string
     *  @return const char *
     */
    const char *error() const
    {
        return size() > 0 ? nullptr : _error;
    }
};
    
/**
 *  End namespace
 */
}}

