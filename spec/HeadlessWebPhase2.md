# HeadlessWeb Next Phase Development Blueprint

## Current Status Assessment

### ✅ **Solid Foundation Completed**
- Core navigation and session management (**95% complete**)
- DOM interaction and form automation (**90% complete**)
- Command chaining and state persistence (**95% complete**)
- Screenshot functionality (**100% complete**)
- Custom attribute management (**100% complete**)

### 🎯 **Production Readiness Gap Analysis**

We have a **powerful automation engine** but lack the **professional tooling layer** needed for:
- **CI/CD Integration** (testing assertions, exit codes)
- **Enterprise Workflows** (file operations, bulk actions)
- **Modern Web Apps** (advanced waiting, network control)
- **Team Collaboration** (recording/replay, standardized output)

---

## Phase 2 Development Plan

### **Priority 1: Testing & CI/CD Foundation** ⚡
*Essential for professional adoption*

### **Priority 2: File & Workflow Operations** 📁
*Covers 80% of remaining use cases*

### **Priority 3: Advanced Web App Support** 🌐
*Modern SPA and dynamic content handling*

### **Priority 4: Productivity & Collaboration** 🤝
*Team workflows and bulk operations*

---

## Priority 1: Testing & CI/CD Foundation

### 1.1 Assertion Commands System

**Goal:** Enable HeadlessWeb to be used in automated testing pipelines with proper exit codes and standardized output.

#### **Implementation Blueprint**

```cpp
// Add to main.cpp command parsing
enum class AssertionResult {
    PASS = 0,
    FAIL = 1,
    ERROR = 2
};

struct AssertionCommand {
    string type;              // "exists", "text", "count", "js"
    string selector;          // CSS selector or JS expression
    string expected_value;    // Expected value for comparison
    string custom_message;    // Optional user message
    bool json_output;         // Format as JSON
    bool silent;              // Don't print PASS/FAIL, just exit code
};

class AssertionManager {
    vector<AssertionResult> results;
    string test_name;
    bool reporting_mode;
    
public:
    AssertionResult assertExists(Browser& browser, const AssertionCommand& cmd) {
        // PSEUDOCODE:
        // 1. Check if element exists using browser.elementExists()
        // 2. Compare result with expected (true/false)
        // 3. Output formatted result based on cmd.json_output
        // 4. Return appropriate exit code
        
        bool exists = browser.elementExists(cmd.selector);
        bool expected = (cmd.expected_value == "true");
        
        if (exists == expected) {
            if (!cmd.silent) {
                if (cmd.json_output) {
                    cout << R"({"assertion": "exists", "selector": ")" << cmd.selector 
                         << R"(", "result": "PASS", "expected": )" << boolalpha << expected 
                         << R"(, "actual": )" << exists << "}" << endl;
                } else {
                    cout << "PASS: Element " << cmd.selector 
                         << (expected ? " exists" : " does not exist") << endl;
                }
            }
            return AssertionResult::PASS;
        } else {
            if (!cmd.silent) {
                if (cmd.json_output) {
                    cout << R"({"assertion": "exists", "selector": ")" << cmd.selector 
                         << R"(", "result": "FAIL", "expected": )" << expected 
                         << R"(, "actual": )" << exists << "}" << endl;
                } else {
                    cout << "FAIL: Element " << cmd.selector 
                         << " - Expected: " << (expected ? "exists" : "not exists")
                         << ", Actual: " << (exists ? "exists" : "not exists") << endl;
                }
            }
            return AssertionResult::FAIL;
        }
    }
    
    AssertionResult assertText(Browser& browser, const AssertionCommand& cmd) {
        // PSEUDOCODE:
        // 1. Get text content using browser.getInnerText()
        // 2. Compare with expected value (exact match or contains)
        // 3. Handle case sensitivity options
        // 4. Format output and return result
        
        string actual_text = browser.getInnerText(cmd.selector);
        bool matches = (actual_text == cmd.expected_value);
        
        // Add fuzzy matching options:
        // - contains: actual_text.find(expected) != string::npos
        // - case_insensitive: to_lower(actual) == to_lower(expected)
        // - regex: regex_match(actual, regex(expected))
        
        return matches ? AssertionResult::PASS : AssertionResult::FAIL;
    }
    
    AssertionResult assertCount(Browser& browser, const AssertionCommand& cmd) {
        // PSEUDOCODE:
        // 1. Count elements using browser.countElements()
        // 2. Compare with expected number
        // 3. Support comparison operators (>, <, >=, <=, ==)
        
        int actual_count = browser.countElements(cmd.selector);
        int expected_count = stoi(cmd.expected_value);
        
        return (actual_count == expected_count) ? AssertionResult::PASS : AssertionResult::FAIL;
    }
    
    AssertionResult assertJavaScript(Browser& browser, const AssertionCommand& cmd) {
        // PSEUDOCODE:
        // 1. Execute JavaScript expression
        // 2. Evaluate result as boolean
        // 3. Handle various return types (boolean, string, number)
        
        string js_result = browser.executeJavascriptSync(cmd.selector); // selector contains JS
        bool result = (js_result == "true" || js_result == "1" || js_result == "yes");
        
        return result ? AssertionResult::PASS : AssertionResult::FAIL;
    }
};
```

