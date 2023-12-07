/**
 *  Worker.cpp
 *
 *  Test whether workers work
 *
 *  @copyright 2014 Copernica BV
 */
#include <iostream>
#include <reactcpp.h>

int main()
{
    React::Loop loop;
    React::Worker worker1;
    React::Worker worker2(&loop);

    worker1.execute([&worker2, &loop]() {
        std::cout << "Hello from child worker!" << std::endl;

        worker2.execute([&loop]() {
            std::cout << "Hello again from the main thread" << std::endl;
            loop.stop();
        });
    });

    loop.run();
    return 0;
}
