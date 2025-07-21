#include <gtest/gtest.h>
#include <sstream>
#include <iostream>
#include "../../src/hweb/Output.h"

class OutputTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Capture stderr for testing output
        original_cerr = std::cerr.rdbuf();
        std::cerr.rdbuf(captured_output.rdbuf());
        
        // Reset output modes
        HWeb::Output::set_json_mode(false);
        HWeb::Output::set_silent_mode(false);
    }

    void TearDown() override {
        // Restore stderr
        std::cerr.rdbuf(original_cerr);
    }

    std::stringstream captured_output;
    std::streambuf* original_cerr;
};

TEST_F(OutputTest, InfoOutputInNormalMode) {
    HWeb::Output::info("Test message");
    
    EXPECT_EQ(captured_output.str(), "Test message\n");
}

TEST_F(OutputTest, InfoOutputInSilentMode) {
    HWeb::Output::set_silent_mode(true);
    HWeb::Output::info("Test message");
    
    EXPECT_EQ(captured_output.str(), "");
}

TEST_F(OutputTest, ErrorOutputAlwaysShows) {
    HWeb::Output::set_silent_mode(true);
    HWeb::Output::error("Error message");
    
    EXPECT_EQ(captured_output.str(), "Error message\n");
}

TEST_F(OutputTest, ModeGettersSetters) {
    EXPECT_FALSE(HWeb::Output::is_json_mode());
    EXPECT_FALSE(HWeb::Output::is_silent_mode());
    
    HWeb::Output::set_json_mode(true);
    HWeb::Output::set_silent_mode(true);
    
    EXPECT_TRUE(HWeb::Output::is_json_mode());
    EXPECT_TRUE(HWeb::Output::is_silent_mode());
}

TEST_F(OutputTest, FormatErrorWithContext) {
    HWeb::Output::format_error("Navigation", "Timeout occurred");
    
    EXPECT_EQ(captured_output.str(), "Error: Navigation: Timeout occurred\n");
}

TEST_F(OutputTest, FormatErrorInJsonMode) {
    HWeb::Output::set_json_mode(true);
    HWeb::Output::format_error("Navigation", "Timeout occurred");
    
    EXPECT_EQ(captured_output.str(), "{\"error\": \"Navigation: Timeout occurred\"}\n");
}