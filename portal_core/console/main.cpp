#include "portal_console.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        Portal::Console::PortalConsole console;
        console.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
    
    return 0;
}
