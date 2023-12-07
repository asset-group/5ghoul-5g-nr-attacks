#ifndef __REACTCPP_LOOP__
#define __REACTCPP_LOOP__

#include "./../libev/ev.h"

/**
 *  Loop.h
 *
 *  Central event loop class
 *
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React
{

   /**
 *  Class definition
 */
   class Loop
   {
   private:
      /**
     *  The loop resource
     *  @var    struct ev_loop
     */
      struct ev_loop *_loop = nullptr;
     

      /**
     *  Did we allocate the loop ourselves?
     *  @var    boolean
     */
      bool _allocated;

      /**
     *  Is the loop currently running?
     *
     *  This returns true when Loop.run() was called and has not returned
     *  yet. In practice means this method will only ever return true from
     *  a callback function.
     *
     *  @return boolean
     */
      inline bool running()
      {
#if EV_VERSION_MAJOR == 3
         // libev has a different function name
         return ev_loop_depth(_loop);
#else
         // normal libev call to check recursion level
         return ev_depth(_loop);
#endif
      }

   protected:
   public:
      ev_async async_wakeup;

      /**
     *  Constructor
     */
      Loop()
          : _loop(ev_loop_new(EVFLAG_AUTO)), _allocated(true) {}

      /**
     *  Constructor around an existing loop.
     *
     *  Note that in this case we do not "manage" the loop,
     *  will not try to free it when we fall out of scope and
     *  that it is your job to make sure it stays valid until
     *  after this object was destructed.
     */
      Loop(struct ev_loop *loop)
          : _loop(loop), _allocated(false) {}

      /**
     *  We cannot be copied
     */
      Loop(const Loop &that) = delete;

      /**
     *  Nor can we be moved
     */
      Loop(Loop &&that) = delete;

      /**
     *  Destructor
     */
      virtual ~Loop()
      {
         // only if we allocated the loop ourselves do we destroy it
         if (_allocated)
            ev_loop_destroy(_loop);
      }

      /**
     *  Casting operator to cast the loop object to the internal libev
     *  loop structure. This makes it possible to pass the loop object
     *  directly to libev functions (but use this with care, as it might
     *  result in a conflicting state of the loop object in C++ and C code).
     *
     *  @return struct ev_loop
     */
      operator struct ev_loop *()
      {
         return _loop;
      }

      /**
     *  Current loop time
     *
     *  With every iteration of the loop, the current time is retrieved and
     *  stored. With the method now() you can retrieve this time. By using this
     *  method instead of doing a system call yourself to find out the time,
     *  you save a loop of system calls, and increase performance.
     *
     *  @return Timestamp;
     */
      Timestamp now() const
      {
         return ev_time();
      }

      static void
      async_cb (EV_P_ ev_async *w, int revents)
      {
         // just used for the side effects
      }

      /**
     *  Run the event loop
     *
     *  This will run the event loop, until no more timers and filedescriptors
     *  are linked to it. If you call this method before you have registered
     *  any timers or filedescriptors, it will therefore immediately return.
     *
     *  @return bool
     */
      bool run(bool never_end = true)
      {
         // not possible if already running
         if (running())
            return false;

#if EV_VERSION_MAJOR == 3
         // start running for old libev library version
         ev_loop(_loop, 0);
#else
         // new libev library version
         // if (never_end)
         // {
         //    this->onInterval(pool_time, []() {
         //       return true;
         //    });
         // }

         // this->onInterval(0.01, []() {
         //    return true;
         // });
         
         ev_async_init(&async_wakeup, async_cb);
         // async_wakeup.data = (void *)&_loop;
         ev_async_start(_loop, &async_wakeup);
         if(never_end)
            ev_ref(_loop);
         ev_run(_loop, 0);
#endif

         // loop has ended
         return true;
      }

      /**
     *  Run a single iteration of the event loop
     *
     *  This will check once all the filedescriptors and timers, and will notify
     *  the appropriate callbacks.
     *
     *  With the boolean parameter you can specify if the operation should block
     *  or not. If you set it to true, the method will wait until at least one
     *  callback can be called. If set to false, it will not wait and immediately
     *  return if no methods can be called
     *
     *  @param  block   block return until the step was completed
     *  @return bool
     */
      bool step(bool block = true)
      {
         // not possible when already running
         if (running())
            return false;

#if EV_VERSION_MAJOR == 3
         // old version
         ev_loop(_loop, EVLOOP_ONESHOT + (block ? 0 : EVRUN_NOWAIT));
#else
         // new version
         ev_run(_loop, EVRUN_ONCE + (block ? 0 : EVRUN_NOWAIT));
#endif

         // done
         return true;
      }

      /**
     *  Stop running the event loop
     *
     *  Normally, the event loop runs until no more filedescriptors and timers
     *  are active. This function will terminate the event loop prematurely.
     *
     *  @return bool
     */
      bool stop()
      {
         // must be running
         if (!running())
            return false;

#if EV_VERSION_MAJOR == 3
         // old version
         ev_unloop(_loop, EVUNLOOP_ONE);
#else
         // new version
         ev_break(_loop, EVBREAK_ONE);
#endif

         // done
         return true;
      }

      /**
     *  Suspend the loop. While the loop is suspended, timers will not be processed,
     *  and the time for the timers does not proceed. Once the loop is resumed, the
     *  timers continue to run.
     */
      void suspend()
      {
         // suspend now
         ev_suspend(_loop);
      }

      /**
     *  Resume the loop after it was suspended
     */
      void resume()
      {
         ev_resume(_loop);
      }

      /**
     *  Register a function that is called the moment a filedescriptor becomes
     *  readable.
     *
     *  This method takes two arguments: the filedescriptor to be checked and
     *  the function that is going to be called the moment the filedescriptor
     *  becomes readable.
     *
     *  The method returns a shared pointer to a watcher object. This watcher
     *  object can be used to later stop watching the filedescriptor. It is
     *  legal to ignore the return value if you don't need it.
     *
     *  @param  fd          The filedescriptor
     *  @param  callback    Function that is called the moment the fd is readable
     *  @return             Object that can be used to stop checking for readability
     */
      std::shared_ptr<ReadWatcher> onReadable(int fd, const ReadCallback &callback);

      /**
     *  Register a function that is called the moment a filedescriptor becomes
     *  writable.
     *
     *  This method takes two arguments: the filedescriptor to be checked and
     *  the function that is going to be called the moment the filedescriptor
     *  becomes writable.
     *
     *  The method returns a shared pointer to a watcher object. This watcher
     *  object can be used to later stop watching the filedescriptor. It is
     *  legal to ignore the return value if you don't need it.
     *
     *  @param  fd          The filedescriptor
     *  @param  callback    Function that is called the moment the fd is readable
     *  @return             Object that can be used to stop checking for writability
     */
      std::shared_ptr<WriteWatcher> onWritable(int fd, const WriteCallback &callback);

      /**
     *  Register a timeout to be called in a certain amount of time
     *
     *  This method takes two arguments: a timeout in seconds that specifies
     *  for how long you'd like to wait, and a callback function that will be
     *  called when the timer expires.
     *
     *  The method returns a shared pointer to a watcher object. This watcher
     *  object can be used to stop the timer. If you ignore the return value
     *  (which is legal) you can not stop the timer.
     *
     *  @param  timeout     The timeout in seconds
     *  @param  callback    Function that is called when the timer expires
     *  @return             Object that can be used to stop or edit the timer
     */
      std::shared_ptr<TimeoutWatcher> onTimeout(Timestamp timeout, const TimeoutCallback &callback);

      /**
     *  Register a timeout to be called in a certain amount of time
     *
     *  This method takes two arguments: a timeout in seconds that specifies
     *  for how long you'd like to wait, and a callback function that will be
     *  called when the timer expires.
     *
     *  The method returns a shared pointer to a watcher object. This watcher
     *  object can be used to stop the timer. If you ignore the return value
     *  (which is legal) you can not stop the timer.
     *
     *  @param  timeout     The timeout in miliseconds
     *  @param  callback    Function that is called when the timer expires
     *  @return             Object that can be used to stop or edit the timer
     */
      std::shared_ptr<TimeoutWatcher> onTimeoutMS(int timeout_ms, const TimeoutCallback &callback);


      /**
     *  Register a timeout to be called in a certain amount of time
     *
     *  This method takes two arguments: a timeout in seconds that specifies
     *  for how long you'd like to wait, and a callback function that will be
     *  called when the timer expires.
     *
     *  The method returns a shared pointer to a watcher object. This watcher
     *  object can be used to stop the timer. If you ignore the return value
     *  (which is legal) you can not stop the timer.
     *
     *  @param mutex         Mutex to take during task execution in the event loop
     *  @param  timeout     The timeout in miliseconds
     *  @param  callback    Function that is called when the timer expires
     *  @return             Object that can be used to stop or edit the timer
     */
      std::shared_ptr<TimeoutWatcher> onTimeoutMSSync(pthread_mutex_t &mutex, int timeout_ms, const TimeoutCallback &callback);


      /**
     *  Register a function to be called periodically at fixed intervals
     *
     *  This method takes two arguments: a timeout in seconds that specifies
     *  the interval time, and a callback function that will be called
     *  every time the interval timer expires.
     *
     *  The method returns a shared pointer to a watcher object. This watcher
     *  object can be used to stop the timer. If you ignore the return value
     *  (which is legal) you can not stop the it.
     *
     *  @param  initial     Initial timeout in seconds
     *  @param  timeout     Subsequent interval in seconds
     *  @param  callback    Function that is called when the timer expires
     *  @return             Object that can be used to stop or edit the interval
     */
      std::shared_ptr<IntervalWatcher> onInterval(Timestamp initial, Timestamp timeout, const IntervalCallback &callback);
      std::shared_ptr<IntervalWatcher> onInterval(Timestamp timeout, const IntervalCallback &callback) { return onInterval(timeout, timeout, callback); }
      std::shared_ptr<IntervalWatcher> onIntervalMS(int timeout_ms, const IntervalCallback &callback);

      /**
     *  Register a synchronize function
     *
     *  In a multi threaded environment, you may have multiple threads that you
     *  sometimes want to synchronize. With the onSychronize() method you can
     *  register a function that is going to be called when the thread is
     *  synchronized from a different thread.
     *
     *  This method returns a thread safe watcher object that need to wakeup
     *  the event loop from an other thread.
     *
     *  @param  callback    The callback that is called
     *  @return             Object that can be used to stop watching, or to synchronize
     */
      std::shared_ptr<SynchronizeWatcher> onSynchronize(const SynchronizeCallback &callback);

     /**
      * @brief 
      * Immediatelly run a task on the event loop thread
      * 
      * @param callback    The callback that is called
      * @return            Object that can be used to stop watching, or to synchronize 
      */
      std::shared_ptr<SynchronizeWatcher> RunTask(const SynchronizeCallback &callback);

     /**
      * @brief 
      * Immediatelly run a task on the event loop thread and take mutex during execution
      * 
      * @param mutex             Mutex to take during task execution in the event loop
      * @param callback          The callback that is called
      * @param lock_on_creation  Take the mutex when creating task (warning: dont't set this to true in reetrant task)
      * @return                  Object that can be used to stop watching, or to synchronize 
      */
      std::shared_ptr<SynchronizeWatcher> RunTaskSync(pthread_mutex_t &mutex, const SynchronizeCallback &callback, bool lock_on_creation = false);

      /**
     *  Register a cleanup function
     *
     *  This method takes a callback to be executed right before the event loop
     *  gets destroyed.
     *
     *  @param  callback    Function to invoke before destroying the loop
     */
      std::shared_ptr<CleanupWatcher> onCleanup(const CleanupCallback &callback);
   }; // namespace React

   /**
 *  End namespace
 */
} // namespace React

#endif
