#include <gtest/gtest.h>
#include "FileOps/PathUtils.h"
#include "../utils/test_helpers.h"
#include <filesystem>

using namespace FileOps;

class PathUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("path_utils_tests");
        
        // Create test files and directories
        test_file = temp_dir->createFile("test.txt", "test content");
        nested_file = temp_dir->createFile("subdir/nested.pdf", "nested content");
        executable_file = temp_dir->createFile("script.sh", "#!/bin/bash\necho hello");
        
        // Make script executable
        std::filesystem::permissions(executable_file, 
            std::filesystem::perms::owner_read | 
            std::filesystem::perms::owner_write | 
            std::filesystem::perms::owner_exec);
    }

    void TearDown() override {
        temp_dir.reset();
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::filesystem::path test_file;
    std::filesystem::path nested_file;
    std::filesystem::path executable_file;
};

// ========== Path Normalization Tests ==========

TEST_F(PathUtilsTest, NormalizePathBasic) {
    std::string normalized = PathUtils::normalizePath("/path/to/file.txt");
    
    EXPECT_FALSE(normalized.empty());
    EXPECT_EQ(normalized.find("//"), std::string::npos); // No double slashes
}

TEST_F(PathUtilsTest, NormalizePathWithDots) {
    std::string path = "/path/./to/../file.txt";
    std::string normalized = PathUtils::normalizePath(path);
    
    EXPECT_EQ(normalized, "/path/file.txt");
}

TEST_F(PathUtilsTest, NormalizePathWindows) {
    std::string path = "C:\\path\\to\\file.txt";
    std::string normalized = PathUtils::normalizePath(path);
    
    // Should normalize to forward slashes on Unix systems
#ifdef _WIN32
    EXPECT_NE(normalized.find("\\"), std::string::npos);
#else
    EXPECT_EQ(normalized.find("\\"), std::string::npos);
#endif
}

TEST_F(PathUtilsTest, ToAbsolutePath) {
    std::string relative = "relative/path/file.txt";
    std::string absolute = PathUtils::toAbsolutePath(relative);
    
    EXPECT_TRUE(absolute.front() == '/' || (absolute.size() > 1 && absolute[1] == ':')); // Unix or Windows absolute
    EXPECT_NE(absolute.find("file.txt"), std::string::npos);
}

TEST_F(PathUtilsTest, ToAbsolutePathAlreadyAbsolute) {
    std::string already_absolute = "/already/absolute/path.txt";
    std::string result = PathUtils::toAbsolutePath(already_absolute);
    
    EXPECT_EQ(result, already_absolute);
}

// ========== Path Component Extraction ==========

TEST_F(PathUtilsTest, GetFileName) {
    EXPECT_EQ(PathUtils::getFileName("/path/to/file.txt"), "file.txt");
    EXPECT_EQ(PathUtils::getFileName("file.txt"), "file.txt");
    EXPECT_EQ(PathUtils::getFileName("/path/to/"), "");
    EXPECT_EQ(PathUtils::getFileName(""), "");
}

TEST_F(PathUtilsTest, GetDirectory) {
    EXPECT_EQ(PathUtils::getDirectory("/path/to/file.txt"), "/path/to");
    EXPECT_EQ(PathUtils::getDirectory("file.txt"), "");
    EXPECT_EQ(PathUtils::getDirectory("/path/to/"), "/path/to");
}

TEST_F(PathUtilsTest, GetExtension) {
    EXPECT_EQ(PathUtils::getExtension("file.txt"), ".txt");
    EXPECT_EQ(PathUtils::getExtension("file.tar.gz"), ".gz");
    EXPECT_EQ(PathUtils::getExtension("file"), "");
    EXPECT_EQ(PathUtils::getExtension(".hidden"), "");
    EXPECT_EQ(PathUtils::getExtension("file."), ".");
}

TEST_F(PathUtilsTest, JoinPaths) {
    std::vector<std::string> components = {"path", "to", "file.txt"};
    std::string joined = PathUtils::joinPaths(components);
    
    char sep = PathUtils::getPathSeparator();
    std::string expected = "path" + std::string(1, sep) + "to" + std::string(1, sep) + "file.txt";
    EXPECT_EQ(joined, expected);
}

