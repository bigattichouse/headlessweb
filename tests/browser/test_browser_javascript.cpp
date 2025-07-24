#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h" // Include global test environment
#include <filesystem>
#include <chrono>
#include <thread>

extern std::unique_ptr<Browser> g_browser;

class BrowserJavaScriptTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("browser_js_tests");
        createJavaScriptTestHtmlFile();
    }

    void TearDown() override {
        temp_dir.reset();
    }

    void createJavaScriptTestHtmlFile() {
        std::string html_content = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>JavaScript Test Page</title>
</head>
<body>
    <div id="content">
        <h1 id="title">JavaScript Test</h1>
        <button id="test-btn" onclick="testFunction()">Test Button</button>
        <div id="result"></div>
        <input id="test-input" type="text" value="initial">
    </div>
    
    <script>
        // Global test variables
        window.testValue = 42;
        window.testString = "Hello World";
        window.testArray = [1, 2, 3, 4, 5];
        window.testObject = {
            name: "Test Object",
            value: 123,
            nested: {
                property: "nested value"
            }
        };
        
        // Test functions
        function testFunction() {
            document.getElementById('result').innerHTML = 'Function executed';
            return 'success';
        }
        
        function asyncTestFunction(callback) {
            setTimeout(function() {
                document.getElementById('result').innerHTML = 'Async function completed';
                if (callback) callback('async success');
            }, 100);
        }
        
        function throwError() {
            throw new Error('Test error message');
        }
        
        function returnValue(value) {
            return value;
        }
        
        function performCalculation(a, b) {
            return a + b;
        }
        
        function manipulateDOM() {
            var element = document.createElement('div');
            element.id = 'dynamic-element';
            element.innerHTML = 'Dynamic content';
            document.getElementById('content').appendChild(element);
            return element.id;
        }
        
        function checkPageReady() {
            return document.readyState === 'complete';
        }
        
        function simulateUserAction() {
            var input = document.getElementById('test-input');
            input.value = 'modified by script';
            input.focus();
            return input.value;
        }
        
        // Promise-based function
        function promiseTest() {
            return new Promise(function(resolve, reject) {
                setTimeout(function() {
                    resolve('promise resolved');
                }, 50);
            });
        }
        
        // Complex object manipulation
        function complexOperation() {
            var result = {
                timestamp: Date.now(),
                userAgent: navigator.userAgent,
                title: document.title,
                url: window.location.href,
                elementCount: document.querySelectorAll('*').length
            };
            return JSON.stringify(result);
        }
    </script>
</body>
</html>
)HTML";
        js_test_file = temp_dir->createFile("jstest.html", html_content);
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::filesystem::path js_test_file;
};

// ========== Basic JavaScript Execution Tests ==========

TEST_F(BrowserJavaScriptTest, BasicJavaScriptExecution) {
    // Test basic JavaScript execution interface
    std::string result;
    EXPECT_NO_THROW({
        g_browser->executeJavascript("console.log('test');", &result);
        g_browser->executeJavascript("1 + 1;", &result);
        g_browser->executeJavascript("document.title;", &result);
    });
}

TEST_F(BrowserJavaScriptTest, SynchronousJavaScriptExecution) {
    // Test synchronous JavaScript execution
    EXPECT_NO_THROW({
        std::string result1 = g_browser->executeJavascriptSync("2 + 2");
        std::string result2 = g_browser->executeJavascriptSync("'Hello World'");
        std::string result3 = g_browser->executeJavascriptSync("Math.PI");
    });
}

TEST_F(BrowserJavaScriptTest, SafeJavaScriptExecution) {
    // Test safe JavaScript execution (should handle errors gracefully)
    EXPECT_NO_THROW({
        std::string result1 = g_browser->executeJavascriptSyncSafe("document.title");
        std::string result2 = g_browser->executeJavascriptSyncSafe("window.location.href");
        std::string result3 = g_browser->executeJavascriptSyncSafe("nonexistentVariable");
        std::string result4 = g_browser->executeJavascriptSyncSafe("invalid.syntax.");
    });
}

// ========== JavaScript Expression Tests ==========

