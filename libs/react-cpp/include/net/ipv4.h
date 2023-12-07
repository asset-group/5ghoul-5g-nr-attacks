/**
 *  Ipv4.h
 *
 *  An IPv4 address
 *
 *  @copyright 2014 Copernica BV
 */

#include <netinet/in.h>

/**
 *  Namespace
 */
namespace React { namespace Net {

/**
 *  Class definition
 */
class Ipv4
{
private:
    /**
     *  The actual address
     *  @var struct in_addr
     */
    struct in_addr _addr;


public:
    /**
     *  Empty constructor, creates a default 0.0.0.0 address
     */
    Ipv4()
    {
        // set to all zero's
        _addr.s_addr = 0;
    }

    /**
     *  Copy constructor
     *  @param  ip      Address that is copied
     */
    Ipv4(const Ipv4 &ip)
    {
        // copy the other address
        _addr.s_addr = ip._addr.s_addr;
    }

    /**
     *  Move constructor
     *  @param  ip      Address that is moved
     */
    Ipv4(Ipv4 &&ip)
    {
        // copy the other address
        _addr.s_addr = ip._addr.s_addr;
    }

    /**
     *  Assign an IP address based on a string representation of an IP
     *  address. This should be a string in the 1.2.3.4 format.
     *
     *  @param  ip      Address that will be parsed
     */
    Ipv4(const char *ip)
    {
        // set to all zero's
        _addr.s_addr = 0;

        // parse the address
        if (inet_pton(AF_INET, ip, &_addr) > 0) return;

        // not a valid address
        _addr.s_addr = 0;
    }

    /**
     *  Assign an IP address based on a string representation of an IP
     *  address. This should be a string in the 1.2.3.4 format.
     *
     *  @param  ip      Address that will be parsed
     */
    Ipv4(const std::string &ip)
    {
        // set to all zero's
        _addr.s_addr = 0;

        // parse the address
        if (inet_pton(AF_INET, ip.c_str(), &_addr) > 0) return;

        // not a valid address
        _addr.s_addr = 0;
    }

    /**
     *  Constructor that accepts an ipv4 address encoded as an unsigned 32 bit integer
     *
     *  @param  ip      IP address
     */
    Ipv4(uint32_t ip)
    {
        _addr.s_addr = htonl(ip);
    }

    /**
     *  Constructor with struct in_addr information
     *
     *  @param  ip      IP address
     */
    Ipv4(const struct in_addr ip)
    {
        // copy the other address
        _addr.s_addr = ip.s_addr;
    }

    /**
     *  Constructor with pointer to a struct in_addr
     *
     *  @param  ip      Ip address pointer
     */
    Ipv4(const struct in_addr *ip)
    {
        // copy the other address
        _addr.s_addr = ip->s_addr;
    }

    /**
     *  Destructor
     */
    virtual ~Ipv4() {}

    /**
     *  Pointer to the internal address
     *  @return struct in_addr*
     */
    const struct in_addr *internal() const
    {
        return &_addr;
    }

    /**
     *  Does the object represent a valid IP address?
     *  @return bool
     */
    bool valid() const
    {
        return _addr.s_addr != 0;
    }

    /**
     *  Assign a different IP address to this object
     *  @param  address The other address
     *  @return Ipv4
     */
    Ipv4 &operator=(const Ipv4 &address)
    {
        if (&address == this) return *this;
        _addr.s_addr = address._addr.s_addr;
        return *this;
    }

    /**
     *  Compare two IP addresses
     *  @param  address The address to compare
     *  @return bool
     */
    bool operator==(const Ipv4 &address) const
    {
        return _addr.s_addr == address._addr.s_addr;
    }

    /**
     *  Compare two IP addresses
     *  @param  address The address to compare
     *  @return bool
     */
    bool operator!=(const Ipv4 &address) const
    {
        return !operator==(address);
    }

    /**
     *  Compare two IP addresses
     *  @param  address The address to compare
     *  @return bool
     */
    bool operator<(const Ipv4 &address) const
    {
        return _addr.s_addr < address._addr.s_addr;
    }

    /**
     *  Compare two IP addresses
     *  @param  address The address to compare
     *  @return bool
     */
    bool operator>(const Ipv4 &address) const
    {
        return _addr.s_addr > address._addr.s_addr;
    }

    /**
     *  String representation of the address
     *  @return string
     */
    const std::string toString() const
    {
        // not valid?
        if (!valid()) return std::string("0.0.0.0");

        // construct a buffer
        char buffer[INET_ADDRSTRLEN];

        // convert
        inet_ntop(AF_INET, &_addr, buffer, INET_ADDRSTRLEN);

        // done
        return std::string(buffer);
    }
};

/**
 *  End namespace
 */
}}

