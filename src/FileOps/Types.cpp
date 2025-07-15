#include "Types.h"
#include "PathUtils.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>

namespace FileOps {
    
    // ========== UploadCommand Methods ==========
    
    bool UploadCommand::isValidFileType(const std::string& filename) const {
        return PathUtils::isAllowedFileType(filename, allowed_types);
    }
    
    std::string UploadCommand::getFileExtension(const std::string& filename) const {
        return PathUtils::getExtension(filename);
    }
    
    // ========== DownloadCommand Methods ==========
    
    bool DownloadCommand::matchesPattern(const std::string& filename) const {
        if (isRegexPattern()) {
            return PathUtils::matchesRegexPattern(filename, filename_pattern);
        } else {
            return PathUtils::matchesGlobPattern(filename, filename_pattern);
        }
    }
    
    bool DownloadCommand::isGlobPattern() const {
        return PathUtils::isGlobPattern(filename_pattern);
    }
    
    bool DownloadCommand::isRegexPattern() const {
        return PathUtils::isRegexPattern(filename_pattern);
    }
    
    // ========== WaitCommand Methods ==========
    
    bool WaitCommand::isValidJavaScript() const {
        if (target_value.empty()) {
            return false;
        }
        
        // Basic JavaScript validation
        // Check for balanced parentheses and brackets
        int paren_count = 0;
        int bracket_count = 0;
        int brace_count = 0;
        
        for (char c : target_value) {
            switch (c) {
                case '(': paren_count++; break;
                case ')': paren_count--; break;
                case '[': bracket_count++; break;
                case ']': bracket_count--; break;
                case '{': brace_count++; break;
                case '}': brace_count--; break;
            }
            
            // Early exit if counts go negative
            if (paren_count < 0 || bracket_count < 0 || brace_count < 0) {
                return false;
            }
        }
        
        // All counts should be zero for balanced expression
        return paren_count == 0 && bracket_count == 0 && brace_count == 0;
    }
    
    bool WaitCommand::isValidSelector() const {
        if (target_value.empty()) {
            return false;
        }
        
        // Basic CSS selector validation
        // Check for common selector patterns
        if (target_value[0] == '#' || target_value[0] == '.' || 
            target_value.find('[') != std::string::npos ||
            std::isalpha(target_value[0])) {
            return true;
        }
        
        return false;
    }
    
    // ========== FileInfo Methods ==========
    
