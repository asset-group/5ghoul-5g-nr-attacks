/**
 *  ThreadWorkerImpl.h
 *
 *  The worker that executes codes in another thread.
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
class ThreadWorkerImpl : public WorkerImpl
{
private:
    /**
     *  Are we still running?
     *  @var    bool
     */
    volatile bool _running = true;

    /**
     *  The callbacks to execute from the main thread
     *  @var    std::deque
     */
    std::deque<std::function<void()>> _callbacks;

    /**
     *  Mutex to protect the callback list
     *  @var    std::mutex
     */
    std::mutex _mutex;

    /**
     *  Condition to signal arrival of new work
     *  @var    std::condition_variable
     */
    std::condition_variable _condition;

    /**
     *  The thread we run (must be last member because it relies on other 
     *  members in the class, and we dont want to run the thread before
     *  the other members are initialized)
     *  @var    std::thread
     */
    std::thread _thread;

    /**
     *  Callback function that is executed from the loop context
     */
    void run()
    {
        // keep going until we are signalled to stop
        while (true)
        {
            // lock the callback list
            std::unique_lock<std::mutex> lock(_mutex);

            // wait for new work to arrive or the worker to be shutdown
            while (_callbacks.empty() && _running) _condition.wait(lock);

            // are we no longer supposed to be running?
            if (_callbacks.empty()) return;

            // retrieve the callback
            auto callback = std::move(_callbacks.front());
            
            // remove from the queue
            _callbacks.pop_front();

            // release the lock
            lock.unlock();

            // run the callback
            callback();
        }
    }

public:
    /**
     *  Construct the thread worker
     */
    ThreadWorkerImpl() : _thread(&ThreadWorkerImpl::run, this) {}

    /**
     *  Clean up the workers
     */
    virtual ~ThreadWorkerImpl()
    {
        // lock the mutex
        _mutex.lock();

        // we should no longer be running
        _running = false;

        // unlock the mutex
        _mutex.unlock();

        // signal thread to stop running
        _condition.notify_one();

        // and join the thread
        _thread.join();
    }

    /**
     *  Execute a function
     *
     *  @param  callback    the code to execute
     */
    virtual void execute(const std::function<void()> &callback) override
    {
        // lock mutex
        _mutex.lock();

        // push the callback onto the list
        _callbacks.push_back(callback);

        // unlock mutex
        _mutex.unlock();

        // signal the worker thread to pick up the work
        _condition.notify_one();
    }

};

/**
 *  End namespace
 */
}

