#include <asio.hpp> 
#include <iostream>

int main()
{
    asio::io_context io_context;
    io_context.run();
    std::cout << "Do you reckon this line displays?" << std::endl;
    return 0;
}
