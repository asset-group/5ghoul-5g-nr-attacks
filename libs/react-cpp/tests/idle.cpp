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
    // note that the type of 'timer' is std::shared_ptr<React::Timer>
    auto timer = loop.onTimeout(5.0, []() {
    
        // report that the timer expired
        std::cout << "timer expired" << std::endl;
    
        // stop the application
        exit(0);
    });
    
    // we'd like to be notified when input is available on stdin
    // the type of 'reader' is std::shared_ptr<React::Reader>
    auto reader = loop.onReadable(STDIN_FILENO, [timer]() -> bool {
    
        // read input
        std::string buffer;
        std::cin >> buffer;
    
        // show what we read
        std::cout << buffer << std::endl;
        
        // set the timer back to five seconds
        timer->set(5.0);
        
        // we want to be notified in the future too for readability events
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
        
        // we dont need another SIGING event
        return false;
    });

    // run the event loop
    loop.run();

    // done
    return 0;
}
