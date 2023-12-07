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
    loop.onReadable(STDIN_FILENO, []() -> bool {
    
        // read input
        std::string buffer;
        std::cin >> buffer;
    
        // show what we read
        std::cout << buffer << std::endl;
        
        // we want to receive more onReadable events
        return true;
    });

    // set a timer to stop the application after five seconds
    loop.onInterval(1.0, 1.0, []() -> bool {
    
        // report that the timer expired
        std::cout << "timer expired" << std::endl;
    
        // stop the application
        exit(0);
    });
    
    // handler when control+c is pressed
    loop.onSignal(SIGINT, []() -> bool {
        
        // report that we got a signal
        std::cout << "control+c detected" << std::endl;
        
        // stop the application
        exit(0);
        
        // done
        return false;
    });

    // run the event loop
    loop.run();

    // done
    return 0;
}
