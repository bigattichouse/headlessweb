#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <glib.h>
#include "SessionManager.h"
#include "Browser.h"

struct Command {
    std::string type;
    std::string selector;
    std::string value;
    int timeout = 10000;
};

void print_usage() {
    std::cerr << "Usage: hweb-poc [options] [commands...]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --session <n>        Use named session (default: 'default')" << std::endl;
    std::cerr << "  --url <url>          Navigate to URL" << std::endl;
    std::cerr << "  --end                End session" << std::endl;
    std::cerr << "  --list               List all sessions" << std::endl;
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
    std::cerr << "  --screenshot [file]          Take screenshot" << std::endl;
    std::cerr << "  --extract <name> <js>        Add custom state extractor" << std::endl;
    std::cerr << "  --record-start               Start recording actions" << std::endl;
    std::cerr << "  --record-stop                Stop recording actions" << std::endl;
    std::cerr << "  --replay <name>              Replay recorded actions" << std::endl;
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
        std::cout << "No active sessions." << std::endl;
        return;
    }
    
    std::cout << "Active sessions:" << std::endl;
    for (const auto& info : sessions) {
        std::cout << "  " << info.name 
                  << " - " << info.url 
                  << " (" << info.sizeStr << ", " 
                  << info.lastAccessedStr << ")" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string sessionName, url;
    bool endSession = false;
    bool listSessions = false;
    std::vector<Command> commands;

    // Parse arguments (enhanced)
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--session" && i + 1 < args.size()) {
            sessionName = args[++i];
        } else if (args[i] == "--url" && i + 1 < args.size()) {
            url = args[++i];
        } else if (args[i] == "--end") {
            endSession = true;
        } else if (args[i] == "--list") {
            listSessions = true;
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

    if (endSession) {
        sessionManager.deleteSession(sessionName);
        std::cout << "Session '" << sessionName << "' ended." << std::endl;
        return 0;
    }

    // Load or create session
    Session session = sessionManager.loadOrCreateSession(sessionName);
    Browser browser;
    
    // Restore the full session state
    browser.restoreSession(session);

    // Navigate if URL provided
    if (!url.empty()) {
        browser.loadUri(url);
        wait_for_completion(browser, 10000);
        session.addToHistory(url);
        session.setCurrentUrl(url);
        std::cout << "Navigated to " << url << std::endl;
    } else if (session.getCurrentUrl().empty() && commands.empty()) {
        // No URL in session and no commands
        std::cout << "No URL in session. Use --url to navigate." << std::endl;
        return 1;
    }

    // Execute commands in sequence
    int exit_code = 0;
    for (const auto& cmd : commands) {
        bool navigation_expected = false;
        
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
            std::cout << "Stored variable '" << cmd.selector << "'" << std::endl;
        } else if (cmd.type == "get") {
            if (session.hasCustomVariable(cmd.selector)) {
                std::cout << session.getCustomVariable(cmd.selector) << std::endl;
            } else {
                std::cerr << "Variable '" << cmd.selector << "' not found" << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "back") {
            if (session.canGoBack()) {
                browser.goBack();
                wait_for_completion(browser, 5000);
                session.setHistoryIndex(session.getHistoryIndex() - 1);
                std::cout << "Navigated back" << std::endl;
                navigation_expected = true;
            } else {
                std::cerr << "Cannot go back - no history" << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "forward") {
            if (session.canGoForward()) {
                browser.goForward();
                wait_for_completion(browser, 5000);
                session.setHistoryIndex(session.getHistoryIndex() + 1);
                std::cout << "Navigated forward" << std::endl;
                navigation_expected = true;
            } else {
                std::cerr << "Cannot go forward - at end of history" << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "reload") {
            browser.reload();
            wait_for_completion(browser, 5000);
            std::cout << "Page reloaded" << std::endl;
        } else if (cmd.type == "type") {
            if (browser.fillInput(cmd.selector, cmd.value)) {
                std::cout << "Typed '" << cmd.value << "' into " << cmd.selector << std::endl;
            } else {
                std::cerr << "Failed to type into " << cmd.selector << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "click") {
            if (browser.clickElement(cmd.selector)) {
                std::cout << "Clicked " << cmd.selector << std::endl;
                navigation_expected = true;
            } else {
                std::cerr << "Failed to click " << cmd.selector << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "submit") {
            if (browser.submitForm(cmd.selector)) {
                std::cout << "Submitted form " << cmd.selector << std::endl;
                navigation_expected = true;
            } else {
                std::cerr << "Failed to submit form " << cmd.selector << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "select") {
            if (browser.selectOption(cmd.selector, cmd.value)) {
                std::cout << "Selected '" << cmd.value << "' in " << cmd.selector << std::endl;
            } else {
                std::cerr << "Failed to select option in " << cmd.selector << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "check") {
            if (browser.checkElement(cmd.selector)) {
                std::cout << "Checked " << cmd.selector << std::endl;
            } else {
                std::cerr << "Failed to check " << cmd.selector << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "uncheck") {
            if (browser.uncheckElement(cmd.selector)) {
                std::cout << "Unchecked " << cmd.selector << std::endl;
            } else {
                std::cerr << "Failed to uncheck " << cmd.selector << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "focus") {
            if (browser.focusElement(cmd.selector)) {
                std::cout << "Focused " << cmd.selector << std::endl;
            } else {
                std::cerr << "Failed to focus " << cmd.selector << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "wait") {
            if (browser.waitForSelector(cmd.selector, cmd.timeout)) {
                std::cout << "Element " << cmd.selector << " appeared" << std::endl;
            } else {
                std::cerr << "Element " << cmd.selector << " not found within timeout" << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "wait-nav") {
            if (browser.waitForNavigation(cmd.timeout)) {
                std::cout << "Navigation completed" << std::endl;
            } else {
                std::cout << "Navigation timeout or no navigation detected" << std::endl;
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
                    std::cout << "Ready condition met: " << cmd.selector << std::endl;
                } else {
                    std::cerr << "Ready condition not met: " << cmd.selector << std::endl;
                    exit_code = 1;
                }
            } else {
                if (browser.waitForJsCondition(cmd.selector, condition.timeout)) {
                    std::cout << "Ready condition met: " << cmd.selector << std::endl;
                } else {
                    std::cerr << "Ready condition not met: " << cmd.selector << std::endl;
                    exit_code = 1;
                }
            }
        } else if (cmd.type == "text") {
            std::string text = browser.getInnerText(cmd.selector);
            std::cout << text << std::endl;
        } else if (cmd.type == "search") {
            if (browser.searchForm(cmd.value)) {
                std::cout << "Search submitted: " << cmd.value << std::endl;
                navigation_expected = true;
            } else {
                std::cerr << "Failed to submit search" << std::endl;
                exit_code = 1;
            }
        } else if (cmd.type == "js") {
            std::string result;
            browser.executeJavascript(cmd.value, &result);
            wait_for_completion(browser, 5000);
            std::cout << result << std::endl;
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
            std::cout << "Screenshot saved to " << cmd.selector << std::endl;
        } else if (cmd.type == "extract") {
            session.addStateExtractor(cmd.selector, cmd.value);
            std::cout << "Added state extractor '" << cmd.selector << "'" << std::endl;
        } else if (cmd.type == "record-start") {
            session.setRecording(true);
            session.clearRecordedActions();
            std::cout << "Started recording actions" << std::endl;
        } else if (cmd.type == "record-stop") {
            session.setRecording(false);
            std::cout << "Stopped recording. " << session.getRecordedActions().size() << " actions recorded." << std::endl;
        } else if (cmd.type == "replay") {
            // Find recorded actions by name (for now, just use the current session's actions)
            const auto& actions = session.getRecordedActions();
            if (actions.empty()) {
                std::cerr << "No recorded actions to replay" << std::endl;
                exit_code = 1;
            } else {
                std::cout << "Replaying " << actions.size() << " actions..." << std::endl;
                if (browser.executeActionSequence(actions)) {
                    std::cout << "Replay completed successfully" << std::endl;
                } else {
                    std::cerr << "Replay failed" << std::endl;
                    exit_code = 1;
                }
            }
        }
        
        // Handle navigation updates
        if (navigation_expected) {
            std::string new_url = browser.getCurrentUrl();
            if (new_url != session.getCurrentUrl()) {
                session.addToHistory(new_url);
                session.setCurrentUrl(new_url);
                std::cout << "Navigation detected: " << new_url << std::endl;
            }
        }
    }

    // Update session state with all browser state
    browser.updateSessionState(session);
    
    // Save the comprehensive session
    sessionManager.saveSession(session);
    
    if (!commands.empty()) {
        std::cout << "Session '" << sessionName << "' saved." << std::endl;
    }

    // Ensure all GTK events are processed before exit
    for (int i = 0; i < 10; i++) {
        while (g_main_context_pending(g_main_context_default())) {
            g_main_context_iteration(g_main_context_default(), FALSE);
        }
        g_usleep(10 * 1000); // 10ms
    }

    return exit_code;
}