#### **Command Line Interface**

```bash
# Basic assertions
./hweb --session test --assert-exists ".error-message"
./hweb --session test --assert-text "h1" "Welcome to Dashboard"
./hweb --session test --assert-count ".product-item" "5"
./hweb --session test --assert-js "window.dataLoaded === true"

# With custom messages
./hweb --session test --assert-exists ".success" --message "Login should succeed"

# JSON output for tool integration
./hweb --session test --json --assert-text ".status" "Complete"
# Output: {"assertion": "text", "selector": ".status", "result": "PASS", "expected": "Complete", "actual": "Complete"}

# Silent mode (exit code only)
./hweb --session test --silent --assert-exists ".dashboard"
echo $? # 0 for pass, 1 for fail

# Comparison operators for count
./hweb --session test --assert-count ".item" ">5"
./hweb --session test --assert-count ".error" "==0"
```

### 1.2 Test Reporting & Suite Management

```cpp
class TestSuite {
    string suite_name;
    vector<AssertionResult> test_results;
    chrono::system_clock::time_point start_time;
    
public:
    void startSuite(const string& name) {
        // PSEUDOCODE:
        // 1. Initialize test tracking
        // 2. Record start time
        // 3. Clear previous results
        // 4. Output suite header
        
        suite_name = name;
        start_time = chrono::system_clock::now();
        test_results.clear();
        
        cout << "Starting test suite: " << name << endl;
    }
    
    void endSuite(bool json_output = false) {
        // PSEUDOCODE:
        // 1. Calculate summary statistics
        // 2. Output results in requested format
        // 3. Set appropriate exit code
        
        auto end_time = chrono::system_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
        
        int total = test_results.size();
        int passed = count(test_results.begin(), test_results.end(), AssertionResult::PASS);
        int failed = total - passed;
        
        if (json_output) {
            cout << R"({)"
                 << R"("suite": ")" << suite_name << R"(",)"
                 << R"("total": )" << total << R"(,)"
                 << R"("passed": )" << passed << R"(,)"
                 << R"("failed": )" << failed << R"(,)"
                 << R"("duration_ms": )" << duration.count()
                 << R"(})" << endl;
        } else {
            cout << "\n=== Test Suite Results ===" << endl;
            cout << "Suite: " << suite_name << endl;
            cout << "Total: " << total << ", Passed: " << passed 
                 << ", Failed: " << failed << endl;
            cout << "Duration: " << duration.count() << "ms" << endl;
        }
        
        exit(failed > 0 ? 1 : 0);
    }
};
```

#### **Command Usage**

```bash
# Test suite management
./hweb --test-suite start "Login Flow Tests"
./hweb --session login --assert-exists "#login-form"
./hweb --session login --assert-text ".error" ""  # No errors
./hweb --test-suite end

# JUnit XML output for CI
./hweb --test-suite start "API Tests" --format junit
# ... run tests ...
./hweb --test-suite end > test-results.xml
```

### 1.3 JSON Output Mode

