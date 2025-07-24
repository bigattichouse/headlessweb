#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "Session/Session.h"
#include "../utils/test_helpers.h"
#include <filesystem>
#include <chrono>
#include <thread>

class BrowserEventsTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("browser_events_tests");
        session = std::make_unique<Session>("test_session");
        session->setCurrentUrl("about:blank");
        session->setViewport(1024, 768);
        
        createEventTestHtmlFile();
    }

    void TearDown() override {
        session.reset();
        temp_dir.reset();
    }

    void createEventTestHtmlFile() {
        std::string html_content = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Event Test Page</title>
    <script>
        var pageLoadComplete = false;
        var elementVisible = false;
        var textAppeared = false;
        
        document.addEventListener('DOMContentLoaded', function() {
            pageLoadComplete = true;
            console.log('Page loaded');
            
            // Simulate dynamic content loading
            setTimeout(function() {
                var dynamicElement = document.createElement('div');
                dynamicElement.id = 'dynamic-content';
                dynamicElement.innerHTML = 'Dynamic content appeared';
                dynamicElement.style.display = 'block';
                document.body.appendChild(dynamicElement);
                elementVisible = true;
                textAppeared = true;
            }, 500);
            
            // Simulate navigation after delay
            setTimeout(function() {
                window.dispatchEvent(new Event('customNavigation'));
            }, 1000);
        });
        
        function simulateSlowOperation() {
            return new Promise(function(resolve) {
                setTimeout(function() {
                    resolve('Operation completed');
                }, 2000);
            });
        }
        
        function checkCondition() {
            return pageLoadComplete && elementVisible;
        }
        
        function waitForElement() {
            return document.getElementById('dynamic-content') !== null;
        }
        
        function showElement(id) {
            var element = document.getElementById(id);
            if (element) {
                element.style.display = 'block';
                element.style.visibility = 'visible';
            }
        }
        
        function hideElement(id) {
            var element = document.getElementById(id);
            if (element) {
                element.style.display = 'none';
                element.style.visibility = 'hidden';
            }
        }
        
        function simulateUserInteraction() {
            var button = document.getElementById('test-button');
            if (button) {
                button.click();
                return true;
            }
            return false;
        }
    </script>
</head>
<body>
    <div id="main-content">
        <h1 id="title">Event Test Page</h1>
        <button id="test-button" onclick="buttonClicked()">Test Button</button>
        <div id="hidden-element" style="display: none;">Hidden Content</div>
        <div id="loading-indicator">Loading...</div>
        <ul id="item-list">
            <li class="item">Item 1</li>
            <li class="item">Item 2</li>
        </ul>
    </div>
    
    <script>
        function buttonClicked() {
            document.getElementById('loading-indicator').innerHTML = 'Button clicked!';
            
            // Add new item to list
            var newItem = document.createElement('li');
            newItem.className = 'item';
            newItem.innerHTML = 'Item ' + (document.querySelectorAll('.item').length + 1);
            document.getElementById('item-list').appendChild(newItem);
        }
        
        // Simulate network activity
        function simulateNetworkRequest() {
            return fetch('data:text/plain,simulated response')
                .then(function(response) {
                    return response.text();
                });
        }
        
        // Page ready state simulation
        function isPageFullyLoaded() {
            return document.readyState === 'complete' && 
                   pageLoadComplete && 
                   document.getElementById('main-content') !== null;
        }
    </script>
</body>
</html>
)HTML";
        event_test_file = temp_dir->createFile("events.html", html_content);
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::unique_ptr<Session> session;
    std::filesystem::path event_test_file;
};

// ========== Basic Event Waiting Tests ==========

TEST_F(BrowserEventsTest, NavigationEventWaiting) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    // Test navigation event waiting interface
    EXPECT_NO_THROW({
        browser.waitForNavigationEvent(1000);  // 1 second timeout
        browser.waitForNavigationEvent(5000);  // 5 second timeout
        browser.waitForNavigationEvent(100);   // Short timeout
    });
}

TEST_F(BrowserEventsTest, NavigationSignalWaiting) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    // Test navigation signal waiting
    EXPECT_NO_THROW({
        browser.waitForNavigationSignal(1000);
        browser.waitForNavigationSignal(3000);
        browser.waitForBackForwardNavigation(2000);
    });
}

TEST_F(BrowserEventsTest, PageReadyEventWaiting) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    // Test page ready event waiting
    EXPECT_NO_THROW({
        browser.waitForPageReadyEvent(2000);
        browser.waitForPageReady(*session);
    });
}

// ========== Selector-Based Event Tests ==========

