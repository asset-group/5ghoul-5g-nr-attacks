/**
 *  Server.cpp
 *
 *  Test program to start a server application
 *
 *  @copyright 2014 Copernica BV
 */
#include <reactcpp.h>
#include <iostream>

/**
 *  Main program
 */
int main()
{
    // we need a main loop
    React::MainLoop loop;

    // and a TCP server
    React::Tcp::Server server(&loop, 8766, [&loop](React::Tcp::Server *server) -> bool {
    
        // create the connection
        auto incoming = std::make_shared<React::Tcp::Connection>(server);
        
        auto callback = [](React::Tcp::Connection *connection, const char *error) {
            
            
            
            
        };
        
        // create the outgoing connection
        auto outgoing = std::make_shared<React::Tcp::Connection>(&loop, "145.255.129.234", 80, callback);
        
        // install handler when data comes in from the incoming connection
        incoming->onData([outgoing](const void *buf, size_t size) -> bool {
            
            outgoing->send(buf, size);
            return true;
        });
            
        // install handler when data comes in from the incoming connection
        outgoing->onData([incoming](const void *buf, size_t size) -> bool {
            
            incoming->send(buf, size);
            return true;
        });
        
        // install handler when connection closes
        incoming->onLost([outgoing]() {
            
            std::cout << "incoming closed" << std::endl;
            
            outgoing->close();
        });
        
        // install handler when connection closes
        outgoing->onLost([incoming]() {
        
            std::cout << "outgoing closed" << std::endl;
            
            incoming->close();
        });
        
        // keep watching for readability
        return true;
    });
    
    // show the server address
    std::cout << server.address() << std::endl;
    
    // we connect to the server ourselves
    React::Tcp::Connection connection(&loop, "127.0.0.1", 8766, [&connection](React::Tcp::Connection *connection, const char *error) {
        
        // fail on error
        if (error) return;
        
        // send data to the connection
        connection.send("this is data\r\n", 14);
    });
    
    // run the event loop
    loop.run();
    
    // done
    return 0;
}

