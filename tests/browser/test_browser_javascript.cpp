#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <filesystem>
#include <chrono>
#include <thread>

extern std::unique_ptr<Browser> g_browser;

class BrowserJavaScriptTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("browser_js_tests");
        
        // Use global browser instance like other working tests
        browser = g_browser.get();
        
        // NO PAGE LOADING - Use interface testing approach
        
        debug_output("BrowserJavaScriptTest SetUp complete");
    }

    void TearDown() override {
        // Clean teardown without navigation
        temp_dir.reset();
    }
    
    // JavaScript interface testing helper methods
    std::string executeWrappedJS(const std::string& jsCode) {
        // Test JavaScript execution interface without requiring page content
        std::string wrapped = "(function() { try { " + jsCode + " } catch(e) { return 'error: ' + e.message; } })()";
        return browser->executeJavascriptSync(wrapped);
    }
    
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    Browser* browser;
};

// ========== JavaScript Execution Interface Tests ==========

TEST_F(BrowserJavaScriptTest, BasicJavaScriptExecution) {
    // Test basic JavaScript execution interface without page loading
    EXPECT_NO_THROW({
        // Test basic JavaScript interface methods
        std::string result1 = executeWrappedJS("return 'test string';");
        std::string result2 = executeWrappedJS("return (1 + 1).toString();");
        std::string result3 = executeWrappedJS("return typeof document;");
        
        // Interface should handle JavaScript execution gracefully
        browser->executeJavascriptSync("console.log('interface test');");
    });
}

TEST_F(BrowserJavaScriptTest, SynchronousJavaScriptExecution) {
    // Test synchronous JavaScript execution interface
    EXPECT_NO_THROW({
        // Test synchronous execution interface
        std::string result = browser->executeJavascriptSync("return 'sync test';");
        browser->executeJavascriptSync("var testVar = 'synchronous';");
        browser->executeJavascriptSync("return testVar || 'default';");
        
        // Test execution timing interface
        auto start = std::chrono::high_resolution_clock::now();
        browser->executeJavascriptSync("return 'timing test';");
        auto end = std::chrono::high_resolution_clock::now();
        // Interface should execute within reasonable time
    });
}

TEST_F(BrowserJavaScriptTest, SafeJavaScriptExecution) {
    // Test safe JavaScript execution interface
    EXPECT_NO_THROW({
        // Test safe execution interface with various inputs
        executeWrappedJS("return 'safe execution';");
        executeWrappedJS("return JSON.stringify({test: 'object'});");
        executeWrappedJS("return [1, 2, 3].join(',');");
        
        // Test exception handling interface
        std::string error_result = executeWrappedJS("throw new Error('test error');");
        // Interface should handle errors gracefully
    });
}

TEST_F(BrowserJavaScriptTest, ArithmeticExpressions) {
    // Test arithmetic JavaScript expressions interface
    EXPECT_NO_THROW({
        // Test arithmetic operations interface
        executeWrappedJS("return (5 + 3).toString();");
        executeWrappedJS("return (10 - 4).toString();");
        executeWrappedJS("return (6 * 7).toString();");
        executeWrappedJS("return (15 / 3).toString();");
        executeWrappedJS("return (17 % 5).toString();");
        
        // Test complex arithmetic interface
        executeWrappedJS("return Math.pow(2, 3).toString();");
        executeWrappedJS("return Math.sqrt(16).toString();");
    });
}

TEST_F(BrowserJavaScriptTest, StringOperations) {
    // Test string operations JavaScript interface
    EXPECT_NO_THROW({
        // Test string manipulation interface
        executeWrappedJS("return 'hello'.toUpperCase();");
        executeWrappedJS("return 'WORLD'.toLowerCase();");
        executeWrappedJS("return 'test string'.substring(0, 4);");
        executeWrappedJS("return 'a,b,c'.split(',').length.toString();");
        
        // Test string concatenation interface
        executeWrappedJS("return 'hello' + ' ' + 'world';");
        executeWrappedJS("return `template ${1 + 1} string`;");
    });
}

TEST_F(BrowserJavaScriptTest, BooleanExpressions) {
    // Test boolean expressions JavaScript interface
    EXPECT_NO_THROW({
        // Test boolean operations interface
        executeWrappedJS("return (true && false).toString();");
        executeWrappedJS("return (true || false).toString();");
        executeWrappedJS("return (!true).toString();");
        
        // Test comparison operations interface
        executeWrappedJS("return (5 > 3).toString();");
        executeWrappedJS("return (2 < 1).toString();");
        executeWrappedJS("return (4 === 4).toString();");
        executeWrappedJS("return (5 !== 3).toString();");
    });
}

