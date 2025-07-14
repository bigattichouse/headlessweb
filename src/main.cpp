#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <glib.h>
#include "Session/Manager.h"
#include "Browser/Browser.h"
#include "Assertion/Manager.h"
#include "Debug.h"

struct Command {
    std::string type;
    std::string selector;
    std::string value;
    int timeout = 10000;
};

// Global assertion manager
static std::unique_ptr<Assertion::Manager> assertionManager;

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
    std::cerr << "  hweb --url example.com --assert-exists '.login-form'" << std::endl;
    std::cerr << "  hweb --assert-text 'h1' 'Welcome' --json" << std::endl;
    std::cerr << "  hweb --assert-count '.item' '>5' --silent" << std::endl;
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
    
    // Initialize assertion manager
    assertionManager = std::make_unique<Assertion::Manager>();
    
    // Current assertion being built
    Assertion::Command current_assertion;
    bool has_pending_assertion = false;
    
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
            // Save any pending assertion first
            if (has_pending_assertion) {
                assertions.push_back(current_assertion);
            }
            
            current_assertion = {};
            current_assertion.type = "exists";
            current_assertion.selector = args[++i];
            current_assertion.expected_value = "true"; // Default
            current_assertion.op = Assertion::ComparisonOperator::EQUALS;
            current_assertion.json_output = json_mode;
            current_assertion.silent = silent_mode;
            current_assertion.case_sensitive = true;
            current_assertion.timeout_ms = 5000; // Default timeout
            has_pending_assertion = true;
            
            // Check for optional true/false value
            if (i + 1 < args.size() && args[i + 1][0] != '-') {
                current_assertion.expected_value = args[++i];
            }
        } else if (args[i] == "--assert-text" && i + 2 < args.size()) {
            // Save any pending assertion first
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
            // Save any pending assertion first
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
            // Save any pending assertion first
            if (has_pending_assertion) {
                assertions.push_back(current_assertion);
            }
            
            current_assertion = {};
            current_assertion.type = "js";
            current_assertion.selector = args[++i]; // JS expression
            current_assertion.expected_value = "true"; // Default
            current_assertion.op = Assertion::ComparisonOperator::EQUALS;
            current_assertion.json_output = json_mode;
            current_assertion.silent = silent_mode;
            current_assertion.case_sensitive = true;
            current_assertion.timeout_ms = 5000;
            has_pending_assertion = true;
            
            // Check for optional true/false value
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
    
    std::string home = std::getenv("HOME");
    SessionManager sessionManager(home + "/.hweb/sessions");

    // Handle list command
    if (listSessions) {
        list_sessions(sessionManager);
        return 0;
    }

    if (sessionName.empty()) {
        sessionName = "default"; // Use default session if none specified
    }

    // Handle end session - keep the session file but clear the URL
    if (endSession) {
        Session session = sessionManager.loadOrCreateSession(sessionName);
        // Don't clear the URL, just save and "end" the session
        sessionManager.saveSession(session);
        info_output("Session '" + sessionName + "' ended.");
        return 0;
    }

    // Load or create session
    Session session = sessionManager.loadOrCreateSession(sessionName);
    
    // Create browser only if we need it
    if (!url.empty() || !commands.empty() || !assertions.empty() || !session.getCurrentUrl().empty()) {
        Browser browser;
        browser.setViewport(browser_width, 800); // Default height
        
        // Determine navigation strategy
        bool should_navigate = false;
        std::string navigation_url;
        bool is_session_restore = false;
        
        if (!url.empty()) {
            // URL explicitly provided - navigate and don't restore state yet
            should_navigate = true;
            navigation_url = url;
            is_session_restore = false;
        } else if (!session.getCurrentUrl().empty() && commands.empty() && assertions.empty()) {
            // No URL provided, no commands, but session has URL - this is pure session restoration
            should_navigate = true;
            navigation_url = session.getCurrentUrl();
            is_session_restore = true;
            info_output("Restoring session URL: " + navigation_url);
        } else if (session.getCurrentUrl().empty() && commands.empty() && assertions.empty()) {
            // No URL anywhere and no commands to run
            error_output("No URL in session. Use --url to navigate.");
            return 1;
        }
        // If we have commands/assertions but no explicit URL, we'll use the session's current URL
        else if (!session.getCurrentUrl().empty() && (!commands.empty() || !assertions.empty())) {
            should_navigate = true;
            navigation_url = session.getCurrentUrl();
            is_session_restore = true; // DO restore state when running commands on existing session
        }
        
        // Navigate if we determined we should
        if (should_navigate) {
            try {
                browser.loadUri(navigation_url);
                
                // Use event-driven navigation waiting instead of polling
                if (!wait_for_navigation_complete(browser, 15000)) {
                    error_output("Navigation timeout for: " + navigation_url);
                    return 1;
                }
                
                // Use event-driven page ready waiting
                if (!wait_for_page_ready(browser, 5000)) {
                    info_output("Warning: Page may not be fully ready, continuing...");
                }
                
                if (!url.empty()) {
                    // Only update history if URL was explicitly provided
                    session.addToHistory(navigation_url);
                    session.setCurrentUrl(navigation_url);
                    info_output("Navigated to " + navigation_url);
                }
                
                // Restore session state after navigation if needed
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

        // Execute commands and assertions in sequence
        int exit_code = 0;
        bool state_modified = false;  // Track if any command modified page state
        bool isHistoryNavigation = false;  // Track if we're doing back/forward navigation
        
        // Combine commands and assertions into a single execution flow
        size_t cmd_index = 0;
        size_t assert_index = 0;
        
        // Execute commands first, then assertions
        for (const auto& cmd : commands) {
            bool navigation_expected = false;
            
            // Check if this command modifies state
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
                action.delay = 500; // Default delay
                session.recordAction(action);
            }
            
            // Execute command (existing command execution logic)
            if (cmd.type == "store") {
                session.setCustomVariable(cmd.selector, cmd.value);
                info_output("Stored variable '" + cmd.selector + "'");
            } else if (cmd.type == "get") {
                if (session.hasCustomVariable(cmd.selector)) {
                    std::cout << session.getCustomVariable(cmd.selector) << std::endl;
                } else {
                    std::cout << "" << std::endl;
                }
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
            } else if (cmd.type == "forward") {
                isHistoryNavigation = true;
                if (session.canGoForward()) {
                    int targetIndex = session.getHistoryIndex() + 1;
                    const auto& history = session.getHistory();
                    
                    if (targetIndex >= 0 && targetIndex < static_cast<int>(history.size())) {
                        std::string targetUrl = history[targetIndex];
                        info_output("Navigating forward to: " + targetUrl);
                        
                        browser.loadUri(targetUrl);
                        
                        if (wait_for_navigation_complete(browser, 5000)) {
                            session.setHistoryIndex(targetIndex);
                            session.setCurrentUrl(targetUrl);
                            info_output("Navigated forward");
                            browser.restoreSession(session);
                        } else {
                            error_output("Forward navigation timeout");
                            exit_code = 1;
                        }
                    } else {
                        error_output("Invalid history index");
                        exit_code = 1;
                    }
                } else {
                    error_output("Cannot go forward - at end of history");
                    exit_code = 1;
                }
            } else if (cmd.type == "reload") {
                browser.reload();
                if (wait_for_navigation_complete(browser, 5000)) {
                    info_output("Page reloaded");
                } else {
                    info_output("Reload timeout, but page may have loaded");
                }
            } else if (cmd.type == "user-agent") {
                browser.setUserAgent(cmd.value);
                session.setUserAgent(cmd.value);
                info_output("Set user agent to: " + cmd.value);
            } else if (cmd.type == "type") {
                if (browser.fillInput(cmd.selector, cmd.value)) {
                    info_output("Typed into " + cmd.selector);
                } else {
                    error_output("Failed to type into " + cmd.selector);
                }
            } else if (cmd.type == "click") {
                if (browser.clickElement(cmd.selector)) {
                    info_output("Clicked " + cmd.selector);
                    navigation_expected = true;
                } else {
                    error_output("Failed to click " + cmd.selector);
                }
            } else if (cmd.type == "submit") {
                if (browser.submitForm(cmd.selector)) {
                    info_output("Submitted form " + cmd.selector);
                    navigation_expected = true;
                } else {
                    error_output("Failed to submit form " + cmd.selector);
                }
            } else if (cmd.type == "select") {
                if (browser.selectOption(cmd.selector, cmd.value)) {
                    info_output("Selected '" + cmd.value + "' in " + cmd.selector);
                } else {
                    error_output("Failed to select option in " + cmd.selector);
                }
            } else if (cmd.type == "check") {
                if (browser.checkElement(cmd.selector)) {
                    info_output("Checked " + cmd.selector);
                } else {
                    error_output("Failed to check " + cmd.selector);
                }
            } else if (cmd.type == "uncheck") {
                if (browser.uncheckElement(cmd.selector)) {
                    info_output("Unchecked " + cmd.selector);
                } else {
                    error_output("Failed to uncheck " + cmd.selector);
                }
            } else if (cmd.type == "focus") {
                if (browser.focusElement(cmd.selector)) {
                    info_output("Focused " + cmd.selector);
                } else {
                    error_output("Failed to focus " + cmd.selector);
                }
            } else if (cmd.type == "wait") {
                if (browser.waitForSelector(cmd.selector, cmd.timeout)) {
                    info_output("Element " + cmd.selector + " appeared");
                } else {
                    error_output("Element " + cmd.selector + " not found within timeout");
                    exit_code = 1;
                }
            } else if (cmd.type == "wait-nav") {
                if (browser.waitForNavigation(cmd.timeout)) {
                    info_output("Navigation completed");
                } else {
                    info_output("Navigation timeout or no navigation detected");
                }
            } else if (cmd.type == "wait-ready") {
                PageReadyCondition condition;
                if (cmd.selector.find("return") != std::string::npos || 
                    cmd.selector.find("==") != std::string::npos ||
                    cmd.selector.find("window.") != std::string::npos ||
                    cmd.selector.find("document.") != std::string::npos) {
                    condition.type = PageReadyCondition::JS_EXPRESSION;
                } else {
                    condition.type = PageReadyCondition::SELECTOR;
                }
                condition.value = cmd.selector;
                condition.timeout = 10000;
                session.addReadyCondition(condition);
                
                if (condition.type == PageReadyCondition::SELECTOR) {
                    if (browser.waitForSelector(cmd.selector, condition.timeout)) {
                        info_output("Ready condition met: " + cmd.selector);
                    } else {
                        error_output("Ready condition not met: " + cmd.selector);
                        exit_code = 1;
                    }
                } else {
                    if (browser.waitForJsCondition(cmd.selector, condition.timeout)) {
                        info_output("Ready condition met: " + cmd.selector);
                    } else {
                        error_output("Ready condition not met: " + cmd.selector);
                        exit_code = 1;
                    }
                }
            } else if (cmd.type == "text") {
                std::string text = browser.getInnerText(cmd.selector);
                if (text.empty()) {
                    std::string anyText = browser.executeJavascriptSyncSafe("(function() { try { return document.body ? document.body.innerText.substring(0, 100) : 'NO_BODY'; } catch(e) { return 'ERROR: ' + e.message; } })()");
                    if (anyText.find("NO_BODY") != std::string::npos || anyText.find("ERROR") != std::string::npos) {
                        std::cout << "NOT_FOUND" << std::endl;
                    } else {
                        std::cout << "" << std::endl;
                    }
                } else {
                    std::cout << text << std::endl;
                }
            } else if (cmd.type == "search") {
                if (browser.searchForm(cmd.value)) {
                    info_output("Search submitted: " + cmd.value);
                    navigation_expected = true;
                } else {
                    error_output("Failed to submit search");
                    exit_code = 1;
                }
            } else if (cmd.type == "js") {
                std::string result;
                browser.executeJavascript(cmd.value, &result);
                browser.waitForJavaScriptCompletion(5000);
                
                std::cout << result << std::endl;
                
                if (cmd.value.find("cookie") != std::string::npos ||
                    cmd.value.find("Storage") != std::string::npos ||
                    cmd.value.find("scroll") != std::string::npos) {
                    browser.updateSessionState(session);
                }
            } else if (cmd.type == "exists") {
                bool exists = browser.elementExists(cmd.selector);
                std::cout << (exists ? "true" : "false") << std::endl;
                exit_code = exists ? 0 : 1;
            } else if (cmd.type == "count") {
                int count = browser.countElements(cmd.selector);
                std::cout << count << std::endl;
            } else if (cmd.type == "html") {
                std::string html = browser.getElementHtml(cmd.selector);
                std::cout << html << std::endl;
            } else if (cmd.type == "attr") {
                std::string value = browser.getAttribute(cmd.selector, cmd.value);
                std::cout << value << std::endl;
            } else if (cmd.type == "set-attr") {
                size_t space_pos = cmd.value.find(' ');
                std::string attribute = cmd.value.substr(0, space_pos);
                std::string value = cmd.value.substr(space_pos + 1);
                if (browser.setAttribute(cmd.selector, attribute, value)) {
                    info_output("Set attribute '" + attribute + "' on " + cmd.selector);
                } else {
                    error_output("Failed to set attribute on " + cmd.selector);
                }
            } else if (cmd.type == "screenshot") {
                browser.takeScreenshot(cmd.selector);
                info_output("Screenshot saved to " + cmd.selector);
            } else if (cmd.type == "screenshot-full") {
                browser.takeFullPageScreenshot(cmd.selector);
                info_output("Full page screenshot saved to " + cmd.selector);
            } else if (cmd.type == "extract") {
                session.addStateExtractor(cmd.selector, cmd.value);
                info_output("Added state extractor '" + cmd.selector + "'");
            } else if (cmd.type == "record-start") {
                session.setRecording(true);
                session.clearRecordedActions();
                info_output("Started recording actions");
            } else if (cmd.type == "record-stop") {
                session.setRecording(false);
                info_output("Stopped recording. " + std::to_string(session.getRecordedActions().size()) + " actions recorded.");
            } else if (cmd.type == "replay") {
                const auto& actions = session.getRecordedActions();
                if (actions.empty()) {
                    error_output("No recorded actions to replay");
                    exit_code = 1;
                } else {
                    info_output("Replaying " + std::to_string(actions.size()) + " actions...");
                    if (browser.executeActionSequence(actions)) {
                        info_output("Replay completed successfully");
                    } else {
                        error_output("Replay failed");
                        exit_code = 1;
                    }
                }
            }
            
            // Handle navigation updates using event-driven detection
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
            
            // Update exit code based on assertion results
            if (result == Assertion::Result::FAIL || result == Assertion::Result::ERROR) {
                exit_code = static_cast<int>(result);
            }
        }

        // Update session state with all browser state (defensive)
        if (state_modified || should_navigate) {
            try {
                browser.updateSessionState(session);
            } catch (const std::exception& e) {
                error_output("Warning: Failed to update session state: " + std::string(e.what()));
            }
        }
        
        // Save the session (defensive)
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
            browser.wait(10); // 10ms
        }

        return exit_code;
    } else {
        // No URL and no commands/assertions - just output message
        error_output("No URL in session. Use --url to navigate.");
        return 1;
    }
}
