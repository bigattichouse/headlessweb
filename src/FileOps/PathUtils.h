#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <filesystem>

namespace FileOps {
    
    class PathUtils {
    public:
        // ========== Path Normalization ==========
        
        /**
         * Normalize a file path for the current platform
         * Converts separators, resolves relative paths, handles encoding
         */
        static std::string normalizePath(const std::string& path);
        
        /**
         * Convert relative path to absolute path
         * Resolves . and .. components, handles current working directory
         */
        static std::string toAbsolutePath(const std::string& path);
        
        /**
         * Get the filename component from a full path
         * Handles both Unix and Windows path separators
         */
        static std::string getFileName(const std::string& path);
        
        /**
         * Get the directory component from a full path
         * Returns directory without trailing separator
         */
        static std::string getDirectory(const std::string& path);
        
        /**
         * Get file extension (including the dot)
         * Returns empty string if no extension found
         */
        static std::string getExtension(const std::string& path);
        
        /**
         * Join multiple path components with proper separators
         * Handles edge cases like empty components and existing separators
         */
        static std::string joinPaths(const std::vector<std::string>& components);
        
        // ========== Platform-Specific Operations ==========
        
        /**
         * Detect the platform's default download directory
         * Checks environment variables and platform conventions
         */
        static std::string getDefaultDownloadDirectory();
        
        /**
         * Get the user's home directory
         * Platform-independent home directory detection
         */
        static std::string getHomeDirectory();
        
        /**
         * Get a platform-appropriate temporary directory
         * Creates hweb-specific temp directory if needed
         */
        static std::string getTempDirectory();
        
        /**
         * Create directory structure if it doesn't exist
         * Creates all parent directories as needed
         */
        static bool createDirectoriesIfNeeded(const std::string& path);
        
        // ========== File System Queries ==========
        
        /**
         * Check if a path exists and is accessible
         * Handles both files and directories
         */
        static bool exists(const std::string& path);
        
        /**
         * Check if path points to a regular file
         * Returns false for directories, symlinks, etc.
         */
        static bool isFile(const std::string& path);
        
        /**
         * Check if path points to a directory
         * Returns false for files, symlinks, etc.
         */
        static bool isDirectory(const std::string& path);
        
        /**
         * Check if file/directory is readable by current user
         * Platform-specific permission checking
         */
        static bool isReadable(const std::string& path);
        
        /**
         * Check if file/directory is writable by current user
         * Platform-specific permission checking
         */
        static bool isWritable(const std::string& path);
        
        /**
         * Get file size in bytes
         * Returns 0 if file doesn't exist or isn't accessible
         */
        static size_t getFileSize(const std::string& path);
        
        /**
         * Get file modification time
         * Returns epoch time if file doesn't exist
         */
        static std::chrono::system_clock::time_point getModificationTime(const std::string& path);
        
        // ========== Security and Validation ==========
        
        /**
         * Validate that a path is safe to use
         * Prevents directory traversal attacks, checks for null bytes, etc.
         */
        static bool isSecurePath(const std::string& path);
        
        /**
         * Sanitize a filename by removing/replacing dangerous characters
         * Handles platform-specific forbidden characters
         */
        static std::string sanitizeFileName(const std::string& filename);
        
        /**
         * Check if path length is within platform limits
         * Different platforms have different path length restrictions
         */
        static bool isValidPathLength(const std::string& path);
        
        /**
         * Validate file extension against allowed list
         * Case-insensitive comparison, handles wildcards
         */
        static bool isAllowedFileType(const std::string& path, const std::vector<std::string>& allowed_types);
        
        // ========== Pattern Matching ==========
        
        /**
         * Find files matching a glob pattern in a directory
         * Supports *, ?, and basic glob patterns
         */
        static std::vector<std::string> findFilesMatchingPattern(
            const std::string& directory, 
            const std::string& pattern
        );
        
        /**
         * Check if filename matches a glob pattern
         * Supports *, ?, and character classes [abc]
         */
        static bool matchesGlobPattern(const std::string& filename, const std::string& pattern);
        
        /**
         * Check if filename matches a regex pattern
         * Pattern should be wrapped in /pattern/ for regex
         */
        static bool matchesRegexPattern(const std::string& filename, const std::string& pattern);
        
        /**
         * Check if string is a glob pattern (contains *, ?, etc.)
         * Used to determine matching strategy
         */
        static bool isGlobPattern(const std::string& pattern);
        
        /**
         * Check if string is a regex pattern (wrapped in //)
         * Used to determine matching strategy
         */
        static bool isRegexPattern(const std::string& pattern);
        
        // ========== File Operations ==========
        
        /**
         * Copy file from source to destination
         * Creates destination directory if needed
         */
        static bool copyFile(const std::string& source, const std::string& destination);
        
        /**
         * Move/rename file from source to destination
         * Handles cross-filesystem moves
         */
        static bool moveFile(const std::string& source, const std::string& destination);
        
        /**
         * Delete file safely
         * Verifies file exists and is not a directory
         */
        static bool deleteFile(const std::string& path);
        
        /**
         * Create empty file at specified path
         * Creates parent directories if needed
         */
        static bool createEmptyFile(const std::string& path);
        
        // ========== Utility Functions ==========
        
        /**
         * Convert file size to human-readable string
         * Returns formats like "1.5 MB", "256 KB", etc.
         */
        static std::string formatFileSize(size_t bytes);
        
        /**
         * Convert file path to URI format
         * Handles proper encoding and file:// prefix
         */
        static std::string pathToUri(const std::string& path);
        
        /**
         * Convert URI to file path
         * Handles decoding and removes file:// prefix
         */
        static std::string uriToPath(const std::string& uri);
        
        /**
         * Generate unique filename by adding suffix if file exists
         * Useful for preventing overwrites
         */
        static std::string generateUniqueFileName(const std::string& path);
        
        /**
         * Escape path for safe use in shell commands
         * Platform-specific escaping for command line usage
         */
        static std::string escapeForShell(const std::string& path);
        
    public:
        // ========== Internal Helper Methods ==========
        
        /**
         * Get platform-specific path separator
         * \ on Windows, / on Unix-like systems
         */
        static char getPathSeparator();
        
        /**
         * Check if character is valid in filename
         * Platform-specific forbidden character checking
         */
        static bool isValidFileNameChar(char c);
        
        /**
         * Get list of forbidden filename characters for current platform
         * Windows has more restrictions than Unix-like systems
         */
        static std::vector<char> getForbiddenChars();
        
        /**
         * Convert glob pattern to regex pattern
         * Internal helper for pattern matching
         */
        static std::string globToRegex(const std::string& glob);
        
        /**
         * Detect platform type at runtime
         * Returns "windows", "macos", or "linux"
         */
        static std::string getPlatformType();
    };
    
} // namespace FileOps
