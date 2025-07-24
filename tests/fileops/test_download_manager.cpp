#include <gtest/gtest.h>
#include "../../src/FileOps/DownloadManager.h"
#include "../../src/FileOps/Types.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace FileOps {

class DownloadManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directory
        test_dir_ = std::filesystem::temp_directory_path() / "hweb_download_test";
        std::filesystem::create_directories(test_dir_);
        
        // Initialize download manager
        download_manager_ = std::make_unique<DownloadManager>();
        download_manager_->setDownloadDirectory(test_dir_.string());
        download_manager_->setDefaultTimeout(5000);
        download_manager_->setPollingInterval(100);
    }
    
    void TearDown() override {
        // Clean up temporary directory
        std::filesystem::remove_all(test_dir_);
    }
    
    // Helper to create test file
    void createTestFile(const std::string& filename, const std::string& content = "test content") {
        std::ofstream file(test_dir_ / filename);
        file << content;
        file.close();
    }
    
    // Helper to create test file with delay (simulates download)
    void createTestFileDelayed(const std::string& filename, int delay_ms, const std::string& content = "test content") {
        std::thread([this, filename, delay_ms, content]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            createTestFile(filename, content);
        }).detach();
    }
    
    // Helper to create progressive file (simulates download in progress)
    void createProgressiveFile(const std::string& filename, int chunks, int delay_per_chunk_ms) {
        std::thread([this, filename, chunks, delay_per_chunk_ms]() {
            std::ofstream file(test_dir_ / filename, std::ios::trunc);
            for (int i = 0; i < chunks; ++i) {
                file << "chunk" << i << "_";
                file.flush();
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_per_chunk_ms));
            }
            file.close();
        }).detach();
    }

    std::filesystem::path test_dir_;
    std::unique_ptr<DownloadManager> download_manager_;
};

// ========== Directory Management Tests ==========

TEST_F(DownloadManagerTest, GetDownloadDirectory) {
    std::string dir = download_manager_->getDownloadDirectory();
    EXPECT_EQ(dir, test_dir_.string());
}

TEST_F(DownloadManagerTest, SetDownloadDirectory_ValidDirectory) {
    std::filesystem::path new_dir = test_dir_ / "subdir";
    std::filesystem::create_directories(new_dir);
    
    EXPECT_TRUE(download_manager_->setDownloadDirectory(new_dir.string()));
    EXPECT_EQ(download_manager_->getDownloadDirectory(), new_dir.string());
}

TEST_F(DownloadManagerTest, SetDownloadDirectory_NonExistentDirectory) {
    std::filesystem::path new_dir = test_dir_ / "nonexistent";
    
    EXPECT_TRUE(download_manager_->setDownloadDirectory(new_dir.string())); // Should create it
    EXPECT_TRUE(std::filesystem::exists(new_dir));
}

TEST_F(DownloadManagerTest, EnsureDownloadDirectoryExists) {
    std::filesystem::path new_dir = test_dir_ / "ensure_test";
    
    EXPECT_TRUE(download_manager_->ensureDownloadDirectoryExists(new_dir.string()));
    EXPECT_TRUE(std::filesystem::exists(new_dir));
}

TEST_F(DownloadManagerTest, GetPotentialDownloadDirectories) {
    auto directories = download_manager_->getPotentialDownloadDirectories();
    EXPECT_FALSE(directories.empty());
    // Should contain at least current working directory and temp
    bool found_cwd = false, found_temp = false;
    for (const auto& dir : directories) {
        if (dir == std::filesystem::current_path().string()) found_cwd = true;
        if (dir.find("tmp") != std::string::npos || dir.find("temp") != std::string::npos) found_temp = true;
    }
    EXPECT_TRUE(found_cwd || found_temp); // At least one should be found
}

// ========== File Detection Tests ==========

TEST_F(DownloadManagerTest, FindMatchingFiles_ExactMatch) {
    createTestFile("test.txt");
    createTestFile("other.doc");
    
    auto files = download_manager_->findMatchingFiles(test_dir_.string(), "test.txt");
    EXPECT_EQ(files.size(), 1);
    EXPECT_TRUE(files[0].find("test.txt") != std::string::npos);
}

TEST_F(DownloadManagerTest, FindMatchingFiles_GlobPattern) {
    createTestFile("test1.txt");
    createTestFile("test2.txt");
    createTestFile("other.doc");
    
    auto files = download_manager_->findMatchingFiles(test_dir_.string(), "test*.txt");
    EXPECT_EQ(files.size(), 2);
}