```cpp
class OutputFormatter {
public:
    static void formatResult(const string& command, const any& result, bool json_mode) {
        // PSEUDOCODE:
        // 1. Detect result type (string, bool, int, object)
        // 2. Format according to mode (text vs JSON)
        // 3. Handle errors and edge cases
        
        if (json_mode) {
            Json::Value output;
            output["command"] = command;
            output["success"] = true;
            
            if (holds_alternative<string>(result)) {
                output["result"] = get<string>(result);
            } else if (holds_alternative<bool>(result)) {
                output["result"] = get<bool>(result);
            } else if (holds_alternative<int>(result)) {
                output["result"] = get<int>(result);
            }
            
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            cout << Json::writeString(builder, output) << endl;
        } else {
            // Regular text output
            if (holds_alternative<string>(result)) {
                cout << get<string>(result) << endl;
            }
            // ... handle other types
        }
    }
    
    static void formatError(const string& command, const string& error, bool json_mode) {
        if (json_mode) {
            Json::Value output;
            output["command"] = command;
            output["success"] = false;
            output["error"] = error;
            
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            cout << Json::writeString(builder, output) << endl;
        } else {
            cerr << "Error: " << error << endl;
        }
    }
};
```

---

## Priority 2: File & Workflow Operations

### 2.1 File Upload System

```cpp
class FileOperations {
public:
    bool uploadFile(Browser& browser, const string& selector, const string& filepath) {
        // PSEUDOCODE:
        // 1. Validate file exists and is readable
        // 2. Find file input element
        // 3. Use WebKit file selection API
        // 4. Wait for upload completion
        // 5. Verify upload success
        
        // Validate file
        if (!filesystem::exists(filepath)) {
            throw runtime_error("File not found: " + filepath);
        }
        
        // Check file size limits
        auto file_size = filesystem::file_size(filepath);
        if (file_size > 100 * 1024 * 1024) { // 100MB limit
            throw runtime_error("File too large: " + to_string(file_size) + " bytes");
        }
        
        // Find and validate file input
        string validate_js = R"(
            (function() {
                var element = document.querySelector(')" + selector + R"(');
                if (!element) return 'not_found';
                if (element.type !== 'file') return 'not_file_input';
                return 'valid';
            })()
        )";
        
        string validation = browser.executeJavascriptSync(validate_js);
        if (validation != "valid") {
            return false;
        }
        
        // Set file using WebKit API
        // Note: This requires platform-specific implementation
        // WebKit doesn't provide direct file upload API in headless mode
        // Would need to use GtkFileChooser simulation or direct DOM manipulation
        
        string upload_js = R"(
            (function() {
                var input = document.querySelector(')" + selector + R"(');
                var event = new Event('change', { bubbles: true });
                
                // Create a File object (this is simplified - real implementation 
                // would need to read file content and create proper File object)
                var file = new File(['file content'], ')" + 
                filesystem::path(filepath).filename().string() + R"(', {
                    type: 'application/octet-stream'
                });
                
                // This is pseudocode - actual implementation needs proper file handling
                Object.defineProperty(input, 'files', {
                    value: [file],
                    writable: false
                });
                
                input.dispatchEvent(event);
                return 'uploaded';
            })()
        )";
        
        string result = browser.executeJavascriptSync(upload_js);
        return result == "uploaded";
    }
    
    bool waitForDownload(const string& expected_filename, int timeout_ms = 30000) {
        // PSEUDOCODE:
        // 1. Monitor downloads directory
        // 2. Wait for file to appear and be complete
        // 3. Verify file integrity
        
        string downloads_dir = getDownloadsDirectory();
        string full_path = downloads_dir + "/" + expected_filename;
        
        auto start_time = chrono::steady_clock::now();
        
        while (chrono::steady_clock::now() - start_time < chrono::milliseconds(timeout_ms)) {
            if (filesystem::exists(full_path)) {
                // Check if file is still being written (size changing)
                auto size1 = filesystem::file_size(full_path);
                this_thread::sleep_for(chrono::milliseconds(500));
                auto size2 = filesystem::file_size(full_path);
                
                if (size1 == size2 && size1 > 0) {
                    return true; // Download complete
                }
            }
            this_thread::sleep_for(chrono::milliseconds(100));
        }
        
        return false; // Timeout
    }
    
private:
    string getDownloadsDirectory() {
        // PSEUDOCODE:
        // 1. Check environment variables
        // 2. Use platform defaults
        // 3. Create directory if needed
        
        string home = getenv("HOME") ? getenv("HOME") : "";
        string downloads = home + "/Downloads";
        
        if (!filesystem::exists(downloads)) {
            downloads = "/tmp/hweb-downloads";
            filesystem::create_directories(downloads);
        }
        
        return downloads;
    }
};
```

#### **Command Usage**

