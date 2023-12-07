/**
 *  Address.h
 *
 *  A network address is the combination of an IP and a port number
 *
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React { namespace Net {

/**
 *  Class definition
 */
class Address
{
private:
    /**
     *  The IP address
     *  @var    Ip
     */
    Ip _ip;
    
    /**
     *  The port number
     *  @var    uint16_t
     */
    uint16_t _port = 0;

public:
    /**
     *  Construct a default (invalid) address
     */
    Address() {}

    /**
     *  Constructor
     *  @param  ip          IP address
     *  @param  port        Port number
     */
    Address(const Ip &ip, uint16_t port) : _ip(ip), _port(port) {}
    
    /**
     *  Destructor
     */
    virtual ~Address() {}
    
    /**
     *  Retrieve the IP
     *  @return Ip
     */
    const Ip &ip() const
    {
        return _ip;
    }
    
    /**
     *  Retrieve the port
     *  @return uint16_t
     */
    uint16_t port() const
    {
        return _port;
    }
    
    /**
     *  Is this a valid address?
     *  @return bool
     */
    bool valid() const
    {
        return _ip.valid();
    }
    
    /**
     *  Convert the object to a string
     *  @return string
     */
    std::string toString() const
    {
        switch (_ip.version()) {
        case 4: return _ip.toString()+":"+std::to_string(_port);
        case 6: return "["+_ip.toString()+"]:"+std::to_string(_port);
        default:return std::string();
        }
    }
};

/**
 *  Function to write an IP to a stream
 *  @param  os
 *  @param  ip
 *  @return ostream
 */
inline std::ostream &operator<<(std::ostream &os, const Address &address)
{
    os << address.toString();
    return os;
}

/**
 *  End namespace
 */
}}


