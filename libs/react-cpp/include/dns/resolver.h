/**
 *  Resolver.h
 *
 *  Class for resolving domain names
 */

/**
 *  Set up namespace
 */
namespace React { namespace Dns {

/**
 *  Class definition
 */
class Resolver : private Base
{
public:
    /**
     *  Constructor
     *  @param  loop    Event loop
     */
    Resolver(Loop *loop) : Base(loop) {}

    /**
     *  Destructor
     */
    virtual ~Resolver() {}

    /**
     *  Find all IP addresses for a certain domain
     *  @param  domain      The domain to fetch the IPs for
     *  @param  version     IP version, can be 4 or 6
     *  @param  callback    callback to invoke when the operation completes
     *  @return bool
     */
    bool ip(const std::string &domain, int version, const IpCallback &callback);

    /**
     *  Find all IP addresses for a certain domain
     *  This method fetches all IPs, no matter the version, both Ipv4 and Ipv6
     *  @param  domain      The domain to fetch the IPs for
     *  @param  callback    callback to invoke when the operation completes
     *  @return bool
     */
    bool ip(const std::string &domain, const IpCallback &callback);

    /**
     *  Find all MX records for a certain domain
     *  @param  domain      The domain name to search MX records for
     *  @param  callback    Callback that is called when found
     *  @return bool
     */
    bool mx(const std::string &domain, const MxCallback &callback);

};

/**
 *  End namespace
 */
}}
