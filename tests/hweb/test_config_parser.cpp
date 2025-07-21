#include <gtest/gtest.h>
#include "../../src/hweb/Config.h"

class ConfigParserTest : public ::testing::Test {
protected:
    HWeb::ConfigParser parser;
};

TEST_F(ConfigParserTest, ParseBasicOptions) {
    std::vector<std::string> args = {"--session", "test", "--url", "http://example.com", "--json", "--silent"};
    
    auto config = parser.parseArguments(args);
    
    EXPECT_EQ(config.sessionName, "test");
    EXPECT_EQ(config.url, "http://example.com");
    EXPECT_TRUE(config.json_mode);
    EXPECT_TRUE(config.silent_mode);
    EXPECT_FALSE(config.endSession);
    EXPECT_FALSE(config.listSessions);
}

TEST_F(ConfigParserTest, ParseBrowserWidth) {
    std::vector<std::string> args = {"--width", "1200"};
    
    auto config = parser.parseArguments(args);
    
    EXPECT_EQ(config.browser_width, 1200);
}

TEST_F(ConfigParserTest, ParseFormInteractionCommands) {
    std::vector<std::string> args = {
        "--type", "#input", "hello world",
        "--click", "#button", 
        "--select", "#dropdown", "option1",
        "--check", "#checkbox",
        "--uncheck", "#radio"
    };
    
    auto config = parser.parseArguments(args);
    
    ASSERT_EQ(config.commands.size(), 5);
    
    EXPECT_EQ(config.commands[0].type, "type");
    EXPECT_EQ(config.commands[0].selector, "#input");
    EXPECT_EQ(config.commands[0].value, "hello world");
    
    EXPECT_EQ(config.commands[1].type, "click");
    EXPECT_EQ(config.commands[1].selector, "#button");
    
    EXPECT_EQ(config.commands[2].type, "select");
    EXPECT_EQ(config.commands[2].selector, "#dropdown");
    EXPECT_EQ(config.commands[2].value, "option1");
    
    EXPECT_EQ(config.commands[3].type, "check");
    EXPECT_EQ(config.commands[3].selector, "#checkbox");
    
    EXPECT_EQ(config.commands[4].type, "uncheck");
    EXPECT_EQ(config.commands[4].selector, "#radio");
}

TEST_F(ConfigParserTest, ParseNavigationCommands) {
    std::vector<std::string> args = {"--back", "--forward", "--reload"};
    
    auto config = parser.parseArguments(args);
    
    ASSERT_EQ(config.commands.size(), 3);
    EXPECT_EQ(config.commands[0].type, "back");
    EXPECT_EQ(config.commands[1].type, "forward");
    EXPECT_EQ(config.commands[2].type, "reload");
}

TEST_F(ConfigParserTest, ParseDataExtractionCommands) {
    std::vector<std::string> args = {
        "--text", "#title",
        "--html", "#content", 
        "--attr", "#link", "href",
        "--exists", "#element",
        "--count", ".items"
    };
    
    auto config = parser.parseArguments(args);
    
    ASSERT_EQ(config.commands.size(), 5);
    
    EXPECT_EQ(config.commands[0].type, "text");
    EXPECT_EQ(config.commands[0].selector, "#title");
    
    EXPECT_EQ(config.commands[1].type, "html");
    EXPECT_EQ(config.commands[1].selector, "#content");
    
    EXPECT_EQ(config.commands[2].type, "attr");
    EXPECT_EQ(config.commands[2].selector, "#link");
    EXPECT_EQ(config.commands[2].value, "href");
    
    EXPECT_EQ(config.commands[3].type, "exists");
    EXPECT_EQ(config.commands[3].selector, "#element");
    
    EXPECT_EQ(config.commands[4].type, "count");
    EXPECT_EQ(config.commands[4].selector, ".items");
}

TEST_F(ConfigParserTest, ParseAdvancedWaitCommands) {
    std::vector<std::string> args = {
        "--wait-text-advanced", "Loading complete",
        "--wait-network-idle", "1000",
        "--wait-element-visible", "#modal",
        "--wait-element-count", ".item", ">", "5"
    };
    
    auto config = parser.parseArguments(args);
    
    ASSERT_EQ(config.commands.size(), 4);
    
    EXPECT_EQ(config.commands[0].type, "wait-text-advanced");
    EXPECT_EQ(config.commands[0].value, "Loading complete");
    
    EXPECT_EQ(config.commands[1].type, "wait-network-idle");
    EXPECT_EQ(config.commands[1].value, "1000");
    
    EXPECT_EQ(config.commands[2].type, "wait-element-visible");
    EXPECT_EQ(config.commands[2].selector, "#modal");
    
    EXPECT_EQ(config.commands[3].type, "wait-element-count");
    EXPECT_EQ(config.commands[3].selector, ".item");
    EXPECT_EQ(config.commands[3].value, "> 5");
}

TEST_F(ConfigParserTest, ParseFileOperationSettings) {
    std::vector<std::string> args = {
        "--max-file-size", "52428800",
        "--allowed-types", "pdf,doc,txt",
        "--download-dir", "/tmp/downloads"
    };
    
    auto config = parser.parseArguments(args);
    
    EXPECT_EQ(config.file_settings.max_file_size, 52428800);
    EXPECT_EQ(config.file_settings.download_dir, "/tmp/downloads");
    ASSERT_EQ(config.file_settings.allowed_types.size(), 3);
    EXPECT_EQ(config.file_settings.allowed_types[0], "pdf");
    EXPECT_EQ(config.file_settings.allowed_types[1], "doc");
    EXPECT_EQ(config.file_settings.allowed_types[2], "txt");
}

TEST_F(ConfigParserTest, ParseAssertionCommands) {
    std::vector<std::string> args = {
        "--assert-exists", "#login-form",
        "--assert-text", "h1", "Welcome",
        "--assert-count", ".item", ">5"
    };
    
    auto config = parser.parseArguments(args);
    
    ASSERT_EQ(config.assertions.size(), 3);
    
    EXPECT_EQ(config.assertions[0].type, "exists");
    EXPECT_EQ(config.assertions[0].selector, "#login-form");
    
    EXPECT_EQ(config.assertions[1].type, "text");
    EXPECT_EQ(config.assertions[1].selector, "h1");
    EXPECT_EQ(config.assertions[1].expected_value, "Welcome");
    
    EXPECT_EQ(config.assertions[2].type, "count");
    EXPECT_EQ(config.assertions[2].selector, ".item");
    EXPECT_EQ(config.assertions[2].expected_value, ">5");
}