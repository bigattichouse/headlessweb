#include "src/Browser/Browser.h"
#include "src/Session/Manager.h"
#include <iostream>
#include <filesystem>

int main() {
    try {
        // Initialize browser with configuration
        HWeb::HWebConfig config;
        config.headless = false; // Set to true for headless mode
        config.allow_external_urls = true;
        
        Browser browser(config);
        
        std::cout << "=== HeadlessWeb Google Search Demo ===" << std::endl;
        std::cout << "Searching for: LLM wiki" << std::endl;
        
        // Step 1: Navigate to Google
        std::cout << "\n1. Navigating to Google..." << std::endl;
        browser.loadUri("https://www.google.com");
        browser.waitForNavigation(5000);
        browser.waitForJavaScriptCompletion(2000);
        
        // Step 2: Handle cookie consent if present
        if (browser.elementExists("button[id*='accept'], button[id*='Accept']")) {
            std::cout << "2. Accepting cookies..." << std::endl;
            browser.clickElement("button[id*='accept'], button[id*='Accept']");
            browser.waitForJavaScriptCompletion(1000);
        }
        
        // Step 3: Perform search
        std::cout << "3. Performing search..." << std::endl;
        browser.waitForSelector("input[name='q']", 3000);
        browser.fillInput("input[name='q']", "LLM wiki");
        
        // Take screenshot of search input
        browser.takeScreenshot("search_input.png");
        std::cout << "   Screenshot saved: search_input.png" << std::endl;
        
        // Submit search
        browser.clickElement("input[name='btnK'], button[name='btnK']");
        browser.waitForNavigation(5000);
        browser.waitForElement("h3", 3000);
        
        // Step 4: Take screenshot of search results
        std::cout << "4. Taking screenshot of search results..." << std::endl;
        browser.takeScreenshot("llm_search_results.png");
        std::cout << "   Screenshot saved: llm_search_results.png" << std::endl;
        
        // Step 5: Get search result details
        std::cout << "5. Extracting search result details..." << std::endl;
        
        if (browser.elementExists("h3")) {
            std::string first_result_title = browser.getInnerText("h3");
            std::cout << "   First result title: " << first_result_title << std::endl;
            
            // Click on first result
            browser.clickElement("h3 a, h3");
            browser.waitForNavigation(8000);
            browser.waitForJavaScriptCompletion(3000);
            
            // Step 6: Take screenshot of the result page
            std::cout << "6. Taking screenshot of result page..." << std::endl;
            browser.takeScreenshot("llm_wiki_page.png");
            std::cout << "   Screenshot saved: llm_wiki_page.png" << std::endl;
            
            // Get page details
            std::string page_title = browser.getPageTitle();
            std::string page_url = browser.getCurrentUrl();
            
            std::cout << "\n=== RESULTS ===" << std::endl;
            std::cout << "Page title: " << page_title << std::endl;
            std::cout << "Current URL: " << page_url << std::endl;
            std::cout << "Screenshots taken:" << std::endl;
            std::cout << "  - search_input.png" << std::endl;
            std::cout << "  - llm_search_results.png" << std::endl;
            std::cout << "  - llm_wiki_page.png" << std::endl;
            
        } else {
            std::cout << "   No search results found!" << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}