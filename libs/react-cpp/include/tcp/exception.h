/**
 *  Exception.h
 *
 *  Exception thrown by the Tcp module
 *
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React { namespace Tcp {

/**
 *  Class definition
 */
class Exception : public React::Exception
{
public:
    /**
     *  Constructor
     *  @param  message
     */
    Exception(const char *message) : React::Exception(message) {}
    
    /**
     *  Destructor
     */
    virtual ~Exception() {}
};

/**
 *  End namespace
 */
}}
