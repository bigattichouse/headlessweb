# HeadlessWeb Phase 2: File & Workflow Operations Implementation

## 🎯 Current Status & Context

**✅ COMPLETED: Priority 1 - Testing & CI/CD Foundation**
- Full assertion system implemented in `src/Assertion/` module
- Commands: `--assert-exists`, `--assert-text`, `--assert-count`, `--assert-js`
- Test suite management: `--test-suite start/end` with JSON/JUnit output
- JSON output mode (`--json`) and silent mode (`--silent`) 
- Proper exit codes for CI/CD integration
- Comprehensive test coverage with `test-assertions.sh`

**🎯 NEXT: Priority 2 - File & Workflow Operations**
*Goal: "Covers 80% of remaining use cases"*

---

## 📋 Implementation Requirements

### **2.1 File Upload System**
Create robust file upload functionality that works with WebKit headless mode:

**Commands to implement:**
```bash
./hweb --session upload --upload "#file-input" "/path/to/document.pdf"
./hweb --session upload --upload "[name='attachment']" "./image.jpg" --timeout 30000
./hweb --session upload --upload ".file-drop" "./data.csv" --wait-upload
```

**Key Requirements:**
- File validation (existence, size limits, type checking)
- WebKit-compatible file input simulation
- Upload progress monitoring
- Error handling for invalid files/selectors
- Support for multiple file inputs
- Cross-platform path handling

### **2.2 Download Monitoring**
Implement download completion detection and verification:

**Commands to implement:**
```bash
./hweb --session download --click "#download-button" --download-wait "report.xlsx"
./hweb --session download --download-wait "invoice.pdf" --timeout 60000
./hweb --session download --download-wait "*.zip" --pattern-match
```

**Key Requirements:**
- Monitor downloads directory for file appearance
- Detect download completion (file size stabilization)
- Support filename patterns and wildcards
- Configurable download directories
- File integrity verification
- Timeout handling for stuck downloads

### **2.3 Advanced Waiting Systems**
Enhance waiting capabilities for modern web applications:

**Commands to implement:**
```bash
./hweb --session spa --wait-text "Data loaded successfully" --timeout 15000
./hweb --session spa --wait-network-idle --idle-time 500 --timeout 30000
./hweb --session spa --wait-function "window.myApp && window.myApp.ready"
./hweb --session spa --wait-elements ".item" --min-count 5
```

**Key Requirements:**
- Text appearance monitoring across entire page
- Network activity tracking (XHR, Fetch API)
- Custom JavaScript condition polling
- Element count-based waiting
- Configurable polling intervals
- SPA navigation support

---

## 🏗️ Module Structure

Create new modules following the established pattern:

### **src/FileOps/** Directory Structure:
```
src/FileOps/
├── Types.h           # Core types and structs
├── UploadManager.h   # File upload functionality  
├── UploadManager.cpp
├── DownloadManager.h # Download monitoring
├── DownloadManager.cpp
├── PathUtils.h       # Cross-platform path utilities
├── PathUtils.cpp
└── CMakeLists.txt    # Build configuration
```

### **Enhanced Browser Module:**
Extend existing `src/Browser/` with:
- `AdvancedWaiting.h/cpp` - New waiting capabilities
- Integration hooks in `Browser.h` for file operations

---

## 🔧 Implementation Blueprint

### **Core Types (src/FileOps/Types.h):**
```cpp
namespace FileOps {
    enum class UploadResult {
        SUCCESS = 0,
        FILE_NOT_FOUND = 1,
        INVALID_SELECTOR = 2,
        UPLOAD_FAILED = 3,
        TIMEOUT = 4
    };
    
    struct UploadCommand {
        std::string selector;
        std::string filepath;
        int timeout_ms = 30000;
        bool wait_completion = true;
        size_t max_file_size = 100 * 1024 * 1024; // 100MB
    };
    
    struct DownloadCommand {
        std::string filename_pattern;
        std::string download_dir;
        int timeout_ms = 30000;
        bool verify_integrity = true;
    };
    
    enum class WaitCondition {
        TEXT_APPEARS,
        NETWORK_IDLE,
        JAVASCRIPT_TRUE,
        ELEMENT_COUNT
    };
}
```

### **Upload Manager Interface:**
```cpp
class UploadManager {
public:
    UploadResult uploadFile(Browser& browser, const UploadCommand& cmd);
    bool validateFile(const std::string& filepath);
    bool waitForUploadCompletion(Browser& browser, const std::string& selector, int timeout_ms);
    
private:
    bool createFileInputSimulation(Browser& browser, const std::string& selector, const std::string& filepath);
    bool monitorUploadProgress(Browser& browser, int timeout_ms);
};
```

### **Download Manager Interface:**
```cpp
class DownloadManager {
public:
    bool waitForDownload(const DownloadCommand& cmd);
    std::string getDownloadDirectory();
    bool verifyDownloadIntegrity(const std::string& filepath);
    
private:
    bool monitorFileSystem(const std::string& pattern, const std::string& directory, int timeout_ms);
    bool isDownloadComplete(const std::string& filepath);
    std::vector<std::string> findMatchingFiles(const std::string& pattern, const std::string& directory);
};
```

### **Advanced Waiting Interface:**
```cpp
class AdvancedWaiting {
public:
    bool waitForText(Browser& browser, const std::string& text, int timeout_ms = 10000);
    bool waitForNetworkIdle(Browser& browser, int idle_time_ms = 500, int timeout_ms = 30000);
    bool waitForFunction(Browser& browser, const std::string& js_function, int timeout_ms = 10000);
    bool waitForElementCount(Browser& browser, const std::string& selector, int min_count, int timeout_ms = 10000);
    
private:
    std::string setupNetworkMonitor(int idle_time_ms, int timeout_ms);
    std::string setupTextMonitor(const std::string& text, int timeout_ms);
    bool pollCondition(Browser& browser, const std::string& condition, int timeout_ms, int poll_interval_ms = 100);
};
```