TEST_F(DownloadManagerTest, FileMatchesPattern_ExactMatch) {
    EXPECT_TRUE(download_manager_->fileMatchesPattern("/path/to/test.txt", "test.txt"));
    EXPECT_FALSE(download_manager_->fileMatchesPattern("/path/to/other.txt", "test.txt"));
}

TEST_F(DownloadManagerTest, FileMatchesPattern_GlobPattern) {
    EXPECT_TRUE(download_manager_->fileMatchesPattern("/path/to/test.txt", "*.txt"));
    EXPECT_TRUE(download_manager_->fileMatchesPattern("/path/to/document.pdf", "*.pdf"));
    EXPECT_FALSE(download_manager_->fileMatchesPattern("/path/to/image.png", "*.txt"));
}

TEST_F(DownloadManagerTest, GetMostRecentMatchingFile) {
    createTestFile("old.txt");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    createTestFile("new.txt");
    
    std::string recent = download_manager_->getMostRecentMatchingFile(test_dir_.string(), "*.txt");
    EXPECT_TRUE(recent.find("new.txt") != std::string::npos);
}

// ========== Pattern Matching Tests ==========

TEST_F(DownloadManagerTest, IsGlobPattern) {
    EXPECT_TRUE(download_manager_->isGlobPattern("*.txt"));
    EXPECT_TRUE(download_manager_->isGlobPattern("test?.doc"));
    EXPECT_TRUE(download_manager_->isGlobPattern("file[0-9].pdf"));
    EXPECT_FALSE(download_manager_->isGlobPattern("exact.txt"));
}

TEST_F(DownloadManagerTest, IsRegexPattern) {
    EXPECT_TRUE(download_manager_->isRegexPattern("/test.*\\.txt/"));
    EXPECT_FALSE(download_manager_->isRegexPattern("*.txt"));
}

TEST_F(DownloadManagerTest, GlobToRegex) {
    std::string regex = download_manager_->globToRegex("*.txt");
    EXPECT_FALSE(regex.empty());
    // Should convert * to .* and escape .
    EXPECT_TRUE(regex.find(".*") != std::string::npos);
}

// ========== Browser Integration Tests ==========

TEST_F(DownloadManagerTest, IsBrowserTempFile) {
    EXPECT_TRUE(download_manager_->isBrowserTempFile("test.txt.crdownload"));
    EXPECT_TRUE(download_manager_->isBrowserTempFile("document.pdf.part"));
    EXPECT_TRUE(download_manager_->isBrowserTempFile("image.png.download"));
    EXPECT_TRUE(download_manager_->isBrowserTempFile("file.tmp"));
    EXPECT_TRUE(download_manager_->isBrowserTempFile("~tempfile"));
    EXPECT_FALSE(download_manager_->isBrowserTempFile("normal.txt"));
}

TEST_F(DownloadManagerTest, ResolveBrowserTempFile) {
    EXPECT_EQ(download_manager_->resolveBrowserTempFile("/path/test.txt.crdownload"), "/path/test.txt");
    EXPECT_EQ(download_manager_->resolveBrowserTempFile("/path/doc.pdf.part"), "/path/doc.pdf");
    EXPECT_EQ(download_manager_->resolveBrowserTempFile("/path/img.png.download"), "/path/img.png");
    EXPECT_EQ(download_manager_->resolveBrowserTempFile("/path/normal.txt"), "/path/normal.txt");
}

TEST_F(DownloadManagerTest, GetBrowserDownloadPatterns) {
    auto patterns = download_manager_->getBrowserDownloadPatterns("test.txt");
    EXPECT_GE(patterns.size(), 4);
    
    // Should include exact filename and browser temp patterns
    bool found_exact = false, found_chrome = false, found_firefox = false;
    for (const auto& pattern : patterns) {
        if (pattern == "test.txt") found_exact = true;
        if (pattern == "test.txt.crdownload") found_chrome = true;
        if (pattern == "test.txt.part") found_firefox = true;
    }
    EXPECT_TRUE(found_exact);
    EXPECT_TRUE(found_chrome);
    EXPECT_TRUE(found_firefox);
}

// ========== Download Completion Tests ==========

TEST_F(DownloadManagerTest, IsFileSizeStable_StableFile) {
    createTestFile("stable.txt", "constant content");
    
    bool stable = download_manager_->isFileSizeStable(
        (test_dir_ / "stable.txt").string(),
        std::chrono::milliseconds(500)
    );
    EXPECT_TRUE(stable);
}

