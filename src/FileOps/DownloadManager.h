#pragma once

#include "Types.h"
#include "PathUtils.h"
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <map>

namespace FileOps {
    
    class DownloadManager {
    public:
        // ========== Main Download Interface ==========
        
        /**
         * Wait for a file download to complete
         * Monitors download directory for file appearance and completion
         */
        DownloadResult waitForDownload(const DownloadCommand& cmd);
        
        /**
         * Wait for multiple downloads to complete
         * Monitors for all specified patterns and returns when all are found
         */
        DownloadResult waitForMultipleDownloads(
            const std::vector<std::string>& patterns,
            const std::string& download_dir = "",
            int timeout_ms = 60000
        );
        
        /**
         * Start asynchronous download monitoring
         * Returns immediately and calls callback when download completes
         */
        void startAsyncDownloadMonitoring(
            const DownloadCommand& cmd,
            std::function<void(DownloadResult, const std::string&)> callback
        );
        
        /**
         * Cancel ongoing download monitoring
         * Stops all monitoring threads and cleans up resources
         */
        void cancelDownloadMonitoring();
        
        // ========== Directory and Path Management ==========
        
        /**
         * Get the default download directory for the current platform
         * Auto-detects based on environment and platform conventions
         */
        std::string getDownloadDirectory();
        
        /**
         * Set custom download directory for monitoring
         * Validates directory exists and is writable
         */
        bool setDownloadDirectory(const std::string& directory);
        
        /**
         * Create download directory if it doesn't exist
         * Creates full directory path with proper permissions
         */
        bool ensureDownloadDirectoryExists(const std::string& directory);
        
        /**
         * Get list of all potential download directories
         * Returns platform-specific common download locations
         */
        std::vector<std::string> getPotentialDownloadDirectories();
        
        // ========== File Detection and Monitoring ==========
        
        /**
         * Find existing files matching pattern in directory
         * Returns list of full file paths that match the pattern
         */
        std::vector<std::string> findMatchingFiles(
            const std::string& directory,
            const std::string& pattern
        );
        
        /**
         * Check if a file exists and matches the pattern
         * Supports glob patterns, regex, and exact matches
         */
        bool fileMatchesPattern(const std::string& filepath, const std::string& pattern);
        
        /**
         * Monitor directory for new files with real-time notifications
         * Uses platform-specific filesystem watching when available
         */
        bool monitorDirectoryForNewFiles(
            const std::string& directory,
            const std::string& pattern,
            int timeout_ms,
            std::function<void(const std::string&)> file_found_callback
        );
        
        /**
         * Check if download is still in progress
         * Detects browser temporary files and partial downloads
         */
        bool isDownloadInProgress(const std::string& filepath);
        
        // ========== Download Completion Detection ==========
        
        /**
         * Wait for file download to be completely finished
         * Monitors file size stability and temporary file cleanup
         */
        bool waitForDownloadCompletion(
            const std::string& filepath,
            int timeout_ms = 30000,
            std::function<void(const DownloadProgress&)> progress_callback = nullptr
        );
        
        /**
         * Verify that download completed successfully
         * Checks file integrity, size, and accessibility
         */
        bool verifyDownloadIntegrity(const std::string& filepath, size_t expected_size = 0);
        
        /**
         * Check if file size has stabilized (download complete)
         * Monitors size changes over time to detect completion
         */
        bool isFileSizeStable(
            const std::string& filepath,
            std::chrono::milliseconds stability_duration = std::chrono::milliseconds(2000)
        );
        
        /**
         * Get current download progress for a file
         * Returns percentage complete or -1 if cannot determine
         */
        int getDownloadProgress(const std::string& filepath, size_t expected_size = 0);
        
        // ========== Pattern Matching and Filtering ==========
        
        /**
         * Convert glob pattern to regex for matching
         * Handles *, ?, and character classes in glob patterns
         */
        std::string globToRegex(const std::string& glob_pattern);
        
        /**
         * Check if pattern is a glob pattern
         * Detects *, ?, and [] characters
         */
        bool isGlobPattern(const std::string& pattern);
        
        /**
         * Check if pattern is a regex pattern
         * Detects /pattern/ format
         */
        bool isRegexPattern(const std::string& pattern);
        