TEST_F(PathUtilsTest, JoinPathsWithEmpty) {
    std::vector<std::string> components = {"path", "", "file.txt"};
    std::string joined = PathUtils::joinPaths(components);
    
    EXPECT_EQ(joined.find("//"), std::string::npos); // No double separators
    EXPECT_NE(joined.find("file.txt"), std::string::npos);
}

TEST_F(PathUtilsTest, JoinPathsEmpty) {
    std::vector<std::string> components;
    std::string joined = PathUtils::joinPaths(components);
    
    EXPECT_TRUE(joined.empty());
}

// ========== Platform-Specific Operations ==========

TEST_F(PathUtilsTest, GetDefaultDownloadDirectory) {
    std::string download_dir = PathUtils::getDefaultDownloadDirectory();
    
    EXPECT_FALSE(download_dir.empty());
    // Should contain "Download" or similar
    EXPECT_TRUE(download_dir.find("Download") != std::string::npos ||
               download_dir.find("download") != std::string::npos ||
               download_dir.find("Downloads") != std::string::npos);
}

TEST_F(PathUtilsTest, GetHomeDirectory) {
    std::string home = PathUtils::getHomeDirectory();
    
    EXPECT_FALSE(home.empty());
    EXPECT_TRUE(std::filesystem::exists(home));
    EXPECT_TRUE(std::filesystem::is_directory(home));
}

TEST_F(PathUtilsTest, GetTempDirectory) {
    std::string temp = PathUtils::getTempDirectory();
    
    EXPECT_FALSE(temp.empty());
    EXPECT_TRUE(std::filesystem::exists(temp));
    EXPECT_TRUE(std::filesystem::is_directory(temp));
}

TEST_F(PathUtilsTest, CreateDirectoriesIfNeeded) {
    std::filesystem::path new_dir = temp_dir->getPath() / "new" / "nested" / "directory";
    
    bool result = PathUtils::createDirectoriesIfNeeded(new_dir.string());
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(std::filesystem::exists(new_dir));
    EXPECT_TRUE(std::filesystem::is_directory(new_dir));
}

TEST_F(PathUtilsTest, CreateDirectoriesIfNeededAlreadyExists) {
    bool result = PathUtils::createDirectoriesIfNeeded(temp_dir->getPath().string());
    
    EXPECT_TRUE(result); // Should succeed even if already exists
}

// ========== File System Queries ==========

TEST_F(PathUtilsTest, ExistsFile) {
    EXPECT_TRUE(PathUtils::exists(test_file.string()));
    EXPECT_FALSE(PathUtils::exists("/nonexistent/file.txt"));
}

TEST_F(PathUtilsTest, IsFile) {
    EXPECT_TRUE(PathUtils::isFile(test_file.string()));
    EXPECT_FALSE(PathUtils::isFile(temp_dir->getPath().string()));
    EXPECT_FALSE(PathUtils::isFile("/nonexistent/file.txt"));
}

TEST_F(PathUtilsTest, IsDirectory) {
    EXPECT_TRUE(PathUtils::isDirectory(temp_dir->getPath().string()));
    EXPECT_FALSE(PathUtils::isDirectory(test_file.string()));
    EXPECT_FALSE(PathUtils::isDirectory("/nonexistent/directory"));
}

TEST_F(PathUtilsTest, IsReadable) {
    EXPECT_TRUE(PathUtils::isReadable(test_file.string()));
    EXPECT_TRUE(PathUtils::isReadable(temp_dir->getPath().string()));
}

TEST_F(PathUtilsTest, IsWritable) {
    EXPECT_TRUE(PathUtils::isWritable(test_file.string()));
    EXPECT_TRUE(PathUtils::isWritable(temp_dir->getPath().string()));
}

TEST_F(PathUtilsTest, GetFileSize) {
    size_t size = PathUtils::getFileSize(test_file.string());
    
    EXPECT_GT(size, 0);
    EXPECT_EQ(size, std::filesystem::file_size(test_file));
}

