/**
 *  SocketAddress.h
 *
 *  Extension to the Net::Address class that can be directly applied to the
 *  socket and that will fetch the local socket address
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
class SocketAddress : public Address
{
public:
    /**
     *  Constructor
     *  @param  fd      Filedescriptor
     */
    SocketAddress(int fd) : Address(getsockname, fd) {}
    
    /**
     *  Constructor
     *  @param  socket  The socket
     */
    SocketAddress(const Fd &socket) : SocketAddress(socket.fd()) {}
    
    /**
     *  Constructor
     *  @param  socket  The socket
     */
    SocketAddress(const Fd *socket) : SocketAddress(socket->fd()) {}
    
    /**
     *  Destructor
     */
    virtual ~SocketAddress() {}
};

/**
 *  End namespace
 */
}}

