#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Session/Manager.h"
#include "Session/Session.h"
#include "../utils/test_helpers.h"
#include <filesystem>
#include <fstream>

class SessionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for testing
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("hweb_sessions");
        test_dir = temp_dir->getPath();
        manager = std::make_unique<SessionManager>(test_dir.string());
    }

    void TearDown() override {
        // Clean up is handled by TemporaryDirectory destructor
        manager.reset();
        temp_dir.reset();
    }

    std::filesystem::path test_dir;
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::unique_ptr<SessionManager> manager;
};

// ========== Constructor and Basic Operations ==========

TEST_F(SessionManagerTest, ConstructorCreatesDirectory) {
    // Directory should exist after constructor
    EXPECT_TRUE(std::filesystem::exists(test_dir));
    EXPECT_TRUE(std::filesystem::is_directory(test_dir));
}

TEST_F(SessionManagerTest, LoadOrCreateNewSession) {
    // Load a session that doesn't exist yet
    Session session = manager->loadOrCreateSession("new_session");
    
    EXPECT_EQ(session.getName(), "new_session");
    EXPECT_EQ(session.getCurrentUrl(), "");
    EXPECT_TRUE(session.getHistory().empty());
    EXPECT_TRUE(session.getCookies().empty());
}

TEST_F(SessionManagerTest, SaveAndLoadSession) {
    // Create a session with some data
    Session originalSession("test_session");
    originalSession.setCurrentUrl("https://example.com");
    originalSession.addToHistory("https://first.com");
    originalSession.addToHistory("https://second.com");
    
    Cookie cookie;
    cookie.name = "test_cookie";
    cookie.value = "test_value";
    cookie.domain = "example.com";
    originalSession.addCookie(cookie);
    
    originalSession.setLocalStorageItem("key", "value");
    originalSession.setCustomVariable("var", "val");
    
    // Save the session
    manager->saveSession(originalSession);
    
    // Load the session back
    Session loadedSession = manager->loadOrCreateSession("test_session");
    
    // Verify all data was preserved
    EXPECT_EQ(loadedSession.getName(), "test_session");
    EXPECT_EQ(loadedSession.getCurrentUrl(), "https://example.com");
    EXPECT_EQ(loadedSession.getHistory().size(), 2);
    EXPECT_EQ(loadedSession.getHistory()[0], "https://first.com");
    EXPECT_EQ(loadedSession.getHistory()[1], "https://second.com");
    EXPECT_EQ(loadedSession.getCookies().size(), 1);
    EXPECT_EQ(loadedSession.getCookies()[0].name, "test_cookie");
    EXPECT_EQ(loadedSession.getLocalStorage().size(), 1);
    EXPECT_EQ(loadedSession.getCustomVariable("var"), "val");
}

// ========== Session File Management ==========

TEST_F(SessionManagerTest, SessionFileCreation) {
    Session session("file_test");
    session.setCurrentUrl("https://test.com");
    
    manager->saveSession(session);
    
    // Check if file was created
    std::filesystem::path expectedFile = test_dir / "file_test.hweb";
    EXPECT_TRUE(std::filesystem::exists(expectedFile));
    EXPECT_TRUE(std::filesystem::is_regular_file(expectedFile));
    
    // File should contain valid JSON
    std::ifstream file(expectedFile);
    EXPECT_TRUE(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty());
    
    // Should be valid JSON (basic check)
    Json::Value root;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(content, root));
    EXPECT_TRUE(root.isMember("name"));
    EXPECT_EQ(root["name"].asString(), "file_test");
}

TEST_F(SessionManagerTest, DeleteSession) {
    // Create and save a session
    Session session("delete_test");
    session.setCurrentUrl("https://delete.com");
    manager->saveSession(session);
    
    // Verify file exists
    std::filesystem::path sessionFile = test_dir / "delete_test.hweb";
    EXPECT_TRUE(std::filesystem::exists(sessionFile));
    
    // Delete the session
    manager->deleteSession("delete_test");
    
    // Verify file is gone
    EXPECT_FALSE(std::filesystem::exists(sessionFile));
}

TEST_F(SessionManagerTest, DeleteNonexistentSession) {
    // Deleting a session that doesn't exist should not throw
    EXPECT_NO_THROW(manager->deleteSession("nonexistent"));
}

// ========== Session Listing ==========

TEST_F(SessionManagerTest, ListEmptySessions) {
    auto sessions = manager->listSessions();
    EXPECT_TRUE(sessions.empty());
}

TEST_F(SessionManagerTest, ListSingleSession) {
    // Create and save a session
    Session session("list_test");
    session.setCurrentUrl("https://list.com");
    manager->saveSession(session);
    
    auto sessions = manager->listSessions();
    EXPECT_EQ(sessions.size(), 1);
    EXPECT_EQ(sessions[0].name, "list_test");
    EXPECT_EQ(sessions[0].url, "https://list.com");
    EXPECT_FALSE(sessions[0].sizeStr.empty());
    EXPECT_FALSE(sessions[0].lastAccessedStr.empty());
}

