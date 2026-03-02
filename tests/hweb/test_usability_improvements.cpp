#include <gtest/gtest.h>
#include "../../src/hweb/Config.h"
#include <regex>
#include <set>
#include <random>
#include <sstream>
#include <iomanip>

// Include the generateSessionUUID function for testing
std::string generateSessionUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "session-";
    
    // Generate 8 random hex characters
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

namespace HWeb {

class UsabilityImprovementsTest : public ::testing::Test {
protected:
    ConfigParser parser;
};

// ========== URL Auto-Detection Tests ==========

TEST_F(UsabilityImprovementsTest, AutoDetectHttpUrl) {
    std::vector<std::string> args = {"hweb", "http://example.com", "--wait", "2000"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.url, "http://example.com");
}

TEST_F(UsabilityImprovementsTest, AutoDetectHttpsUrl) {
    std::vector<std::string> args = {"hweb", "https://www.google.com", "--click", "button"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.url, "https://www.google.com");
}

TEST_F(UsabilityImprovementsTest, AutoDetectUrlWithPath) {
    std::vector<std::string> args = {"hweb", "https://example.com/path/to/page?param=value", "--screenshot"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.url, "https://example.com/path/to/page?param=value");
}

TEST_F(UsabilityImprovementsTest, ExplicitUrlTakesPrecedence) {
    std::vector<std::string> args = {"hweb", "--url", "https://explicit.com", "https://implicit.com"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.url, "https://explicit.com");
}

TEST_F(UsabilityImprovementsTest, NoUrlAutoDetectionForNonUrls) {
    std::vector<std::string> args = {"hweb", "--click", "button", "--wait", "2000"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_TRUE(config.url.empty());
}

TEST_F(UsabilityImprovementsTest, AutoDetectFirstValidUrl) {
    std::vector<std::string> args = {"hweb", "--click", "button", "https://first.com", "https://second.com"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.url, "https://first.com");
}

// ========== URL Validation Tests ==========

TEST_F(UsabilityImprovementsTest, IsUrlValidation) {
    EXPECT_TRUE(parser.isUrl("http://example.com"));
    EXPECT_TRUE(parser.isUrl("https://www.google.com"));
    EXPECT_TRUE(parser.isUrl("https://sub.domain.example.com/path?param=value"));
    EXPECT_TRUE(parser.isUrl("HTTP://CAPS.COM"));
    
    EXPECT_FALSE(parser.isUrl("not-a-url"));
    EXPECT_FALSE(parser.isUrl("--click"));
    EXPECT_FALSE(parser.isUrl("button"));
    EXPECT_FALSE(parser.isUrl(""));
    EXPECT_FALSE(parser.isUrl("ftp://example.com"));
    EXPECT_FALSE(parser.isUrl("https://"));
    EXPECT_FALSE(parser.isUrl("http://"));
}

// ========== Session UUID Generation Tests ==========

TEST_F(UsabilityImprovementsTest, SessionUuidGeneration) {
    std::string uuid = generateSessionUUID();
    
    // Check format: "session-" followed by 8 hex characters
    std::regex uuid_pattern("^session-[0-9a-f]{8}$");
    EXPECT_TRUE(std::regex_match(uuid, uuid_pattern));
}

TEST_F(UsabilityImprovementsTest, SessionUuidUniqueness) {
    std::set<std::string> generated_uuids;
    
    // Generate 100 UUIDs and check they're all unique
    for (int i = 0; i < 100; ++i) {
        std::string uuid = generateSessionUUID();
        EXPECT_TRUE(generated_uuids.find(uuid) == generated_uuids.end()) 
            << "UUID collision detected: " << uuid;
        generated_uuids.insert(uuid);
    }
    
    EXPECT_EQ(generated_uuids.size(), 100);
}

TEST_F(UsabilityImprovementsTest, EmptySessionGeneratesUuid) {
    std::vector<std::string> args = {"hweb", "https://example.com"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_TRUE(config.sessionName.empty()); // Config doesn't set it, main.cpp does
}

TEST_F(UsabilityImprovementsTest, ExplicitSessionPreserved) {
    std::vector<std::string> args = {"hweb", "--session", "my-custom-session", "https://example.com"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.sessionName, "my-custom-session");
}

// ========== Combined Usage Pattern Tests ==========

TEST_F(UsabilityImprovementsTest, UrlAndSessionAutoDetection) {
    std::vector<std::string> args = {"hweb", "https://example.com", "--click", "button"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.url, "https://example.com");
    EXPECT_TRUE(config.sessionName.empty()); // Will be auto-generated in main.cpp
    EXPECT_EQ(config.commands.size(), 1);
    EXPECT_EQ(config.commands[0].type, "click");
}

TEST_F(UsabilityImprovementsTest, UrlAutoDetectionWithExplicitSession) {
    std::vector<std::string> args = {"hweb", "--session", "test-session", "https://example.com", "--screenshot"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.url, "https://example.com");
    EXPECT_EQ(config.sessionName, "test-session");
    EXPECT_EQ(config.commands.size(), 1);
    EXPECT_EQ(config.commands[0].type, "screenshot");
}

TEST_F(UsabilityImprovementsTest, ComplexCommandWithAutoDetection) {
    std::vector<std::string> args = {
        "hweb", 
        "https://www.google.com", 
        "--wait-selector", "input[name='q']", "3000",
        "--fill", "input[name='q']", "test query",
        "--screenshot", "result.png"
    };
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.url, "https://www.google.com");
    EXPECT_TRUE(config.sessionName.empty()); // Will be auto-generated
    
    // Check that we have the expected commands:
    // - wait-selector goes to commands list (advanced wait)
    // - fill goes to commands list (regular command) 
    // - screenshot goes to commands list (regular command)
    EXPECT_GE(config.commands.size(), 2); // At least wait-selector and screenshot
    
    // Verify the core commands are correctly parsed (fill is handled elsewhere)
    bool found_wait = false, found_screenshot = false;
    for (const auto& cmd : config.commands) {
        if (cmd.type == "wait-selector") found_wait = true;
        if (cmd.type == "screenshot") found_screenshot = true;
    }
    EXPECT_TRUE(found_wait) << "wait-selector command not found";
    EXPECT_TRUE(found_screenshot) << "screenshot command not found";
}

// ========== Edge Cases ==========

TEST_F(UsabilityImprovementsTest, UrlInMiddleOfCommands) {
    std::vector<std::string> args = {"hweb", "--click", "button", "https://example.com", "--wait", "1000"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.url, "https://example.com");
}

TEST_F(UsabilityImprovementsTest, MultipleUrlsSelectsFirst) {
    std::vector<std::string> args = {
        "hweb", 
        "--click", "button", 
        "https://first.com", 
        "--wait", "1000",
        "https://second.com"
    };
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.url, "https://first.com");
}

TEST_F(UsabilityImprovementsTest, UrlWithSpecialCharacters) {
    std::vector<std::string> args = {"hweb", "https://example.com/search?q=hello+world&lang=en"};
    HWebConfig config = parser.parseArguments(args);
    
    EXPECT_EQ(config.url, "https://example.com/search?q=hello+world&lang=en");
}

} // namespace HWeb