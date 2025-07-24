#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "browser_test_environment.h"
#include <iostream>

// Main test runner for HeadlessWeb unit tests
int main(int argc, char **argv) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Add global test environment
    ::testing::AddGlobalTestEnvironment(new BrowserTestEnvironment);
    
    // Initialize Google Mock
    ::testing::InitGoogleMock(&argc, argv);
    
    // Set up test environment
    std::cout << "Starting HeadlessWeb Unit Tests" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Run all tests
    int result = RUN_ALL_TESTS();
    
    // Cleanup and results
    if (result == 0) {
        std::cout << std::endl << "All tests passed successfully!" << std::endl;
    } else {
        std::cout << std::endl << "Some tests failed. Check output above for details." << std::endl;
    }
    
    return result;
}