TEST_F(BrowserJavaScriptTest, ArithmeticExpressions) {
    std::vector<std::string> arithmetic_tests = {
        "1 + 1",
        "10 - 5",
        "3 * 4",
        "15 / 3",
        "17 % 5",
        "Math.pow(2, 3)",
        "Math.sqrt(16)",
        "Math.max(1, 2, 3, 4, 5)",
        "Math.min(5, 4, 3, 2, 1)"
    };
    
    for (const auto& expression : arithmetic_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSync(expression);
        });
    }
}

TEST_F(BrowserJavaScriptTest, StringOperations) {
    std::vector<std::string> string_tests = {
        "'Hello' + ' ' + 'World'",
        "'Test String'.length",
        "'test string'.toUpperCase()",
        "'TEST STRING'.toLowerCase()",
        "'Hello World'.substring(0, 5)",
        "'Test,String,Split'.split(',')",
        "JSON.stringify({name: 'test', value: 123})"
    };
    
    for (const auto& expression : string_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSync(expression);
        });
    }
}

TEST_F(BrowserJavaScriptTest, BooleanExpressions) {
    std::vector<std::string> boolean_tests = {
        "true",
        "false",
        "1 === 1",
        "1 !== 2",
        "5 > 3",
        "2 < 10",
        "5 >= 5",
        "3 <= 4",
        "true && true",
        "false || true",
        "!false"
    };
    
    for (const auto& expression : boolean_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSync(expression);
        });
    }
}

// ========== DOM Manipulation via JavaScript ==========

TEST_F(BrowserJavaScriptTest, DOMQueryOperations) {
    std::vector<std::string> dom_tests = {
        "document.title",
        "document.URL",
        "document.readyState",
        "document.getElementById('content')",
        "document.querySelector('#title')",
        "document.querySelectorAll('div').length",
        "document.getElementsByTagName('script').length",
        "document.createElement('div')"
    };
    
    for (const auto& expression : dom_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSyncSafe(expression);
        });
    }
}

TEST_F(BrowserJavaScriptTest, DOMModificationOperations) {
    std::vector<std::string> modification_tests = {
        "document.getElementById('result').innerHTML = 'Modified by test'",
        "document.getElementById('test-input').value = 'New value'",
        "document.title = 'Modified Title'",
        "var elem = document.createElement('span'); elem.id = 'test-span'; elem"
    };
    
    for (const auto& expression : modification_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSyncSafe(expression);
        });
    }
}

// ========== Function Execution Tests ==========

TEST_F(BrowserJavaScriptTest, FunctionCalling) {
    // Test calling functions defined in the page
    std::vector<std::string> function_tests = {
        "testFunction()",
        "returnValue('test input')",
        "performCalculation(5, 3)",
        "manipulateDOM()",
        "checkPageReady()",
        "simulateUserAction()"
    };
    
    for (const auto& expression : function_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSyncSafe(expression);
        });
    }
}

TEST_F(BrowserJavaScriptTest, AnonymousFunctionExecution) {
    std::vector<std::string> anonymous_tests = {
        "(function() { return 'anonymous function'; })()",
        "(function(a, b) { return a * b; })(4, 5)",
        "(function() { return Math.random(); })()",
        "(function() { return new Date().getTime(); })()"
    };
    
    for (const auto& expression : anonymous_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSync(expression);
        });
    }
}

// ========== Error Handling Tests ==========

TEST_F(BrowserJavaScriptTest, SyntaxErrorHandling) {
    std::vector<std::string> syntax_errors = {
        "",                    // Empty script
        "var unclosed =",      // Incomplete statement
        "function() {",        // Unclosed function
        "invalid.syntax.",     // Invalid syntax
        "undefined.property",  // Reference error
        "null.method()",       // Method on null
        "throw new Error('test error')" // Thrown error
    };
    
    for (const auto& invalid_script : syntax_errors) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSyncSafe(invalid_script);
        });
    }
}

TEST_F(BrowserJavaScriptTest, RuntimeErrorHandling) {
    std::vector<std::string> runtime_errors = {
        "nonexistentFunction()",
        "document.getElementById('nonexistent').click()",
        "window.nonexistentProperty.method()",
        "JSON.parse('invalid json')",
        "parseInt('not a number').toFixed(2)"
    };
    
    for (const auto& error_script : runtime_errors) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSyncSafe(error_script);
        });
    }
}

// ========== Complex Data Type Tests ==========

