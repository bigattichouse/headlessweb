// Legacy wrapper for the refactored hweb application  
// This file now delegates to the new modular structure in src/hweb/

// The actual main function is now implemented in src/hweb/main.cpp
// This wrapper exists to maintain the same build structure

// Include the new main function declaration
namespace HWeb {
    int main(int argc, char* argv[]);
}

int main(int argc, char* argv[]) {
    // Simply delegate to the new modular main function
    return HWeb::main(argc, argv);
}