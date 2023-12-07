/**
 *  PeerAddress.h
 *
 *  Extension to the Net::Address class that can be directly applied to the
 *  socket and that will fetch the peer address
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
class PeerAddress : public Address
{
public:    
    /**
     *  Constructor
     *  @param  fd      Filedescriptor
     */
    PeerAddress(int fd) : Address(getpeername, fd) {}
    
    /**
     *  Constructor
     *  @param  socket  The socket
     */
    PeerAddress(const Fd &socket) : PeerAddress(socket.fd()) {}
    
    /**
     *  Constructor
     *  @param  socket  The socket
     */
    PeerAddress(const Fd *socket) : PeerAddress(socket->fd()) {}
    
    /**
     *  Destructor
     */
    virtual ~PeerAddress() {}
};

/**
 *  End namespace
 */
}}