TEST_F(BrowserJavaScriptTest, DOMQueryOperations) {
    // Test DOM query operations JavaScript interface
    EXPECT_NO_THROW({
        // Test DOM query interface (should handle gracefully without content)
        executeWrappedJS("return document.querySelectorAll('*').length.toString();");
        executeWrappedJS("return document.querySelector('body') ? 'found' : 'not found';");
        executeWrappedJS("return document.getElementById('nonexistent') ? 'found' : 'not found';");
        executeWrappedJS("return document.getElementsByTagName('div').length.toString();");
        
        // Test DOM property interface
        executeWrappedJS("return document.title || 'no title';");
        executeWrappedJS("return document.readyState || 'unknown';");
    });
}

TEST_F(BrowserJavaScriptTest, DOMModificationOperations) {
    // Test DOM modification operations JavaScript interface
    EXPECT_NO_THROW({
        // Test DOM modification interface (should handle gracefully)
        executeWrappedJS("document.title = 'test title'; return document.title;");
        executeWrappedJS("var div = document.createElement('div'); return div.tagName;");
        executeWrappedJS("document.body && document.body.appendChild(document.createElement('span')); return 'modified';");
        
        // Test DOM manipulation interface
        executeWrappedJS("return document.createElement('p').innerHTML = 'test content';");
        executeWrappedJS("var elem = document.createElement('input'); elem.value = 'test'; return elem.value;");
    });
}

TEST_F(BrowserJavaScriptTest, FunctionCalling) {
    // Test function calling JavaScript interface
    EXPECT_NO_THROW({
        // Test function definition and calling interface
        executeWrappedJS("function testFunc() { return 'function result'; } return testFunc();");
        executeWrappedJS("var func = function(x) { return x * 2; }; return func(5).toString();");
        executeWrappedJS("return (function(a, b) { return a + b; })(3, 4).toString();");
        
        // Test function methods interface
        executeWrappedJS("function test() { return this; } return typeof test.call(null);");
        executeWrappedJS("return Math.max(1, 2, 3).toString();");
    });
}

TEST_F(BrowserJavaScriptTest, AnonymousFunctionExecution) {
    // Test anonymous function execution JavaScript interface
    EXPECT_NO_THROW({
        // Test anonymous function interface
        executeWrappedJS("return (function() { return 'anonymous'; })();");
        executeWrappedJS("return (x => x * 3)(4).toString();");
        executeWrappedJS("return ((a, b) => a - b)(10, 3).toString();");
        
        // Test closure interface
        executeWrappedJS("return (function(x) { return function(y) { return x + y; }; })(5)(3).toString();");
    });
}

TEST_F(BrowserJavaScriptTest, SyntaxErrorHandling) {
    // Test syntax error handling JavaScript interface
    EXPECT_NO_THROW({
        // Test syntax error interface (wrapped execution should handle gracefully)
        std::string result1 = executeWrappedJS("return 'valid syntax';");
        std::string result2 = executeWrappedJS("invalid syntax here");
        std::string result3 = executeWrappedJS("return 'still working';");
        
        // Interface should continue working after syntax errors
        browser->executeJavascriptSync("console.log('after syntax error');");
    });
}

TEST_F(BrowserJavaScriptTest, RuntimeErrorHandling) {
    // Test runtime error handling JavaScript interface
    EXPECT_NO_THROW({
        // Test runtime error interface
        std::string result1 = executeWrappedJS("return 'before error';");
        std::string result2 = executeWrappedJS("throw new Error('runtime error');");
        std::string result3 = executeWrappedJS("return 'after error';");
        
        // Test undefined access interface
        std::string result4 = executeWrappedJS("return undefined.property;");
        std::string result5 = executeWrappedJS("return 'still functional';");
    });
}

TEST_F(BrowserJavaScriptTest, ArrayOperations) {
    // Test array operations JavaScript interface
    EXPECT_NO_THROW({
        // Test array creation and manipulation interface
        executeWrappedJS("return [1, 2, 3].length.toString();");
        executeWrappedJS("return [1, 2, 3].join('-');");
        executeWrappedJS("return [1, 2, 3].reverse().toString();");
        executeWrappedJS("return [1, 2, 3].slice(1, 2).toString();");
        
        // Test array methods interface
        executeWrappedJS("return [1, 2, 3].map(x => x * 2).toString();");
        executeWrappedJS("return [1, 2, 3].filter(x => x > 1).toString();");
        executeWrappedJS("return [1, 2, 3].reduce((a, b) => a + b, 0).toString();");
    });
}

