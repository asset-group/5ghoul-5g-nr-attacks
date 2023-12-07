REACT-CPP
=========

REACT-CPP is an event loop library that utilizes the new C++11 lambda functions 
to notify you when there is activity on a filedescriptor or on a timer. 
Internally, it is a wrapper around the libev library, and does therefore also
depend on that library.

ABOUT
=====

The REACT-CPP library is created and maintained by Copernica (www.copernica.com). 
Do you appreciate our work and are you looking for other high quality solutions? 
Then check out our other solutions:

* PHP-CPP (www.php-cpp.com)
* PHP-JS (www.php-js.com)
* Copernica Marketing Suite (www.copernica.com)
* MailerQ MTA (www.mailerq.com)
* Responsive Email web service (www.responsiveemail.com)


EVENT LOOP
==========

The React::Loop and the React::MainLoop classes are the central classes
of this library. These classes have methods to set timers and to register 
callback functions that will be called when a filedescriptor becomes readable 
or writable.

In a typical application you create an instance of the mainloop class, and
then you register filedescriptors that you'd like to watch for readability,
register event handlers and timers:

````c++
#include <reactcpp.h>
#include <unistd.h>
#include <iostream>

/**
 *  Main application procedure
 *  @return int
 */
int main()
{
    // create an event loop
    React::MainLoop loop;

    // set a timer to stop the application after five seconds
    loop.onTimeout(5.0, []() {
    
        // report that the timer expired
        std::cout << "timer expired" << std::endl;
    
        // stop the application
        exit(0);
    });
    
    // we'd like to be notified when input is available on stdin
    loop.onReadable(STDIN_FILENO, []() -> bool {
    
        // read input
        std::string buffer;
        std::cin >> buffer;
    
        // show what we read
        std::cout << buffer << std::endl;
        
        // return true, so that we also return future read events
        return true;
    });

    // handler when control+c is pressed
    loop.onSignal(SIGINT, []() -> bool {
        
        // report that we got a signal
        std::cout << "control+c detected" << std::endl;
        
        // stop the application
        exit(0);
        
        // although this code is unreachable, we return false because
        // we're no longer interested in future SIGINT signals
        return false;
    });

    // run the event loop
    loop.run();

    // done
    return 0;
}
````

The above example contains a very simple echo application. Everything that
the application reads from stdin is directly echo'd back to stdout. After five
seconds the application automatically stops, and when the SIGINT signal is 
caught, the application also exits.

There is a subtle difference between the React::MainLoop that we use in the 
example above, and the React::Loop class that is also available. The React::MainLoop 
is supposed to run the main event loop for the entire application, while the 
React::Loop classes are additional event loops that you can (for example) use in 
additional threads. In normal circumstances, you will never have to instantiate 
more than once instance of the React::MainLoop class, while it is perfectly 
legal to create as many React::Loop objects as you wish.

Because the React::MainLoop class is intended to control the entire application,
it has some additional methods to register signal handlers and handlers to
monitor child processes. Such methods are not available in the regular 
React::Loop class.


CALLBACK RETURN VALUES
======================

In the first example we showed how to install handlers on the loop object.
Once such a handler is set, the loop will keep calling it every time
a filedescriptor becomes active. But what if you no longer are interested in
these events? In that case you have a number of options to stop a callback 
from being called.

The first one is by having your callback function return false. If your callback
returns a boolean false value, your handler function is removed from the event
loop and you will no longer be notified. If you return true on the other hand,
the handler will stay in the event loop, and will also be called in the future.

````c++
#include <reactcpp.h>
#include <unistd.h>
#include <iostream>

int main()
{
    // create the event loop
    React::MainLoop loop;
    
    // we'd like to be notified when input is available on stdin
    loop.onReadable(STDIN_FILENO, []() -> bool {
    
        // read input
        std::string buffer;
        std::cin >> buffer;
    
        // show what we read
        std::cout << buffer << std::endl;
        
        // from this moment on, we no longer want to receive updates
        return false;
    });
    
    // run the event loop
    loop.run();
    
    // done
    return 0;
}
````

