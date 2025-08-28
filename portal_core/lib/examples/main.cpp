#include "portal_example.h"
#include <iostream>

int main() {
    try {
        Portal::Example::PortalSystemExample example;
        example.run_example();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
