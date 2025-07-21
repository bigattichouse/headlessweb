#include "BasicCommands.h"
#include "../Output.h"
#include <iostream>

namespace HWeb {

BasicCommandHandler::BasicCommandHandler() {
}

BasicCommandHandler::~BasicCommandHandler() {
}

int BasicCommandHandler::handle_command(Browser& browser, Session& session, const Command& cmd) {
    // Session-specific commands
    if (cmd.type == "store" || cmd.type == "get") {
        return handle_session_command(session, cmd);
    }
    
    // History navigation commands
    if (cmd.type == "back" || cmd.type == "forward") {
        return handle_history_navigation(browser, session, cmd);
    }
    
    // Navigation commands
    if (cmd.type == "reload") {
        return handle_navigation_command(browser, session, cmd);
    }
    
    // Interaction commands
    if (cmd.type == "type" || cmd.type == "click" || cmd.type == "submit" || 
        cmd.type == "select" || cmd.type == "check" || cmd.type == "uncheck" || 
        cmd.type == "focus" || cmd.type == "js") {
        return handle_interaction_command(browser, cmd);
    }
    
    // Data extraction commands
    if (cmd.type == "text" || cmd.type == "exists" || cmd.type == "count" || 
        cmd.type == "html" || cmd.type == "attr") {
        return handle_data_extraction_command(browser, cmd);
    }
    
    // Other commands
    if (cmd.type == "wait" || cmd.type == "wait-nav" || cmd.type == "wait-ready" ||
        cmd.type == "search" || cmd.type == "screenshot" || cmd.type == "screenshot-full" ||
        cmd.type == "extract" || cmd.type == "record-start" || cmd.type == "record-stop" ||
        cmd.type == "replay") {
        // Handle these commands directly
        return handle_interaction_command(browser, cmd);
    }
    
    return 0;
}

int BasicCommandHandler::handle_session_command(Session& session, const Command& cmd) {
    if (cmd.type == "store") {
        session.setCustomVariable(cmd.selector, cmd.value);
        Output::info("Stored variable '" + cmd.selector + "'");
        return 0;
    } else if (cmd.type == "get") {
        if (session.hasCustomVariable(cmd.selector)) {
            std::cout << session.getCustomVariable(cmd.selector) << std::endl;
        } else {
            std::cout << "" << std::endl;
        }
        return 0;
    }
    
    return 0;
}

int BasicCommandHandler::handle_history_navigation(Browser& browser, Session& session, const Command& cmd) {
    if (cmd.type == "back") {
        if (session.canGoBack()) {
            int targetIndex = session.getHistoryIndex() - 1;
            const auto& history = session.getHistory();
            
            if (targetIndex >= 0 && targetIndex < static_cast<int>(history.size())) {
                std::string targetUrl = history[targetIndex];
                Output::info("Navigating back to: " + targetUrl);
                
                browser.loadUri(targetUrl);
                
                if (wait_for_navigation_complete(browser, 5000)) {
                    session.setHistoryIndex(targetIndex);
                    session.setCurrentUrl(targetUrl);
                    Output::info("Navigated back");
                    browser.restoreSession(session);
                    return 0;
                } else {
                    Output::error("Back navigation timeout");
                    return 1;
                }
            } else {
                Output::error("Invalid history index");
                return 1;
            }
        } else {
            Output::error("Cannot go back - no history");
            return 1;
        }
    } else if (cmd.type == "forward") {
        // Similar implementation for forward navigation
        if (session.canGoForward()) {
            int targetIndex = session.getHistoryIndex() + 1;
            const auto& history = session.getHistory();
            
            if (targetIndex < static_cast<int>(history.size())) {
                std::string targetUrl = history[targetIndex];
                Output::info("Navigating forward to: " + targetUrl);
                
                browser.loadUri(targetUrl);
                
                if (wait_for_navigation_complete(browser, 5000)) {
                    session.setHistoryIndex(targetIndex);
                    session.setCurrentUrl(targetUrl);
                    Output::info("Navigated forward");
                    browser.restoreSession(session);
                    return 0;
                } else {
                    Output::error("Forward navigation timeout");
                    return 1;
                }
            } else {
                Output::error("Invalid history index");
                return 1;
            }
        } else {
            Output::error("Cannot go forward - no history");
            return 1;
        }
    }
    
    return 0;
}

int BasicCommandHandler::handle_navigation_command(Browser& browser, Session& session, const Command& cmd) {
    if (cmd.type == "reload") {
        try {
            browser.reload();
            if (wait_for_navigation_complete(browser, 5000)) {
                Output::info("Page reloaded");
                return 0;
            } else {
                Output::error("Reload timeout");
                return 1;
            }
        } catch (const std::exception& e) {
            Output::error("Reload failed: " + std::string(e.what()));
            return 1;
        }
    }
    
    return 0;
}

int BasicCommandHandler::handle_interaction_command(Browser& browser, const Command& cmd) {
    try {
        if (cmd.type == "type") {
            browser.fillInput(cmd.selector, cmd.value);
            Output::info("Typed text into: " + cmd.selector);
        } else if (cmd.type == "click") {
            browser.clickElement(cmd.selector);
            Output::info("Clicked: " + cmd.selector);
        } else if (cmd.type == "submit") {
            browser.submitForm(cmd.selector);
            Output::info("Submitted form: " + cmd.selector);
        } else if (cmd.type == "select") {
            browser.selectOption(cmd.selector, cmd.value);
            Output::info("Selected option: " + cmd.value + " in " + cmd.selector);
        } else if (cmd.type == "check") {
            browser.checkElement(cmd.selector);
            Output::info("Checked: " + cmd.selector);
        } else if (cmd.type == "uncheck") {
            browser.uncheckElement(cmd.selector);
            Output::info("Unchecked: " + cmd.selector);
        } else if (cmd.type == "focus") {
            browser.focusElement(cmd.selector);
            Output::info("Focused: " + cmd.selector);
        } else if (cmd.type == "js") {
            std::string result = browser.executeJavascriptSync(cmd.value);
            if (!Output::is_silent_mode()) {
                std::cout << result << std::endl;
            }
        } else if (cmd.type == "wait") {
            if (browser.waitForSelector(cmd.selector, cmd.timeout)) {
                Output::info("Element found: " + cmd.selector);
            } else {
                Output::error("Element not found: " + cmd.selector);
                return 1;
            }
        }
        // Add other interaction commands as needed
        
        return 0;
    } catch (const std::exception& e) {
        Output::error("Command failed: " + std::string(e.what()));
        return 1;
    }
}

int BasicCommandHandler::handle_data_extraction_command(Browser& browser, const Command& cmd) {
    try {
        if (cmd.type == "text") {
            std::string text = browser.getInnerText(cmd.selector);
            std::cout << text << std::endl;
        } else if (cmd.type == "exists") {
            bool exists = browser.elementExists(cmd.selector);
            std::cout << (exists ? "true" : "false") << std::endl;
        } else if (cmd.type == "count") {
            int count = browser.countElements(cmd.selector);
            std::cout << count << std::endl;
        } else if (cmd.type == "html") {
            std::string html = browser.getElementHtml(cmd.selector);
            std::cout << html << std::endl;
        } else if (cmd.type == "attr") {
            std::string attr_value = browser.getAttribute(cmd.selector, cmd.value);
            std::cout << attr_value << std::endl;
        }
        
        return 0;
    } catch (const std::exception& e) {
        Output::error("Data extraction failed: " + std::string(e.what()));
        return 1;
    }
}

bool BasicCommandHandler::wait_for_navigation_complete(Browser& browser, int timeout_ms) {
    return browser.waitForNavigationSignal(timeout_ms);
}

} // namespace HWeb