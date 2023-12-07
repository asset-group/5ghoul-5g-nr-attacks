/**
 *  Test.cpp
 * 
 *  Simple test case for the library
 * 
 *  @copyright 2014 Copernica BV
 */
#include <iostream>
#include <reactcpp.h>
#include <thread>

using namespace std;


int main()
{
    React::Loop loop;
    
//    loop.onInterval(0.5, []() {
//       
//        std::cout << "interval expired" << std::endl;
//        
//    });

    auto timer2 = loop.onTimeout(0.5, []() {
        
        std::cout << "TIMEOUT1" << std::endl;
    });

    auto timer = loop.onTimeout(1.0, []() {
        
        std::cout << "TIMEOUT3" << std::endl;
    });
    
    thread t = thread([&](){
    	loop.run();
    });

    this_thread::sleep_for(700ms);
    cout << "new" << endl;
    auto timer3 = loop.onTimeout(0.1, [&]() {
        timer->cancel();
        std::cout << "TIMEOUT2" << std::endl;
    });
    this_thread::sleep_for(2s);
    t.join();
    
    return 0;
    
}