TEST_F(PathUtilsTest, GetFileSizeNonExistent) {
    size_t size = PathUtils::getFileSize("/nonexistent/file.txt");
    
    EXPECT_EQ(size, 0);
}

TEST_F(PathUtilsTest, GetModificationTime) {
    auto mod_time = PathUtils::getModificationTime(test_file.string());
    
    EXPECT_NE(mod_time, std::chrono::system_clock::time_point{});
}

// ========== Security and Validation ==========

TEST_F(PathUtilsTest, IsSecurePathValid) {
    EXPECT_TRUE(PathUtils::isSecurePath("/safe/path/file.txt"));
    EXPECT_TRUE(PathUtils::isSecurePath("relative/safe/path.txt"));
}

TEST_F(PathUtilsTest, IsSecurePathDangerous) {
    EXPECT_FALSE(PathUtils::isSecurePath("../../../etc/passwd"));
    
    // Construct string with null byte properly
    std::string null_path = "/path/with/null";
    null_path += '\0';
    null_path += "byte";
    EXPECT_FALSE(PathUtils::isSecurePath(null_path));
    
    EXPECT_FALSE(PathUtils::isSecurePath("path/with\\..\\traversal"));
}

TEST_F(PathUtilsTest, SanitizeFileName) {
    EXPECT_EQ(PathUtils::sanitizeFileName("safe_file.txt"), "safe_file.txt");
    
    std::string dangerous = "file<>:\"|?*.txt";
    std::string sanitized = PathUtils::sanitizeFileName(dangerous);
    
    EXPECT_EQ(sanitized.find("<"), std::string::npos);
    EXPECT_EQ(sanitized.find(">"), std::string::npos);
    EXPECT_EQ(sanitized.find(":"), std::string::npos);
    EXPECT_NE(sanitized.find(".txt"), std::string::npos); // Extension preserved
}

TEST_F(PathUtilsTest, IsValidPathLength) {
    std::string normal_path = "/normal/path/file.txt";
    std::string very_long_path(5000, 'a'); // Very long path
    
    EXPECT_TRUE(PathUtils::isValidPathLength(normal_path));
    EXPECT_FALSE(PathUtils::isValidPathLength(very_long_path));
}

TEST_F(PathUtilsTest, IsAllowedFileType) {
    std::vector<std::string> allowed = {"txt", "pdf", "doc"};
    
    EXPECT_TRUE(PathUtils::isAllowedFileType("file.txt", allowed));
    EXPECT_TRUE(PathUtils::isAllowedFileType("file.PDF", allowed)); // Case insensitive
    EXPECT_FALSE(PathUtils::isAllowedFileType("file.exe", allowed));
}

TEST_F(PathUtilsTest, IsAllowedFileTypeWildcard) {
    std::vector<std::string> allowed = {"*"};
    
    EXPECT_TRUE(PathUtils::isAllowedFileType("any.file", allowed));
    EXPECT_TRUE(PathUtils::isAllowedFileType("file.exe", allowed));
}

// ========== Pattern Matching ==========

TEST_F(PathUtilsTest, FindFilesMatchingPattern) {
    std::vector<std::string> matches = PathUtils::findFilesMatchingPattern(
        temp_dir->getPath().string(), "*.txt");
    
    EXPECT_GE(matches.size(), 1);
    bool found_test_file = false;
    for (const auto& match : matches) {
        if (match.find("test.txt") != std::string::npos) {
            found_test_file = true;
            break;
        }
    }
    EXPECT_TRUE(found_test_file);
}

TEST_F(PathUtilsTest, MatchesGlobPattern) {
    EXPECT_TRUE(PathUtils::matchesGlobPattern("file.txt", "*.txt"));
    EXPECT_TRUE(PathUtils::matchesGlobPattern("test.pdf", "test.*"));
    EXPECT_TRUE(PathUtils::matchesGlobPattern("file1.doc", "file?.doc"));
    EXPECT_FALSE(PathUtils::matchesGlobPattern("file.txt", "*.pdf"));
}