The program above is only interested in read events until the first line
from stdin is read. After that it returns false, to inform the event loop 
that it is no longer interested in read events.

This also means that the program in the example automatically exits after
the first line. The reason for this is that the run() method of 
the React::Loop and React::MainLoop classes automatically stops running when 
there are no more callback functions active. By returning false, the last and 
only registered callback function is cancelled, and the event loop has nothing 
left to monitor.


RETURN VALUE OF LOOP METHODS
============================

The Loop::onReadable(), Loop::onWritable(), etcetera methods all return a 
(shared) pointer to a watcher object. In the first example we had not used this 
return value, but you can store this watcher object in a variable. If you
have access to this watcher object, you can cancel calls to your handler
without having to wait for your callback to be called first.

The returned watcher is a shared_ptr. Internally, the library also keeps a 
pointer to this object, so that even if you decide to discard the watcher object,
it will live on inside the lib. The only way to stop the callback from being 
active is either by calling the cancel() method on the watcher object, or by 
having your callback function return false.

With this knowledge we are going to modify our earlier example. The echo 
application that we showed before is updated to set the timer back to five
seconds every time that some input is read. The application will therefore no
longer stop five seconds after it was started, but five seconds after the last
input was received. We also change the signal watcher: the moment CTRL+C is 
pressed, the application stops responding, and will delay it's exit 
for one second.

The watcher objects all have in common that they have a cancel() method that 
stops further events from being delivered to your callback function. Next to the 
cancel() method, additional methods are available to deal with the specific 
behavior of the item being watched.


````c++
#include <reactcpp.h>
#include <unistd.h>
#include <iostream>

/**
 *  Main application procedure
 *  @return int
 */
int main()
{
    // create an event loop
    React::MainLoop loop;

    // set a timer to stop the application if it is idle for five seconds
    // note that the type of 'timer' is std::shared_ptr<React::TimeoutWatcher>,
    // also note that the timer callback does not return a boolean, as a timer
    // stops anyway after it expires.
    auto timer = loop.onTimeout(5.0, []() {
    
        // report that the timer expired
        std::cout << "timer expired" << std::endl;
    
        // stop the application
        exit(0);
    });
    
    // we'd like to be notified when input is available on stdin
    // the type of 'reader' is std::shared_ptr<React::ReadWatcher>
    auto reader = loop.onReadable(STDIN_FILENO, [timer]() -> bool {
    
        // read input
        std::string buffer;
        std::cin >> buffer;
    
        // show what we read
        std::cout << buffer << std::endl;
        
        // set the timer back to five seconds
        timer->set(5.0);
        
        // keep checking for readability
        return true;
    });

    // handler when control+c is pressed
    loop.onSignal(SIGINT, [&loop, timer, reader]() -> bool {
        
        // report that we got a signal
        std::cout << "control+c detected" << std::endl;
        
        // both the timer, and the input checker can be cancelled now
        timer->cancel();
        reader->cancel();
        
        // stop the application in one second
        loop.onTimeout(1.0, []() {
        
            // exit the application
            exit(0);
        });
        
        // no longer check for signals
        return false;
    });

    // run the event loop
    loop.run();

    // done
    return 0;
}
````

CONSTRUCT WATCHER OBJECTS
=========================

Up to now we had registered callback methods via the Loop::onSomething()
methods. These methods return a shared pointer to an object that keeps the
watcher state. It is also possible to create such objects directly, without 
calling a Loop::onSomething method(). This can be very convenient, because
you will have ownership of the object (instead of the event loop) and you can 
unregister your handler function by just destructing the object.

````c++
#include <reactcpp.h>
#include <unistd.h>
#include <iostream>

/**
 *  Main application procedure
 *  @return int
 */
int main()
{
    // create an event loop
    React::MainLoop loop;

    // we'd like to be notified when input is available on stdin
    React::ReadWatcher reader(loop, STDIN_FILENO, []() -> bool {
    
        // read input
        std::string buffer;
        std::cin >> buffer;
    
        // show what we read
        std::cout << buffer << std::endl;
        
        // keep checking readability
        return true;
    });

    // run the event loop
    loop.run();

    // done
    return 0;
}
````