TEST_F(DownloadManagerTest, IsFileSizeStable_ChangingFile) {
    createProgressiveFile("changing.txt", 3, 200);
    
    bool stable = download_manager_->isFileSizeStable(
        (test_dir_ / "changing.txt").string(),
        std::chrono::milliseconds(500)
    );
    EXPECT_FALSE(stable);
}

TEST_F(DownloadManagerTest, IsDownloadInProgress_RegularFile) {
    createTestFile("regular.txt");
    
    EXPECT_FALSE(download_manager_->isDownloadInProgress((test_dir_ / "regular.txt").string()));
}

TEST_F(DownloadManagerTest, IsDownloadInProgress_TempFile) {
    createTestFile("temp.txt.crdownload");
    
    EXPECT_TRUE(download_manager_->isDownloadInProgress((test_dir_ / "temp.txt.crdownload").string()));
}

TEST_F(DownloadManagerTest, WaitForDownloadCompletion_ImmediateComplete) {
    createTestFile("complete.txt", "final content");
    
    bool completed = download_manager_->waitForDownloadCompletion(
        (test_dir_ / "complete.txt").string(),
        2000
    );
    EXPECT_TRUE(completed);
}

TEST_F(DownloadManagerTest, WaitForDownloadCompletion_DelayedComplete) {
    createTestFileDelayed("delayed.txt", 500, "final content");
    
    bool completed = download_manager_->waitForDownloadCompletion(
        (test_dir_ / "delayed.txt").string(),
        2000
    );
    EXPECT_TRUE(completed);
}

TEST_F(DownloadManagerTest, WaitForDownloadCompletion_Timeout) {
    bool completed = download_manager_->waitForDownloadCompletion(
        (test_dir_ / "nonexistent.txt").string(),
        500
    );
    EXPECT_FALSE(completed);
}

// ========== Download Integrity Tests ==========

TEST_F(DownloadManagerTest, VerifyDownloadIntegrity_ValidFile) {
    std::string content = "test content for integrity check";
    createTestFile("integrity.txt", content);
    
    bool valid = download_manager_->verifyDownloadIntegrity(
        (test_dir_ / "integrity.txt").string(),
        content.length()
    );
    EXPECT_TRUE(valid);
}

TEST_F(DownloadManagerTest, VerifyDownloadIntegrity_WrongSize) {
    createTestFile("wrongsize.txt", "short");
    
    bool valid = download_manager_->verifyDownloadIntegrity(
        (test_dir_ / "wrongsize.txt").string(),
        1000 // Expected much larger
    );
    EXPECT_FALSE(valid);
}

TEST_F(DownloadManagerTest, VerifyDownloadIntegrity_NonExistentFile) {
    bool valid = download_manager_->verifyDownloadIntegrity(
        (test_dir_ / "nonexistent.txt").string(),
        100
    );
    EXPECT_FALSE(valid);
}

TEST_F(DownloadManagerTest, VerifyDownloadIntegrity_ZeroByteFile) {
    createTestFile("empty.txt", "");
    
    bool valid = download_manager_->verifyDownloadIntegrity(
        (test_dir_ / "empty.txt").string(),
        0
    );
    EXPECT_FALSE(valid); // Zero byte files should fail integrity check
}

// ========== Progress and Statistics Tests ==========

TEST_F(DownloadManagerTest, GetDownloadProgress_ValidFile) {
    createTestFile("progress.txt", "1234567890"); // 10 bytes
    
    int progress = download_manager_->getDownloadProgress(
        (test_dir_ / "progress.txt").string(),
        20 // Expected 20 bytes
    );
    EXPECT_EQ(progress, 50); // 10/20 * 100 = 50%
}

TEST_F(DownloadManagerTest, GetDownloadProgress_NonExistentFile) {
    int progress = download_manager_->getDownloadProgress(
        (test_dir_ / "nonexistent.txt").string(),
        100
    );
    EXPECT_EQ(progress, -1);
}

TEST_F(DownloadManagerTest, GetDownloadStatistics) {
    auto stats = download_manager_->getDownloadStatistics();
    EXPECT_GE(stats.active_downloads, 0);
    EXPECT_GE(stats.completed_downloads, 0);
    EXPECT_GE(stats.failed_downloads, 0);
}

// ========== Configuration Tests ==========

