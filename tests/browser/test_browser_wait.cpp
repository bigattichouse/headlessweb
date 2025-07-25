#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace std::chrono_literals;

class BrowserWaitTest : public ::testing::Test {
protected:
    void SetUp() override {
        HWeb::HWebConfig test_config;
        browser = std::make_unique<Browser>(test_config);
        
        // Create temporary directory for HTML test files
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("wait_tests");
        
        // Load a comprehensive test page for wait method testing
        setupTestPage();
        
        // Wait for page to be ready
        std::this_thread::sleep_for(800ms);
    }
    
    void setupTestPage() {
        std::string test_html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Wait Methods Test Page</title>
    <style>
        body { margin: 0; padding: 20px; font-family: Arial, sans-serif; }
        .test-section { margin: 20px 0; border: 1px solid #ccc; padding: 10px; }
        .hidden { display: none; }
        .visible { display: block; }
        .loading { opacity: 0.5; }
        .loaded { opacity: 1; }
        .dynamic-content { height: 100px; background: #f0f0f0; margin: 10px 0; }
        .status { font-weight: bold; padding: 5px; margin: 5px 0; }
        .success { background: #d4edda; color: #155724; }
        .pending { background: #fff3cd; color: #856404; }
        .error { background: #f8d7da; color: #721c24; }
        input, button, select { margin: 5px; padding: 8px; }
        .tall-content { height: 2000px; background: linear-gradient(to bottom, #e3f2fd, #1976d2); }
    </style>
</head>
<body>
    <h1>Advanced Wait Methods Test Page</h1>
    
    <!-- Text Content Testing -->
    <div class="test-section">
        <h2>Text Content Testing</h2>
        <div id="text-content">Initial text content</div>
        <div id="case-sensitive-text">CaseSensitive Text</div>
        <div id="exact-match-text">Exact Match Text</div>
        <button onclick="changeTextContent()">Change Text</button>
        <button onclick="addTextContent()">Add Text</button>
        <button onclick="removeTextContent()">Remove Text</button>
    </div>
    
    <!-- Network Activity Testing -->
    <div class="test-section">
        <h2>Network Activity Testing</h2>
        <div id="network-status">Network idle</div>
        <button onclick="simulateXHRRequest()">Simulate XHR</button>
        <button onclick="simulateFetchRequest()">Simulate Fetch</button>
        <button onclick="simulateMultipleRequests()">Multiple Requests</button>
        <button onclick="simulateNetworkRequest('/api/test')">Specific Request</button>
    </div>
    
    <!-- Element Visibility Testing -->
    <div class="test-section">
        <h2>Element Visibility Testing</h2>
        <div id="visibility-target" class="hidden">Hidden Element</div>
        <button onclick="showElement()">Show Element</button>
        <button onclick="hideElement()">Hide Element</button>
        <button onclick="toggleVisibility()">Toggle Visibility</button>
    </div>
    
    <!-- Element Count Testing -->
    <div class="test-section">
        <h2>Element Count Testing</h2>
        <div id="count-container">
            <div class="count-item">Item 1</div>
            <div class="count-item">Item 2</div>
        </div>
        <button onclick="addCountItem()">Add Item</button>
        <button onclick="removeCountItem()">Remove Item</button>
        <button onclick="clearCountItems()">Clear All</button>
    </div>
    
    <!-- Attribute Testing -->
    <div class="test-section">
        <h2>Attribute Testing</h2>
        <div id="attribute-target" data-status="initial" class="pending">Attribute Test Element</div>
        <button onclick="changeAttribute('ready')">Set Ready</button>
        <button onclick="changeAttribute('complete')">Set Complete</button>
        <button onclick="changeAttribute('error')">Set Error</button>
    </div>
    
    <!-- URL and Navigation Testing -->
    <div class="test-section">
        <h2>URL and Navigation Testing</h2>
        <div id="nav-info">Current: <span id="current-url"></span></div>
        <button onclick="changeHash('section1')">Change Hash #section1</button>
        <button onclick="changeHash('section2')">Change Hash #section2</button>
        <button onclick="simulateSPANavigation()">Simulate SPA Navigation</button>
    </div>
    
    <!-- Title Testing -->
    <div class="test-section">
        <h2>Title Testing</h2>
        <div id="title-info">Current title: <span id="current-title"></span></div>
        <button onclick="changeTitle('New Page Title')">Change Title</button>
        <button onclick="changeTitle('Dynamic Title')">Dynamic Title</button>
        <button onclick="restoreTitle()">Restore Title</button>
    </div>
    
    <!-- Framework Ready Testing -->
    <div class="test-section">
        <h2>Framework Ready Testing</h2>
        <div id="framework-status">No framework loaded</div>
        <button onclick="simulateJQuery()">Load jQuery</button>
        <button onclick="simulateReact()">Load React</button>
        <button onclick="setAppReady()">Set App Ready</button>
    </div>
    
    <!-- DOM Mutation Testing -->
    <div class="test-section">
        <h2>DOM Mutation Testing</h2>
        <div id="mutation-target">
            <p>Original content</p>
        </div>
        <button onclick="addElement()">Add Element</button>
        <button onclick="modifyElement()">Modify Element</button>
        <button onclick="removeElement()">Remove Element</button>
    </div>
    
    <!-- Content Change Testing -->
    <div class="test-section">
        <h2>Content Change Testing</h2>
        <div id="content-target" data-value="initial">Initial content value</div>
        <input id="content-input" value="initial input" />
        <button onclick="changeContent()">Change Content</button>
        <button onclick="changeInput()">Change Input</button>
        <button onclick="changeHTML()">Change HTML</button>
    </div>
    
    <!-- Loading and Status -->
    <div class="test-section">
        <h2>Loading States</h2>
        <div id="loading-indicator" class="status pending">Loading...</div>
        <div class="tall-content" style="height: 200px;"></div>
    </div>
    
    <script>
        let itemCounter = 2;
        let requestCounter = 0;
        
        // Text content functions
        function changeTextContent() {
            document.getElementById('text-content').textContent = 'Changed text content at ' + new Date().toLocaleTimeString();
        }
        
        function addTextContent() {
            const el = document.getElementById('text-content');
            el.textContent += ' - Additional text';
        }
        
        function removeTextContent() {
            document.getElementById('text-content').textContent = '';
        }
        
        // Network simulation functions
        function simulateXHRRequest() {
            updateNetworkStatus('XHR request starting...');
            const xhr = new XMLHttpRequest();
            xhr.open('GET', '/fake-endpoint-xhr');
            xhr.onload = () => updateNetworkStatus('XHR completed');
            xhr.onerror = () => updateNetworkStatus('XHR error');
            // Don't actually send to avoid real network calls
            setTimeout(() => updateNetworkStatus('XHR simulated'), 500);
        }
        
        function simulateFetchRequest() {
            updateNetworkStatus('Fetch request starting...');
            // Simulate fetch without actual network call
            setTimeout(() => updateNetworkStatus('Fetch simulated'), 300);
        }
        
        function simulateMultipleRequests() {
            updateNetworkStatus('Multiple requests starting...');
            setTimeout(() => simulateXHRRequest(), 100);
            setTimeout(() => simulateFetchRequest(), 200);
            setTimeout(() => updateNetworkStatus('All requests completed'), 800);
        }
        
        function simulateNetworkRequest(endpoint) {
            updateNetworkStatus('Request to ' + endpoint + ' starting...');
            requestCounter++;
            setTimeout(() => {
                updateNetworkStatus('Request to ' + endpoint + ' completed');
            }, 400);
        }
        
        function updateNetworkStatus(status) {
            document.getElementById('network-status').textContent = status;
        }
        
        // Visibility functions
        function showElement() {
            const el = document.getElementById('visibility-target');
            el.className = 'visible';
            el.style.display = 'block';
        }
        
        function hideElement() {
            const el = document.getElementById('visibility-target');
            el.className = 'hidden';
            el.style.display = 'none';
        }
        
        function toggleVisibility() {
            const el = document.getElementById('visibility-target');
            if (el.style.display === 'none') {
                showElement();
            } else {
                hideElement();
            }
        }
        
        // Count functions
        function addCountItem() {
            itemCounter++;
            const container = document.getElementById('count-container');
            const newItem = document.createElement('div');
            newItem.className = 'count-item';
            newItem.textContent = 'Item ' + itemCounter;
            container.appendChild(newItem);
        }
        
        function removeCountItem() {
            const items = document.querySelectorAll('.count-item');
            if (items.length > 0) {
                items[items.length - 1].remove();
            }
        }
        
        function clearCountItems() {
            const container = document.getElementById('count-container');
            container.innerHTML = '';
        }
        
        // Attribute functions
        function changeAttribute(status) {
            const el = document.getElementById('attribute-target');
            el.setAttribute('data-status', status);
            el.className = status === 'ready' ? 'success' : 
                          status === 'complete' ? 'success' : 
                          status === 'error' ? 'error' : 'pending';
            el.textContent = 'Status: ' + status;
        }
        
        // Navigation functions
        function changeHash(hash) {
            window.location.hash = hash;
            updateNavInfo();
        }
        
        function simulateSPANavigation() {
            // Simulate Single Page Application navigation
            window.history.pushState({}, '', '/app/dashboard');
            updateNavInfo();
        }
        
        function updateNavInfo() {
            document.getElementById('current-url').textContent = window.location.href;
        }
        
        // Title functions
        function changeTitle(newTitle) {
            document.title = newTitle;
            updateTitleInfo();
        }
        
        function restoreTitle() {
            document.title = 'Wait Methods Test Page';
            updateTitleInfo();
        }
        
        function updateTitleInfo() {
            document.getElementById('current-title').textContent = document.title;
        }
        
        // Framework simulation
        function simulateJQuery() {
            window.jQuery = { isReady: true };
            document.getElementById('framework-status').textContent = 'jQuery loaded';
        }
        
        function simulateReact() {
            window.React = { version: '18.0.0' };
            document.getElementById('framework-status').textContent = 'React loaded';
        }
        
        function setAppReady() {
            window.APP_READY = true;
            document.getElementById('framework-status').textContent = 'App ready';
        }
        
        // DOM mutation functions
        function addElement() {
            const container = document.getElementById('mutation-target');
            const newEl = document.createElement('div');
            newEl.textContent = 'Added element at ' + new Date().toLocaleTimeString();
            container.appendChild(newEl);
        }
        
        function modifyElement() {
            const target = document.querySelector('#mutation-target p');
            if (target) {
                target.textContent = 'Modified content at ' + new Date().toLocaleTimeString();
            }
        }
        
        function removeElement() {
            const target = document.querySelector('#mutation-target div:last-child');
            if (target) {
                target.remove();
            }
        }
        
        // Content change functions
        function changeContent() {
            const el = document.getElementById('content-target');
            el.textContent = 'Changed content at ' + new Date().toLocaleTimeString();
            el.setAttribute('data-value', 'changed');
        }
        
        function changeInput() {
            const input = document.getElementById('content-input');
            input.value = 'Changed input at ' + new Date().toLocaleTimeString();
        }
        
        function changeHTML() {
            const el = document.getElementById('content-target');
            el.innerHTML = '<strong>Changed HTML</strong> at ' + new Date().toLocaleTimeString();
        }
        
        // Initialize page
        function initializePage() {
            updateNavInfo();
            updateTitleInfo();
            document.getElementById('loading-indicator').textContent = 'Page ready';
            document.getElementById('loading-indicator').className = 'status success';
        }
        
        // Run initialization after page loads
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', initializePage);
        } else {
            initializePage();
        }
        
        // Simulate progressive loading
        setTimeout(() => {
            const indicator = document.getElementById('loading-indicator');
            indicator.textContent = 'Content loaded';
            indicator.className = 'status success';
        }, 1000);
    </script>
</body>
</html>
)HTML";
        
        // CRITICAL FIX: Create HTML file and use file:// URL to avoid data: URL restrictions
        test_html_file = temp_dir->createFile("wait_test.html", test_html);
        std::string file_url = "file://" + test_html_file.string();
        browser->loadUri(file_url);
        
        // Wait for page load
        std::this_thread::sleep_for(1200ms);
    }

    void TearDown() override {
        browser.reset();
        temp_dir.reset();
    }

    std::unique_ptr<Browser> browser;
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::filesystem::path test_html_file;
};

// ========== Text Advanced Wait Tests ==========

TEST_F(BrowserWaitTest, WaitForTextAdvancedBasic) {
    // Wait for initial text that should already be present
    bool result = browser->waitForTextAdvanced("Initial text content", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForTextAdvancedCaseSensitive) {
    // Test case sensitive matching
    bool result = browser->waitForTextAdvanced("CaseSensitive Text", 2000, true, false);
    EXPECT_TRUE(result);
    
    // This should fail with case sensitive
    result = browser->waitForTextAdvanced("casesensitive text", 1000, true, false);
    EXPECT_FALSE(result);
}

TEST_F(BrowserWaitTest, WaitForTextAdvancedCaseInsensitive) {
    // Test case insensitive matching
    bool result = browser->waitForTextAdvanced("casesensitive text", 2000, false, false);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForTextAdvancedExactMatch) {
    // Test exact match
    bool result = browser->waitForTextAdvanced("Exact Match Text", 2000, false, true);
    EXPECT_TRUE(result);
    
    // Partial text should fail with exact match
    result = browser->waitForTextAdvanced("Exact", 1000, false, true);
    EXPECT_FALSE(result);
}

TEST_F(BrowserWaitTest, WaitForTextAdvancedDynamic) {
    // Change text and wait for it
    browser->executeJavascriptSync("setTimeout(() => changeTextContent(), 500);");
    
    bool result = browser->waitForTextAdvanced("Changed text content", 2000);
    EXPECT_TRUE(result);
}

// ========== Network Idle Tests ==========

TEST_F(BrowserWaitTest, WaitForNetworkIdleImmediate) {
    // Should be idle immediately
    bool result = browser->waitForNetworkIdle(100, 1000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForNetworkIdleAfterActivity) {
    // Simulate network activity then wait for idle
    browser->executeJavascriptSync("setTimeout(() => simulateXHRRequest(), 200);");
    
    bool result = browser->waitForNetworkIdle(300, 2000);
    EXPECT_TRUE(result);
}

// ========== Network Request Tests ==========

TEST_F(BrowserWaitTest, WaitForNetworkRequestPattern) {
    // Trigger a specific network request
    browser->executeJavascriptSync("setTimeout(() => simulateNetworkRequest('/api/test'), 300);");
    
    bool result = browser->waitForNetworkRequest("/api/test", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForNetworkRequestTimeout) {
    // Wait for a request that won't happen
    bool result = browser->waitForNetworkRequest("/nonexistent", 500);
    EXPECT_FALSE(result);
}

// ========== Element Visibility Tests ==========

TEST_F(BrowserWaitTest, WaitForElementVisibleInitiallyHidden) {
    // Element should be hidden initially
    bool visible_initially = browser->waitForElementVisible("#visibility-target", 500);
    EXPECT_FALSE(visible_initially);
    
    // Show the element and wait for visibility
    browser->executeJavascriptSync("setTimeout(() => showElement(), 300);");
    
    bool result = browser->waitForElementVisible("#visibility-target", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForElementVisibleAlreadyVisible) {
    // Show element first
    browser->executeJavascriptSync("showElement();");
    std::this_thread::sleep_for(100ms);
    
    bool result = browser->waitForElementVisible("#visibility-target", 1000);
    EXPECT_TRUE(result);
}

// ========== Element Count Tests ==========

TEST_F(BrowserWaitTest, WaitForElementCountEqual) {
    // Should have 2 count-item elements initially
    bool result = browser->waitForElementCount(".count-item", "==", 2, 1000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForElementCountGreaterThan) {
    // Add an item and wait for count > 2
    browser->executeJavascriptSync("setTimeout(() => addCountItem(), 300);");
    
    bool result = browser->waitForElementCount(".count-item", ">", 2, 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForElementCountLessThan) {
    // Remove an item and wait for count < 2
    browser->executeJavascriptSync("setTimeout(() => removeCountItem(), 300);");
    
    bool result = browser->waitForElementCount(".count-item", "<", 2, 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForElementCountNotEqual) {
    // Change count and wait for != 2
    browser->executeJavascriptSync("setTimeout(() => addCountItem(), 300);");
    
    bool result = browser->waitForElementCount(".count-item", "!=", 2, 2000);
    EXPECT_TRUE(result);
}

// ========== Attribute Tests ==========

TEST_F(BrowserWaitTest, WaitForAttributeChange) {
    // Wait for attribute change
    browser->executeJavascriptSync("setTimeout(() => changeAttribute('ready'), 400);");
    
    bool result = browser->waitForAttribute("#attribute-target", "data-status", "ready", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForAttributeExisting) {
    // Should already have initial attribute
    bool result = browser->waitForAttribute("#attribute-target", "data-status", "initial", 1000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForAttributeNonexistent) {
    // Wait for attribute that won't be set
    bool result = browser->waitForAttribute("#attribute-target", "data-status", "nonexistent", 500);
    EXPECT_FALSE(result);
}

// ========== URL Change Tests ==========

TEST_F(BrowserWaitTest, WaitForUrlChangeHash) {
    // Change hash and wait for URL change
    browser->executeJavascriptSync("setTimeout(() => changeHash('section1'), 300);");
    
    bool result = browser->waitForUrlChange("section1", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForUrlChangeAny) {
    // Change URL and wait for any change
    browser->executeJavascriptSync("setTimeout(() => changeHash('newsection'), 300);");
    
    bool result = browser->waitForUrlChange("", 2000);
    EXPECT_TRUE(result);
}

// ========== Title Change Tests ==========

TEST_F(BrowserWaitTest, WaitForTitleChange) {
    // Change title and wait for it
    browser->executeJavascriptSync("setTimeout(() => changeTitle('New Dynamic Title'), 400);");
    
    bool result = browser->waitForTitleChange("Dynamic", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForTitleChangeAny) {
    // Change title and wait for any change
    browser->executeJavascriptSync("setTimeout(() => changeTitle('Any New Title'), 300);");
    
    bool result = browser->waitForTitleChange("", 2000);
    EXPECT_TRUE(result);
}

// ========== SPA Navigation Tests ==========

TEST_F(BrowserWaitTest, WaitForSPANavigationSpecific) {
    // Simulate SPA navigation to dashboard
    browser->executeJavascriptSync("setTimeout(() => simulateSPANavigation(), 300);");
    
    bool result = browser->waitForSPANavigation("dashboard", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForSPANavigationAny) {
    // Wait for any navigation change
    browser->executeJavascriptSync("setTimeout(() => changeHash('spa-test'), 300);");
    
    bool result = browser->waitForSPANavigation("", 2000);
    EXPECT_TRUE(result);
}

// ========== Framework Ready Tests ==========

TEST_F(BrowserWaitTest, WaitForFrameworkReadyJQuery) {
    // Simulate jQuery loading
    browser->executeJavascriptSync("setTimeout(() => simulateJQuery(), 300);");
    
    bool result = browser->waitForFrameworkReady("jquery", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForFrameworkReadyReact) {
    // Simulate React loading
    browser->executeJavascriptSync("setTimeout(() => simulateReact(), 300);");
    
    bool result = browser->waitForFrameworkReady("react", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForFrameworkReadyAuto) {
    // Should already be ready (document.readyState === 'complete')
    bool result = browser->waitForFrameworkReady("auto", 1000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForFrameworkReadyCustom) {
    // Set custom app ready flag
    browser->executeJavascriptSync("setTimeout(() => setAppReady(), 300);");
    
    bool result = browser->waitForFrameworkReady("auto", 2000);
    EXPECT_TRUE(result);
}

// ========== DOM Change Tests ==========

TEST_F(BrowserWaitTest, WaitForDOMChangeAddElement) {
    // Add element and wait for DOM change
    browser->executeJavascriptSync("setTimeout(() => addElement(), 400);");
    
    bool result = browser->waitForDOMChange("#mutation-target", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForDOMChangeModifyElement) {
    // Modify element and wait for DOM change
    browser->executeJavascriptSync("setTimeout(() => modifyElement(), 300);");
    
    bool result = browser->waitForDOMChange("#mutation-target", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForDOMChangeRemoveElement) {
    // First add an element, then remove it
    browser->executeJavascriptSync("addElement();");
    std::this_thread::sleep_for(100ms);
    
    browser->executeJavascriptSync("setTimeout(() => removeElement(), 300);");
    
    bool result = browser->waitForDOMChange("#mutation-target", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForDOMChangeTimeout) {
    // Wait for DOM change that won't happen
    bool result = browser->waitForDOMChange("#nonexistent-element", 500);
    EXPECT_FALSE(result);
}

// ========== Content Change Tests ==========

TEST_F(BrowserWaitTest, WaitForContentChangeText) {
    // Change text content
    browser->executeJavascriptSync("setTimeout(() => changeContent(), 400);");
    
    bool result = browser->waitForContentChange("#content-target", "text", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForContentChangeHTML) {
    // Change HTML content
    browser->executeJavascriptSync("setTimeout(() => changeHTML(), 300);");
    
    bool result = browser->waitForContentChange("#content-target", "html", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForContentChangeInputValue) {
    // Change input value
    browser->executeJavascriptSync("setTimeout(() => changeInput(), 300);");
    
    bool result = browser->waitForContentChange("#content-input", "value", 2000);
    EXPECT_TRUE(result);
}

TEST_F(BrowserWaitTest, WaitForContentChangeTimeout) {
    // Wait for content change that won't happen
    bool result = browser->waitForContentChange("#content-target", "text", 500);
    EXPECT_FALSE(result);
}

// ========== Edge Cases and Error Handling ==========

TEST_F(BrowserWaitTest, WaitMethodsWithNonexistentElements) {
    // Test various wait methods with selectors that don't exist
    EXPECT_FALSE(browser->waitForElementVisible("#nonexistent", 300));
    EXPECT_FALSE(browser->waitForAttribute("#nonexistent", "data-test", "value", 300));
    EXPECT_FALSE(browser->waitForDOMChange("#nonexistent", 300));
    EXPECT_FALSE(browser->waitForContentChange("#nonexistent", "text", 300));
}

TEST_F(BrowserWaitTest, WaitMethodsWithZeroTimeout) {
    // Test wait methods with minimal timeout
    // These should return quickly (false for most cases since changes need time)
    EXPECT_FALSE(browser->waitForTextAdvanced("nonexistent text", 0));
    EXPECT_FALSE(browser->waitForElementCount(".nonexistent", "==", 1, 0));
}

TEST_F(BrowserWaitTest, WaitMethodsIntegration) {
    // Test multiple wait conditions in sequence
    
    // 1. Wait for initial state
    bool initial = browser->waitForElementCount(".count-item", "==", 2, 1000);
    EXPECT_TRUE(initial);
    
    // 2. Change state and wait for new condition  
    browser->executeJavascriptSync("setTimeout(() => { addCountItem(); changeAttribute('ready'); }, 300);");
    
    bool count_changed = browser->waitForElementCount(".count-item", ">", 2, 2000);
    EXPECT_TRUE(count_changed);
    
    bool attribute_changed = browser->waitForAttribute("#attribute-target", "data-status", "ready", 2000);
    EXPECT_TRUE(attribute_changed);
    
    // 3. Verify final state
    std::string count_result = browser->executeJavascriptSync("document.querySelectorAll('.count-item').length");
    EXPECT_EQ(count_result, "3");
}