/**
 *  WorkerImpl.h
 *
 *  The base for both the thread worker and the loop worker
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
class WorkerImpl
{
public:
    /**
     *  Destructor
     */
    virtual ~WorkerImpl() {}

    /**
     *  Execute a function
     *
     *  @param  function    the code to execute
     */
    virtual void execute(const std::function<void()> &function) = 0;
};

/**
 *  End namespace
 */
}