```bash
# File uploads
./hweb --session upload --upload "#file-input" "/path/to/document.pdf"
./hweb --session upload --upload "[name='attachment']" "./image.jpg"

# Download waiting
./hweb --session download --click "#download-button" --download-wait "report.xlsx"
./hweb --session download --download-wait "invoice.pdf" --timeout 60000

# Combined workflow
./hweb --session workflow \
  --url "https://app.com/upload" \
  --upload "#document" "./contract.pdf" \
  --click "#process-button" \
  --wait ".processing-complete" \
  --click "#download-result" \
  --download-wait "processed-contract.pdf"
```

### 2.2 Advanced Waiting Systems

```cpp
class AdvancedWaiting {
public:
    bool waitForText(Browser& browser, const string& text, int timeout_ms = 10000) {
        // PSEUDOCODE:
        // 1. Poll page content for text appearance
        // 2. Use case-insensitive matching by default
        // 3. Support partial and exact matching modes
        
        auto start_time = chrono::steady_clock::now();
        
        string wait_js = R"(
            (function(searchText, timeout) {
                return new Promise(function(resolve) {
                    var startTime = Date.now();
                    
                    function checkText() {
                        var bodyText = document.body.innerText || document.body.textContent || '';
                        if (bodyText.toLowerCase().includes(searchText.toLowerCase())) {
                            resolve(true);
                            return;
                        }
                        
                        if (Date.now() - startTime > timeout) {
                            resolve(false);
                            return;
                        }
                        
                        setTimeout(checkText, 100);
                    }
                    
                    checkText();
                });
            })(')" + text + "', " + to_string(timeout_ms) + R")";
        
        string result = browser.executeJavascriptSync(wait_js);
        return result == "true";
    }
    
    bool waitForNetworkIdle(Browser& browser, int idle_time_ms = 500, int timeout_ms = 30000) {
        // PSEUDOCODE:
        // 1. Monitor network activity using JavaScript
        // 2. Track XMLHttpRequest and fetch API calls
        // 3. Wait for specified idle period
        
        string network_monitor_js = R"(
            (function(idleTime, timeout) {
                return new Promise(function(resolve) {
                    var requestCount = 0;
                    var lastRequestTime = 0;
                    var startTime = Date.now();
                    
                    // Intercept XMLHttpRequest
                    var originalXHR = XMLHttpRequest.prototype.open;
                    XMLHttpRequest.prototype.open = function() {
                        requestCount++;
                        lastRequestTime = Date.now();
                        
                        this.addEventListener('load', function() {
                            requestCount--;
                            if (requestCount === 0) {
                                lastRequestTime = Date.now();
                            }
                        });
                        
                        this.addEventListener('error', function() {
                            requestCount--;
                            if (requestCount === 0) {
                                lastRequestTime = Date.now();
                            }
                        });
                        
                        return originalXHR.apply(this, arguments);
                    };
                    
                    // Intercept fetch API
                    var originalFetch = window.fetch;
                    window.fetch = function() {
                        requestCount++;
                        lastRequestTime = Date.now();
                        
                        return originalFetch.apply(this, arguments).finally(function() {
                            requestCount--;
                            if (requestCount === 0) {
                                lastRequestTime = Date.now();
                            }
                        });
                    };
                    
                    function checkIdle() {
                        var now = Date.now();
                        
                        if (now - startTime > timeout) {
                            resolve(false);
                            return;
                        }
                        
                        if (requestCount === 0 && (now - lastRequestTime) > idleTime) {
                            resolve(true);
                            return;
                        }
                        
                        setTimeout(checkIdle, 50);
                    }
                    
                    // Start checking after initial delay
                    setTimeout(checkIdle, 100);
                });
            })()" + to_string(idle_time_ms) + ", " + to_string(timeout_ms) + R")";
        
        string result = browser.executeJavascriptSync(network_monitor_js);
        return result == "true";
    }
    
    bool waitForFunction(Browser& browser, const string& js_function, int timeout_ms = 10000) {
        // PSEUDOCODE:
        // 1. Execute JavaScript function repeatedly
        // 2. Wait until it returns true
        // 3. Handle function errors gracefully
        
        auto start_time = chrono::steady_clock::now();
        
        while (chrono::steady_clock::now() - start_time < chrono::milliseconds(timeout_ms)) {
            try {
                string result = browser.executeJavascriptSync("(" + js_function + ")()");
                if (result == "true") {
                    return true;
                }
            } catch (const exception& e) {
                // Continue polling even if function throws
            }
            
            this_thread::sleep_for(chrono::milliseconds(100));
        }
        
        return false;
    }
};
```