---

## 🔗 Integration Points

### **main.cpp Command Parsing:**
Add new command parsing for:
```cpp
// File operations
else if (args[i] == "--upload" && i + 2 < args.size()) {
    commands.push_back({"upload", args[i+1], args[i+2]});
    i += 2;
} else if (args[i] == "--download-wait" && i + 1 < args.size()) {
    commands.push_back({"download-wait", args[++i], ""});
} else if (args[i] == "--wait-text" && i + 1 < args.size()) {
    commands.push_back({"wait-text", "", args[++i]});
} else if (args[i] == "--wait-network-idle") {
    commands.push_back({"wait-network-idle", "", ""});
} else if (args[i] == "--wait-function" && i + 1 < args.size()) {
    commands.push_back({"wait-function", "", args[++i]});
}
```

### **Command Execution Logic:**
```cpp
} else if (cmd.type == "upload") {
    FileOps::UploadCommand upload_cmd;
    upload_cmd.selector = cmd.selector;
    upload_cmd.filepath = cmd.value;
    
    FileOps::UploadManager upload_mgr;
    auto result = upload_mgr.uploadFile(browser, upload_cmd);
    
    if (result == FileOps::UploadResult::SUCCESS) {
        info_output("File uploaded: " + cmd.value);
    } else {
        error_output("Upload failed: " + cmd.value);
        exit_code = static_cast<int>(result);
    }
} else if (cmd.type == "download-wait") {
    FileOps::DownloadCommand download_cmd;
    download_cmd.filename_pattern = cmd.selector;
    
    FileOps::DownloadManager download_mgr;
    if (download_mgr.waitForDownload(download_cmd)) {
        info_output("Download completed: " + cmd.selector);
    } else {
        error_output("Download timeout: " + cmd.selector);
        exit_code = 1;
    }
}
```

### **CMakeLists.txt Updates:**
```cmake
# Add subdirectory
add_subdirectory(src/FileOps)

# Include in executable
add_executable(hweb
    # ... existing sources ...
    ${ASSERTION_MODULE_SOURCES}
    ${FILEOPS_MODULE_SOURCES}  # Add this line
)
```

---

## 🧪 Testing Requirements

### **test-fileops.sh Script:**
Create comprehensive test script covering:

1. **File Upload Tests:**
   - Valid file uploads with various formats (PDF, image, text)
   - Invalid file handling (non-existent, too large, wrong selector)
   - Multiple file upload scenarios
   - Upload progress monitoring

2. **Download Tests:**
   - Basic download waiting
   - Pattern matching for filenames
   - Download timeout handling
   - File integrity verification

3. **Advanced Waiting Tests:**
   - Text appearance on dynamic pages
   - Network idle detection with AJAX
   - JavaScript condition polling
   - Element count-based waiting

4. **Integration Tests:**
   - Combined upload/download workflows
   - File operations with assertions
   - Real-world document processing scenarios

### **Test File Structure:**
```bash
test-files/
├── sample.pdf
├── image.jpg
├── data.csv
└── large-file.zip  # For size limit testing
```

---

## 🎯 Success Criteria

### **Functional Requirements:**
- [ ] File uploads work with common input types
- [ ] Downloads are reliably detected and verified
- [ ] Advanced waiting handles modern SPA patterns
- [ ] Proper error handling and user feedback
- [ ] Cross-platform compatibility (Linux, macOS, Windows)

### **Performance Requirements:**
- [ ] File operations complete within reasonable timeouts
- [ ] Waiting mechanisms don't consume excessive CPU
- [ ] Large file handling (up to 100MB) works smoothly
- [ ] Network monitoring doesn't interfere with page functionality

### **Integration Requirements:**
- [ ] Commands integrate seamlessly with existing functionality
- [ ] File operations work with session management
- [ ] Error codes and output formats match existing patterns
- [ ] Documentation covers all new features with examples

---

## 🚀 Implementation Order

### **Phase 2A: Core File Operations (Week 1)**
1. Create `src/FileOps/` module structure
2. Implement basic file upload functionality
3. Add download monitoring capabilities
4. Integrate with main.cpp command parsing

### **Phase 2B: Advanced Waiting (Week 2)**
1. Implement text-based waiting
2. Add network idle detection
3. Create JavaScript condition polling
4. Add element count waiting

### **Phase 2C: Integration & Testing (Week 3)**
1. Create comprehensive test suite
2. Add real-world workflow examples
3. Document all new features
4. Performance optimization and bug fixes

---

## 📚 Reference Implementation

Key implementation notes:

1. **WebKit File Upload:** Use DOM manipulation to set file input values, as WebKit headless mode has limitations with native file dialogs
2. **Download Detection:** Monitor filesystem changes in downloads directory using polling with file size stabilization
3. **Network Monitoring:** Inject JavaScript to intercept XMLHttpRequest and Fetch API calls
4. **Cross-Platform:** Use `std::filesystem` for path operations and platform-specific download directory detection

This module will transform HeadlessWeb from a basic automation tool into a comprehensive file processing and workflow automation platform suitable for enterprise document workflows, data extraction pipelines, and complex web application testing.

---

## 💡 Next Session Prompt

**"I need to implement Priority 2: File & Workflow Operations for HeadlessWeb. Based on the Phase 2 blueprint and current codebase, please implement the file upload/download system and advanced waiting capabilities. Start with creating the src/FileOps/ module structure, then implement the core upload and download functionality, followed by advanced waiting systems for modern web applications. Ensure full integration with the existing command structure and maintain the same quality standards as the assertion system."**