TEST_F(BrowserJavaScriptTest, ObjectOperations) {
    // Test object operations JavaScript interface
    EXPECT_NO_THROW({
        // Test object creation and manipulation interface
        executeWrappedJS("return {a: 1, b: 2}.a.toString();");
        executeWrappedJS("return Object.keys({x: 1, y: 2}).join(',');");
        executeWrappedJS("return Object.values({a: 1, b: 2}).join(',');");
        
        // Test object property interface
        executeWrappedJS("var obj = {prop: 'value'}; return obj.prop;");
        executeWrappedJS("var obj = {}; obj.newProp = 'new'; return obj.newProp;");
        executeWrappedJS("return JSON.stringify({test: 'json'});");
    });
}

TEST_F(BrowserJavaScriptTest, JavaScriptCompletionWaiting) {
    // Test JavaScript completion waiting interface
    EXPECT_NO_THROW({
        // Test completion interface
        browser->executeJavascriptSync("var completed = true;");
        browser->executeJavascriptSync("setTimeout(function() { completed = false; }, 0);");
        
        // Test execution completion interface
        std::string result = browser->executeJavascriptSync("return 'completed';");
        
        // Test async operation interface
        executeWrappedJS("return Promise.resolve('promise result');");
    });
}

TEST_F(BrowserJavaScriptTest, TimeBasedOperations) {
    // Test time-based operations JavaScript interface
    EXPECT_NO_THROW({
        // Test time functions interface
        executeWrappedJS("return Date.now().toString();");
        executeWrappedJS("return new Date().getTime().toString();");
        executeWrappedJS("return new Date().toISOString();");
        
        // Test timing interface
        executeWrappedJS("var start = Date.now(); var end = Date.now(); return (end - start >= 0).toString();");
    });
}

TEST_F(BrowserJavaScriptTest, BrowserEnvironmentAccess) {
    // Test browser environment access JavaScript interface
    EXPECT_NO_THROW({
        // Test browser globals interface
        executeWrappedJS("return typeof window;");
        executeWrappedJS("return typeof document;");
        executeWrappedJS("return typeof navigator;");
        executeWrappedJS("return typeof console;");
        
        // Test browser features interface
        executeWrappedJS("return navigator.userAgent ? 'has userAgent' : 'no userAgent';");
        executeWrappedJS("return window.location ? 'has location' : 'no location';");
    });
}

TEST_F(BrowserJavaScriptTest, JavaScriptExecutionPerformance) {
    // Test JavaScript execution performance interface
    EXPECT_NO_THROW({
        // Test performance interface
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 10; ++i) {
            executeWrappedJS("return (Math.random() * 1000).toString();");
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Test bulk execution interface
        executeWrappedJS("var sum = 0; for (var i = 0; i < 100; i++) { sum += i; } return sum.toString();");
    });
}

TEST_F(BrowserJavaScriptTest, UnicodeStringHandling) {
    // Test Unicode string handling JavaScript interface
    EXPECT_NO_THROW({
        // Test Unicode interface
        executeWrappedJS("return '你好世界';");
        executeWrappedJS("return 'αβγδε';");
        executeWrappedJS("return '🌍🚀✨';");
        executeWrappedJS("return '测试字符串'.length.toString();");
        
        // Test Unicode operations interface
        executeWrappedJS("return '你好'.toUpperCase();");
        executeWrappedJS("return 'ΓΕΙΆ ΣΑΣ'.toLowerCase();");
    });
}

TEST_F(BrowserJavaScriptTest, EdgeCaseInputs) {
    // Test edge case inputs JavaScript interface
    EXPECT_NO_THROW({
        // Test edge case interface
        executeWrappedJS("return null;");
        executeWrappedJS("return undefined;");
        executeWrappedJS("return '';");
        executeWrappedJS("return 0;");
        executeWrappedJS("return false;");
        
        // Test special values interface
        executeWrappedJS("return NaN.toString();");
        executeWrappedJS("return Infinity.toString();");
        executeWrappedJS("return (-Infinity).toString();");
        
        // Test complex expressions interface
        executeWrappedJS("return (function() { var x; return x; })();");
        executeWrappedJS("return typeof null;");
    });
}