TEST_F(BrowserJavaScriptTest, ArrayOperations) {
    std::vector<std::string> array_tests = {
        "[1, 2, 3, 4, 5]",
        "[1, 2, 3].length",
        "[1, 2, 3].join(',')",
        "[1, 2, 3].slice(1, 2)",
        "[3, 1, 2].sort()",
        "['a', 'b', 'c'].reverse()",
        "[1, 2, 3].map(function(x) { return x * 2; })"
    };
    
    for (const auto& expression : array_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSync(expression);
        });
    }
}

TEST_F(BrowserJavaScriptTest, ObjectOperations) {
    std::vector<std::string> object_tests = {
        "{name: 'test', value: 42}",
        "Object.keys({a: 1, b: 2, c: 3})",
        "Object.values({x: 10, y: 20})",
        "JSON.stringify({test: 'value', number: 123})",
        "JSON.parse('{\"key\": \"value\"}')",
        "({a: 1, b: 2}).hasOwnProperty('a')"
    };
    
    for (const auto& expression : object_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSync(expression);
        });
    }
}

// ========== Timing and Asynchronous Tests ==========

TEST_F(BrowserJavaScriptTest, JavaScriptCompletionWaiting) {
    // Test JavaScript completion waiting
    EXPECT_NO_THROW({
        g_browser->waitForJavaScriptCompletion(1000);   // 1 second timeout
        g_browser->waitForJavaScriptCompletion(5000);   // 5 second timeout
        g_browser->waitForJavaScriptCompletion(100);    // Short timeout
    });
}

TEST_F(BrowserJavaScriptTest, TimeBasedOperations) {
    std::vector<std::string> time_tests = {
        "new Date().getTime()",
        "Date.now()",
        "new Date().toISOString()",
        "performance.now()",
        "Math.floor(Date.now() / 1000)" // Unix timestamp
    };
    
    for (const auto& expression : time_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSync(expression);
        });
    }
}

// ========== Browser Environment Tests ==========

TEST_F(BrowserJavaScriptTest, BrowserEnvironmentAccess) {
    std::vector<std::string> browser_tests = {
        "navigator.userAgent",
        "window.location.href",
        "document.documentElement.lang",
        "screen.width",
        "screen.height",
        "window.innerWidth",
        "window.innerHeight"
    };
    
    for (const auto& expression : browser_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSyncSafe(expression);
        });
    }
}

// ========== Performance Tests ==========

TEST_F(BrowserJavaScriptTest, JavaScriptExecutionPerformance) {
    auto start = std::chrono::steady_clock::now();
    
    // Execute multiple JavaScript operations
    EXPECT_NO_THROW({
        for (int i = 0; i < 50; ++i) {
            g_browser->executeJavascriptSync("Math.random()");
            g_browser->executeJavascriptSync("1 + " + std::to_string(i));
            g_browser->executeJavascriptSyncSafe("document.title");
        }
    });
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete within reasonable time
    EXPECT_LT(duration.count(), 10000); // Less than 10 seconds for 150 operations
}

// ========== Unicode and Special Characters ==========

TEST_F(BrowserJavaScriptTest, UnicodeStringHandling) {
    std::vector<std::string> unicode_tests = {
        "'测试文本'",
        "'العربية'",
        "'Русский'",
        "'🎉🔧💻'",
        "'Ñiño José Müller'",
        "'Κόσμος'",
        "'こんにちは世界'"
    };
    
    for (const auto& unicode_str : unicode_tests) {
        EXPECT_NO_THROW({
            std::string result = g_browser->executeJavascriptSync(unicode_str);
            std::string length_check = g_browser->executeJavascriptSync(unicode_str + ".length");
        });
    }
}

// ========== Edge Cases and Boundary Tests ==========

TEST_F(BrowserJavaScriptTest, EdgeCaseInputs) {
    // Test edge case inputs
    EXPECT_NO_THROW({
        g_browser->executeJavascriptSyncSafe(std::string(10000, 'a')); // Very long script
        g_browser->executeJavascriptSyncSafe("null");
        g_browser->executeJavascriptSyncSafe("undefined");
        g_browser->executeJavascriptSyncSafe("Infinity");
        g_browser->executeJavascriptSyncSafe("NaN");
        g_browser->executeJavascriptSyncSafe("0");
        g_browser->executeJavascriptSyncSafe("-0");
    });
}