Conceptually, there is not a big difference between calling Loop::onReadable()
to register a callback function, or by instantiating a React::ReadWatcher object
yourself. In my opinion, the code that utilizes a call to Loop::onReadable() 
is easier to understand and maintain, but by creating a ReadWatcher class yourself,
you have full ownership of the class and can destruct it whenever you like -
which can be useful too.


FILEDESCRIPTORS
===============

Filedescriptors can be checked for activity by registering callbacks for 
readability and writability. The loop object has two methods for that:

````c++
std::shared_ptr<ReadWatcher> Loop::onReadable(int fd, const ReadCallback &callback);
std::shared_ptr<WriteWatcher> Loop::onWritable(int fd, const WriteCallback &callback);
````

The callbacks are simple functions that return a bool, and that do not take
any parameters. If they return true, the filedescriptor will stay in the event
loop and your callback will also be called in the future if the filedescriptor
becomes readable or writable again. If the function returns false, the 
descriptor is removed from the event loop.

You can also create a ReadWatcher or WriteWatcher object yourself. In that case you will
not have to use the Loop::onReadable() or Loop::onWritable() methods:

````c++
ReadWatcher watcher(&loop, fd, []() -> bool { ...; return true; });
WriteWatcher watcher(&loop, fd, []() -> bool { ...; return true; });
````

TIMERS AND INTERVALS
====================

The React library supports both intervals and timers. A timer is triggered
only once, an interval on the other hand calls the registered callback method 
every time the interval time has expired.

When you create an interval, you can specify both the initial expire time as
well as the interval between all subsequent calls. If you omit the initial time,
the callback will be first called after the first interval has passed.

````c++
std::shared_ptr<TimeoutWatcher> 
Loop::onTimeout(Timestamp seconds, const TimeoutCallback &callback);

std::shared_ptr<IntervalWatcher> 
Loop::onInterval(Timestamp interval, const IntervalCallback &callback);

std::shared_ptr<IntervalWatcher> 
Loop::onInterval(Timestamp initial, Timestamp interval, const IntervalCallback &callback);
````

Just like the callbacks for filedescriptors, the callback for intervals should
return a boolean value to indicate whether the interval timer should be kept
in the event loop or not. The callback function for timeouts does not return
any value, because timeouts only trigger once, and are never kept in the event
loop.

````c++
loop.onTimeout(3.0, []() { ... });
loop.onInterval(5.0, []() -> bool { ...; return true; });
loop.onInterval(0.0, 5.0, []() -> bool { ...; return true; });
````

And you can of course also instantiate React::TimeoutWatcher and 
React::IntervalWatcher objects directly:

````c++
TimeoutWatcher watcher(&loop, 3.0, []() { ... });
IntervalWatcher watcher(&loop, 5.0, []() -> bool { ...; return true; });
IntervalWatcher watcher(&loop, 2.0, 5.0, []() -> bool { ...; return true; });
````

SIGNALS
=======

Signals can be watched too. Normally, signals are delivered to your application
in an asynchronous way, and the signal handling code could be started when your
application is in the middle of running some other algorithm. By registering a 
signal handler via the React::MainLoop class, you can prevent this, and have 
your signal handling be executed as part of the event loop.

Setting up a signal handler is just as easy as setting up callback functions
for filedescriptors or timers. The loop object has a simple onSignal() method
for it:

````c++
std::shared_ptr<SignalWatcher> MainLoop::onSignal(int signum, const SignalCallback &callback);
````

And the callback function (of course) should return a boolean value to tell
the event loop if the handler should be kept in the loop or not.

````c++
loop.onSignal(SIGTERM, []() -> bool { ...; return true; });
````

And for signals it also is possible to bypass the methods on the 
loop class, and create a React::Signal object yourself:

````c++
SIgnalWatcher watcher(&loop, SIGTERM, []() -> bool { ...; return true; });
````

CHILD PROCESSES
===============

