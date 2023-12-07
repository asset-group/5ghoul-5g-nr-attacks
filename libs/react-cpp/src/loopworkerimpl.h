/**
 *  LoopWorkerImpl.h
 *
 *  Worker that executes code in the scope of the main loop
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
class LoopWorkerImpl : public WorkerImpl
{
    /**
     *  The loop in which we run
     */
    Loop *_loop;

    /**
     *  The watcher to synchronize with the main thread
     *  @var    SynchronizeWatcher
     */
    SynchronizeWatcher _watcher;

    /**
     *  The callbacks to execute from the main thread
     *  @var    deque
     */
    std::deque<std::function<void()>> _callbacks;

    /**
     *  Mutex to protect the callback list
     *  @var    mutex
     */
    std::mutex _mutex;

    /**
     *  Callback function that is executed from the loop context
     *  This function is called everytime we are notified that an instruction is ready
     */
    void run()
    {
        // keep going until all the work is done
        while (true)
        {
            // lock the callback list
            std::unique_lock<std::mutex> lock(_mutex);

            // are there any callbacks to execute?
            if (_callbacks.empty()) return;

            // retrieve the first callback
            auto callback = std::move(_callbacks.front());
            
            // remove it from the queue
            _callbacks.pop_front();

            // unlock the list
            lock.unlock();

            // and execute the callback
            callback();
        }
    }

public:
    /**
     *  Construct the loop worker
     *
     *  @param  loop    loop to execute code in
     */
    LoopWorkerImpl(Loop *loop) :
        _loop(loop),
        _watcher(loop, std::bind(&LoopWorkerImpl::run, this))
    {
        // the synchronize watcher has just added a reference
        // to the loop, keeping it active, which we don't want
        ev_unref(*_loop);
    }

    /**
     *  Destructor
     */
    virtual ~LoopWorkerImpl()
    {
        // the watcher will remove a reference from the loop
        // that we already removed, so we add one now to keep
        // the total balance equal
        ev_ref(*_loop);
    }

    /**
     *  Execute a function
     *
     *  @param  callback    the code to execute
     */
    virtual void execute(const std::function<void()> &callback) override
    {
        // lock the callback list
        _mutex.lock();

        // push the callback onto the list
        _callbacks.push_back(callback);

        // done with mutex
        _mutex.unlock();

        // execute the synchronize watcher
        _watcher.synchronize();
    }
};

/**
 *  End namespace
 */
}
