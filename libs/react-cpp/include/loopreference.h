/**
 *  LoopReference.h
 *
 *  This class will keep the loop alive by adding a reference
 *  to it. Normally you would not need to use this, as watching
 *  file descriptors, running timers and other watchers all keep
 *  a reference of their own.
 *
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React {

/**
 *  LoopReference class
 */
class LoopReference
{
    /**
     *  The loop we are adding a reference to
     */
    Loop *_loop;
public:
    /**
     *  Constructor
     *
     *  @param  loop    the loop to add a reference to
     */
    LoopReference(Loop *loop) :
        _loop(loop)
    {
        // add reference to the loop
        ev_ref(*_loop);
    }

    /**
     *  Destructor
     */
    virtual ~LoopReference()
    {
        // remove reference from the loop
        ev_unref(*_loop);
    }
};

/**
 *  End namespace
 */
}