TEST_F(DownloadManagerTest, SetDefaultTimeout) {
    download_manager_->setDefaultTimeout(10000);
    // No direct getter, but this shouldn't crash
    SUCCEED();
}

TEST_F(DownloadManagerTest, SetStabilityCheckDuration) {
    download_manager_->setStabilityCheckDuration(std::chrono::milliseconds(1000));
    SUCCEED();
}

TEST_F(DownloadManagerTest, SetIntegrityVerificationEnabled) {
    download_manager_->setIntegrityVerificationEnabled(false);
    
    // With integrity disabled, even wrong size should pass
    createTestFile("nointegrity.txt", "short");
    bool valid = download_manager_->verifyDownloadIntegrity(
        (test_dir_ / "nointegrity.txt").string(),
        1000
    );
    EXPECT_TRUE(valid); // Should pass with integrity disabled
}

TEST_F(DownloadManagerTest, SetPollingInterval) {
    download_manager_->setPollingInterval(50);
    SUCCEED();
}

// ========== Error Handling Tests ==========

TEST_F(DownloadManagerTest, DownloadResultToString) {
    EXPECT_EQ(download_manager_->downloadResultToString(DownloadResult::SUCCESS), "SUCCESS");
    EXPECT_EQ(download_manager_->downloadResultToString(DownloadResult::TIMEOUT), "TIMEOUT");
    EXPECT_EQ(download_manager_->downloadResultToString(DownloadResult::FILE_NOT_FOUND), "FILE_NOT_FOUND");
}

TEST_F(DownloadManagerTest, GetErrorMessage) {
    std::string msg = download_manager_->getErrorMessage(DownloadResult::TIMEOUT, "*.txt");
    EXPECT_TRUE(msg.find("timeout") != std::string::npos);
    EXPECT_TRUE(msg.find("*.txt") != std::string::npos);
}

// ========== Advanced Features Tests ==========

TEST_F(DownloadManagerTest, SetDownloadCompletionHook) {
    bool hook_called = false;
    std::string hook_filename;
    
    download_manager_->setDownloadCompletionHook([&](const std::string& filename) {
        hook_called = true;
        hook_filename = filename;
    });
    
    // Test the hook by performing a download operation
    createTestFile("hook_test.txt");
    
    DownloadCommand cmd;
    cmd.filename_pattern = "hook_test.txt";
    cmd.download_dir = test_dir_.string();
    cmd.timeout_ms = 2000;
    
    DownloadResult result = download_manager_->waitForDownload(cmd);
    EXPECT_EQ(result, DownloadResult::SUCCESS);
    EXPECT_TRUE(hook_called);
    EXPECT_TRUE(hook_filename.find("hook_test.txt") != std::string::npos);
}

TEST_F(DownloadManagerTest, CreateAndCheckDownloadManifest) {
    std::vector<std::string> expected_files = {"file1.txt", "file2.txt", "file3.txt"};
    std::string manifest_path = (test_dir_ / "manifest.txt").string();
    
    download_manager_->createDownloadManifest(expected_files, manifest_path);
    
    EXPECT_TRUE(std::filesystem::exists(manifest_path));
    EXPECT_FALSE(download_manager_->isDownloadManifestComplete(manifest_path)); // All pending
}

// ========== Polling File Monitor Tests ==========

TEST_F(DownloadManagerTest, StartPollingFileMonitor_FileFound) {
    // Start monitoring in background
    bool file_found = false;
    std::string found_file;
    
    auto callback = [&](const std::string& filepath) {
        file_found = true;
        found_file = filepath;
    };
    
    // Create file after short delay
    createTestFileDelayed("polling_test.txt", 200);
    
    bool result = download_manager_->startPollingFileMonitor(
        test_dir_.string(),
        "polling_test.txt",
        100, // 100ms polling interval
        2000, // 2s timeout
        callback
    );
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(file_found);
    EXPECT_TRUE(found_file.find("polling_test.txt") != std::string::npos);
}

TEST_F(DownloadManagerTest, StartPollingFileMonitor_Timeout) {
    bool file_found = false;
    
    auto callback = [&](const std::string& filepath) {
        file_found = true;
    };
    
    bool result = download_manager_->startPollingFileMonitor(
        test_dir_.string(),
        "never_created.txt",
        100, // 100ms polling interval
        500, // 500ms timeout
        callback
    );
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(file_found);
}

// ========== Platform Detection Tests ==========