The MainLoop class also allows you to watch for status change events from child
processes. This is useful if your application forks off child processes, and 
wants to be notified when one of these child processes changes its status (like
exiting).

Both the pid and trace parameters are optional. If you do not specify a pid (or
set the parameter to zero), your callback function will be called for every
single child process that changes its status. The boolean trace parameter can
be used to indicate whether you'd like to be notified for every possible status
change (including changes between paused and running state), or only when child
processes terminate. Set trace to true to receive all notifications, and to false
to receive only for process exits.

````c++
std::shared_ptr<StatusWatcher> 
MainLoop::onStatusChange(pid_t pid, bool trace, const SignalCallback &callback);
````

The callback function has a different signature than most of the other callbacks,
as it should accept two parameters: the pid of the process for which the status
changed, and its new status. The return value should be true if you want to keep
the child watcher active, or false if you no longer want to be informed about
child process status changes.

````c++
loop.onStatusChange(pid, false, [](pid_t pid, int status) -> bool { ...; return true; });
````

And just like all other watcher objects, you can also create StatusWatcher objects
yourself:

````c++
StatusWatcher watcher(&loop, pid, trace, [](pid_t pid, int status) -> bool { ... });
````

THREAD SYNCHRONIZATION
======================

Let's introduce a topic that has not been addressed in one of the
examples: running multiple threads and optionally multiple thread loops.

If your application runs multiple threads, there is a pretty good chance
that sooner or later you want to get these threads in sync. When you have, for
example, a worker thread that wants to report its results back to the
main thread, it should somehow notify that thread that the result of the
calculatations are somewhere to be picked up. If the main thread is busy running
an event loop, it should be able to interupt that event loop, so that the data 
can be picked up. This all can be done with the Loop::onSynchronize() method.

````c++
#include <reactcpp.h>
#include <unistd.h>
#include <iostream>
#include <thread>

/**
 *  Main procedure
 */
int main()
{
    // create a thread loop
    React::MainLoop loop;
    
    // install a callback that can be called by a worker thread.
    // the returned synchronizer object is of type std::shared_ptr<SynchronizeWatcher>, 
    // and contains a thread safe object that can be accessed from other threads 
    // to notify us
    auto synchronizer = loop.onSynchronize([]() {
        std::cout << "other thread has finished running" << std::endl;
    });
    
    // start a new thread
    std::thread thread([synchronizer]() {
    
        // some long running algorithm
        sleep(1);
        
        // notify the main event loop that the task was completed
        synchronizer->synchronize();
    });
    
    // we'd like to be notified when input is available on stdin
    loop.onReadable(STDIN_FILENO, []() -> bool {
    
        // read input
        std::string buffer;
        std::cin >> buffer;
    
        // show what we read
        std::cout << buffer << std::endl;
        
        // keep receiving readability notifications
        return true;
    });

    // run the event loop
    loop.run();
    
    // join the thread
    thread.join();
    
    // done
    return 0;
}
````

The example above demonstrates how threads can synchronize with each other.
First, you create an endpoint that the other thread can use to call the
main thread, and you install a handler that will be called when the
other thread uses that endpoint. Both steps are taken by a simple call to
Loop::onSynchronize(). This installs the callback function that runs in the 
main thread, and it returns the thread safe endpoint that can be used by other
thread to interup the main event loop.

The SynchronizeWatcher is similar to classes like React::ReadWatcher, 
React::WriteWatcher, React::TimeoutWatcher, etcetera. However, the callback is 
slightly different as it does not return a value. The watcher is active for as
long as you have a reference to the synchronizer object.


````c++
// example how to install a synchronizer via the Loop class
auto watcher = loop.onSynchronize([]() { ... });

// example how to install the synchronizer as an object
SynchronizeWatcher watcher(loop, []() { ... });
````

When you use this technology to synchronize threads, you probably need to have
some shared data. You could for example use a queue that is accessible by both
threads. The worker thread pushes results to it, then calls synchronizer->synchronnize()
to notify the master thread, which can then pick up the results from the queue.
If the queue object is not thread safe, you must make sure that you protect
access to it, for example by using mutex variables.