TEST_F(PathUtilsTest, MatchesRegexPattern) {
    EXPECT_TRUE(PathUtils::matchesRegexPattern("file123.txt", "/file\\d+\\.txt/"));
    EXPECT_FALSE(PathUtils::matchesRegexPattern("fileabc.txt", "/file\\d+\\.txt/"));
}

TEST_F(PathUtilsTest, IsGlobPattern) {
    EXPECT_TRUE(PathUtils::isGlobPattern("*.txt"));
    EXPECT_TRUE(PathUtils::isGlobPattern("file?.doc"));
    EXPECT_TRUE(PathUtils::isGlobPattern("test[123].pdf"));
    EXPECT_FALSE(PathUtils::isGlobPattern("normal_file.txt"));
}

TEST_F(PathUtilsTest, IsRegexPattern) {
    EXPECT_TRUE(PathUtils::isRegexPattern("/.*\\.txt$/"));
    EXPECT_FALSE(PathUtils::isRegexPattern("*.txt"));
    EXPECT_FALSE(PathUtils::isRegexPattern("normal_file.txt"));
}

// ========== File Operations ==========

TEST_F(PathUtilsTest, CopyFile) {
    std::filesystem::path destination = temp_dir->getPath() / "copied.txt";
    
    bool result = PathUtils::copyFile(test_file.string(), destination.string());
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(std::filesystem::exists(destination));
    EXPECT_TRUE(std::filesystem::exists(test_file)); // Original should still exist
}

TEST_F(PathUtilsTest, CopyFileNonExistent) {
    std::filesystem::path destination = temp_dir->getPath() / "copied.txt";
    
    bool result = PathUtils::copyFile("/nonexistent/file.txt", destination.string());
    
    EXPECT_FALSE(result);
}

TEST_F(PathUtilsTest, MoveFile) {
    std::filesystem::path source = temp_dir->createFile("to_move.txt", "content");
    std::filesystem::path destination = temp_dir->getPath() / "moved.txt";
    
    bool result = PathUtils::moveFile(source.string(), destination.string());
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(std::filesystem::exists(destination));
    EXPECT_FALSE(std::filesystem::exists(source));
}

TEST_F(PathUtilsTest, DeleteFile) {
    std::filesystem::path to_delete = temp_dir->createFile("delete_me.txt", "content");
    
    bool result = PathUtils::deleteFile(to_delete.string());
    
    EXPECT_TRUE(result);
    EXPECT_FALSE(std::filesystem::exists(to_delete));
}

TEST_F(PathUtilsTest, DeleteFileNonExistent) {
    bool result = PathUtils::deleteFile("/nonexistent/file.txt");
    
    EXPECT_FALSE(result);
}

TEST_F(PathUtilsTest, CreateEmptyFile) {
    std::filesystem::path new_file = temp_dir->getPath() / "empty.txt";
    
    bool result = PathUtils::createEmptyFile(new_file.string());
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(std::filesystem::exists(new_file));
    EXPECT_EQ(std::filesystem::file_size(new_file), 0);
}

// ========== Utility Functions ==========

TEST_F(PathUtilsTest, FormatFileSize) {
    EXPECT_EQ(PathUtils::formatFileSize(0), "0 B");
    EXPECT_EQ(PathUtils::formatFileSize(1024), "1.0 KB");
    EXPECT_EQ(PathUtils::formatFileSize(1024 * 1024), "1.0 MB");
    EXPECT_EQ(PathUtils::formatFileSize(1024 * 1024 * 1024), "1.0 GB");
}

TEST_F(PathUtilsTest, PathToUri) {
    std::string path = "/path/to/file.txt";
    std::string uri = PathUtils::pathToUri(path);
    
    EXPECT_NE(uri.find("file://"), std::string::npos);
    EXPECT_NE(uri.find("file.txt"), std::string::npos);
}

TEST_F(PathUtilsTest, UriToPath) {
    std::string uri = "file:///path/to/file.txt";
    std::string path = PathUtils::uriToPath(uri);
    
    EXPECT_EQ(path.find("file://"), std::string::npos);
    EXPECT_NE(path.find("file.txt"), std::string::npos);
}