        /**
         * Get most recently created file matching pattern
         * Useful when multiple files match but you want the newest
         */
        std::string getMostRecentMatchingFile(
            const std::string& directory,
            const std::string& pattern
        );
        
        // ========== File System Watching ==========
        
        /**
         * Start native filesystem watcher for directory
         * Uses inotify/kqueue/ReadDirectoryChangesW when available
         */
        bool startNativeFileWatcher(
            const std::string& directory,
            std::function<void(const std::string&, const std::string&)> change_callback
        );
        
        /**
         * Stop native filesystem watcher
         * Cleans up platform-specific watching resources
         */
        void stopNativeFileWatcher();
        
        /**
         * Check if native file watching is available on this platform
         * Returns true for Linux (inotify), macOS (kqueue), Windows (ReadDirectoryChanges)
         */
        bool isNativeFileWatchingAvailable();
        
        /**
         * Fallback to polling-based file monitoring
         * Used when native watching is unavailable or fails
         */
        bool startPollingFileMonitor(
            const std::string& directory,
            const std::string& pattern,
            int poll_interval_ms,
            int timeout_ms,
            std::function<void(const std::string&)> file_found_callback
        );
        
        // ========== Download Management ==========
        
        /**
         * Clean up completed downloads
         * Removes temporary files and browser artifacts
         */
        void cleanupDownloadArtifacts(const std::string& directory);
        
        /**
         * Move completed download to different location
         * Useful for organizing downloads after completion
         */
        bool moveDownloadToDestination(
            const std::string& source_path,
            const std::string& destination_path
        );
        
        /**
         * Get detailed information about a downloaded file
         * Returns comprehensive metadata including size, timestamps, etc.
         */
        FileInfo getDownloadInfo(const std::string& filepath);
        
        /**
         * Validate that downloaded file is not corrupted
         * Performs basic integrity checks
         */
        bool validateDownloadedFile(const std::string& filepath);
        
        // ========== Browser Integration ==========
        
        /**
         * Detect common browser download patterns
         * Recognizes Chrome, Firefox, Safari download behaviors
         */
        std::vector<std::string> getBrowserDownloadPatterns(const std::string& filename);
        
        /**
         * Handle browser-specific temporary file extensions
         * Maps .crdownload, .part, etc. to final filenames
         */
        std::string resolveBrowserTempFile(const std::string& temp_filepath);
        
        /**
         * Check if file is a browser temporary download file
         * Detects various browser temporary file patterns
         */
        bool isBrowserTempFile(const std::string& filepath);
        
        /**
         * Wait for browser to finish writing file
         * Handles browser-specific download completion signals
         */
        bool waitForBrowserWriteCompletion(const std::string& filepath, int timeout_ms);
        
        // ========== Configuration and Options ==========
        
        /**
         * Set default timeout for download operations
         * Applied when individual commands don't specify timeout
         */
        void setDefaultTimeout(int timeout_ms);
        
        /**
         * Set file size stability check duration
         * Time to wait for size changes before considering download complete
         */
        void setStabilityCheckDuration(std::chrono::milliseconds duration);
        
        /**
         * Enable or disable integrity verification
         * Can be disabled for performance in bulk operations
         */
        void setIntegrityVerificationEnabled(bool enabled);
        
        /**
         * Set custom file change detection interval
         * Frequency of polling when native watching unavailable
         */
        void setPollingInterval(int interval_ms);
        
        // ========== Utility and Helper Methods ==========
        
        /**
         * Convert download result to human-readable string
         * For error reporting and logging
         */
        std::string downloadResultToString(DownloadResult result);
        
        /**
         * Get download statistics for monitoring
         * Returns info about active downloads, completion rates, etc.
         */
        struct DownloadStats {
            int active_downloads;
            int completed_downloads;
            int failed_downloads;
            std::chrono::milliseconds average_completion_time;
        };
        
        DownloadStats getDownloadStatistics();
        
        /**
         * Generate detailed error message for download failures
         * Provides user-friendly descriptions of what went wrong
         */
        std::string getErrorMessage(DownloadResult result, const std::string& pattern);
        
        /**
         * Check disk space availability in download directory
         * Warns if insufficient space for expected downloads
         */
        bool hasInsufficientDiskSpace(const std::string& directory, size_t required_bytes);
        
