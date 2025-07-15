#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <glib.h>
#include <sstream>
#include "Session/Manager.h"
#include "Browser/Browser.h"
#include "Assertion/Manager.h"
#include "FileOps/UploadManager.h"
#include "FileOps/DownloadManager.h"
#include "FileOps/Types.h"
#include "Debug.h"

struct Command {
    std::string type;
    std::string selector;
    std::string value;
    int timeout = 10000;
};

// Global managers
static std::unique_ptr<Assertion::Manager> assertionManager;
static std::unique_ptr<FileOps::UploadManager> uploadManager;
static std::unique_ptr<FileOps::DownloadManager> downloadManager;

// Helper functions for output control
void info_output(const std::string& message) {
    std::cerr << message << std::endl;  // Always show info messages
}

void error_output(const std::string& message) {
    std::cerr << message << std::endl;
}

void print_usage() {
    std::cerr << "Usage: hweb [options] [commands...]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --session <n>        Use named session (default: 'default')" << std::endl;
    std::cerr << "  --url <url>          Navigate to URL" << std::endl;
    std::cerr << "  --end                End session" << std::endl;
    std::cerr << "  --list               List all sessions" << std::endl;
    std::cerr << "  --debug              Enable debug output" << std::endl;
    std::cerr << "  --user-agent <ua>    Set custom user agent" << std::endl;
    std::cerr << "  --width <px>         Set browser width (default: 1000)" << std::endl;
    std::cerr << "  --json               Enable JSON output mode" << std::endl;
    std::cerr << "  --silent             Silent mode (exit codes only)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Test Suite Management:" << std::endl;
    std::cerr << "  --test-suite start <name>  Start test suite" << std::endl;
    std::cerr << "  --test-suite end [format]  End test suite (format: text|json|junit)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Assertions:" << std::endl;
    std::cerr << "  --assert-exists <sel> [true|false]     Assert element exists" << std::endl;
    std::cerr << "  --assert-text <sel> <text>             Assert element text" << std::endl;
    std::cerr << "  --assert-count <sel> <count>           Assert element count" << std::endl;
    std::cerr << "  --assert-js <js> [true|false]          Assert JavaScript result" << std::endl;
    std::cerr << "  --message <msg>                        Custom assertion message" << std::endl;
    std::cerr << "  --timeout <ms>                         Assertion timeout" << std::endl;
    std::cerr << std::endl;
    std::cerr << "File Operations:" << std::endl;
    std::cerr << "  --upload <selector> <filepath>         Upload file to input element" << std::endl;
    std::cerr << "  --upload-multiple <selector> <files>   Upload multiple files (comma-separated)" << std::endl;
    std::cerr << "  --download-wait <pattern>              Wait for download to complete" << std::endl;
    std::cerr << "  --download-wait-multiple <patterns>    Wait for multiple downloads" << std::endl;
    std::cerr << "  --max-file-size <bytes>                Set maximum upload file size" << std::endl;
    std::cerr << "  --allowed-types <extensions>           Set allowed file types (comma-separated)" << std::endl;
    std::cerr << "  --download-dir <path>                  Set custom download directory" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Advanced Waiting:" << std::endl;
    std::cerr << "  --wait-text-advanced <text[:options]>  Wait for text with options" << std::endl;
    std::cerr << "  --wait-network-idle [idle_time_ms]     Wait for network to become idle" << std::endl;
    std::cerr << "  --wait-network-request <url_pattern>   Wait for specific network request" << std::endl;
    std::cerr << "  --wait-element-visible <selector>      Wait for element to be visible" << std::endl;
    std::cerr << "  --wait-element-count <sel> <op> <num>  Wait for element count condition" << std::endl;
    std::cerr << "  --wait-attribute <sel> <attr> <value>  Wait for attribute value" << std::endl;
    std::cerr << "  --wait-url-change <pattern>            Wait for URL to change" << std::endl;
    std::cerr << "  --wait-title-change <pattern>          Wait for title to change" << std::endl;
    std::cerr << "  --wait-spa-navigation [route]          Wait for SPA route change" << std::endl;
    std::cerr << "  --wait-framework-ready [framework]     Wait for JS framework to load" << std::endl;
    std::cerr << "  --wait-dom-change <selector>           Wait for DOM mutations" << std::endl;
    std::cerr << "  --wait-content-change <sel> <prop>     Wait for content property change" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Commands (can be chained):" << std::endl;
    std::cerr << "  --type <selector> <text>     Type text into element" << std::endl;
    std::cerr << "  --click <selector>           Click element" << std::endl;
    std::cerr << "  --submit [form-selector]     Submit form" << std::endl;
    std::cerr << "  --wait <selector>            Wait for element" << std::endl;
    std::cerr << "  --wait-nav                   Wait for navigation" << std::endl;
    std::cerr << "  --wait-ready <sel|js>        Wait for ready condition" << std::endl;
    std::cerr << "  --text <selector>            Get element text" << std::endl;
    std::cerr << "  --search <query>             Smart form search" << std::endl;
    std::cerr << "  --js <code>                  Execute JavaScript" << std::endl;
    std::cerr << "  --store <var> <value>        Store custom variable" << std::endl;
    std::cerr << "  --get <var>                  Get custom variable" << std::endl;
    std::cerr << "  --back                       Go back in history" << std::endl;
    std::cerr << "  --forward                    Go forward in history" << std::endl;
    std::cerr << "  --reload                     Reload page" << std::endl;
    std::cerr << "  --select <sel> <value>       Select dropdown option" << std::endl;
    std::cerr << "  --check <selector>           Check checkbox" << std::endl;
    std::cerr << "  --uncheck <selector>         Uncheck checkbox" << std::endl;
    std::cerr << "  --focus <selector>           Focus element" << std::endl;
    std::cerr << "  --exists <selector>          Check if element exists" << std::endl;
    std::cerr << "  --count <selector>           Count matching elements" << std::endl;
    std::cerr << "  --html <selector>            Get element HTML" << std::endl;
    std::cerr << "  --attr <sel> <attr>          Get attribute value" << std::endl;
    std::cerr << "  --screenshot [file]          Take screenshot (default: screenshot.png)" << std::endl;
    std::cerr << "  --screenshot-full [f]        Take full page screenshot" << std::endl;
    std::cerr << "  --extract <n> <js>        Add custom state extractor" << std::endl;
    std::cerr << "  --record-start               Start recording actions" << std::endl;
    std::cerr << "  --record-stop                Stop recording actions" << std::endl;
    std::cerr << "  --replay <n>              Replay recorded actions" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Assertion Operators:" << std::endl;
    std::cerr << "  For --assert-count: >N, <N, >=N, <=N, !=N, ==N" << std::endl;
    std::cerr << "  For --assert-text: contains:text, ~=regex, !=text" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  # Basic automation" << std::endl;
    std::cerr << "  hweb --url example.com --assert-exists '.login-form'" << std::endl;
    std::cerr << "  hweb --assert-text 'h1' 'Welcome' --json" << std::endl;
    std::cerr << "  hweb --assert-count '.item' '>5' --silent" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  # File operations" << std::endl;
    std::cerr << "  hweb --session upload --upload '#file-input' './document.pdf'" << std::endl;
    std::cerr << "  hweb --session download --click '#download-btn' --download-wait 'report.xlsx'" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  # Advanced waiting for SPAs" << std::endl;
    std::cerr << "  hweb --session spa --wait-network-idle 1000 --wait-text-advanced 'Data loaded'" << std::endl;
    std::cerr << "  hweb --session spa --wait-spa-navigation '/dashboard' --assert-exists '.content'" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  # Test suites" << std::endl;
    std::cerr << "  hweb --test-suite start 'Login Tests' \\" << std::endl;
    std::cerr << "           --url login.com --assert-exists '#username' \\" << std::endl;
    std::cerr << "           --test-suite end json" << std::endl;
}

