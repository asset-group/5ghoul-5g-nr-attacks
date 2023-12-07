/**
 *  MxRecord.h
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
class MxRecord
{
private:
    /**
     *  Host name
     *  @var    std::string
     */
    std::string _hostname;
    
    /**
     *  Priority
     *  @var    int
     */
    int _priority = 0;
    
    /**
     *  Time to live
     *  @var    int
     */
    int _ttl = 600;

public:
    /**
     *  Constructor
     * 
     *  You normally don't construct MxRecord objects yourself, but retrieve
     *  them with a call to Resolver::mx().
     * 
     *  @param  hostname    the hostname that handles incoming mail
     *  @param  priority    the priority (lower gets priority)
     */
    MxRecord(const char *hostname, int priority) : _hostname(hostname), _priority(priority) {}

    /**
     *  Destructor
     */
    virtual ~MxRecord() {}
    
    /**
     *  Retrieve the hostname
     *  @return Ip
     */
    const std::string &hostname() const
    {
        return _hostname;
    }
    
    /**
     *  Retrieve the priority
     *  @return int
     */
    int priority() const
    {
        return _priority;
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
     *  @param  that    record to compare to
     *  @return bool
     */
    bool operator==(const MxRecord &that) const
    {
        return _hostname == that._hostname && _ttl == that._ttl && _priority == that._priority;
    }

    /**
     *  Compare two objects
     *  @param  that    record to compare to
     *  @return bool
     */
    bool operator!=(const MxRecord &that) const
    {
        return _hostname != that._hostname || _ttl != that._ttl || _priority != that._priority;
    }
    
    /**
     *  Compare two objects
     *  @param  that    record to compare to
     *  @return bool
     */
    bool operator<(const MxRecord &that) const
    {
        if (_priority != that._priority) return _priority < that._priority;
        if (_hostname != that._hostname) return _hostname < that._hostname;
        return _ttl < that._ttl;
    }

    /**
     *  Compare two objects
     *  @param  that    record to compare to
     *  @return bool
     */
    bool operator>(const MxRecord &that) const
    {
        if (_priority != that._priority) return _priority > that._priority;
        if (_hostname != that._hostname) return _hostname > that._hostname;
        return _ttl > that._ttl;
    }
};

/**
 *  Function to write a record to a stream
 *  @param  os  stream to write to
 *  @param  mx  the mx record to display
 *  @return ostream
 */
inline std::ostream &operator<<(std::ostream &os, const React::Dns::MxRecord &mx)
{
    os << std::to_string(mx.priority()) << " " << mx.hostname();
    return os;
}

/**
 *  End namespace
 */
}}

