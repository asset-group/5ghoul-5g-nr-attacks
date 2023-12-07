/**
 *  Timeval.h
 *
 *  Utility class that makes it easy to convert Timestamp times into
 *  struct timeval objects.
 *
 *  The timeval class is an extended struct timeval
 *
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Class definition
 */
class Timeval : public timeval
{
public:
    /**
     *  Default constructor
     */
    Timeval()
    {
        tv_sec = tv_usec = 0;
    }

    /**
     *  Constructor to convert from a ev_tstamp value
     *  @param  timestamp   ev_tstamp structure
     */
    Timeval(Timestamp timestamp)
    {
        tv_sec  = (long)timestamp;
        tv_usec = (long)((timestamp - (Timestamp)tv_sec) * 1e6);
    }
    
    /**
     *  Destructor
     */
    virtual ~Timeval() {}
    
    /**
     *  Convert the object to a ev_tstamp value
     *  @return ev_tstamp
     */
    operator Timestamp () const
    {
        return tv_sec + tv_usec * 1e-6;
    }
};

/**
 *  End namespace
 */
}
