/**
 *  Address.h
 *
 *  Extension to the Net::Address class that can be directly applied to the
 *  socket and that will fetch the local socket address, or the peer address
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
class Address : public Net::Address
{
protected:
    /**
     *  Helper function that instantiates a Net::Address
     *  @param  fd      Filedescriptor
     *  @return Net::Address
     */
    template <typename FUNCTION>
    Net::Address instantiate(const FUNCTION &function, int fd) const
    {
        // create a union that holds all possible values
        union {
            struct sockaddr s;
            struct sockaddr_in v4;
            struct sockaddr_in6 v6;
        } u;
        
        // size of the buffer
        socklen_t slen = sizeof(u); 
     
        // check for success
        if (function(fd, &u.s, &slen) == 0)
        {
            // check family
            if (u.s.sa_family == AF_INET6) return Net::Address(Net::Ip(u.v6.sin6_addr), ntohs(u.v6.sin6_port));
            if (u.s.sa_family == AF_INET) return Net::Address(Net::Ip(u.v4.sin_addr), ntohs(u.v4.sin_port));

            // basically a hack because Net::Address doesn't support unix domain address
            if (u.s.sa_family == AF_LOCAL) return Net::Address(Net::Ip("127.0.0.1"), 0);
        }
    
        // default invalid address
        return Net::Address();
    }

    /**
     *  Constructor
     *  @param  fd      Filedescriptor
     */
    template <typename FUNCTION>
    Address(const FUNCTION &function, int fd) : Net::Address(instantiate(function, fd)) {}
    
public:    
    /**
     *  Destructor
     */
    virtual ~Address() {}
};

/**
 *  End namespace
 */
}}

