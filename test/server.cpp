#include "demo/Stickly.hpp"

int main() {
    try {
        asio::io_context io_context;
        Server server(io_context, 8080);
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}