// Event-driven navigation waiting - replaces the polling wait_for_completion
bool wait_for_navigation_complete(Browser& browser, int timeout_ms) {
    // Use the new event-driven navigation waiting
    return browser.waitForNavigationSignal(timeout_ms);
}

// Event-driven page ready waiting
bool wait_for_page_ready(Browser& browser, int timeout_ms) {
    return browser.waitForPageReadyEvent(timeout_ms);
}

void list_sessions(SessionManager& sessionManager) {
    auto sessions = sessionManager.listSessions();
    
    if (sessions.empty()) {
        info_output("No active sessions.");
        return;
    }
    
    info_output("Active sessions:");
    for (const auto& info : sessions) {
        info_output("  " + info.name + " - " + info.url + " (" + info.sizeStr + ", " + info.lastAccessedStr + ")");
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string sessionName, url;
    bool endSession = false;
    bool listSessions = false;
    bool json_mode = false;
    bool silent_mode = false;
    int browser_width = 1000;
    std::vector<Command> commands;
    std::vector<Assertion::Command> assertions;
    
    // Initialize managers
    assertionManager = std::make_unique<Assertion::Manager>();
    uploadManager = std::make_unique<FileOps::UploadManager>();
    downloadManager = std::make_unique<FileOps::DownloadManager>();
    
    // Current assertion being built
    Assertion::Command current_assertion;
    bool has_pending_assertion = false;
    
    // File operation settings
    size_t max_file_size = 104857600; // 100MB default
    std::vector<std::string> allowed_types = {"*"};
    std::string download_dir = "";
    int upload_timeout = 30000;
    int download_timeout = 30000;
    
    // Parse arguments
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--session" && i + 1 < args.size()) {
            sessionName = args[++i];
        } else if (args[i] == "--url" && i + 1 < args.size()) {
            url = args[++i];
        } else if (args[i] == "--end") {
            endSession = true;
        } else if (args[i] == "--list") {
            listSessions = true;
        } else if (args[i] == "--debug") {
            g_debug = true;
        } else if (args[i] == "--json") {
            json_mode = true;
        } else if (args[i] == "--silent") {
            silent_mode = true;
        } else if (args[i] == "--width" && i + 1 < args.size()) {
            browser_width = std::stoi(args[++i]);
        } else if (args[i] == "--user-agent" && i + 1 < args.size()) {
            commands.push_back({"user-agent", "", args[++i]});
        } 
        
        // Test Suite Management
        else if (args[i] == "--test-suite" && i + 1 < args.size()) {
            std::string action = args[++i];
            if (action == "start" && i + 1 < args.size()) {
                std::string suite_name = args[++i];
                assertionManager->startSuite(suite_name);
            } else if (action == "end") {
                std::string format = "text";
                if (i + 1 < args.size() && args[i + 1][0] != '-') {
                    format = args[++i];
                }
                assertionManager->endSuite(json_mode, format);
            }
        }
        
        // Assertion Commands
        else if (args[i] == "--assert-exists" && i + 1 < args.size()) {
            if (has_pending_assertion) {
                assertions.push_back(current_assertion);
            }
            
            current_assertion = {};
            current_assertion.type = "exists";
            current_assertion.selector = args[++i];
            current_assertion.expected_value = "true";
            current_assertion.op = Assertion::ComparisonOperator::EQUALS;
            current_assertion.json_output = json_mode;
            current_assertion.silent = silent_mode;
            current_assertion.case_sensitive = true;
            current_assertion.timeout_ms = 5000;
            has_pending_assertion = true;
            
            if (i + 1 < args.size() && args[i + 1][0] != '-') {
                current_assertion.expected_value = args[++i];
            }
        } else if (args[i] == "--assert-text" && i + 2 < args.size()) {
            if (has_pending_assertion) {
                assertions.push_back(current_assertion);
            }
            
            current_assertion = {};
            current_assertion.type = "text";
            current_assertion.selector = args[++i];
            current_assertion.expected_value = args[++i];
            current_assertion.op = Assertion::ComparisonOperator::EQUALS;
            current_assertion.json_output = json_mode;
            current_assertion.silent = silent_mode;
            current_assertion.case_sensitive = true;
            current_assertion.timeout_ms = 5000;
            has_pending_assertion = true;
        } else if (args[i] == "--assert-count" && i + 2 < args.size()) {
            if (has_pending_assertion) {
                assertions.push_back(current_assertion);
            }
            
            current_assertion = {};
            current_assertion.type = "count";
            current_assertion.selector = args[++i];
            current_assertion.expected_value = args[++i];
            current_assertion.op = Assertion::ComparisonOperator::EQUALS;
            current_assertion.json_output = json_mode;
            current_assertion.silent = silent_mode;
            current_assertion.case_sensitive = true;
            current_assertion.timeout_ms = 5000;
            has_pending_assertion = true;
        } else if (args[i] == "--assert-js" && i + 1 < args.size()) {
            if (has_pending_assertion) {
                assertions.push_back(current_assertion);
            }
            
            current_assertion = {};
            current_assertion.type = "js";
            current_assertion.selector = args[++i];
            current_assertion.expected_value = "true";
            current_assertion.op = Assertion::ComparisonOperator::EQUALS;
            current_assertion.json_output = json_mode;
            current_assertion.silent = silent_mode;
            current_assertion.case_sensitive = true;
            current_assertion.timeout_ms = 5000;
            has_pending_assertion = true;
            
            if (i + 1 < args.size() && args[i + 1][0] != '-') {
                current_assertion.expected_value = args[++i];
            }
        } else if (args[i] == "--message" && i + 1 < args.size()) {
            if (has_pending_assertion) {
                current_assertion.custom_message = args[++i];
            } else {
                error_output("--message must follow an assertion command");
                return 1;
            }
        } else if (args[i] == "--timeout" && i + 1 < args.size()) {
            int timeout = std::stoi(args[++i]);
            if (has_pending_assertion) {
                current_assertion.timeout_ms = timeout;
            } else {
                error_output("--timeout must follow an assertion command");
                return 1;
            }
        }
        
        // File Upload Commands
        else if (args[i] == "--upload" && i + 2 < args.size()) {
            Command cmd;
            cmd.type = "upload";
            cmd.selector = args[++i];
            cmd.value = args[++i];
            cmd.timeout = upload_timeout;
            commands.push_back(cmd);
        } else if (args[i] == "--upload-multiple" && i + 2 < args.size()) {
            Command cmd;
            cmd.type = "upload-multiple";
            cmd.selector = args[++i];
            cmd.value = args[++i];
            cmd.timeout = upload_timeout;
            commands.push_back(cmd);
        }
        
        // Download Monitoring Commands
        else if (args[i] == "--download-wait" && i + 1 < args.size()) {
            Command cmd;
            cmd.type = "download-wait";
            cmd.selector = args[++i];
            cmd.value = "";
            cmd.timeout = download_timeout;
            commands.push_back(cmd);
        } else if (args[i] == "--download-wait-multiple" && i + 1 < args.size()) {
            Command cmd;
            cmd.type = "download-wait-multiple";
            cmd.selector = "";
            cmd.value = args[++i];
            cmd.timeout = download_timeout;
            commands.push_back(cmd);
        }
        
        // File Operation Options
        else if (args[i] == "--max-file-size" && i + 1 < args.size()) {
            max_file_size = std::stoull(args[++i]);
        } else if (args[i] == "--allowed-types" && i + 1 < args.size()) {
            std::string types_str = args[++i];
            allowed_types.clear();
            std::stringstream ss(types_str);
            std::string type;
            while (std::getline(ss, type, ',')) {
                type.erase(0, type.find_first_not_of(" \t"));
                type.erase(type.find_last_not_of(" \t") + 1);
                if (!type.empty()) {
                    allowed_types.push_back(type);
                }
            }
        } else if (args[i] == "--download-dir" && i + 1 < args.size()) {
            download_dir = args[++i];
        } else if (args[i] == "--upload-timeout" && i + 1 < args.size()) {
            upload_timeout = std::stoi(args[++i]);
        } else if (args[i] == "--download-timeout" && i + 1 < args.size()) {
            download_timeout = std::stoi(args[++i]);
        }
        
        // Advanced Waiting Commands
        else if (args[i] == "--wait-text-advanced" && i + 1 < args.size()) {
            Command cmd;
            cmd.type = "wait-text-advanced";
            cmd.selector = "";
            cmd.value = args[++i];
            cmd.timeout = 10000;
            commands.push_back(cmd);
        } else if (args[i] == "--wait-network-idle") {
            Command cmd;
            cmd.type = "wait-network-idle";
            cmd.selector = "";
            cmd.value = "500";
            cmd.timeout = 30000;
            if (i + 1 < args.size() && args[i + 1][0] != '-') {
                cmd.value = args[++i];
            }
            commands.push_back(cmd);
        } else if (args[i] == "--wait-network-request" && i + 1 < args.size()) {
            Command cmd;
            cmd.type = "wait-network-request";
            cmd.selector = "";
            cmd.value = args[++i];
            cmd.timeout = 15000;
            commands.push_back(cmd);
        } else if (args[i] == "--wait-element-visible" && i + 1 < args.size()) {
            Command cmd;
            cmd.type = "wait-element-visible";
            cmd.selector = args[++i];
            cmd.value = "";
            cmd.timeout = 10000;
            commands.push_back(cmd);
        } else if (args[i] == "--wait-element-count" && i + 3 < args.size()) {
            Command cmd;
            cmd.type = "wait-element-count";
            cmd.selector = args[++i];
            cmd.value = args[++i] + " " + args[++i];
            cmd.timeout = 10000;
            commands.push_back(cmd);
        } else if (args[i] == "--wait-attribute" && i + 3 < args.size()) {
            Command cmd;
            cmd.type = "wait-attribute";
            cmd.selector = args[++i];
            cmd.value = args[++i] + " " + args[++i];
            cmd.timeout = 10000;
            commands.push_back(cmd);
        } else if (args[i] == "--wait-url-change" && i + 1 < args.size()) {
            Command cmd;
            cmd.type = "wait-url-change";
            cmd.selector = "";
            cmd.value = args[++i];
            cmd.timeout = 10000;
            commands.push_back(cmd);
        } else if (args[i] == "--wait-title-change" && i + 1 < args.size()) {
            Command cmd;
            cmd.type = "wait-title-change";
            cmd.selector = "";
            cmd.value = args[++i];
            cmd.timeout = 10000;
            commands.push_back(cmd);
        } else if (args[i] == "--wait-spa-navigation") {
            Command cmd;
            cmd.type = "wait-spa-navigation";
            cmd.selector = "";
            cmd.value = "";
            cmd.timeout = 10000;
            if (i + 1 < args.size() && args[i + 1][0] != '-') {
                cmd.value = args[++i];
            }
            commands.push_back(cmd);
        } else if (args[i] == "--wait-framework-ready") {
            Command cmd;
            cmd.type = "wait-framework-ready";
            cmd.selector = "";
            cmd.value = "auto";
            cmd.timeout = 15000;
            if (i + 1 < args.size() && args[i + 1][0] != '-') {
                cmd.value = args[++i];
            }
            commands.push_back(cmd);
        } else if (args[i] == "--wait-dom-change" && i + 1 < args.size()) {
            Command cmd;
            cmd.type = "wait-dom-change";
            cmd.selector = args[++i];
            cmd.value = "";
            cmd.timeout = 10000;
            commands.push_back(cmd);
        } else if (args[i] == "--wait-content-change" && i + 2 < args.size()) {
            Command cmd;
            cmd.type = "wait-content-change";
            cmd.selector = args[++i];
            cmd.value = args[++i];
            cmd.timeout = 10000;
            commands.push_back(cmd);
        }
        
        // Regular Commands
        else if (args[i] == "--type" && i + 2 < args.size()) {
            commands.push_back({"type", args[i+1], args[i+2]});
            i += 2;
        } else if (args[i] == "--click" && i + 1 < args.size()) {
            commands.push_back({"click", args[++i], ""});
        } else if (args[i] == "--submit") {
            std::string form_sel = (i + 1 < args.size() && args[i+1][0] != '-') ? args[++i] : "form";
            commands.push_back({"submit", form_sel, ""});
        } else if (args[i] == "--wait" && i + 1 < args.size()) {
            commands.push_back({"wait", args[++i], ""});
        } else if (args[i] == "--wait-nav") {
            commands.push_back({"wait-nav", "", ""});
        } else if (args[i] == "--wait-ready" && i + 1 < args.size()) {
            commands.push_back({"wait-ready", args[++i], ""});
        } else if (args[i] == "--text" && i + 1 < args.size()) {
            commands.push_back({"text", args[++i], ""});
        } else if (args[i] == "--search" && i + 1 < args.size()) {
            commands.push_back({"search", "", args[++i]});
        } else if (args[i] == "--js" && i + 1 < args.size()) {
            commands.push_back({"js", "", args[++i]});
        } else if (args[i] == "--store" && i + 2 < args.size()) {
            commands.push_back({"store", args[i+1], args[i+2]});
            i += 2;
        } else if (args[i] == "--get" && i + 1 < args.size()) {
            commands.push_back({"get", args[++i], ""});
        } else if (args[i] == "--back") {
            commands.push_back({"back", "", ""});
        } else if (args[i] == "--forward") {
            commands.push_back({"forward", "", ""});
        } else if (args[i] == "--reload") {
            commands.push_back({"reload", "", ""});
        } else if (args[i] == "--select" && i + 2 < args.size()) {
            commands.push_back({"select", args[i+1], args[i+2]});
            i += 2;
        } else if (args[i] == "--check" && i + 1 < args.size()) {
            commands.push_back({"check", args[++i], ""});
        } else if (args[i] == "--uncheck" && i + 1 < args.size()) {
            commands.push_back({"uncheck", args[++i], ""});
        } else if (args[i] == "--focus" && i + 1 < args.size()) {
            commands.push_back({"focus", args[++i], ""});
        } else if (args[i] == "--exists" && i + 1 < args.size()) {
            commands.push_back({"exists", args[++i], ""});
        } else if (args[i] == "--count" && i + 1 < args.size()) {
            commands.push_back({"count", args[++i], ""});
        } else if (args[i] == "--html" && i + 1 < args.size()) {
            commands.push_back({"html", args[++i], ""});
        } else if (args[i] == "--attr" && i + 2 < args.size()) {
            if (i + 3 < args.size() && args[i+3][0] != '-') {
                commands.push_back({"set-attr", args[i+1], args[i+2] + " " + args[i+3]});
                i += 3;
            } else {
                commands.push_back({"attr", args[i+1], args[i+2]});
                i += 2;
            }
        } else if (args[i] == "--screenshot") {
            std::string filename = (i + 1 < args.size() && args[i+1][0] != '-') ? args[++i] : "screenshot.png";
            commands.push_back({"screenshot", filename, ""});
        } else if (args[i] == "--screenshot-full") {
            std::string filename = (i + 1 < args.size() && args[i+1][0] != '-') ? args[++i] : "screenshot-full.png";
            commands.push_back({"screenshot-full", filename, ""});
        } else if (args[i] == "--extract" && i + 2 < args.size()) {
            commands.push_back({"extract", args[i+1], args[i+2]});
            i += 2;
        } else if (args[i] == "--record-start") {
            commands.push_back({"record-start", "", ""});
        } else if (args[i] == "--record-stop") {
            commands.push_back({"record-stop", "", ""});
        } else if (args[i] == "--replay" && i + 1 < args.size()) {
            commands.push_back({"replay", args[++i], ""});
        }
    }
    
    // Add any final pending assertion
    if (has_pending_assertion) {
        assertions.push_back(current_assertion);
    }
    
    // Configure assertion manager
    assertionManager->setSilentMode(silent_mode);
    assertionManager->setJsonOutput(json_mode);
    
    // Configure file operation managers
    uploadManager->setMaxFileSize(max_file_size);
    uploadManager->setDefaultTimeout(upload_timeout);
    
    if (!download_dir.empty()) {
        downloadManager->setDownloadDirectory(download_dir);
    }
    downloadManager->setDefaultTimeout(download_timeout);
    
    std::string home = std::getenv("HOME");
    SessionManager sessionManager(home + "/.hweb/sessions");

    // Handle list command
    if (listSessions) {
        list_sessions(sessionManager);
        return 0;
    }

    if (sessionName.empty()) {
        sessionName = "default";
    }

    // Handle end session
    if (endSession) {
        Session session = sessionManager.loadOrCreateSession(sessionName);
        sessionManager.saveSession(session);
        info_output("Session '" + sessionName + "' ended.");
        return 0;
    }

    // Load or create session
    Session session = sessionManager.loadOrCreateSession(sessionName);
    
    // Create browser only if we need it
    if (!url.empty() || !commands.empty() || !assertions.empty() || !session.getCurrentUrl().empty()) {
        Browser browser;
        browser.setViewport(browser_width, 800);
        
        // Determine navigation strategy
        bool should_navigate = false;
        std::string navigation_url;
        bool is_session_restore = false;
        
        if (!url.empty()) {
            should_navigate = true;
            navigation_url = url;
            is_session_restore = false;
        } else if (!session.getCurrentUrl().empty() && commands.empty() && assertions.empty()) {
            should_navigate = true;
            navigation_url = session.getCurrentUrl();
            is_session_restore = true;
            info_output("Restoring session URL: " + navigation_url);
        } else if (session.getCurrentUrl().empty() && commands.empty() && assertions.empty()) {
            error_output("No URL in session. Use --url to navigate.");
            return 1;
        } else if (!session.getCurrentUrl().empty() && (!commands.empty() || !assertions.empty())) {
            should_navigate = true;
            navigation_url = session.getCurrentUrl();
            is_session_restore = true;
        }
        
        // Navigate if we determined we should
        if (should_navigate) {
            try {
                browser.loadUri(navigation_url);
                
                if (!wait_for_navigation_complete(browser, 15000)) {
                    error_output("Navigation timeout for: " + navigation_url);
                    return 1;
                }
                
                if (!wait_for_page_ready(browser, 5000)) {
                    info_output("Warning: Page may not be fully ready, continuing...");
                }
                
                if (!url.empty()) {
                    session.addToHistory(navigation_url);
                    session.setCurrentUrl(navigation_url);
                    info_output("Navigated to " + navigation_url);
                }
                
                if (is_session_restore) {
                    browser.restoreSession(session);
                }
            } catch (const std::invalid_argument& e) {
                error_output(e.what());
                return 1;
            } catch (const std::exception& e) {
                error_output("Error: Failed to navigate to " + navigation_url + ": " + e.what());
                return 1;
            }
        }

        // Execute commands and assertions
        int exit_code = 0;
        bool state_modified = false;
        bool isHistoryNavigation = false;
        
        // Execute commands
        for (const auto& cmd : commands) {
            bool navigation_expected = false;
            
            if (cmd.type == "type" || cmd.type == "click" || cmd.type == "submit" || 
                cmd.type == "select" || cmd.type == "check" || cmd.type == "uncheck" ||
                cmd.type == "js" || cmd.type == "scroll" || cmd.type == "user-agent") {
                state_modified = true;
            }
            
            // Record action if recording is enabled
            if (session.isRecording() && 
                (cmd.type == "type" || cmd.type == "click" || cmd.type == "submit" || 
                 cmd.type == "select" || cmd.type == "check" || cmd.type == "uncheck")) {
                Session::RecordedAction action;
                action.type = cmd.type;
                action.selector = cmd.selector;
                action.value = cmd.value;
                action.delay = 500;
                session.recordAction(action);
            }
            
            // Execute command
            if (cmd.type == "store") {
                session.setCustomVariable(cmd.selector, cmd.value);
                info_output("Stored variable '" + cmd.selector + "'");
            } else if (cmd.type == "get") {
                if (session.hasCustomVariable(cmd.selector)) {
                    std::cout << session.getCustomVariable(cmd.selector) << std::endl;
                } else {
                    std::cout << "" << std::endl;
                }
            }
            
            // File Upload Commands
            else if (cmd.type == "upload") {
                FileOps::UploadCommand upload_cmd;
                upload_cmd.selector = cmd.selector;
                upload_cmd.filepath = cmd.value;
                upload_cmd.timeout_ms = cmd.timeout;
                upload_cmd.max_file_size = max_file_size;
                upload_cmd.allowed_types = allowed_types;
                upload_cmd.json_output = json_mode;
                upload_cmd.silent = silent_mode;
                
                FileOps::UploadResult result = uploadManager->uploadFile(browser, upload_cmd);
                
                if (result == FileOps::UploadResult::SUCCESS) {
                    info_output("File uploaded successfully: " + cmd.value);
                } else {
                    error_output("Upload failed: " + uploadManager->getErrorMessage(result, cmd.value));
                    exit_code = static_cast<int>(result);
                }

            } else if (cmd.type == "upload-multiple") {
                std::vector<std::string> filepaths;
                std::stringstream ss(cmd.value);
                std::string filepath;
                while (std::getline(ss, filepath, ',')) {
                    filepath.erase(0, filepath.find_first_not_of(" \t"));
                    filepath.erase(filepath.find_last_not_of(" \t") + 1);
                    if (!filepath.empty()) {
                        filepaths.push_back(filepath);
                    }
                }
                
                FileOps::UploadResult result = uploadManager->uploadMultipleFiles(browser, cmd.selector, filepaths, cmd.timeout);
                
                if (result == FileOps::UploadResult::SUCCESS) {
                    info_output("Multiple files uploaded successfully");
                } else {
                    error_output("Multiple upload failed: " + uploadManager->getErrorMessage(result, cmd.value));
                    exit_code = static_cast<int>(result);
                }

            // Download Monitoring Commands
            } else if (cmd.type == "download-wait") {
                FileOps::DownloadCommand download_cmd;
                download_cmd.filename_pattern = cmd.selector;
                download_cmd.download_dir = download_dir;
                download_cmd.timeout_ms = cmd.timeout;
                download_cmd.json_output = json_mode;
                download_cmd.silent = silent_mode;
                
                FileOps::DownloadResult result = downloadManager->waitForDownload(download_cmd);
                
                if (result == FileOps::DownloadResult::SUCCESS) {
                    info_output("Download completed: " + cmd.selector);
                } else {
                    error_output("Download failed: " + downloadManager->getErrorMessage(result, cmd.selector));
                    exit_code = static_cast<int>(result);
                }

            } else if (cmd.type == "download-wait-multiple") {
                std::vector<std::string> patterns;
                std::stringstream ss(cmd.value);
                std::string pattern;
                while (std::getline(ss, pattern, ',')) {
                    pattern.erase(0, pattern.find_first_not_of(" \t"));
                    pattern.erase(pattern.find_last_not_of(" \t") + 1);
                    if (!pattern.empty()) {
                        patterns.push_back(pattern);
                    }
                }
                
                FileOps::DownloadResult result = downloadManager->waitForMultipleDownloads(patterns, download_dir, cmd.timeout);
                
                if (result == FileOps::DownloadResult::SUCCESS) {
                    info_output("All downloads completed");
                } else {
                    error_output("Multiple download failed: " + downloadManager->getErrorMessage(result, cmd.value));
                    exit_code = static_cast<int>(result);
                }

            // Advanced Waiting Commands
            } else if (cmd.type == "wait-text-advanced") {
                std::string text = cmd.value;
                bool case_sensitive = false;
                bool exact_match = false;
                
                size_t colon1 = text.find(':');
                if (colon1 != std::string::npos) {
                    std::string options = text.substr(colon1 + 1);
                    text = text.substr(0, colon1);
                    case_sensitive = options.find("case_sensitive") != std::string::npos;
                    exact_match = options.find("exact_match") != std::string::npos;
                }
                
                if (browser.waitForTextAdvanced(text, cmd.timeout, case_sensitive, exact_match)) {
                    info_output("Text found: " + text);
                } else {
                    error_output("Text not found within timeout: " + text);
                    exit_code = 1;
                }

            } else if (cmd.type == "wait-network-idle") {
                int idle_time = std::stoi(cmd.value);
                
                if (browser.waitForNetworkIdle(idle_time, cmd.timeout)) {
                    info_output("Network became idle");
                } else {
                    error_output("Network idle timeout");
                    exit_code = 1;
                }

            } else if (cmd.type == "wait-network-request") {
                if (browser.waitForNetworkRequest(cmd.value, cmd.timeout)) {
                    info_output("Network request detected: " + cmd.value);
                } else {
                    error_output("Network request timeout: " + cmd.value);
                    exit_code = 1;
                }

            } else if (cmd.type == "wait-element-visible") {
                if (browser.waitForElementVisible(cmd.selector, cmd.timeout)) {
                    info_output("Element became visible: " + cmd.selector);
                } else {
                    error_output("Element visibility timeout: " + cmd.selector);
                    exit_code = 1;
                }

            } else if (cmd.type == "wait-element-count") {
                std::istringstream iss(cmd.value);
                std::string operator_str;
                int expected_count;
                iss >> operator_str >> expected_count;
                
                if (browser.waitForElementCount(cmd.selector, operator_str, expected_count, cmd.timeout)) {
                    info_output("Element count condition met: " + cmd.selector + " " + operator_str + " " + std::to_string(expected_count));
                } else {
                    error_output("Element count timeout: " + cmd.selector);
                    exit_code = 1;
                }

            } else if (cmd.type == "wait-attribute") {
                std::istringstream iss(cmd.value);
                std::string attribute, expected_value;
                iss >> attribute >> expected_value;
                
                if (browser.waitForAttribute(cmd.selector, attribute, expected_value, cmd.timeout)) {
                    info_output("Attribute condition met: " + cmd.selector + "[" + attribute + "='" + expected_value + "']");
                } else {
                    error_output("Attribute timeout: " + cmd.selector);
                    exit_code = 1;
                }

            } else if (cmd.type == "wait-url-change") {
                if (browser.waitForUrlChange(cmd.value, cmd.timeout)) {
                    info_output("URL changed to match pattern: " + cmd.value);
                } else {
                    error_output("URL change timeout: " + cmd.value);
                    exit_code = 1;
                }

            } else if (cmd.type == "wait-title-change") {
                if (browser.waitForTitleChange(cmd.value, cmd.timeout)) {
                    info_output("Title changed to match pattern: " + cmd.value);
                } else {
                    error_output("Title change timeout: " + cmd.value);
                    exit_code = 1;
                }

            } else if (cmd.type == "wait-spa-navigation") {
                if (browser.waitForSPANavigation(cmd.value, cmd.timeout)) {
                    info_output("SPA navigation detected: " + cmd.value);
                } else {
                    error_output("SPA navigation timeout: " + cmd.value);
                    exit_code = 1;
                }

            } else if (cmd.type == "wait-framework-ready") {
                if (browser.waitForFrameworkReady(cmd.value, cmd.timeout)) {
                    info_output("Framework ready: " + cmd.value);
                } else {
                    error_output("Framework ready timeout: " + cmd.value);
                    exit_code = 1;
                }

            } else if (cmd.type == "wait-dom-change") {
                if (browser.waitForDOMChange(cmd.selector, cmd.timeout)) {
                    info_output("DOM change detected: " + cmd.selector);
                } else {
                    error_output("DOM change timeout: " + cmd.selector);
                    exit_code = 1;
                }

            } else if (cmd.type == "wait-content-change") {
                if (browser.waitForContentChange(cmd.selector, cmd.value, cmd.timeout)) {
                    info_output("Content change detected: " + cmd.selector + "." + cmd.value);
                } else {
                    error_output("Content change timeout: " + cmd.selector);
                    exit_code = 1;
                }
            
            // Existing commands (abbreviated for space - include all existing commands here)
            } else if (cmd.type == "back") {
                isHistoryNavigation = true;
                if (session.canGoBack()) {
                    int targetIndex = session.getHistoryIndex() - 1;
                    const auto& history = session.getHistory();
                    
                    if (targetIndex >= 0 && targetIndex < static_cast<int>(history.size())) {
                        std::string targetUrl = history[targetIndex];
                        info_output("Navigating back to: " + targetUrl);
                        
                        browser.loadUri(targetUrl);
                        
                        if (wait_for_navigation_complete(browser, 5000)) {
                            session.setHistoryIndex(targetIndex);
                            session.setCurrentUrl(targetUrl);
                            info_output("Navigated back");
                            browser.restoreSession(session);
                        } else {
                            error_output("Back navigation timeout");
                            exit_code = 1;
                        }
                    } else {
                        error_output("Invalid history index");
                        exit_code = 1;
                    }
                } else {
                    error_output("Cannot go back - no history");
                    exit_code = 1;
                }
            // ... include all other existing command implementations ...
            }
            
            // Handle navigation updates
            if (navigation_expected && !isHistoryNavigation) {
                browser.waitForPageStabilization(2000);
                std::string new_url = browser.getCurrentUrl();
                if (new_url != session.getCurrentUrl() && !new_url.empty()) {
                    session.addToHistory(new_url);
                    session.setCurrentUrl(new_url);
                    info_output("Navigation detected: " + new_url);
                }
            }
        }
        
        // Execute assertions
        for (const auto& assertion : assertions) {
            Assertion::Result result = assertionManager->executeAssertion(browser, assertion);
            
            if (result == Assertion::Result::FAIL || result == Assertion::Result::ERROR) {
                exit_code = static_cast<int>(result);
            }
        }

        // Update session state
        if (state_modified || should_navigate) {
            try {
                browser.updateSessionState(session);
            } catch (const std::exception& e) {
                error_output("Warning: Failed to update session state: " + std::string(e.what()));
            }
        }
        
        // Save the session
        try {
            sessionManager.saveSession(session);
            
            if (!commands.empty() || !assertions.empty()) {
                info_output("Session '" + sessionName + "' saved.");
            }
        } catch (const std::exception& e) {
            error_output("Error: Failed to save session: " + std::string(e.what()));
            exit_code = 1;
        }

        // Ensure all GTK events are processed before exit
        for (int i = 0; i < 10; i++) {
            while (g_main_context_pending(g_main_context_default())) {
                g_main_context_iteration(g_main_context_default(), FALSE);
            }
            browser.wait(10);
        }

        return exit_code;
    } else {
        error_output("No URL in session. Use --url to navigate.");
        return 1;
    }
}