        // ========== Advanced Features ==========
        
        /**
         * Set up download completion hooks
         * Allows custom actions when downloads complete
         */
        void setDownloadCompletionHook(std::function<void(const std::string&)> hook);
        
        /**
         * Enable download progress notifications
         * Provides real-time progress updates via callback
         */
        void setProgressNotificationCallback(
            std::function<void(const std::string&, int)> progress_callback
        );
        
        /**
         * Create download manifest for tracking multiple files
         * Useful for batch download operations
         */
        void createDownloadManifest(
            const std::vector<std::string>& expected_files,
            const std::string& manifest_path
        );
        
        /**
         * Check download manifest completion status
         * Returns true when all files in manifest are downloaded
         */
        bool isDownloadManifestComplete(const std::string& manifest_path);
        
    private:
        // ========== Internal State ==========
        
        std::string default_download_dir_;
        int default_timeout_ms_ = 30000;
        std::chrono::milliseconds stability_check_duration_ = std::chrono::milliseconds(2000);
        bool integrity_verification_enabled_ = true;
        int polling_interval_ms_ = 500;
        
        std::atomic<bool> monitoring_active_{false};
        std::vector<std::thread> monitoring_threads_;
        
        // Callbacks
        std::function<void(const std::string&)> completion_hook_;
        std::function<void(const std::string&, int)> progress_callback_;
        
        // Statistics
        std::atomic<int> active_downloads_{0};
        std::atomic<int> completed_downloads_{0};
        std::atomic<int> failed_downloads_{0};
        
        // Platform-specific file watching handles
        #ifdef _WIN32
            void* directory_handle_ = nullptr;
            void* completion_port_ = nullptr;
        #elif defined(__linux__)
            int inotify_fd_ = -1;
            int watch_descriptor_ = -1;
        #elif defined(__APPLE__)
            int kqueue_fd_ = -1;
        #endif
        
        // ========== Internal Helper Methods ==========
        
        /**
         * Initialize platform-specific file watching
         * Sets up native OS file system monitoring
         */
        bool initializePlatformWatcher(const std::string& directory);
        
        /**
         * Cleanup platform-specific resources
         * Properly releases OS handles and resources
         */
        void cleanupPlatformWatcher();
        
        /**
         * Process file system events from native watcher
         * Handles platform-specific event structures
         */
        void processFileSystemEvents(
            std::function<void(const std::string&, const std::string&)> callback
        );
        
        /**
         * Convert file path to platform-appropriate format
         * Handles path separators and encoding differences
         */
        std::string normalizePath(const std::string& path);
        
        /**
         * Check if file appears to be completely downloaded
         * Uses multiple heuristics to determine completion
         */
        bool appearsCompletelyDownloaded(const std::string& filepath);
        
        /**
         * Get file creation time for sorting by newest
         * Platform-specific file timestamp retrieval
         */
        std::chrono::system_clock::time_point getFileCreationTime(const std::string& filepath);
        
        /**
         * Monitor single file for completion
         * Internal method for watching specific file download
         */
        bool monitorSingleFileCompletion(
            const std::string& filepath,
            int timeout_ms,
            std::function<void(const DownloadProgress&)> progress_callback
        );
        
        /**
         * Generate regex pattern for file matching
         * Converts various pattern types to standardized regex
         */
        std::string generateMatchingRegex(const std::string& pattern);
        
        /**
         * Update download statistics
         * Thread-safe statistics tracking
         */
        void updateDownloadStats(bool completed, bool failed);
        
        /**
         * Check if directory is being actively written to
         * Detects ongoing download activity in directory
         */
        bool isDirectoryActivelyChanging(const std::string& directory);
        
        // ========== C++17 Compatible String Helpers ==========
        
        /**
         * Check if string ends with suffix (C++17 compatible)
         */
        static bool endsWith(const std::string& str, const std::string& suffix) {
            if (suffix.length() > str.length()) return false;
            return str.substr(str.length() - suffix.length()) == suffix;
        }
        
        /**
         * Check if string starts with prefix (C++17 compatible)
         */
        static bool startsWith(const std::string& str, const std::string& prefix) {
            if (prefix.length() > str.length()) return false;
            return str.substr(0, prefix.length()) == prefix;
        }
    };
    
} // namespace FileOps
