/**
 *  IpRecord.h
 *
 *  Class holding an IP record retrieved from DNS. This can be either a
 *  IPv4 or an IPv6 record
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
class IpRecord
{
private:
    /**
     *  IP address
     *  @var    Ip
     */
    Net::Ip _ip;
    
    /**
     *  Time to live
     *  @var    int
     */
    int _ttl;

public:
    /**
     *  Constructor
     * 
     *  You normally don't construct IpRecord objects yourself, but retrieve
     *  them with a call to Resolver::ips().
     * 
     *  @param  addr    address and time-to-live
     */
    IpRecord(struct ares_addrttl *addr) : _ip(addr->ipaddr), _ttl(addr->ttl) {}

    /**
     *  Constructor
     * 
     *  You normally don't construct IpRecord objects yourself, but retrieve
     *  them with a call to Resolver::ips().
     * 
     *  @param  addr    address and time-to-live
     */
    IpRecord(struct ares_addr6ttl *addr) : _ip(addr->ip6addr), _ttl(addr->ttl) {}
    
    /**
     *  Destructor
     */
    virtual ~IpRecord() {}
    
    /**
     *  Retrieve the IP
     *  @return Ip
     */
    const Net::Ip &ip() const
    {
        return _ip;
    }
    
    /**
     *  Retrieve the TTL
     *  @return int
     */
    int ttl() const
    {
        return _ttl;
    }
    
    /**
     *  Compare two objects
     *  @param  that    address to compare to
     *  @return bool
     */
    bool operator==(const IpRecord &that) const
    {
        return _ip == that._ip && _ttl == that._ttl;
    }

    /**
     *  Compare two objects
     *  @param  that    address to compare to
     *  @return bool
     */
    bool operator!=(const IpRecord &that) const
    {
        return _ip != that._ip || _ttl != that._ttl;
    }
    
    /**
     *  Compare two objects
     *  @param  that    address to compare to
     *  @return bool
     */
    bool operator<(const IpRecord &that) const
    {
        return _ip == that._ip ? _ttl < that._ttl : _ip < that._ip;
    }

    /**
     *  Compare two objects
     *  @param  that    address to compare to
     *  @return bool
     */
    bool operator>(const IpRecord &that) const
    {
        return _ip == that._ip ? _ttl > that._ttl : _ip > that._ip;
    }
};

/**
 *  Function to write an IP to a stream
 *  @param  os  output stream to write to
 *  @param  ip  ip address to write
 *  @return ostream
 */
inline std::ostream &operator<<(std::ostream &os, const React::Dns::IpRecord &ip)
{
    os << ip.ip();
    return os;
}

/**
 *  End namespace
 */
}}

