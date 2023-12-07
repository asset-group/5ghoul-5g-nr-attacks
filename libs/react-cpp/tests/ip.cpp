#include <reactcpp.h>
#include <iostream>

int main()
{
    React::Net::Ipv4 ipv4("192.168.1.0");
    
    std::cout << ipv4.toString() << std::endl;
    
    return 0;

}
