/**
 *  MxResult.h
 *
 *  Implementation-only class that parses the result of a MX records
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
class MxResult : public std::set<MxRecord>
{
public:
    /**
     *  Constructor for an empty result set
     */
    MxResult() {}

    /**
     *  Constructor
     *  @param  buffer      Received data
     *  @param  len         qqSize of the data
     */
    MxResult(const unsigned char *buffer, int len)
    {
        // initial size and data
        struct ares_mx_reply *first = NULL;
        
        // parse the data
        if (ares_parse_mx_reply(buffer, len, &first) != ARES_SUCCESS) return;
        
        // loop over the results
        for (auto current = first; current; current = current->next) 
        {
            // create record
            insert(MxRecord(current->host, current->priority));
        }

        // free used memory
        ares_free_data(first);
    }
    
    /**
     *  Destructor
     */
    virtual ~MxResult() {}
};
    
/**
 *  End namespace
 */
}}