TEST_F(SessionManagerTest, ListMultipleSessions) {
    // Create multiple sessions
    Session session1("session1");
    session1.setCurrentUrl("https://one.com");
    session1.addToHistory("https://history1.com");
    
    Session session2("session2");
    session2.setCurrentUrl("https://two.com");
    session2.setLocalStorageItem("key", "value");
    
    Session session3("session3");
    session3.setCurrentUrl("https://three.com");
    Cookie cookie;
    cookie.name = "test";
    cookie.value = "value";
    session3.addCookie(cookie);
    
    manager->saveSession(session1);
    manager->saveSession(session2);
    manager->saveSession(session3);
    
    auto sessions = manager->listSessions();
    EXPECT_EQ(sessions.size(), 3);
    
    // Find each session in the list
    bool found1 = false, found2 = false, found3 = false;
    for (const auto& info : sessions) {
        if (info.name == "session1" && info.url == "https://one.com") {
            found1 = true;
        } else if (info.name == "session2" && info.url == "https://two.com") {
            found2 = true;
        } else if (info.name == "session3" && info.url == "https://three.com") {
            found3 = true;
        }
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
    EXPECT_TRUE(found3);
}

// ========== Session Size Information ==========

TEST_F(SessionManagerTest, SessionSizeTracking) {
    // Create sessions of different sizes
    Session smallSession("small");
    smallSession.setCurrentUrl("https://small.com");
    
    Session largeSession("large");
    largeSession.setCurrentUrl("https://large.com");
    // Add lots of data to make it larger
    for (int i = 0; i < 100; i++) {
        largeSession.addToHistory("https://history" + std::to_string(i) + ".com");
        largeSession.setCustomVariable("var" + std::to_string(i), "value" + std::to_string(i));
    }
    
    manager->saveSession(smallSession);
    manager->saveSession(largeSession);
    
    auto sessions = manager->listSessions();
    EXPECT_EQ(sessions.size(), 2);
    
    // Find both sessions and compare sizes
    SessionInfo smallInfo, largeInfo;
    for (const auto& info : sessions) {
        if (info.name == "small") {
            smallInfo = info;
        } else if (info.name == "large") {
            largeInfo = info;
        }
    }
    
    EXPECT_FALSE(smallInfo.sizeStr.empty());
    EXPECT_FALSE(largeInfo.sizeStr.empty());
    // Large session should have more data (this is a rough check)
    EXPECT_NE(smallInfo.sizeStr, largeInfo.sizeStr);
}

// ========== Edge Cases and Error Handling ==========

TEST_F(SessionManagerTest, InvalidSessionNames) {
    // Test with various edge case names
    Session emptyName("");
    EXPECT_NO_THROW(manager->saveSession(emptyName));
    
    Session spaceName("session with spaces");
    EXPECT_NO_THROW(manager->saveSession(spaceName));
    
    Session specialChars("session-with_special.chars");
    EXPECT_NO_THROW(manager->saveSession(specialChars));
    
    Session unicodeName("会话名称");
    EXPECT_NO_THROW(manager->saveSession(unicodeName));
}

TEST_F(SessionManagerTest, VeryLargeSession) {
    Session largeSession("large_session");
    largeSession.setCurrentUrl("https://large.com");
    
    // Add a large amount of data
    for (int i = 0; i < 1000; i++) {
        largeSession.addToHistory("https://page" + std::to_string(i) + ".com");
        largeSession.setCustomVariable("key" + std::to_string(i), 
                                     std::string(1000, 'x')); // 1KB per variable
        
        Cookie cookie;
        cookie.name = "cookie" + std::to_string(i);
        cookie.value = std::string(100, 'y'); // 100 bytes per cookie
        cookie.domain = "domain" + std::to_string(i) + ".com";
        largeSession.addCookie(cookie);
    }
    
    // Should be able to save and load large sessions
    EXPECT_NO_THROW(manager->saveSession(largeSession));
    
    Session loaded = manager->loadOrCreateSession("large_session");
    EXPECT_EQ(loaded.getHistory().size(), 1000);
    EXPECT_EQ(loaded.getCookies().size(), 1000);
    EXPECT_TRUE(loaded.hasCustomVariable("key999"));
}

TEST_F(SessionManagerTest, ConcurrentAccess) {
    // Test saving the same session name multiple times
    Session session1("concurrent");
    session1.setCurrentUrl("https://first.com");
    
    Session session2("concurrent");
    session2.setCurrentUrl("https://second.com");
    session2.setCustomVariable("version", "2");
    
    manager->saveSession(session1);
    manager->saveSession(session2); // Should overwrite
    
    Session loaded = manager->loadOrCreateSession("concurrent");
    EXPECT_EQ(loaded.getCurrentUrl(), "https://second.com");
    EXPECT_EQ(loaded.getCustomVariable("version"), "2");
}

TEST_F(SessionManagerTest, CorruptedSessionFile) {
    // Create a corrupted session file manually
    std::filesystem::path corruptedFile = test_dir / "corrupted.hweb";
    std::ofstream file(corruptedFile);
    file << "{ invalid json content }";
    file.close();
    
    // Loading should return a new session, not crash
    Session loaded = manager->loadOrCreateSession("corrupted");
    EXPECT_EQ(loaded.getName(), "corrupted");
    EXPECT_EQ(loaded.getCurrentUrl(), ""); // Should be empty/default
}

TEST_F(SessionManagerTest, ReadOnlyDirectory) {
    // Create a session first
    Session session("readonly_test");
    session.setCurrentUrl("https://test.com");
    manager->saveSession(session);
    
    // Make directory read-only
    std::filesystem::permissions(test_dir, 
                                std::filesystem::perms::owner_read | 
                                std::filesystem::perms::group_read | 
                                std::filesystem::perms::others_read);
    
    // Loading should still work
    Session loaded = manager->loadOrCreateSession("readonly_test");
    EXPECT_EQ(loaded.getCurrentUrl(), "https://test.com");
    
    // Restore permissions for cleanup
    std::filesystem::permissions(test_dir, 
                                std::filesystem::perms::owner_all | 
                                std::filesystem::perms::group_read | 
                                std::filesystem::perms::others_read);
}

// ========== Session Path Handling ==========

TEST_F(SessionManagerTest, SessionFileNaming) {
    // Test different session names and their file mappings
    std::vector<std::string> sessionNames = {
        "simple",
        "with-dashes",
        "with_underscores",
        "with.dots",
        "MixedCase",
        "123numeric",
        "session with spaces"
    };
    
    for (const auto& name : sessionNames) {
        Session session(name);
        session.setCurrentUrl("https://" + name + ".com");
        manager->saveSession(session);
        
        // Should be able to load it back
        Session loaded = manager->loadOrCreateSession(name);
        EXPECT_EQ(loaded.getName(), name);
        EXPECT_EQ(loaded.getCurrentUrl(), "https://" + name + ".com");
    }
    
    // All sessions should be listed
    auto sessions = manager->listSessions();
    EXPECT_EQ(sessions.size(), sessionNames.size());
}

TEST_F(SessionManagerTest, NestedDirectoryCreation) {
    // Test with a SessionManager that uses a nested directory
    std::filesystem::path nested_dir = test_dir / "level1" / "level2" / "sessions";
    SessionManager nested_manager(nested_dir.string());
    
    Session session("nested_test");
    session.setCurrentUrl("https://nested.com");
    
    // Should create nested directories and save successfully
    EXPECT_NO_THROW(nested_manager.saveSession(session));
    EXPECT_TRUE(std::filesystem::exists(nested_dir));
    
    Session loaded = nested_manager.loadOrCreateSession("nested_test");
    EXPECT_EQ(loaded.getCurrentUrl(), "https://nested.com");
}

TEST_F(SessionManagerTest, SessionLastAccessedTime) {
    Session session("time_test");
    session.setCurrentUrl("https://time.com");
    session.updateLastAccessed(); // Set a specific time
    
    manager->saveSession(session);
    
    auto sessions = manager->listSessions();
    EXPECT_EQ(sessions.size(), 1);
    EXPECT_FALSE(sessions[0].lastAccessedStr.empty());
    
    // Last accessed string should contain reasonable time format
    const std::string& timeStr = sessions[0].lastAccessedStr;
    EXPECT_TRUE(timeStr.find(":") != std::string::npos); // Should contain time separator
}

// ========== Memory and Performance ==========

TEST_F(SessionManagerTest, ManySessionsPerformance) {
    // Create many sessions to test performance
    const int sessionCount = 100;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < sessionCount; i++) {
        Session session("perf_session_" + std::to_string(i));
        session.setCurrentUrl("https://perf" + std::to_string(i) + ".com");
        session.addToHistory("https://history.com");
        session.setCustomVariable("id", std::to_string(i));
        manager->saveSession(session);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Saving 100 sessions should be reasonably fast (less than 10 seconds)
    EXPECT_LT(duration.count(), 10000);
    
    // Listing should also be fast
    start = std::chrono::high_resolution_clock::now();
    auto sessions = manager->listSessions();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(sessions.size(), sessionCount);
    EXPECT_LT(duration.count(), 1000); // Less than 1 second to list 100 sessions
}