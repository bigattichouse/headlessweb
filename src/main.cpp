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
    std::cerr << "  --session <n>        Use named session" << std::endl;
    std::cerr << "  --url <url>            Navigate to URL" << std::endl;
    std::cerr << "  --end                  End session" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Commands (can be chained):" << std::endl;
    std::cerr << "  --type <selector> <text>     Type text into element" << std::endl;
    std::cerr << "  --click <selector>           Click element" << std::endl;
    std::cerr << "  --submit [form-selector]     Submit form" << std::endl;
    std::cerr << "  --wait <selector>            Wait for element" << std::endl;
    std::cerr << "  --wait-nav                   Wait for navigation" << std::endl;
    std::cerr << "  --text <selector>            Get element text" << std::endl;
    std::cerr << "  --search <query>             Smart form search" << std::endl;
    std::cerr << "  --js <code>                  Execute JavaScript" << std::endl;
}

void wait_for_completion(Browser& browser, int timeout_ms) {
    int elapsed_time = 0;
    while (!browser.isOperationCompleted() && elapsed_time < timeout_ms) {
        g_main_context_iteration(g_main_context_default(), FALSE);
        g_usleep(10 * 1000);
        elapsed_time += 10;
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string sessionName, url;
    bool endSession = false;
    std::vector<Command> commands;

    // Parse arguments
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--session" && i + 1 < args.size()) {
            sessionName = args[++i];
        } else if (args[i] == "--url" && i + 1 < args.size()) {
            url = args[++i];
        } else if (args[i] == "--end") {
            endSession = true;
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
        } else if (args[i] == "--text" && i + 1 < args.size()) {
            commands.push_back({"text", args[++i], ""});
        } else if (args[i] == "--search" && i + 1 < args.size()) {
            commands.push_back({"search", "", args[++i]});
        } else if (args[i] == "--js" && i + 1 < args.size()) {
            commands.push_back({"js", "", args[++i]});
        }
    }

    if (sessionName.empty()) {
        print_usage();
        return 1;
    }

    std::string home = std::getenv("HOME");
    SessionManager sessionManager(home + "/.hweb-poc/sessions");

    if (endSession) {
        sessionManager.deleteSession(sessionName);
        std::cout << "Session '" << sessionName << "' ended." << std::endl;
        return 0;
    }

    Session session = sessionManager.loadOrCreateSession(sessionName);
    Browser browser;
    browser.setCookies(session.getCookies());

    // Navigate if URL provided
    if (!url.empty()) {
        browser.loadUri(url);
        wait_for_completion(browser, 10000);
        session.setUrl(url);
        session.setCookies(browser.getCookies());
        sessionManager.saveSession(session);
        std::cout << "Navigated to " << url << std::endl;
    } else if (!session.getUrl().empty()) {
        // Load session URL
        browser.loadUri(session.getUrl());
        wait_for_completion(browser, 10000);
    }

    // Execute commands in sequence
    for (const auto& cmd : commands) {
        bool navigation_expected = false;
        
        if (cmd.type == "type") {
            if (browser.fillInput(cmd.selector, cmd.value)) {
                std::cout << "Typed '" << cmd.value << "' into " << cmd.selector << std::endl;
            } else {
                std::cerr << "Failed to type into " << cmd.selector << std::endl;
            }
        } else if (cmd.type == "click") {
            if (browser.clickElement(cmd.selector)) {
                std::cout << "Clicked " << cmd.selector << std::endl;
                navigation_expected = true;
            } else {
                std::cerr << "Failed to click " << cmd.selector << std::endl;
            }
        } else if (cmd.type == "submit") {
            if (browser.submitForm(cmd.selector)) {
                std::cout << "Submitted form " << cmd.selector << std::endl;
                navigation_expected = true;
            } else {
                std::cerr << "Failed to submit form " << cmd.selector << std::endl;
            }
        } else if (cmd.type == "wait") {
            if (browser.waitForSelector(cmd.selector, cmd.timeout)) {
                std::cout << "Element " << cmd.selector << " appeared" << std::endl;
            } else {
                std::cerr << "Element " << cmd.selector << " not found within timeout" << std::endl;
            }
        } else if (cmd.type == "wait-nav") {
            if (browser.waitForNavigation(cmd.timeout)) {
                std::cout << "Navigation completed" << std::endl;
            } else {
                std::cout << "Navigation timeout or no navigation detected" << std::endl;
            }
        } else if (cmd.type == "text") {
            std::string text = browser.getInnerText(cmd.selector);
            std::cout << "Text from " << cmd.selector << ": " << text << std::endl;
        } else if (cmd.type == "search") {
            if (browser.searchForm(cmd.value)) {
                std::cout << "Search submitted: " << cmd.value << std::endl;
                navigation_expected = true;
            } else {
                std::cerr << "Failed to submit search" << std::endl;
            }
        } else if (cmd.type == "js") {
            std::string result;
            browser.executeJavascript(cmd.value, &result);
            wait_for_completion(browser, 5000);
            std::cout << "JavaScript result: " << result << std::endl;
        }
        
        // Handle navigation if expected
        if (navigation_expected) {
            std::string new_url = browser.getCurrentUrl();
            if (new_url != session.getUrl()) {
                session.setUrl(new_url);
                std::cout << "Navigation detected: " << new_url << std::endl;
            }
        }
    }

    // Save session state
    session.setCookies(browser.getCookies());
    std::string final_url = browser.getCurrentUrl();
    session.setUrl(final_url);
    sessionManager.saveSession(session);

    return 0;
}
