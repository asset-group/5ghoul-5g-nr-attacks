/**
 *  Worker.h
 *
 *  A worker implements synchronous, thread-safe execution
 *  of code in another thread.
 *
 *  When a worker is instantiated around a loop, the code that is passed
 *  to the execute() method will be executed in the context of the loop
 *  object passed to the constructor. If no loop is provided, a thread
 *  is started to execute code that is not supposed to block the main thread.
 *
 *  Please note that the worker will not keep the loop active, if there
 *  is nothing else keeping the loop running, you will need to add a
 *  reference using ev_ref.
 *
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Forward declaration
 */
class WorkerImpl;

/**
 *  Class definition
 */
class Worker
{
private:
    /**
     *  The underlying worker implementation
     *  @var    WorkerImpl
     */
    WorkerImpl *_impl;

public:
    /**
     *  Constructor
     *
     *  Create a worker that will execute code from the context of the main thread.
     *
     *  @param  loop    event loop where the worker will execute code
     */
    Worker(Loop *loop);

    /**
     *  Constructor
     *
     *  Create a worker that will execute code in another thread.
     */
    Worker();

    /**
     *  Destructor
     */
    virtual ~Worker();

    /**
     *  Execute a function
     *
     *  @param  function    the code to execute
     */
    void execute(const std::function<void()> &function);
};

/**
 *  End namespace
 */
}
