#pragma once

#include "Types.h"
#include "PathUtils.h"
#include "../Browser/Browser.h"
#include <string>
#include <functional>
#include <chrono>

namespace FileOps {
    
    class UploadManager {
    public:
        // ========== Main Upload Interface ==========
        
        /**
         * Upload a file to a web form input element
         * Handles all aspects of file validation, DOM manipulation, and progress monitoring
         */
        UploadResult uploadFile(Browser& browser, const UploadCommand& cmd);
        
        /**
         * Upload multiple files to the same input element
         * Useful for multi-file upload scenarios
         */
        UploadResult uploadMultipleFiles(
            Browser& browser, 
            const std::string& selector,
            const std::vector<std::string>& filepaths,
            int timeout_ms = 30000
        );
        
        // ========== Validation Methods ==========
        
        /**
         * Comprehensive file validation before upload attempt
         * Checks existence, permissions, size, and type restrictions
         */
        bool validateFile(const std::string& filepath, const UploadCommand& cmd);
        
        /**
         * Validate that the target element is suitable for file upload
         * Checks if selector points to file input and is accessible
         */
        bool validateUploadTarget(Browser& browser, const std::string& selector);
        
        /**
         * Check if file size is within acceptable limits
         * Uses command-specified limits or reasonable defaults
         */
        bool validateFileSize(const std::string& filepath, size_t max_size);
        
        /**
         * Validate file type against allowed extensions/MIME types
         * Supports wildcards and pattern matching
         */
        bool validateFileType(const std::string& filepath, const std::vector<std::string>& allowed_types);
        
        // ========== Upload Progress Monitoring ==========
        
        /**
         * Wait for upload completion with progress monitoring
         * Detects upload progress indicators and form state changes
         */
        bool waitForUploadCompletion(
            Browser& browser, 
            const std::string& selector, 
            int timeout_ms,
            std::function<void(int)> progress_callback = nullptr
        );
        
        /**
         * Monitor upload progress via JavaScript progress events
         * Tracks XMLHttpRequest upload progress if available
         */
        bool monitorUploadProgress(
            Browser& browser,
            int timeout_ms,
            std::function<void(int)> progress_callback = nullptr
        );
        
        /**
         * Verify that upload was successful by checking DOM state
         * Looks for success indicators, error messages, or form changes
         */
        bool verifyUploadSuccess(Browser& browser, const std::string& selector);
        
        // ========== File Preparation Methods ==========
        
        /**
         * Prepare file for upload (validate, read metadata, etc.)
         * Returns FileInfo structure with all relevant details
         */
        FileInfo prepareFile(const std::string& filepath);
        
        /**
         * Generate appropriate MIME type for file
         * Uses file extension and content sniffing when possible
         */
        std::string detectMimeType(const std::string& filepath);
        
        /**
         * Create a safe, portable filename for upload
         * Sanitizes filename while preserving extension
         */
        std::string sanitizeFileName(const std::string& filepath);
        
        // ========== WebKit Integration Methods ==========
        
        /**
         * Simulate file selection via WebKit DOM manipulation
         * Creates File objects and assigns to input.files property
         */
        bool simulateFileSelection(
            Browser& browser, 
            const std::string& selector, 
            const std::string& filepath
        );
        
        /**
         * Trigger appropriate events after file selection
         * Fires change, input, and other relevant events
         */
        bool triggerFileInputEvents(Browser& browser, const std::string& selector);
        
        /**
         * Handle drag-and-drop style file inputs
         * Simulates drop events for drag-drop upload areas
         */
        bool simulateFileDrop(
            Browser& browser,
            const std::string& selector,
            const std::string& filepath
        );
        
        // ========== Error Handling and Recovery ==========
        
        /**
         * Attempt upload with retry logic
         * Handles transient failures with exponential backoff
         */
        UploadResult uploadWithRetry(
            Browser& browser,
            const UploadCommand& cmd,
            int max_retries = 3
        );
        
        /**
         * Clear any partial upload state
         * Resets form inputs and clears temporary data
         */
        void clearUploadState(Browser& browser, const std::string& selector);
        
        /**
         * Generate detailed error message for upload failures
         * Provides user-friendly descriptions of what went wrong
         */
        std::string getErrorMessage(UploadResult result, const std::string& filepath);
        