#### **Command Usage**

```bash
# Advanced waiting
./hweb --session spa --wait-text "Data loaded successfully"
./hweb --session spa --wait-network-idle --timeout 15000
./hweb --session spa --wait-function "window.myApp && window.myApp.ready"

# Combined with actions
./hweb --session api \
  --url "https://app.com/dashboard" \
  --click "#refresh-data" \
  --wait-network-idle \
  --assert-text ".status" "Updated"
```

---

## Priority 3: Advanced Web App Support

### 3.1 Multi-Element Operations

```cpp
class BulkOperations {
public:
    bool clickAll(Browser& browser, const string& selector, int delay_ms = 100) {
        // PSEUDOCODE:
        // 1. Find all matching elements
        // 2. Click each with specified delay
        // 3. Handle elements that become invalid during iteration
        // 4. Return success count
        
        string count_js = "document.querySelectorAll('" + selector + "').length";
        int element_count = stoi(browser.executeJavascriptSync(count_js));
        
        if (element_count == 0) {
            return false;
        }
        
        int successful_clicks = 0;
        
        for (int i = 0; i < element_count; i++) {
            string click_js = R"(
                (function(selector, index) {
                    var elements = document.querySelectorAll(')" + selector + R"(');
                    if (elements[)" + to_string(i) + R"(]) {
                        elements[)" + to_string(i) + R"(].click();
                        return 'clicked';
                    }
                    return 'not_found';
                })()
            )";
            
            string result = browser.executeJavascriptSync(click_js);
            if (result == "clicked") {
                successful_clicks++;
            }
            
            if (delay_ms > 0 && i < element_count - 1) {
                browser.wait(delay_ms);
            }
        }
        
        return successful_clicks == element_count;
    }
    
    bool typeAll(Browser& browser, const string& selector, const string& text) {
        // PSEUDOCODE:
        // 1. Find all input elements matching selector
        // 2. Fill each with the same text
        // 3. Trigger appropriate events for each
        
        string type_all_js = R"(
            (function(selector, text) {
                var elements = document.querySelectorAll(')" + selector + R"(');
                var count = 0;
                
                for (var i = 0; i < elements.length; i++) {
                    var el = elements[i];
                    if (el.tagName === 'INPUT' || el.tagName === 'TEXTAREA') {
                        el.focus();
                        el.value = ')" + text + R"(';
                        el.dispatchEvent(new Event('input', { bubbles: true }));
                        el.dispatchEvent(new Event('change', { bubbles: true }));
                        count++;
                    }
                }
                
                return count;
            })()
        )";
        
        string result = browser.executeJavascriptSync(type_all_js);
        return stoi(result) > 0;
    }
    
    vector<string> extractAll(Browser& browser, const string& selector, const string& attribute = "") {
        // PSEUDOCODE:
        // 1. Find all matching elements
        // 2. Extract specified attribute or text content
        // 3. Return as vector of strings
        
        string extract_js;
        if (attribute.empty()) {
            extract_js = R"(
                (function(selector) {
                    var elements = document.querySelectorAll(')" + selector + R"(');
                    var results = [];
                    for (var i = 0; i < elements.length; i++) {
                        results.push(elements[i].innerText || elements[i].textContent || '');
                    }
                    return JSON.stringify(results);
                })()
            )";
        } else {
            extract_js = R"(
                (function(selector, attr) {
                    var elements = document.querySelectorAll(')" + selector + R"(');
                    var results = [];
                    for (var i = 0; i < elements.length; i++) {
                        results.push(elements[i].getAttribute(')" + attribute + R"(') || '');
                    }
                    return JSON.stringify(results);
                })()
            )";
        }
        
        string json_result = browser.executeJavascriptSync(extract_js);
        
        // Parse JSON array into vector<string>
        Json::Value root;
        Json::Reader reader;
        vector<string> results;
        
        if (reader.parse(json_result, root) && root.isArray()) {
            for (const auto& item : root) {
                results.push_back(item.asString());
            }
        }
        
        return results;
    }
};
```

### 3.2 Recording & Replay System