TEST_F(BrowserEventsTest, SelectorEventWaiting) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    std::vector<std::string> test_selectors = {
        "#test-button",
        "#dynamic-content",
        ".item",
        "div",
        "#nonexistent-element"
    };
    
    for (const auto& selector : test_selectors) {
        EXPECT_NO_THROW({
            browser.waitForSelectorEvent(selector, 1000);
            browser.waitForSelector(selector, 1000);
        });
    }
}

TEST_F(BrowserEventsTest, VisibilityEventWaiting) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    std::vector<std::string> visibility_selectors = {
        "#hidden-element",
        "#test-button",
        "#loading-indicator",
        ".item",
        "#nonexistent"
    };
    
    for (const auto& selector : visibility_selectors) {
        EXPECT_NO_THROW({
            browser.waitForVisibilityEvent(selector, 1000);
        });
    }
}

TEST_F(BrowserEventsTest, ElementContentWaiting) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    std::vector<std::string> content_selectors = {
        "#title",
        "#loading-indicator", 
        ".item",
        "#main-content"
    };
    
    for (const auto& selector : content_selectors) {
        EXPECT_NO_THROW({
            browser.waitForElementWithContent(selector, 1000);
        });
    }
}

// ========== JavaScript Condition Waiting Tests ==========

TEST_F(BrowserEventsTest, JavaScriptConditionWaiting) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    std::vector<std::string> js_conditions = {
        "true",
        "false",
        "document.readyState === 'complete'",
        "pageLoadComplete",
        "elementVisible", 
        "checkCondition()",
        "waitForElement()",
        "isPageFullyLoaded()",
        "document.getElementById('test-button') !== null",
        "document.querySelectorAll('.item').length > 2"
    };
    
    for (const auto& condition : js_conditions) {
        EXPECT_NO_THROW({
            browser.waitForConditionEvent(condition, 1000);
            browser.waitForJsCondition(condition, 1000);
        });
    }
}

TEST_F(BrowserEventsTest, ComplexJavaScriptConditions) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    std::vector<std::string> complex_conditions = {
        "document.readyState === 'complete' && document.getElementById('main-content')",
        "document.querySelectorAll('.item').length >= 2",
        "window.pageLoadComplete === true",
        "typeof document.getElementById('test-button').click === 'function'",
        "document.title.length > 0",
        "window.location.href.indexOf('file://') === 0"
    };
    
    for (const auto& condition : complex_conditions) {
        EXPECT_NO_THROW({
            browser.waitForConditionEvent(condition, 1500);
        });
    }
}

// ========== Text-Based Waiting Tests ==========

TEST_F(BrowserEventsTest, TextAppearanceWaiting) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    std::vector<std::string> text_targets = {
        "Event Test Page",
        "Test Button",
        "Loading...",
        "Item 1",
        "Hidden Content",
        "Nonexistent text",
        "Dynamic content appeared"
    };
    
    for (const auto& text : text_targets) {
        EXPECT_NO_THROW({
            browser.waitForText(text, 1000);
        });
    }
}

TEST_F(BrowserEventsTest, UnicodeTextWaiting) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    std::vector<std::string> unicode_texts = {
        "测试文本",
        "العربية", 
        "Русский",
        "🎉🔧💻",
        "Ñiño José"
    };
    
    for (const auto& text : unicode_texts) {
        EXPECT_NO_THROW({
            browser.waitForText(text, 1000);
        });
    }
}

// ========== Timeout Handling Tests ==========

TEST_F(BrowserEventsTest, TimeoutVariations) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    std::vector<int> timeout_values = {
        1,      // Very short
        100,    // Short
        1000,   // Normal
        5000,   // Long
        10000,  // Very long
        0       // Zero timeout
    };
    
    for (int timeout : timeout_values) {
        EXPECT_NO_THROW({
            browser.waitForSelector("#test-button", timeout);
            browser.waitForNavigation(timeout);
            browser.waitForJsCondition("true", timeout);
        });
    }
}

TEST_F(BrowserEventsTest, NegativeTimeoutHandling) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    // Test handling of negative timeouts
    EXPECT_NO_THROW({
        browser.waitForSelector("#test-button", -1);
        browser.waitForNavigation(-100);
        browser.waitForJsCondition("true", -1000);
    });
}

// ========== Multiple Concurrent Events ==========

TEST_F(BrowserEventsTest, ConcurrentEventWaiting) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    // Test that multiple event waiting operations don't interfere
    EXPECT_NO_THROW({
        browser.waitForSelector("#test-button", 500);
        browser.waitForText("Test Button", 500);
        browser.waitForJsCondition("document.readyState === 'complete'", 500);
        browser.waitForVisibilityEvent("#test-button", 500);
    });
}