    FileInfo FileInfo::create(const std::string& filepath) {
        FileInfo info;
        info.filepath = PathUtils::normalizePath(filepath);
        info.filename = PathUtils::getFileName(filepath);
        info.exists = PathUtils::exists(filepath);
        info.is_readable = PathUtils::isReadable(filepath);
        info.size_bytes = PathUtils::getFileSize(filepath);
        info.last_modified = PathUtils::getModificationTime(filepath);
        
        // Simple MIME type detection based on extension
        std::string extension = PathUtils::getExtension(filepath);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        static const std::map<std::string, std::string> mime_types = {
            {".txt", "text/plain"},
            {".pdf", "application/pdf"},
            {".jpg", "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".png", "image/png"},
            {".gif", "image/gif"},
            {".zip", "application/zip"},
            {".doc", "application/msword"},
            {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
            {".xls", "application/vnd.ms-excel"},
            {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"}
        };
        
        auto it = mime_types.find(extension);
        info.mime_type = (it != mime_types.end()) ? it->second : "application/octet-stream";
        
        return info;
    }
    
    std::string FileInfo::getSizeString() const {
        return PathUtils::formatFileSize(size_bytes);
    }
    
    bool FileInfo::isOlderThan(std::chrono::minutes age) const {
        auto now = std::chrono::system_clock::now();
        auto file_age = now - last_modified;
        return file_age > age;
    }
    
    // ========== DownloadProgress Methods ==========
    
    double DownloadProgress::getProgressPercent() const {
        if (expected_size == 0) {
            return -1.0; // Unknown progress
        }
        
        return (static_cast<double>(current_size) / expected_size) * 100.0;
    }
    
    std::chrono::milliseconds DownloadProgress::getElapsedTime() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time
        );
    }
    
    bool DownloadProgress::isStable(std::chrono::milliseconds stability_time) const {
        auto time_since_update = std::chrono::system_clock::now() - last_update;
        return time_since_update >= stability_time;
    }
    
    // ========== NetworkRequest Methods ==========
    
    std::chrono::milliseconds NetworkRequest::getDuration() const {
        if (is_complete) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        } else {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - start_time
            );
        }
    }
    
    bool NetworkRequest::isActive() const {
        return !is_complete;
    }
    
    // ========== NetworkState Methods ==========
    
    bool NetworkState::isIdle(std::chrono::milliseconds idle_time) const {
        if (!active_requests.empty()) {
            return false; // Still have active requests
        }
        
        auto time_since_activity = std::chrono::system_clock::now() - last_activity;
        return time_since_activity >= idle_time;
    }
    
    void NetworkState::addRequest(const NetworkRequest& request) {
        active_requests.push_back(request);
        last_activity = std::chrono::system_clock::now();
        total_requests++;
    }
    
    void NetworkState::completeRequest(const std::string& url) {
        auto it = std::find_if(active_requests.begin(), active_requests.end(),
            [&url](const NetworkRequest& req) { return req.url == url; });
        
        if (it != active_requests.end()) {
            active_requests.erase(it);
            completed_requests++;
            last_activity = std::chrono::system_clock::now();
        }
    }
    
    void NetworkState::clear() {
        active_requests.clear();
        last_activity = std::chrono::system_clock::now();
        total_requests = 0;
        completed_requests = 0;
    }
    
    // ========== Utility Functions ==========
    
    std::string uploadResultToString(UploadResult result) {
        switch (result) {
            case UploadResult::SUCCESS: return "SUCCESS";
            case UploadResult::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
            case UploadResult::INVALID_SELECTOR: return "INVALID_SELECTOR";
            case UploadResult::UPLOAD_FAILED: return "UPLOAD_FAILED";
            case UploadResult::TIMEOUT: return "TIMEOUT";
            case UploadResult::PERMISSION_DENIED: return "PERMISSION_DENIED";
            case UploadResult::FILE_TOO_LARGE: return "FILE_TOO_LARGE";
            case UploadResult::INVALID_FILE_TYPE: return "INVALID_FILE_TYPE";
            case UploadResult::ELEMENT_NOT_FOUND: return "ELEMENT_NOT_FOUND";
            case UploadResult::JAVASCRIPT_ERROR: return "JAVASCRIPT_ERROR";
            default: return "UNKNOWN";
        }
    }
    
    std::string downloadResultToString(DownloadResult result) {
        switch (result) {
            case DownloadResult::SUCCESS: return "SUCCESS";
            case DownloadResult::TIMEOUT: return "TIMEOUT";
            case DownloadResult::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
            case DownloadResult::INTEGRITY_CHECK_FAILED: return "INTEGRITY_CHECK_FAILED";
            case DownloadResult::PERMISSION_DENIED: return "PERMISSION_DENIED";
            case DownloadResult::DIRECTORY_NOT_FOUND: return "DIRECTORY_NOT_FOUND";
            case DownloadResult::PATTERN_MATCH_FAILED: return "PATTERN_MATCH_FAILED";
            default: return "UNKNOWN";
        }
    }
    
    std::string waitConditionToString(WaitCondition condition) {
        switch (condition) {
            case WaitCondition::TEXT_APPEARS: return "TEXT_APPEARS";
            case WaitCondition::NETWORK_IDLE: return "NETWORK_IDLE";
            case WaitCondition::JAVASCRIPT_TRUE: return "JAVASCRIPT_TRUE";
            case WaitCondition::ELEMENT_COUNT: return "ELEMENT_COUNT";
            case WaitCondition::ELEMENT_VISIBLE: return "ELEMENT_VISIBLE";
            case WaitCondition::ATTRIBUTE_CHANGED: return "ATTRIBUTE_CHANGED";
            case WaitCondition::URL_CHANGED: return "URL_CHANGED";
            case WaitCondition::TITLE_CHANGED: return "TITLE_CHANGED";
            default: return "UNKNOWN";
        }
    }
    
    std::string comparisonOperatorToString(ComparisonOperator op) {
        switch (op) {
            case ComparisonOperator::EQUALS: return "==";
            case ComparisonOperator::NOT_EQUALS: return "!=";
            case ComparisonOperator::GREATER_THAN: return ">";
            case ComparisonOperator::LESS_THAN: return "<";
            case ComparisonOperator::GREATER_EQUAL: return ">=";
            case ComparisonOperator::LESS_EQUAL: return "<=";
            default: return "==";
        }
    }
    
    // ========== Validation Utilities ==========
    
    bool isValidFilePath(const std::string& path) {
        return PathUtils::isSecurePath(path) && !path.empty();
    }
    
    bool isValidSelector(const std::string& selector) {
        if (selector.empty()) {
            return false;
        }
        
        // Basic CSS selector validation
        try {
            // Check for common selector patterns
            if (selector[0] == '#' || selector[0] == '.' || 
                selector.find('[') != std::string::npos ||
                std::isalpha(selector[0])) {
                return true;
            }
        } catch (...) {
            return false;
        }
        
        return false;
    }
    
    bool isValidJavaScript(const std::string& js) {
        if (js.empty()) {
            return false;
        }
        
        // Basic JavaScript validation
        int paren_count = 0;
        for (char c : js) {
            if (c == '(') paren_count++;
            else if (c == ')') paren_count--;
            if (paren_count < 0) return false;
        }
        
        return paren_count == 0;
    }
    
    bool isValidPattern(const std::string& pattern) {
        if (pattern.empty()) {
            return false;
        }
        
        // Check if it's a valid glob or regex pattern
        if (PathUtils::isRegexPattern(pattern)) {
            try {
                std::string regex_str = pattern.substr(1, pattern.length() - 2);
                std::regex test_regex(regex_str);
                return true;
            } catch (const std::regex_error& e) {
                return false;
            }
        }
        
        // For glob patterns, just check basic validity
        return PathUtils::isGlobPattern(pattern) || true; // Simple strings are valid too
    }
    
    // ========== Platform Utilities ==========
    
    std::string getDefaultDownloadDirectory() {
        return PathUtils::getDefaultDownloadDirectory();
    }
    
    std::string normalizePath(const std::string& path) {
        return PathUtils::normalizePath(path);
    }
    
    bool createDirectoryIfNotExists(const std::string& path) {
        return PathUtils::createDirectoriesIfNeeded(path);
    }
    
    // ========== Time Utilities ==========
    
    std::string formatDuration(std::chrono::milliseconds duration) {
        auto total_ms = duration.count();
        
        if (total_ms < 1000) {
            return std::to_string(total_ms) + "ms";
        }
        
        auto seconds = total_ms / 1000;
        auto ms = total_ms % 1000;
        
        if (seconds < 60) {
            if (ms == 0) {
                return std::to_string(seconds) + "s";
            } else {
                return std::to_string(seconds) + "." + std::to_string(ms / 100) + "s";
            }
        }
        
        auto minutes = seconds / 60;
        seconds = seconds % 60;
        
        if (minutes < 60) {
            return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
        }
        
        auto hours = minutes / 60;
        minutes = minutes % 60;
        
        return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
    }
    
    std::string formatFileSize(size_t bytes) {
        return PathUtils::formatFileSize(bytes);
    }
    
} // namespace FileOps