```cpp
struct RecordedAction {
    string type;        // "click", "type", "select", "wait", "navigate"
    string selector;    // CSS selector or URL
    string value;       // Input value or option value
    int timestamp;      // Relative timestamp in ms
    int delay;          // Delay after action
};

class ActionRecorder {
    vector<RecordedAction> recorded_actions;
    bool is_recording;
    chrono::steady_clock::time_point recording_start;
    
public:
    void startRecording() {
        // PSEUDOCODE:
        // 1. Clear previous recording
        // 2. Start timestamp tracking
        // 3. Begin action capture
        
        recorded_actions.clear();
        is_recording = true;
        recording_start = chrono::steady_clock::now();
        
        cout << "Recording started..." << endl;
    }
    
    void recordAction(const string& type, const string& selector, const string& value = "") {
        if (!is_recording) return;
        
        auto now = chrono::steady_clock::now();
        int timestamp = chrono::duration_cast<chrono::milliseconds>(now - recording_start).count();
        
        RecordedAction action;
        action.type = type;
        action.selector = selector;
        action.value = value;
        action.timestamp = timestamp;
        action.delay = 0; // Will be calculated based on next action
        
        // Calculate delay from previous action
        if (!recorded_actions.empty()) {
            recorded_actions.back().delay = timestamp - recorded_actions.back().timestamp;
        }
        
        recorded_actions.push_back(action);
        
        cout << "Recorded: " << type << " " << selector 
             << (value.empty() ? "" : " = " + value) << endl;
    }
    
    void stopRecording(const string& recording_name) {
        // PSEUDOCODE:
        // 1. Stop recording mode
        // 2. Save recording to file
        // 3. Calculate final delays
        
        is_recording = false;
        
        // Save to JSON file
        Json::Value recording;
        recording["name"] = recording_name;
        recording["created"] = chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();
        
        Json::Value actions(Json::arrayValue);
        for (const auto& action : recorded_actions) {
            Json::Value json_action;
            json_action["type"] = action.type;
            json_action["selector"] = action.selector;
            json_action["value"] = action.value;
            json_action["timestamp"] = action.timestamp;
            json_action["delay"] = action.delay;
            actions.append(json_action);
        }
        recording["actions"] = actions;
        
        // Save to file
        string filename = "recordings/" + recording_name + ".json";
        filesystem::create_directories("recordings");
        
        ofstream file(filename);
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        file << Json::writeString(builder, recording);
        
        cout << "Recording saved: " << filename 
             << " (" << recorded_actions.size() << " actions)" << endl;
    }
    
    bool replayRecording(Browser& browser, const string& recording_name, float speed_multiplier = 1.0) {
        // PSEUDOCODE:
        // 1. Load recording from file
        // 2. Execute each action in sequence
        // 3. Respect original timing with speed adjustment
        // 4. Handle errors gracefully
        
        string filename = "recordings/" + recording_name + ".json";
        if (!filesystem::exists(filename)) {
            cerr << "Recording not found: " << filename << endl;
            return false;
        }
        
        // Load recording
        ifstream file(filename);
        Json::Value recording;
        file >> recording;
        
        Json::Value actions = recording["actions"];
        cout << "Replaying: " << recording_name 
             << " (" << actions.size() << " actions)" << endl;
        
        for (const auto& action : actions) {
            string type = action["type"].asString();
            string selector = action["selector"].asString();
            string value = action["value"].asString();
            int delay = action["delay"].asInt();
            
            cout << "Replaying: " << type << " " << selector << endl;
            
            // Execute action
            bool success = false;
            if (type == "click") {
                success = browser.clickElement(selector);
            } else if (type == "type") {
                success = browser.fillInput(selector, value);
            } else if (type == "select") {
                success = browser.selectOption(selector, value);
            } else if (type == "navigate") {
                browser.loadUri(selector);
                success = browser.waitForNavigation(10000);
            } else if (type == "wait") {
                success = browser.waitForSelector(selector, 10000);
            }
            
            if (!success) {
                cerr << "Replay failed at: " << type << " " << selector << endl;
                return false;
            }
            
            // Apply delay with speed adjustment
            if (delay > 0) {
                int adjusted_delay = static_cast<int>(delay / speed_multiplier);
                browser.wait(adjusted_delay);
            }
        }
        
        cout << "Replay completed successfully" << endl;
        return true;
    }
};
```

#### **Command Usage**

