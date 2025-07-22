#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <filesystem>
#include <functional>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <sys/wait.h>

namespace TestHelpers {

// File system utilities
std::filesystem::path createTemporaryDirectory(const std::string& prefix = "hweb_test");
void createTestFile(const std::filesystem::path& filepath, const std::string& content);
void createTestFile(const std::filesystem::path& filepath, size_t size_bytes, char fill_char = 'x');
std::string readFileContent(const std::filesystem::path& filepath);
void cleanupDirectory(const std::filesystem::path& directory);

// Waiting utilities
bool waitForCondition(std::function<bool()> condition, 
                     std::chrono::milliseconds timeout = std::chrono::milliseconds(5000),
                     std::chrono::milliseconds poll_interval = std::chrono::milliseconds(100));
bool waitForFileExists(const std::filesystem::path& filepath,
                      std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
bool waitForFileSize(const std::filesystem::path& filepath,
                    size_t expected_size,
                    std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

// Test server management
class TestServerManager {
public:
    TestServerManager(const std::string& server_url = "http://localhost:9876", 
                     const std::string& server_script_path = "../test_server/start_test_server.sh");
    ~TestServerManager();
    
    bool isServerRunning();
    bool startServer();
    void stopServer();
    std::string getServerUrl() const { return server_url_; }

private:
    std::string server_url_;
    std::string server_script_path_;
    pid_t server_pid_;
    bool server_started_by_us_;
    
    bool checkServerHealth();
};

// Random data generation
std::string generateRandomString(size_t length);
std::vector<uint8_t> generateRandomBinaryData(size_t size);

// RAII Guards and helpers
class FileSystemGuard {
public:
    explicit FileSystemGuard(const std::filesystem::path& path);
    ~FileSystemGuard();
    void cleanup();
    void disableAutoCleanup();

private:
    std::filesystem::path protected_path;
    bool original_existed = false;
    std::string original_content;
    bool auto_cleanup;
};

class TemporaryDirectory {
public:
    explicit TemporaryDirectory(const std::string& prefix = "hweb_test");
    ~TemporaryDirectory();
    
    const std::filesystem::path& getPath() const;
    std::filesystem::path createFile(const std::string& filename, const std::string& content = "test");
    std::filesystem::path createSubdirectory(const std::string& dirname);

private:
    std::filesystem::path path;
};

class Timer {
public:
    Timer();
    void start();
    std::chrono::milliseconds elapsed() const;
    bool hasElapsed(std::chrono::milliseconds duration) const;

private:
    std::chrono::high_resolution_clock::time_point start_time;
};

// Test macros
#define EXPECT_NEAR_DURATION(actual, expected, tolerance_ms) \
    EXPECT_NEAR(std::chrono::duration_cast<std::chrono::milliseconds>(actual).count(), \
                std::chrono::duration_cast<std::chrono::milliseconds>(expected).count(), \
                tolerance_ms)

#define ASSERT_FILE_EXISTS(filepath) \
    ASSERT_TRUE(std::filesystem::exists(filepath)) << "File does not exist: " << filepath

#define ASSERT_FILE_NOT_EXISTS(filepath) \
    ASSERT_FALSE(std::filesystem::exists(filepath)) << "File should not exist: " << filepath

#define EXPECT_FILE_SIZE(filepath, expected_size) \
    do { \
        ASSERT_FILE_EXISTS(filepath); \
        auto actual_size = std::filesystem::file_size(filepath); \
        EXPECT_EQ(actual_size, expected_size) << "File size mismatch for: " << filepath; \
    } while(0)

} // namespace TestHelpers