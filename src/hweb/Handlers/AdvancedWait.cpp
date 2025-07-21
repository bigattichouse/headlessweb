#include "AdvancedWait.h"
#include "../Output.h"
#include <sstream>

namespace HWeb {

AdvancedWaitHandler::AdvancedWaitHandler() {
}

AdvancedWaitHandler::~AdvancedWaitHandler() {
}

int AdvancedWaitHandler::handle_command(Browser& browser, const Command& cmd) {
    if (cmd.type == "wait-text-advanced") {
        return handle_wait_text_advanced(browser, cmd);
    } else if (cmd.type == "wait-network-idle") {
        return handle_wait_network_idle(browser, cmd);
    } else if (cmd.type == "wait-network-request") {
        return handle_wait_network_request(browser, cmd);
    } else if (cmd.type == "wait-element-visible") {
        return handle_wait_element_visible(browser, cmd);
    } else if (cmd.type == "wait-element-count") {
        return handle_wait_element_count(browser, cmd);
    } else if (cmd.type == "wait-attribute") {
        return handle_wait_attribute(browser, cmd);
    } else if (cmd.type == "wait-url-change") {
        return handle_wait_url_change(browser, cmd);
    } else if (cmd.type == "wait-title-change") {
        return handle_wait_title_change(browser, cmd);
    } else if (cmd.type == "wait-spa-navigation") {
        return handle_wait_spa_navigation(browser, cmd);
    } else if (cmd.type == "wait-framework-ready") {
        return handle_wait_framework_ready(browser, cmd);
    } else if (cmd.type == "wait-dom-change") {
        return handle_wait_dom_change(browser, cmd);
    } else if (cmd.type == "wait-content-change") {
        return handle_wait_content_change(browser, cmd);
    }
    
    return 0;
}

int AdvancedWaitHandler::handle_wait_text_advanced(Browser& browser, const Command& cmd) {
    std::string text = cmd.value;
    bool case_sensitive = false;
    bool exact_match = false;
    
    size_t colon_pos = text.find(':');
    if (colon_pos != std::string::npos) {
        std::string options = text.substr(colon_pos + 1);
        text = text.substr(0, colon_pos);
        case_sensitive = options.find("case_sensitive") != std::string::npos;
        exact_match = options.find("exact_match") != std::string::npos;
    }
    
    if (browser.waitForTextAdvanced(text, cmd.timeout, case_sensitive, exact_match)) {
        Output::info("Text found: " + text);
        return 0;
    } else {
        Output::error("Text not found within timeout: " + text);
        return 1;
    }
}

int AdvancedWaitHandler::handle_wait_network_idle(Browser& browser, const Command& cmd) {
    int idle_time = std::stoi(cmd.value);
    
    if (browser.waitForNetworkIdle(idle_time, cmd.timeout)) {
        Output::info("Network became idle");
        return 0;
    } else {
        Output::error("Network idle timeout");
        return 1;
    }
}

int AdvancedWaitHandler::handle_wait_network_request(Browser& browser, const Command& cmd) {
    if (browser.waitForNetworkRequest(cmd.value, cmd.timeout)) {
        Output::info("Network request detected: " + cmd.value);
        return 0;
    } else {
        Output::error("Network request timeout: " + cmd.value);
        return 1;
    }
}

int AdvancedWaitHandler::handle_wait_element_visible(Browser& browser, const Command& cmd) {
    if (browser.waitForElementVisible(cmd.selector, cmd.timeout)) {
        Output::info("Element became visible: " + cmd.selector);
        return 0;
    } else {
        Output::error("Element visibility timeout: " + cmd.selector);
        return 1;
    }
}

int AdvancedWaitHandler::handle_wait_element_count(Browser& browser, const Command& cmd) {
    std::istringstream iss(cmd.value);
    std::string operator_str;
    int expected_count;
    iss >> operator_str >> expected_count;
    
    if (browser.waitForElementCount(cmd.selector, operator_str, expected_count, cmd.timeout)) {
        Output::info("Element count condition met: " + cmd.selector + " " + operator_str + " " + std::to_string(expected_count));
        return 0;
    } else {
        Output::error("Element count timeout: " + cmd.selector);
        return 1;
    }
}

int AdvancedWaitHandler::handle_wait_attribute(Browser& browser, const Command& cmd) {
    std::istringstream iss(cmd.value);
    std::string attribute, expected_value;
    iss >> attribute >> expected_value;
    
    if (browser.waitForAttribute(cmd.selector, attribute, expected_value, cmd.timeout)) {
        Output::info("Attribute condition met: " + cmd.selector + "[" + attribute + "='" + expected_value + "']");
        return 0;
    } else {
        Output::error("Attribute timeout: " + cmd.selector);
        return 1;
    }
}

int AdvancedWaitHandler::handle_wait_url_change(Browser& browser, const Command& cmd) {
    if (browser.waitForUrlChange(cmd.value, cmd.timeout)) {
        Output::info("URL changed to match pattern: " + cmd.value);
        return 0;
    } else {
        Output::error("URL change timeout: " + cmd.value);
        return 1;
    }
}

int AdvancedWaitHandler::handle_wait_title_change(Browser& browser, const Command& cmd) {
    if (browser.waitForTitleChange(cmd.value, cmd.timeout)) {
        Output::info("Title changed to match pattern: " + cmd.value);
        return 0;
    } else {
        Output::error("Title change timeout: " + cmd.value);
        return 1;
    }
}

int AdvancedWaitHandler::handle_wait_spa_navigation(Browser& browser, const Command& cmd) {
    if (browser.waitForSPANavigation(cmd.value, cmd.timeout)) {
        Output::info("SPA navigation detected: " + cmd.value);
        return 0;
    } else {
        Output::error("SPA navigation timeout: " + cmd.value);
        return 1;
    }
}

int AdvancedWaitHandler::handle_wait_framework_ready(Browser& browser, const Command& cmd) {
    if (browser.waitForFrameworkReady(cmd.value, cmd.timeout)) {
        Output::info("Framework ready: " + cmd.value);
        return 0;
    } else {
        Output::error("Framework ready timeout: " + cmd.value);
        return 1;
    }
}

int AdvancedWaitHandler::handle_wait_dom_change(Browser& browser, const Command& cmd) {
    if (browser.waitForDOMChange(cmd.selector, cmd.timeout)) {
        Output::info("DOM change detected: " + cmd.selector);
        return 0;
    } else {
        Output::error("DOM change timeout: " + cmd.selector);
        return 1;
    }
}

int AdvancedWaitHandler::handle_wait_content_change(Browser& browser, const Command& cmd) {
    if (browser.waitForContentChange(cmd.selector, cmd.value, cmd.timeout)) {
        Output::info("Content change detected: " + cmd.selector + "." + cmd.value);
        return 0;
    } else {
        Output::error("Content change timeout: " + cmd.selector);
        return 1;
    }
}

} // namespace HWeb