TEST_F(DownloadManagerTest, IsNativeFileWatchingAvailable) {
    bool available = download_manager_->isNativeFileWatchingAvailable();
    // Should be true on Linux, macOS, Windows
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    EXPECT_TRUE(available);
#else
    EXPECT_FALSE(available);
#endif
}

// ========== Integration Tests ==========

TEST_F(DownloadManagerTest, WaitForDownload_ExistingFile) {
    createTestFile("existing.txt", "pre-existing content");
    
    DownloadCommand cmd;
    cmd.filename_pattern = "existing.txt";
    cmd.download_dir = test_dir_.string();
    cmd.timeout_ms = 1000;
    cmd.verify_integrity = true;
    
    DownloadResult result = download_manager_->waitForDownload(cmd);
    EXPECT_EQ(result, DownloadResult::SUCCESS);
}

TEST_F(DownloadManagerTest, WaitForDownload_NewFile) {
    // Create file after delay to simulate download
    createTestFileDelayed("new_download.txt", 500, "downloaded content");
    
    DownloadCommand cmd;
    cmd.filename_pattern = "new_download.txt";
    cmd.download_dir = test_dir_.string();
    cmd.timeout_ms = 2000;
    cmd.verify_integrity = true;
    
    DownloadResult result = download_manager_->waitForDownload(cmd);
    EXPECT_EQ(result, DownloadResult::SUCCESS);
}

TEST_F(DownloadManagerTest, WaitForDownload_DirectoryNotFound) {
    DownloadCommand cmd;
    cmd.filename_pattern = "test.txt";
    cmd.download_dir = "/nonexistent/directory";
    cmd.timeout_ms = 1000;
    
    DownloadResult result = download_manager_->waitForDownload(cmd);
    EXPECT_EQ(result, DownloadResult::DIRECTORY_NOT_FOUND);
}

TEST_F(DownloadManagerTest, WaitForMultipleDownloads_Success) {
    // Create files with delays
    createTestFileDelayed("multi1.txt", 300, "content1");
    createTestFileDelayed("multi2.txt", 600, "content2");
    
    std::vector<std::string> patterns = {"multi1.txt", "multi2.txt"};
    
    DownloadResult result = download_manager_->waitForMultipleDownloads(
        patterns,
        test_dir_.string(),
        3000
    );
    
    EXPECT_EQ(result, DownloadResult::SUCCESS);
}

TEST_F(DownloadManagerTest, WaitForMultipleDownloads_Timeout) {
    std::vector<std::string> patterns = {"never1.txt", "never2.txt"};
    
    DownloadResult result = download_manager_->waitForMultipleDownloads(
        patterns,
        test_dir_.string(),
        500
    );
    
    EXPECT_EQ(result, DownloadResult::TIMEOUT);
}

// ========== Enhanced Browser Integration Tests ==========

TEST_F(DownloadManagerTest, EnhancedBrowserTempFileDetection) {
    // Test all browser temporary file patterns
    EXPECT_TRUE(download_manager_->isBrowserTempFile("download.crdownload")); // Chrome
    EXPECT_TRUE(download_manager_->isBrowserTempFile("download.part")); // Firefox
    EXPECT_TRUE(download_manager_->isBrowserTempFile("download.download")); // Safari
    EXPECT_TRUE(download_manager_->isBrowserTempFile("download.partial")); // Edge
    EXPECT_TRUE(download_manager_->isBrowserTempFile("download.tmp")); // Generic
    EXPECT_TRUE(download_manager_->isBrowserTempFile("download.temp")); // Generic
    EXPECT_TRUE(download_manager_->isBrowserTempFile("~download.txt")); // Temp prefix
    EXPECT_TRUE(download_manager_->isBrowserTempFile("temp_download.txt")); // Temp prefix
    EXPECT_TRUE(download_manager_->isBrowserTempFile(".tmp_download")); // Hidden temp
    EXPECT_TRUE(download_manager_->isBrowserTempFile("download.opr")); // Opera
    
    // Test case insensitivity
    EXPECT_TRUE(download_manager_->isBrowserTempFile("DOWNLOAD.CRDOWNLOAD"));
    EXPECT_TRUE(download_manager_->isBrowserTempFile("Download.Part"));
    
    // Test non-temp files
    EXPECT_FALSE(download_manager_->isBrowserTempFile("normal_file.txt"));
    EXPECT_FALSE(download_manager_->isBrowserTempFile("document.pdf"));
    EXPECT_FALSE(download_manager_->isBrowserTempFile("image.jpg"));
}


} // namespace FileOps