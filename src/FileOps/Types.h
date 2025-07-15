#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace FileOps {
    
    // ========== Result Enumerations ==========
    
    enum class UploadResult {
        SUCCESS = 0,
        FILE_NOT_FOUND = 1,
        INVALID_SELECTOR = 2,
        UPLOAD_FAILED = 3,
        TIMEOUT = 4,
        PERMISSION_DENIED = 5,
        FILE_TOO_LARGE = 6,
        INVALID_FILE_TYPE = 7,
        ELEMENT_NOT_FOUND = 8,
        JAVASCRIPT_ERROR = 9
    };
    
    enum class DownloadResult {
        SUCCESS = 0,
        TIMEOUT = 1,
        FILE_NOT_FOUND = 2,
        INTEGRITY_CHECK_FAILED = 3,
        PERMISSION_DENIED = 4,
        DIRECTORY_NOT_FOUND = 5,
        PATTERN_MATCH_FAILED = 6
    };
    
    enum class WaitCondition {
        TEXT_APPEARS,
        NETWORK_IDLE,
        JAVASCRIPT_TRUE,
        ELEMENT_COUNT,
        ELEMENT_VISIBLE,
        ATTRIBUTE_CHANGED,
        URL_CHANGED,
        TITLE_CHANGED
    };
    
    enum class ComparisonOperator {
        EQUALS,           // ==
        NOT_EQUALS,       // !=
        GREATER_THAN,     // >
        LESS_THAN,        // <
        GREATER_EQUAL,    // >=
        LESS_EQUAL        // <=
    };
    
    // ========== Command Structures ==========
    
    struct UploadCommand {
        std::string selector;
        std::string filepath;
        int timeout_ms = 30000;
        bool wait_completion = true;
        size_t max_file_size = 104857600; // 100MB default
        std::vector<std::string> allowed_types = {"*"}; // Default: all types
        bool verify_upload = true;
        std::string custom_message;
        bool json_output = false;
        bool silent = false;
        
        // Validation helpers
        bool isValidFileType(const std::string& filename) const;
        std::string getFileExtension(const std::string& filename) const;
    };
    
    struct DownloadCommand {
        std::string filename_pattern;
        std::string download_dir; // Auto-detect if empty
        int timeout_ms = 30000;
        bool verify_integrity = true;
        size_t expected_size = 0; // 0 = no size check
        bool delete_on_completion = false;
        std::string custom_message;
        bool json_output = false;
        bool silent = false;
        
        // Pattern matching helpers
        bool matchesPattern(const std::string& filename) const;
        bool isGlobPattern() const;
        bool isRegexPattern() const;
    };
    
    struct WaitCommand {
        WaitCondition condition_type;
        std::string target_value; // Text, selector, or JS expression
        int timeout_ms = 10000;
        int poll_interval_ms = 100;
        int retry_count = 3;
        ComparisonOperator comparison_op = ComparisonOperator::EQUALS;
        int expected_count = 1; // For element count waiting
        bool case_sensitive = false; // For text waiting
        std::string custom_message;
        bool json_output = false;
        bool silent = false;
        
        // Validation helpers
        bool isValidJavaScript() const;
        bool isValidSelector() const;
    };
    
    // ========== File Information Structures ==========
    
    struct FileInfo {
        std::string filepath;
        std::string filename;
        size_t size_bytes;
        std::string mime_type;
        std::chrono::system_clock::time_point last_modified;
        bool is_readable;
        bool exists;
        
        static FileInfo create(const std::string& filepath);
        std::string getSizeString() const;
        bool isOlderThan(std::chrono::minutes age) const;
    };
    
    struct DownloadProgress {
        std::string filepath;
        size_t current_size;
        size_t expected_size;
        bool is_complete;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point last_update;
        
        double getProgressPercent() const;
        std::chrono::milliseconds getElapsedTime() const;
        bool isStable(std::chrono::milliseconds stability_time = std::chrono::milliseconds(2000)) const;
    };
    
    // ========== Network Monitoring Structures ==========
    
    struct NetworkRequest {
        std::string url;
        std::string method;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        bool is_complete;
        int status_code;
        
        std::chrono::milliseconds getDuration() const;
        bool isActive() const;
    };
    
    struct NetworkState {
        std::vector<NetworkRequest> active_requests;
        std::chrono::system_clock::time_point last_activity;
        int total_requests = 0;
        int completed_requests = 0;
        
        bool isIdle(std::chrono::milliseconds idle_time) const;
        void addRequest(const NetworkRequest& request);
        void completeRequest(const std::string& url);
        void clear();
    };
    
    // ========== Utility Functions ==========
    
    // String conversion utilities
    std::string uploadResultToString(UploadResult result);
    std::string downloadResultToString(DownloadResult result);
    std::string waitConditionToString(WaitCondition condition);
    std::string comparisonOperatorToString(ComparisonOperator op);
    
    // Validation utilities
    bool isValidFilePath(const std::string& path);
    bool isValidSelector(const std::string& selector);
    bool isValidJavaScript(const std::string& js);
    bool isValidPattern(const std::string& pattern);
    
    // Platform utilities
    std::string getDefaultDownloadDirectory();
    std::string normalizePath(const std::string& path);
    bool createDirectoryIfNotExists(const std::string& path);
    
    // Time utilities
    std::string formatDuration(std::chrono::milliseconds duration);
    std::string formatFileSize(size_t bytes);
    
} // namespace FileOps
