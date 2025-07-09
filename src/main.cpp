#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h> // For sleep
#include <glib.h> // For g_main_context_iteration
#include "SessionManager.h"
#include "Browser.h"

void print_usage() {
    std::cerr << "Usage: hweb-poc --session <name> --url <url>" << std::endl;
    std::cerr << "       hweb-poc --session <name> --js <javascript>" << std::endl;
    std::cerr << "       hweb-poc --session <name> --end" << std::endl;
}

// Helper function to process GTK events until operation is completed or timeout
void wait_for_completion(Browser& browser, int timeout_ms) {
    int elapsed_time = 0;
    while (!browser.isOperationCompleted() && elapsed_time < timeout_ms) {
        g_main_context_iteration(g_main_context_default(), FALSE);
        g_usleep(10 * 1000); // Sleep for 10ms
        elapsed_time += 10;
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string sessionName, url, js;
    bool endSession = false;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--session" && i + 1 < args.size()) {
            sessionName = args[++i];
        } else if (args[i] == "--url" && i + 1 < args.size()) {
            url = args[++i];
        } else if (args[i] == "--js" && i + 1 < args.size()) {
            js = args[++i];
        } else if (args[i] == "--end") {
            endSession = true;
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

    if (!url.empty()) {
        browser.loadUri(url);
        wait_for_completion(browser, 10000); // Wait up to 10 seconds for page load
        session.setUrl(url);
        session.setCookies(browser.getCookies());
        sessionManager.saveSession(session);
        std::cout << "Navigated to " << url << " and saved session '" << sessionName << "'." << std::endl;
    } else if (!js.empty()) {
        if (!session.getUrl().empty()) {
            browser.loadUri(session.getUrl());
            wait_for_completion(browser, 10000); // Wait up to 10 seconds for page load
        }
        
        // Wait for the H1 element to appear
        std::cout << "Waiting for H1 element..." << std::endl;
        if (!browser.waitForSelector("h1", 10000)) { // Wait up to 10 seconds for H1
            std::cerr << "Error: H1 element not found within timeout." << std::endl;
            return 1;
        }
        
        // Get H1 text
        std::string h1_text = browser.getInnerText("h1");
        std::cout << "H1 text: " << h1_text << std::endl;

        // Execute JavaScript
        std::string js_result_str;
        browser.executeJavascript(js, &js_result_str);
        wait_for_completion(browser, 5000); // Wait up to 5 seconds for JS execution
        
        // If the JavaScript might cause navigation (like form submission), wait for it
        if (js.find(".click()") != std::string::npos || 
            js.find(".submit()") != std::string::npos ||
            js.find("window.location") != std::string::npos ||
            js.find("location.href") != std::string::npos) {
            std::cout << "Detected navigation JavaScript, waiting for page load..." << std::endl;
            
            // First, wait a bit for the form submission to initiate
            g_usleep(1000 * 1000); // Wait 1 second
            
            // Wait for navigation to complete - using the load-changed signal
            wait_for_completion(browser, 15000); // Wait up to 15 seconds for navigation
            
            // Give extra time for any redirects or dynamic loading
            g_usleep(3000 * 1000); // Wait 3 more seconds
            
            // Get the current URL after navigation
            std::string current_url = "";
            browser.executeJavascript("window.location.href;", &current_url);
            wait_for_completion(browser, 5000);
            
            // Also check the page title to see if we're on a different page
            std::string current_title = "";
            browser.executeJavascript("document.title;", &current_title);
            wait_for_completion(browser, 5000);
            
            std::cout << "Navigation completed. New URL: " << current_url << std::endl;
            std::cout << "Page title: " << current_title << std::endl;
            
            // Update session with new URL (even if it's the same, might have query params)
            session.setUrl(current_url);
        }
        
        session.setCookies(browser.getCookies());
        sessionManager.saveSession(session);
        std::cout << "Executed JavaScript in session '" << sessionName << "'. Result: " << js_result_str << std::endl;
    } else {
        print_usage();
        return 1;
    }

    return 0;
}