TEST_F(PathUtilsTest, GenerateUniqueFileName) {
    std::filesystem::path unique1 = PathUtils::generateUniqueFileName(test_file.string());
    std::filesystem::path unique2 = PathUtils::generateUniqueFileName(test_file.string());
    
    EXPECT_NE(unique1, test_file.string());
    EXPECT_NE(unique2, test_file.string());
    EXPECT_NE(unique1, unique2);
    EXPECT_NE(unique1.string().find("test"), std::string::npos);
}

TEST_F(PathUtilsTest, EscapeForShell) {
    std::string dangerous = "file with spaces & special;chars.txt";
    std::string escaped = PathUtils::escapeForShell(dangerous);
    
    // Should be properly escaped for shell use
    EXPECT_NE(escaped, dangerous); // Should be different after escaping
}

// ========== Internal Helper Methods ==========

TEST_F(PathUtilsTest, GetPathSeparator) {
    char sep = PathUtils::getPathSeparator();
    
#ifdef _WIN32
    EXPECT_EQ(sep, '\\');
#else
    EXPECT_EQ(sep, '/');
#endif
}

TEST_F(PathUtilsTest, IsValidFileNameChar) {
    EXPECT_TRUE(PathUtils::isValidFileNameChar('a'));
    EXPECT_TRUE(PathUtils::isValidFileNameChar('1'));
    EXPECT_TRUE(PathUtils::isValidFileNameChar('_'));
    EXPECT_TRUE(PathUtils::isValidFileNameChar('.'));
    
    EXPECT_FALSE(PathUtils::isValidFileNameChar('<'));
    EXPECT_FALSE(PathUtils::isValidFileNameChar('>'));
    EXPECT_FALSE(PathUtils::isValidFileNameChar(':'));
}

TEST_F(PathUtilsTest, GetForbiddenChars) {
    std::vector<char> forbidden = PathUtils::getForbiddenChars();
    
    EXPECT_FALSE(forbidden.empty());
    // Should include common forbidden characters
    EXPECT_NE(std::find(forbidden.begin(), forbidden.end(), '<'), forbidden.end());
    EXPECT_NE(std::find(forbidden.begin(), forbidden.end(), '>'), forbidden.end());
}

TEST_F(PathUtilsTest, GlobToRegex) {
    EXPECT_EQ(PathUtils::globToRegex("*.txt"), ".*\\.txt");
    EXPECT_EQ(PathUtils::globToRegex("file?.doc"), "file.\\.doc");
    
    std::string complex_glob = "test[abc]*.pdf";
    std::string regex = PathUtils::globToRegex(complex_glob);
    EXPECT_NE(regex.find("[abc]"), std::string::npos);
}

TEST_F(PathUtilsTest, GetPlatformType) {
    std::string platform = PathUtils::getPlatformType();
    
    EXPECT_FALSE(platform.empty());
    EXPECT_TRUE(platform == "windows" || platform == "macos" || platform == "linux");
}

// ========== Edge Cases ==========

TEST_F(PathUtilsTest, HandleEmptyPaths) {
    EXPECT_EQ(PathUtils::getFileName(""), "");
    EXPECT_EQ(PathUtils::getDirectory(""), "");
    EXPECT_EQ(PathUtils::getExtension(""), "");
    EXPECT_FALSE(PathUtils::exists(""));
}

TEST_F(PathUtilsTest, HandleUnicodePaths) {
    std::string unicode_name = "测试_файл_🔧.txt";
    std::filesystem::path unicode_file = temp_dir->createFile(unicode_name, "unicode content");
    
    EXPECT_TRUE(PathUtils::exists(unicode_file.string()));
    EXPECT_EQ(PathUtils::getFileName(unicode_file.string()), unicode_name);
    EXPECT_EQ(PathUtils::getExtension(unicode_file.string()), ".txt");
}

TEST_F(PathUtilsTest, HandleVeryLongPaths) {
    std::string long_component(100, 'a');
    std::vector<std::string> components = {long_component, long_component, "file.txt"};
    std::string long_path = PathUtils::joinPaths(components);
    
    EXPECT_NE(long_path.find("file.txt"), std::string::npos);
    EXPECT_GT(long_path.length(), 200);
}