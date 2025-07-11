#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <glib.h>
#include "SessionManager.h"
#include "Browser.h"
#include "Debug.h"

struct Command {
    std::string type;
    std::string selector;
    std::string value;
    int timeout = 10000;
};

// Helper functions for output control
void info_output(const std::string& message) {
    std::cerr << message << std::endl;  // Always show info messages
}

void error_output(const std::string& message) {
    std::cerr << message << std::endl;
}

void print_usage() {
    std::cerr << "Usage: hweb-poc [options] [commands...]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --session <n>        Use named session (default: 'default')" << std::endl;
    std::cerr << "  --url <url>          Navigate to URL" << std::endl;
    std::cerr << "  --end                End session" << std::endl;
    std::cerr << "  --list               List all sessions" << std::endl;
    std::cerr << "  --debug              Enable debug output" << std::endl;
    std::cerr << "  --user-agent <ua>    Set custom user agent" << std::endl;
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
}

void wait_for_completion(Browser& browser, int timeout_ms) {
    int elapsed_time = 0;
    while (!browser.isOperationCompleted() && elapsed_time < timeout_ms) {
        g_main_context_iteration(g_main_context_default(), FALSE);
        g_usleep(10 * 1000);
        elapsed_time += 10;
    }
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
    std::vector<Command> commands;

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
        } else if (args[i] == "--user-agent" && i + 1 < args.size()) {
            commands.push_back({"user-agent", "", args[++i]});
        } else if (args[i] == "--type" && i + 2 < args.size()) {
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
            commands.push_back({"attr", args[i+1], args[i+2]});
            i += 2;
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

    std::string home = std::getenv("HOME");
    SessionManager sessionManager(home + "/.hweb-poc/sessions");

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
    if (!url.empty() || !commands.empty() || !session.getCurrentUrl().empty()) {
        Browser browser;
        
        // Determine navigation strategy
        bool should_navigate = false;
        std::string navigation_url;
        bool is_session_restore = false;
        
        if (!url.empty()) {
            // URL explicitly provided - navigate and don't restore state yet
            should_navigate = true;
            navigation_url = url;
            is_session_restore = false;
        } else if (!session.getCurrentUrl().empty() && commands.empty()) {
            // No URL provided, no commands, but session has URL - this is pure session restoration
            should_navigate = true;
            navigation_url = session.getCurrentUrl();
            is_session_restore = true;
            info_output("Restoring session URL: " + navigation_url);
        } else if (session.getCurrentUrl().empty() && commands.empty()) {
            // No URL anywhere and no commands to run
            error_output("No URL in session. Use --url to navigate.");
            return 1;
        }
        // If we have commands but no explicit URL, we'll use the session's current URL
        else if (!session.getCurrentUrl().empty() && !commands.empty()) {
            should_navigate = true;
            navigation_url = session.getCurrentUrl();
            is_session_restore = true; // DO restore state when running commands on existing session
        }
        
        // Navigate if we determined we should
        if (should_navigate) {
            browser.loadUri(navigation_url);
            wait_for_completion(browser, 15000);  // Increased timeout for file URLs
            
            // Wait a bit more for the page to stabilize
            browser.waitForPageStabilization(2000);
            
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
        }

        // Execute commands in sequence
        int exit_code = 0;
        bool state_modified = false;  // Track if any command modified page state
        
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
            
            if (cmd.type == "store") {
                session.setCustomVariable(cmd.selector, cmd.value);
                info_output("Stored variable '" + cmd.selector + "'");
            } else if (cmd.type == "get") {
                if (session.hasCustomVariable(cmd.selector)) {
                    std::cout << session.getCustomVariable(cmd.selector) << std::endl;
                } else {
                    // Return empty string instead of error for missing variables
                    std::cout << "" << std::endl;
                }
            } else if (cmd.type == "back") {
                if (session.canGoBack()) {
                    browser.goBack();
                    wait_for_completion(browser, 5000);
                    session.setHistoryIndex(session.getHistoryIndex() - 1);
                    info_output("Navigated back");
                    navigation_expected = true;
                } else {
                    error_output("Cannot go back - no history");
                    exit_code = 1;
                }
            } else if (cmd.type == "forward") {
                if (session.canGoForward()) {
                    browser.goForward();
                    wait_for_completion(browser, 5000);
                    session.setHistoryIndex(session.getHistoryIndex() + 1);
                    info_output("Navigated forward");
                    navigation_expected = true;
                } else {
                    error_output("Cannot go forward - at end of history");
                    exit_code = 1;
                }
            } else if (cmd.type == "reload") {
                browser.reload();
                wait_for_completion(browser, 5000);
                info_output("Page reloaded");
            } else if (cmd.type == "user-agent") {
                browser.setUserAgent(cmd.value);
                session.setUserAgent(cmd.value);
                info_output("Set user agent to: " + cmd.value);
            } else if (cmd.type == "type") {
                // Wait for page to be ready before typing
                browser.waitForPageStabilization(1000);
                if (browser.fillInput(cmd.selector, cmd.value)) {
                    info_output("Typed into " + cmd.selector);
                } else {
                    error_output("Failed to type into " + cmd.selector);
                    // Don't fail the whole script for form failures
                }
            } else if (cmd.type == "click") {
                if (browser.clickElement(cmd.selector)) {
                    info_output("Clicked " + cmd.selector);
                    navigation_expected = true;
                } else {
                    error_output("Failed to click " + cmd.selector);
                    // Don't fail the whole script for click failures
                }
            } else if (cmd.type == "submit") {
                if (browser.submitForm(cmd.selector)) {
                    info_output("Submitted form " + cmd.selector);
                    navigation_expected = true;
                } else {
                    error_output("Failed to submit form " + cmd.selector);
                }
            } else if (cmd.type == "select") {
                browser.waitForPageStabilization(1000);
                if (browser.selectOption(cmd.selector, cmd.value)) {
                    info_output("Selected '" + cmd.value + "' in " + cmd.selector);
                } else {
                    error_output("Failed to select option in " + cmd.selector);
                }
            } else if (cmd.type == "check") {
                browser.waitForPageStabilization(1000);
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
                // Determine if it's a selector or JS expression
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
                    // Try to get any text from the page for debugging
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
                wait_for_completion(browser, 5000);
                
                // Output ONLY the JavaScript result to stdout
                std::cout << result << std::endl;
                
                // If the JS modified state (cookies, storage, etc), update session
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
                // Find recorded actions by name (for now, just use the current session's actions)
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
            
            // Handle navigation updates
            if (navigation_expected) {
                // Give navigation time to complete
                browser.waitForPageStabilization(2000);
                std::string new_url = browser.getCurrentUrl();
                if (new_url != session.getCurrentUrl() && !new_url.empty()) {
                    session.addToHistory(new_url);
                    session.setCurrentUrl(new_url);
                    info_output("Navigation detected: " + new_url);
                }
            }
        }

        // Update session state with all browser state (defensive)
        // Always update if state was modified or if we navigated
        if (state_modified || should_navigate) {
            try {
                browser.updateSessionState(session);
            } catch (const std::exception& e) {
                error_output("Warning: Failed to update session state: " + std::string(e.what()));
                // Continue anyway, at least save what we can
            }
        }
        
        // Save the session (defensive)
        try {
            sessionManager.saveSession(session);
            
            if (!commands.empty()) {
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
            g_usleep(10 * 1000); // 10ms
        }

        return exit_code;
    } else {
        // No URL and no commands - just output message
        error_output("No URL in session. Use --url to navigate.");
        return 1;
    }
}
