/**
 *  Dns.cpp
 *
 *  Test program for the resolver
 *
 *  @copyright 2014 Copernica BV
 */
#include <reactcpp.h>
#include <iostream>

/**
 *  Main procedure
 *  @return int
 */
int main()
{
    // we need (of course) an event loop
    React::MainLoop loop;

    // and a resolver
    React::Dns::Resolver resolver(&loop);
    
    // fetch all IP's
    resolver.ip("www.copernica.com", [](React::Dns::IpResult &&ips, const char *error) {
     
        std::cout << "IP" << std::endl;
        for (auto &ip : ips) std::cout << ip << std::endl;
    });
    
    // fetch MX records
    resolver.mx("copernica.com", [](React::Dns::MxResult &&mxs, const char *error) {
        
        std::cout << "MX" << std::endl;
        for (auto &record : mxs) std::cout << record << std::endl;
    });
    
    // run the event loop
    loop.run();

    // done
    return 0;
}