        // ========== Configuration and Options ==========
        
        /**
         * Set global upload timeout for all operations
         * Default timeout applied when command doesn't specify one
         */
        void setDefaultTimeout(int timeout_ms);
        
        /**
         * Set maximum allowed file size for uploads
         * Global limit applied in addition to per-command limits
         */
        void setMaxFileSize(size_t max_bytes);
        
        /**
         * Enable or disable upload progress monitoring
         * Can be disabled for performance in bulk operations
         */
        void setProgressMonitoringEnabled(bool enabled);
        
        /**
         * Set custom MIME type detection function
         * Allows override of default MIME type detection
         */
        void setMimeTypeDetector(std::function<std::string(const std::string&)> detector);
        
        // ========== Utility Methods ==========
        
        /**
         * Get list of common file input selectors to try
         * Useful when specific selector isn't known
         */
        std::vector<std::string> getCommonFileInputSelectors();
        
        /**
         * Detect if page has any file upload inputs
         * Scans DOM for file input elements
         */
        bool hasFileInputs(Browser& browser);
        
        /**
         * Get information about all file inputs on page
         * Returns selectors and metadata for each input found
         */
        std::vector<std::string> findFileInputs(Browser& browser);
        
        /**
         * Convert upload result to human-readable string
         * For error reporting and logging
         */
        std::string uploadResultToString(UploadResult result);
        
    private:
        // ========== Internal State ==========
        
        int default_timeout_ms_ = 30000;
        size_t max_file_size_ = 104857600; // 100MB
        bool progress_monitoring_enabled_ = true;
        std::function<std::string(const std::string&)> mime_type_detector_;
        
        // ========== Internal Helper Methods ==========
        
        /**
         * Generate JavaScript for file upload simulation
         * Creates script to manipulate file input element
         */
        std::string generateFileUploadScript(
            const std::string& selector,
            const std::string& filepath,
            const std::string& filename,
            const std::string& mime_type
        );
        
        /**
         * Generate JavaScript for upload progress monitoring
         * Creates script to track XMLHttpRequest progress
         */
        std::string generateProgressMonitorScript(int timeout_ms);
        
        /**
         * Generate JavaScript for upload verification
         * Creates script to check upload success indicators
         */
        std::string generateUploadVerificationScript(const std::string& selector);
        
        /**
         * Parse file input element attributes
         * Extracts accept attribute, multiple flag, etc.
         */
        struct FileInputInfo {
            bool accepts_multiple;
            std::vector<std::string> accepted_types;
            bool is_required;
            std::string form_id;
        };
        
        FileInputInfo parseFileInputInfo(Browser& browser, const std::string& selector);
        
        /**
         * Wait for specific DOM state changes after upload
         * Monitors for progress bars, success messages, etc.
         */
        bool waitForDOMStateChange(
            Browser& browser,
            const std::string& condition,
            int timeout_ms
        );
        
        /**
         * Extract file content as base64 for upload simulation
         * Reads file and encodes for JavaScript File object creation
         */
        std::string encodeFileAsBase64(const std::string& filepath);
        
        /**
         * Handle different types of file input implementations
         * Adapts approach based on input type and page framework
         */
        enum class InputType {
            STANDARD_FILE_INPUT,
            DRAG_DROP_AREA,
            CUSTOM_UPLOAD_WIDGET,
            IFRAME_UPLOAD
        };
        
        InputType detectInputType(Browser& browser, const std::string& selector);
        
        /**
         * Apply upload approach based on detected input type
         * Uses different strategies for different input implementations
         */
        UploadResult uploadByInputType(
            Browser& browser,
            const UploadCommand& cmd,
            InputType type
        );
        
        /**
         * Escape string for safe JavaScript injection
         * Handles quotes, newlines, and other special characters
         */
        std::string escapeForJavaScript(const std::string& input);
        
        /**
         * Generate unique identifier for upload tracking
         * Creates unique ID for monitoring specific uploads
         */
        std::string generateUploadId();
        
        /**
         * Clean up resources after upload completion
         * Removes event listeners and temporary DOM elements
         */
        void cleanupUploadResources(Browser& browser, const std::string& upload_id);
    };
    
} // namespace FileOps
