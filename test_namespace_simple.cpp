#include "src/backend/interpreter/core/namespace_registry.h"
#include <iostream>

int main() {
    std::cerr << "Creating NamespaceRegistry..." << std::endl;
    NamespaceRegistry registry;
    
    std::cerr << "Calling registerNamespace..." << std::endl;
    registry.registerNamespace("test", nullptr, false);
    
    std::cerr << "Success!" << std::endl;
    return 0;
}
