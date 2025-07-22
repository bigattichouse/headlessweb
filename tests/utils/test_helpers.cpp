#include "test_helpers.h"
#include <fstream>
#include <random>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <signal.h>

namespace TestHelpers {

std::filesystem::path createTemporaryDirectory(const std::string& prefix) {
    auto temp_dir = std::filesystem::temp_directory_path();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::string dir_name = prefix + "_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));
    auto test_dir = temp_dir / dir_name;
    
    std::filesystem::create_directories(test_dir);
    return test_dir;
}

void createTestFile(const std::filesystem::path& filepath, const std::string& content) {
    // Create parent directories if they don't exist
    std::filesystem::create_directories(filepath.parent_path());
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create test file: " + filepath.string());
    }
    file << content;
    file.close();
}

void createTestFile(const std::filesystem::path& filepath, size_t size_bytes, char fill_char) {
    std::filesystem::create_directories(filepath.parent_path());
    
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create test file: " + filepath.string());
    }
    
    // Write in chunks to avoid memory issues with large files
    const size_t chunk_size = 8192;
    std::string chunk(std::min(chunk_size, size_bytes), fill_char);
    
    size_t remaining = size_bytes;
    while (remaining > 0) {
        size_t to_write = std::min(chunk_size, remaining);
        if (to_write < chunk_size) {
            chunk.resize(to_write);
        }
        file.write(chunk.data(), to_write);
        remaining -= to_write;
    }
    file.close();
}

std::string readFileContent(const std::filesystem::path& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to read file: " + filepath.string());
    }
    
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

void cleanupDirectory(const std::filesystem::path& directory) {
    if (std::filesystem::exists(directory)) {
        std::error_code ec;
        std::filesystem::remove_all(directory, ec);
        // Ignore errors during cleanup
    }
}

bool waitForCondition(std::function<bool()> condition, 
                     std::chrono::milliseconds timeout,
                     std::chrono::milliseconds poll_interval) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        if (condition()) {
            return true;
        }
        std::this_thread::sleep_for(poll_interval);
    }
    
    return false;
}

bool waitForFileExists(const std::filesystem::path& filepath,
                      std::chrono::milliseconds timeout) {
    return waitForCondition([&filepath]() {
        return std::filesystem::exists(filepath);
    }, timeout);
}

bool waitForFileSize(const std::filesystem::path& filepath,
                    size_t expected_size,
                    std::chrono::milliseconds timeout) {
    return waitForCondition([&filepath, expected_size]() {
        if (!std::filesystem::exists(filepath)) {
            return false;
        }
        std::error_code ec;
        auto size = std::filesystem::file_size(filepath, ec);
        return !ec && size == expected_size;
    }, timeout);
}

std::string generateRandomString(size_t length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    std::string result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        result += chars[dis(gen)];
    }
    
    return result;
}

std::vector<uint8_t> generateRandomBinaryData(size_t size) {
    std::vector<uint8_t> data(size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    
    for (size_t i = 0; i < size; ++i) {
        data[i] = dis(gen);
    }
    
    return data;
}

FileSystemGuard::FileSystemGuard(const std::filesystem::path& path) 
    : protected_path(path), auto_cleanup(true) {
    if (std::filesystem::exists(path)) {
        original_existed = true;
        if (std::filesystem::is_regular_file(path)) {
            original_content = readFileContent(path);
        }
    }
}

FileSystemGuard::~FileSystemGuard() {
    if (auto_cleanup) {
        cleanup();
    }
}

void FileSystemGuard::cleanup() {
    if (original_existed) {
        if (!original_content.empty()) {
            // Restore original file content
            createTestFile(protected_path, original_content);
        } else if (!std::filesystem::exists(protected_path)) {
            // Recreate directory if it was a directory
            std::filesystem::create_directories(protected_path);
        }
    } else {
        // Remove if it didn't exist originally
        cleanupDirectory(protected_path);
    }
}

void FileSystemGuard::disableAutoCleanup() {
    auto_cleanup = false;
}

TemporaryDirectory::TemporaryDirectory(const std::string& prefix) {
    path = createTemporaryDirectory(prefix);
}

TemporaryDirectory::~TemporaryDirectory() {
    cleanupDirectory(path);
}

const std::filesystem::path& TemporaryDirectory::getPath() const {
    return path;
}

std::filesystem::path TemporaryDirectory::createFile(const std::string& filename, 
                                                   const std::string& content) {
    auto filepath = path / filename;
    createTestFile(filepath, content);
    return filepath;
}

std::filesystem::path TemporaryDirectory::createSubdirectory(const std::string& dirname) {
    auto dirpath = path / dirname;
    std::filesystem::create_directories(dirpath);
    return dirpath;
}

Timer::Timer() {
    start();
}

void Timer::start() {
    start_time = std::chrono::high_resolution_clock::now();
}

std::chrono::milliseconds Timer::elapsed() const {
    auto end_time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
}

bool Timer::hasElapsed(std::chrono::milliseconds duration) const {
    return elapsed() >= duration;
}

// ========== Test Server Management ==========

TestServerManager::TestServerManager(const std::string& server_url, const std::string& server_script_path)
    : server_url_(server_url), server_script_path_(server_script_path), server_pid_(-1), server_started_by_us_(false) {
}

TestServerManager::~TestServerManager() {
    if (server_started_by_us_) {
        stopServer();
    }
}

bool TestServerManager::isServerRunning() {
    return checkServerHealth();
}

bool TestServerManager::startServer() {
    // Check if server is already running
    if (isServerRunning()) {
        return true; // Server already running
    }
    
    // Get the absolute path to the script
    std::filesystem::path script_path = std::filesystem::absolute(server_script_path_);
    if (!std::filesystem::exists(script_path)) {
        std::cerr << "Test server script not found: " << script_path << std::endl;
        return false;
    }
    
    // Fork a child process to run the server
    server_pid_ = fork();
    if (server_pid_ == -1) {
        std::cerr << "Failed to fork process for test server" << std::endl;
        return false;
    }
    
    if (server_pid_ == 0) {
        // Child process: execute the server script
        std::string script_dir = script_path.parent_path().string();
        chdir(script_dir.c_str());
        
        // Redirect stdout/stderr to avoid cluttering test output
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        
        execl("/bin/bash", "bash", script_path.filename().c_str(), nullptr);
        exit(1); // If execl fails
    }
    
    // Parent process: wait for server to be ready
    server_started_by_us_ = true;
    
    // Wait up to 15 seconds for server to start (Node.js startup can be slow)
    for (int i = 0; i < 150; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (checkServerHealth()) {
            return true;
        }
    }
    
    std::cerr << "Test server failed to start within 15 seconds" << std::endl;
    stopServer();
    return false;
}

void TestServerManager::stopServer() {
    if (server_pid_ > 0 && server_started_by_us_) {
        kill(server_pid_, SIGTERM);
        
        // Wait for process to terminate
        int status;
        waitpid(server_pid_, &status, 0);
        
        server_pid_ = -1;
        server_started_by_us_ = false;
    }
}

bool TestServerManager::checkServerHealth() {
    // Simple check using wget (more commonly available than curl)
    std::string health_url = server_url_ + "/health";
    std::string cmd = "wget -q --timeout=1 --tries=1 -O /dev/null " + health_url + " 2>/dev/null";
    int result = system(cmd.c_str());
    if (result != 0) {
        // Fallback to curl if wget is not available
        cmd = "curl -s --connect-timeout 1 --max-time 1 " + health_url + " > /dev/null 2>&1";
        result = system(cmd.c_str());
    }
    return result == 0;
}

} // namespace TestHelpers