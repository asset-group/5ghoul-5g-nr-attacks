/**
 *  Shared.h
 *
 *  The Loop object has a number of on*() methods (like onReadable(), onTimeout(),
 *  et cetera). When one of these methods is called, a shared pointer is returned.
 *
 *  As long as the handler is active, the library also holds a shared pointer.
 *  This is the base class for such shared-pointer-holding classes
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
template <typename WATCHER>
class Shared
{
protected:
    /**
     *  Shared pointer to the object self
     *  @var    std::shared_ptr
     */
    std::shared_ptr<WATCHER> _self;

    /**
     *  Weak pointer to make it possible to regain ownership
     *  @var    std::weak_ptr
     */
    std::weak_ptr<WATCHER> _weak;
    
protected:
    /**
     *  Constructor
     *  @param  derived     The actual derived class pointer
     */
    Shared(WATCHER *derived) : _self(derived), _weak(_self) {}
    
    /**
     *  Forget the shared pointer
     */
    void reset()
    {
        _self = nullptr;
    }
    
    /**
     *  Restore the self pointer
     */
    void restore()
    {
        _self = _weak.lock();
    }

public:
    /**
     *  Destructor
     */
    virtual ~Shared() {}
    
    /**
     *  The sharable pointer
     *  @return shared_ptr
     */
    std::shared_ptr<WATCHER> pointer()
    {
        return _weak.lock();
    }
};

/**
 *  End namespace
 */
}

