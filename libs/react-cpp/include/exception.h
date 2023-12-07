/**
 *  Exception.h
 *
 *  Exceptions thrown by the React library
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
class Exception : public std::runtime_error
{
public:
    /**
     *  Constructor
     *  @param  message
     */
    Exception(const char *message) : std::runtime_error(message) {}
    
    /**
     *  Destructor
     */
    virtual ~Exception() noexcept {}
};

/**
 *  End namespace
 */
}