TEST_F(BrowserEventsTest, SequentialEventOperations) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    // Test sequential event operations
    EXPECT_NO_THROW({
        for (int i = 0; i < 10; ++i) {
            browser.waitForSelector("#test-button", 100);
            browser.waitForJsCondition("true", 100);
            browser.waitForText("Event Test Page", 100);
        }
    });
}

// ========== Event Notification Tests ==========

TEST_F(BrowserEventsTest, EventNotificationMethods) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    // Test event notification methods (should not crash)
    EXPECT_NO_THROW({
        browser.notifyNavigationComplete();
        browser.notifyUriChanged();
        browser.notifyTitleChanged();
        browser.notifyReadyToShow();
    });
}

TEST_F(BrowserEventsTest, RepeatedEventNotifications) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    // Test repeated event notifications
    EXPECT_NO_THROW({
        for (int i = 0; i < 5; ++i) {
            browser.notifyNavigationComplete();
            browser.notifyUriChanged();
            browser.notifyTitleChanged();
            browser.notifyReadyToShow();
        }
    });
}

// ========== Error Handling in Events ==========

TEST_F(BrowserEventsTest, InvalidSelectorEventHandling) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    std::vector<std::string> invalid_selectors = {
        "",
        "#",
        ".",
        "[invalid",
        ">>bad",
        std::string(1000, 'a') // Very long selector
    };
    
    for (const auto& selector : invalid_selectors) {
        EXPECT_NO_THROW({
            browser.waitForSelector(selector, 500);
            browser.waitForSelectorEvent(selector, 500);
            browser.waitForVisibilityEvent(selector, 500);
        });
    }
}

TEST_F(BrowserEventsTest, InvalidJavaScriptConditionHandling) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    std::vector<std::string> invalid_conditions = {
        "",
        "undefined.property",
        "invalid.syntax.",
        "nonexistentFunction()",
        "throw new Error('test')",
        std::string(1000, 'x') // Very long condition
    };
    
    for (const auto& condition : invalid_conditions) {
        EXPECT_NO_THROW({
            browser.waitForJsCondition(condition, 500);
            browser.waitForConditionEvent(condition, 500);
        });
    }
}

// ========== Performance and Timing Tests ==========

TEST_F(BrowserEventsTest, EventWaitingPerformance) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    auto start = std::chrono::steady_clock::now();
    
    // Test performance of multiple event operations
    EXPECT_NO_THROW({
        for (int i = 0; i < 20; ++i) {
            browser.waitForSelector("#test-button", 50);
            browser.waitForJsCondition("true", 50);
            browser.waitForText("Test", 50);
        }
    });
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete within reasonable time
    EXPECT_LT(duration.count(), 10000); // Less than 10 seconds for 60 operations
}

TEST_F(BrowserEventsTest, TimingAccuracy) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    // Test that timeouts are reasonably accurate
    std::vector<int> test_timeouts = {100, 500, 1000};
    
    for (int timeout : test_timeouts) {
        auto start = std::chrono::steady_clock::now();
        
        EXPECT_NO_THROW({
            browser.waitForSelector("#nonexistent-element", timeout);
        });
        
        auto end = std::chrono::steady_clock::now();
        auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Allow some variance in timing (within 50% of expected)
        EXPECT_GE(actual_duration.count(), timeout / 2);
        EXPECT_LE(actual_duration.count(), timeout * 2);
    }
}

// ========== Complex Event Scenarios ==========

TEST_F(BrowserEventsTest, ComplexEventScenarios) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    // Test complex event scenarios that might occur in real usage
    EXPECT_NO_THROW({
        // Wait for page ready
        browser.waitForPageReady(*session);
        
        // Wait for specific element
        browser.waitForSelector("#test-button", 1000);
        
        // Wait for text to appear
        browser.waitForText("Test Button", 1000);
        
        // Wait for JavaScript condition
        browser.waitForJsCondition("document.readyState === 'complete'", 1000);
        
        // Wait for element visibility
        browser.waitForVisibilityEvent("#test-button", 1000);
        
        // Wait for navigation (simulated)
        browser.waitForNavigation(1000);
    });
}

// ========== Edge Cases ==========

TEST_F(BrowserEventsTest, EdgeCaseEventHandling) {
    HWeb::HWebConfig test_config;
    Browser browser(test_config);
    
    // Test edge cases in event handling
    EXPECT_NO_THROW({
        // Empty text waiting
        browser.waitForText("", 100);
        
        // Very long text waiting
        browser.waitForText(std::string(1000, 'a'), 100);
        
        // Complex Unicode text
        browser.waitForText("测试🎉Русский", 100);
        
        // Null character in conditions
        std::string null_condition = "true";
        null_condition += '\0';
        null_condition += "false";
        browser.waitForJsCondition(null_condition, 100);
    });
}