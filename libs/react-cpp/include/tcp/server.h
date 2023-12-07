/**
 *  Server.h
 *
 *  A TCP server, that accepts incoming connections
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
class Server
{
private:
    /**
     *  We need a socket
     *  @var    Socket
     */
    Socket _socket;

    /**
     *  The Connection class is a friend
     */
    friend class Connection;

public:
    /**
     *  Constructor to listen to a specific port on a specific IP
     *  @param  loop        Event loop
     *  @param  ip          IP address to listen to
     *  @param  port        Port number to listen to
     */
    Server(Loop *loop, const Net::Ip &ip, uint16_t port) :
        _socket(loop, ip, port)
    {
        // listen to the socket
        if (!_socket.listen()) throw Exception(strerror(errno));
    }

    /**
     *  Constructor to listen to a random port on a specific IP
     *  @param  loop        Event loop
     *  @param  ip          IP address to listen to
     */
    Server(Loop *loop, const Net::Ip &ip) :
        Server(loop, ip, 0) {}

    /**
     *  Constructor to listen to a specific port
     *  @param  loop        Event loop
     *  @param  port        Port number to listen to
     */
    Server(Loop *loop, uint16_t port) :
        Server(loop, Net::Ip(), port) {}

    /**
     *  Constructor to listen on a unix domain socket
     *
     *  @param  loop        Event loop
     *  @param  path        Path for the socket
     */
    Server(Loop *loop, const char *path) :
        _socket(loop, path)
    {
        // listen to the socket
        if (!_socket.listen()) throw Exception(strerror(errno));
    }

    /**
     *  Constructor to listen to a random port
     *  @param  loop        Event loop
     */
    Server(Loop *loop) :
        Server(loop, Net::Ip(), 0) {}

    /**
     *  Destructor
     */
    virtual ~Server() {}

    /**
     *  Install connect handler
     *  Your method is called every time that a connection can be accepted. The
     *  previous handler will be overwritten
     *  @param  callback
     */
    void onConnect(const ReadCallback &callback)
    {
        // install in socket
        _socket.onReadable(callback);
    }

    /**
     *  Retrieve the address to which the server is listening
     *  @return Net::Address
     */
    Net::Address address() const
    {
        // return the socket address
        return SocketAddress(_socket);
    }

    /**
     *  Retrieve the IP address to which it is listening
     *  @return Net::Ip
     */
    Net::Ip ip() const
    {
        // fetch the ip from the socket address
        return SocketAddress(_socket).ip();
    }

    /**
     *  Retrieve the port number to which the server is listening
     *  @return uint16_t
     */
    uint16_t port() const
    {
        // fetch the port number from the socket address
        return SocketAddress(_socket).port();
    }
};

/**
 *  End namespace
 */
}}
