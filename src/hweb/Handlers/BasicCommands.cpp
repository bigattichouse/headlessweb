#include "BasicCommands.h"
#include "../Output.h"
#include <iostream>
#include <fstream>

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
    if (cmd.type == "type" || cmd.type == "fill-enhanced" || cmd.type == "click" || cmd.type == "submit" || 
        cmd.type == "select" || cmd.type == "check" || cmd.type == "uncheck" || 
        cmd.type == "focus" || cmd.type == "js") {
        return handle_interaction_command(browser, cmd);
    }
    
    // Data extraction commands
    if (cmd.type == "text" || cmd.type == "exists" || cmd.type == "count" || 
        cmd.type == "html" || cmd.type == "attr") {
        return handle_data_extraction_command(browser, cmd);
    }
    
    // Special commands
    if (cmd.type == "wait" || cmd.type == "wait-nav" || cmd.type == "wait-ready" ||
        cmd.type == "search" || cmd.type == "screenshot" || cmd.type == "screenshot-full" ||
        cmd.type == "extract" || cmd.type == "record-start" || cmd.type == "record-stop" ||
        cmd.type == "replay" || cmd.type == "set-attr") {
        return handle_special_command(browser, session, cmd);
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
            browser.fillInputEnhanced(cmd.selector, cmd.value);
            Output::info("Typed text into: " + cmd.selector);
        } else if (cmd.type == "fill-enhanced") {
            browser.fillInputEnhanced(cmd.selector, cmd.value);
            Output::info("Enhanced fill into: " + cmd.selector);
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

int BasicCommandHandler::handle_special_command(Browser& browser, Session& session, const Command& cmd) {
    try {
        if (cmd.type == "wait") {
            if (browser.waitForSelector(cmd.selector, cmd.timeout)) {
                Output::info("Element found: " + cmd.selector);
                return 0;
            } else {
                Output::error("Element not found: " + cmd.selector);
                return 1;
            }
        } else if (cmd.type == "wait-nav") {
            if (wait_for_navigation_complete(browser, 10000)) {
                Output::info("Navigation completed");
                return 0;
            } else {
                Output::error("Navigation timeout");
                return 1;
            }
        } else if (cmd.type == "wait-ready") {
            // Check if it's a selector or JavaScript condition
            if (cmd.selector.find("(") != std::string::npos || cmd.selector.find("function") != std::string::npos) {
                // JavaScript condition
                std::string js = "return " + cmd.selector;
                if (browser.waitForJsCondition(js, cmd.timeout)) {
                    Output::info("Ready condition met");
                    return 0;
                } else {
                    Output::error("Ready condition timeout");
                    return 1;
                }
            } else {
                // Element selector
                if (browser.waitForSelector(cmd.selector, cmd.timeout)) {
                    Output::info("Element ready: " + cmd.selector);
                    return 0;
                } else {
                    Output::error("Element not ready: " + cmd.selector);
                    return 1;
                }
            }
        } else if (cmd.type == "search") {
            browser.searchForm(cmd.value);
            Output::info("Searched for: " + cmd.value);
            return 0;
        } else if (cmd.type == "screenshot") {
            browser.takeScreenshot(cmd.selector);
            Output::info("Screenshot saved: " + cmd.selector);
            return 0;
        } else if (cmd.type == "screenshot-full") {
            browser.takeFullPageScreenshot(cmd.selector);
            Output::info("Full page screenshot saved: " + cmd.selector);
            return 0;
        } else if (cmd.type == "set-attr") {
            // Parse "attribute value" from cmd.value
            size_t space_pos = cmd.value.find(' ');
            if (space_pos != std::string::npos) {
                std::string attribute = cmd.value.substr(0, space_pos);
                std::string value = cmd.value.substr(space_pos + 1);
                browser.setAttribute(cmd.selector, attribute, value);
                Output::info("Set attribute " + attribute + " to " + value + " on " + cmd.selector);
                return 0;
            } else {
                Output::error("Invalid attribute format. Use: --attr selector attribute value");
                return 1;
            }
        } else if (cmd.type == "extract") {
            // Extract data and save to file
            std::string data = browser.executeJavascriptSync(cmd.value);
            // Save data to file specified in cmd.selector
            std::ofstream file(cmd.selector);
            if (file.is_open()) {
                file << data;
                file.close();
                Output::info("Data extracted to: " + cmd.selector);
                return 0;
            } else {
                Output::error("Failed to write to file: " + cmd.selector);
                return 1;
            }
        } else if (cmd.type == "record-start") {
            session.setRecording(true);
            Output::info("Recording started");
            return 0;
        } else if (cmd.type == "record-stop") {
            session.setRecording(false);
            Output::info("Recording stopped");
            return 0;
        } else if (cmd.type == "replay") {
            // Replay recorded actions from session
            const auto& actions = session.getRecordedActions();
            for (const auto& action : actions) {
                if (action.type == "click") {
                    browser.clickElement(action.selector);
                } else if (action.type == "type") {
                    browser.fillInput(action.selector, action.value);
                } else if (action.type == "submit") {
                    browser.submitForm(action.selector);
                }
                // Add delay between actions
                browser.wait(action.delay);
            }
            Output::info("Replayed " + std::to_string(actions.size()) + " actions");
            return 0;
        }
        
        return 0;
    } catch (const std::exception& e) {
        Output::error("Special command failed: " + std::string(e.what()));
        return 1;
    }
}

bool BasicCommandHandler::wait_for_navigation_complete(Browser& browser, int timeout_ms) {
    return browser.waitForNavigationSignal(timeout_ms);
}

} // namespace HWeb