```bash
# Multi-element operations
./hweb --session bulk --click-all ".delete-button" --delay 500
./hweb --session bulk --type-all ".quantity" "1"
./hweb --session bulk --check-all "[name='selected']"

# Extract data from multiple elements
./hweb --session extract --extract-all ".product-name"        # Get text
./hweb --session extract --extract-all "img" "src"          # Get image URLs
./hweb --session extract --extract-all "a" "href" --json    # Get links as JSON

# Recording workflows
./hweb --record-start "checkout-flow"
./hweb --session shop --url "https://store.com"
./hweb --session shop --click ".product"
./hweb --session shop --click "#add-to-cart"
./hweb --session shop --click "#checkout"
./hweb --record-stop

# Replay recordings
./hweb --replay "checkout-flow"
./hweb --replay "checkout-flow" --speed 2.0    # 2x speed
./hweb --replay "checkout-flow" --speed 0.5    # Half speed
```

---

## Implementation Session Prompts

### **Session 1: Testing Foundation**
```
I need to implement a comprehensive testing and assertion system for HeadlessWeb. The system should:

1. Add assertion commands (--assert-exists, --assert-text, --assert-count, --assert-js)
2. Implement JSON output mode (--json flag)  
3. Add test suite management (--test-suite start/end)
4. Provide proper exit codes for CI/CD integration
5. Support custom assertion messages and comparison operators

Please implement the AssertionManager class and integrate it into the main command parsing. Include comprehensive error handling and support for both text and JSON output formats. The system should be compatible with popular CI/CD tools like Jenkins, GitHub Actions, and GitLab CI.
```

### **Session 2: File Operations**
```
I need to implement file upload and download functionality for HeadlessWeb. The system should:

1. Add file upload command (--upload selector filepath)
2. Implement download waiting (--download-wait filename)
3. Support various file types and size validation
4. Handle file input validation and error cases
5. Monitor download completion and file integrity

Please implement the FileOperations class with proper WebKit integration for file uploads and filesystem monitoring for downloads. Include cross-platform support for different download directories and proper error handling for file access permissions.
```

### **Session 3: Advanced Waiting**
```
I need to implement advanced waiting capabilities for modern web applications. The system should:

1. Add text-based waiting (--wait-text "specific text")
2. Implement network idle detection (--wait-network-idle)
3. Add JavaScript function waiting (--wait-function "custom.condition()")
4. Support configurable timeouts and polling intervals
5. Handle SPA navigation and dynamic content loading

Please implement the AdvancedWaiting class with robust JavaScript-based monitoring. The system should intercept network requests, monitor DOM changes, and provide reliable waiting for modern web applications with heavy AJAX usage.
```

### **Session 4: Bulk Operations & Recording**
```
I need to implement bulk operations and workflow recording for HeadlessWeb. The system should:

1. Add multi-element operations (--click-all, --type-all, --extract-all)
2. Implement action recording (--record-start/stop)
3. Add replay functionality (--replay with speed control)
4. Support data extraction from multiple elements
5. Provide workflow automation and reusability

Please implement the BulkOperations and ActionRecorder classes. Include JSON-based recording storage, timing preservation, error handling during replay, and support for workflow templates that can be shared between team members.
```

---

## Success Metrics

### **Technical Metrics**
- **Test Coverage**: 90%+ assertion pass rate in CI/CD
- **Performance**: <500ms startup time with new features
- **Reliability**: <1% failure rate on standard web apps
- **Compatibility**: Works with top 10 web frameworks

### **User Experience Metrics**
- **Learning Curve**: New users productive within 30 minutes
- **Workflow Efficiency**: 50% reduction in script development time
- **Error Clarity**: Self-explanatory error messages 95% of time
- **Documentation**: Complete examples for all features

### **Enterprise Readiness**
- **CI/CD Integration**: Native support for Jenkins, GitHub Actions, GitLab
- **Output Formats**: JSON, JUnit XML, CSV, plain text
- **Error Handling**: Proper exit codes and error categorization
- **Scalability**: Handle 100+ concurrent sessions

---

## Development Timeline

### **Week 1-2: Testing Foundation**
- Assertion commands and JSON output
- Test suite management
- CI/CD integration testing

### **Week 3-4: File Operations** 
- Upload/download functionality
- Cross-platform file handling
- Integration with common file workflows

### **Week 5-6: Advanced Features**
- Multi-element operations
- Recording and replay system
- Advanced waiting capabilities

### **Week 7: Integration & Polish**
- Performance optimization
- Documentation completion
- Real-world testing and refinement

**Expected Outcome**: Production-ready web automation tool suitable for enterprise adoption and professional development workflows.
