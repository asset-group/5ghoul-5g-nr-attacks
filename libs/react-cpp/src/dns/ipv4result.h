/**
 *  Ipsv4Result.h
 *
 *  Implementation-only class that parses the result of an IPv4 lookup
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
class Ipv4Result : public IpResult
{
public:
    /**
     *  Constructor
     *  @param  buffer      Received data
     *  @param  len         qqSize of the data
     */
    Ipv4Result(const unsigned char *buffer, int len)
    {
        // try parsing answer
        for (int size = 8; true; size += 4)
        {
            // allocate memory
            auto *addresses = new struct ares_addrttl[size];
            
            // wrap in smarty pointer so that it gets destructed
            std::unique_ptr<struct ares_addrttl[]> ptr(addresses);
            
            // copy size
            int matches = size;
            
            // parse the answer
            int result = ares_parse_a_reply(buffer, len, nullptr, addresses, &matches);

            // on failure we leap out, otherwise we continue to allocate more memory
            if (result != ARES_SUCCESS) return;
            
            // check if we had enough space
            if (matches >= size) continue;
            
            // we have enough space, copy the results to the object
            for (int i=0; i<matches; i++) insert(IpRecord(&addresses[i]));
            
            // done
            return;
        }
    }
    
    /**
     *  Destructor
     */
    virtual ~Ipv4Result() {}
};
    
/**
 *  End namespace
 */
}}

