#!/bin/bash

# JavaScript Execution Test
# Tests JavaScript execution, DOM queries, and data extraction

set -e

# Helper functions
source "$(dirname "$0")/test_helpers.sh"

# Configuration
TEST_FILE="/tmp/hweb_js_test.html"

# Create JavaScript test HTML
create_js_test_html() {
    cat > "$TEST_FILE" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>JavaScript Test Page</title>
    <script>
        // Test data and functions
        window.testData = {
            number: 42,
            string: "Hello World",
            boolean: true,
            array: [1, 2, 3, 4, 5],
            object: {
                name: "Test Object",
                value: 100
            }
        };
        
        window.testFunctions = {
            simpleFunction: function() {
                return "Function called successfully";
            },
            
            calculateSum: function(a, b) {
                return a + b;
            },
            
            manipulateDOM: function() {
                var div = document.createElement('div');
                div.id = 'dynamic-element';
                div.innerText = 'Dynamically created';
                document.body.appendChild(div);
                return 'DOM element created';
            },
            
            getFormData: function() {
                var username = document.getElementById('username').value;
                var email = document.getElementById('email').value;
                return {username: username, email: email};
            },
            
            complexCalculation: function() {
                var sum = 0;
                for (var i = 1; i <= 100; i++) {
                    sum += i;
                }
                return sum; // Should be 5050
            },
            
            asyncOperation: function() {
                return new Promise(function(resolve) {
                    setTimeout(function() {
                        resolve('Async operation completed');
                    }, 100);
                });
            }
        };
        
        // Event handlers
        window.addEventListener('load', function() {
            document.getElementById('js-trigger').addEventListener('click', function() {
                document.getElementById('click-result').innerText = 'Button clicked via JS';
            });
        });
    </script>
</head>
<body>
    <h1>JavaScript Test Page</h1>
    
    <!-- DOM elements for testing -->
    <div id="test-container">
        <p id="test-paragraph">Original text content</p>
        <div class="test-item">Item 1</div>
        <div class="test-item">Item 2</div>
        <div class="test-item">Item 3</div>
        
        <input type="text" id="username" value="testuser">
        <input type="email" id="email" value="test@example.com">
        
        <button id="js-trigger">Click Me</button>
        <div id="click-result"></div>
        
        <div id="data-display"></div>
    </div>
    
    <!-- Test elements with attributes -->
    <div id="attr-test" data-value="123" data-name="test-element" class="important">
        Attribute test element
    </div>
    
    <!-- List for counting tests -->
    <ul id="test-list">
        <li>List item 1</li>
        <li>List item 2</li>
        <li>List item 3</li>
        <li>List item 4</li>
    </ul>
    
    <!-- Table for complex DOM queries -->
    <table id="test-table">
        <thead>
            <tr><th>Name</th><th>Value</th></tr>
        </thead>
        <tbody>
            <tr><td>Row 1</td><td>100</td></tr>
            <tr><td>Row 2</td><td>200</td></tr>
            <tr><td>Row 3</td><td>300</td></tr>
        </tbody>
    </table>
</body>
</html>
EOF
    echo "Created JavaScript test HTML: $TEST_FILE"
}

# Test basic JavaScript execution
test_basic_javascript() {
    echo "=== Test: Basic JavaScript Execution ==="
    
    create_js_test_html
    
    # Setup session
    $HWEB_EXECUTABLE --session 'js-test' --url "file://$TEST_FILE" >/dev/null 2>&1
    
    # Test simple JavaScript expressions
    check_command "$HWEB_EXECUTABLE --session 'js-test' --js '2 + 2'" "Simple arithmetic"
    
    MATH_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "2 + 2" 2>/dev/null | tail -n 1)
    verify_value "$MATH_RESULT" "4" "Arithmetic result"
    
    # Test string operations
    STRING_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "'Hello' + ' ' + 'World'" 2>/dev/null | tail -n 1)
    verify_value "$STRING_RESULT" "Hello World" "String concatenation"
    
    # Test boolean operations
    BOOL_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "true && false" 2>/dev/null | tail -n 1)
    verify_value "$BOOL_RESULT" "false" "Boolean operation"
    
    echo ""
}

# Test DOM access and manipulation
test_dom_access() {
    echo "=== Test: DOM Access and Manipulation ==="
    
    # Test document title access
    TITLE_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "document.title" 2>/dev/null | tail -n 1)
    verify_value "$TITLE_RESULT" "JavaScript Test Page" "Document title access"
    
    # Test element selection
    ELEMENT_TEXT=$($HWEB_EXECUTABLE --session "js-test" --js "document.getElementById('test-paragraph').innerText" 2>/dev/null | tail -n 1)
    verify_value "$ELEMENT_TEXT" "Original text content" "Element text access"
    
    # Test element counting
    ITEM_COUNT=$($HWEB_EXECUTABLE --session "js-test" --js "document.querySelectorAll('.test-item').length" 2>/dev/null | tail -n 1)
    verify_value "$ITEM_COUNT" "3" "Element counting"
    
    # Test DOM manipulation
    check_command "$HWEB_EXECUTABLE --session 'js-test' --js \"document.getElementById('test-paragraph').innerText = 'Modified text'; 'Text modified'\"" "DOM text modification"
    
    MODIFIED_TEXT=$($HWEB_EXECUTABLE --session "js-test" --js "document.getElementById('test-paragraph').innerText" 2>/dev/null | tail -n 1)
    verify_value "$MODIFIED_TEXT" "Modified text" "DOM modification result"
    
    echo ""
}

# Test global object access
test_global_objects() {
    echo "=== Test: Global Object Access ==="
    
    # Test window object access
    WINDOW_LOCATION=$($HWEB_EXECUTABLE --session "js-test" --js "window.location.protocol" 2>/dev/null | tail -n 1)
    verify_contains "$WINDOW_LOCATION" "file:" "Window location access"
    
    # Test custom global data
    NUMBER_DATA=$($HWEB_EXECUTABLE --session "js-test" --js "window.testData.number" 2>/dev/null | tail -n 1)
    verify_value "$NUMBER_DATA" "42" "Custom global number"
    
    STRING_DATA=$($HWEB_EXECUTABLE --session "js-test" --js "window.testData.string" 2>/dev/null | tail -n 1)
    verify_value "$STRING_DATA" "Hello World" "Custom global string"
    
    ARRAY_LENGTH=$($HWEB_EXECUTABLE --session "js-test" --js "window.testData.array.length" 2>/dev/null | tail -n 1)
    verify_value "$ARRAY_LENGTH" "5" "Custom global array length"
    
    OBJECT_NAME=$($HWEB_EXECUTABLE --session "js-test" --js "window.testData.object.name" 2>/dev/null | tail -n 1)
    verify_value "$OBJECT_NAME" "Test Object" "Custom global object property"
    
    echo ""
}

# Test function calls
test_function_calls() {
    echo "=== Test: Function Calls ==="
    
    # Test simple function call
    FUNC_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "window.testFunctions.simpleFunction()" 2>/dev/null | tail -n 1)
    verify_value "$FUNC_RESULT" "Function called successfully" "Simple function call"
    
    # Test function with parameters
    SUM_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "window.testFunctions.calculateSum(10, 20)" 2>/dev/null | tail -n 1)
    verify_value "$SUM_RESULT" "30" "Function with parameters"
    
    # Test complex calculation
    CALC_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "window.testFunctions.complexCalculation()" 2>/dev/null | tail -n 1)
    verify_value "$CALC_RESULT" "5050" "Complex calculation function"
    
    # Test DOM manipulation function
    DOM_FUNC_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "window.testFunctions.manipulateDOM()" 2>/dev/null | tail -n 1)
    verify_value "$DOM_FUNC_RESULT" "DOM element created" "DOM manipulation function"
    
    # Verify the DOM was actually modified
    DYNAMIC_ELEMENT=$($HWEB_EXECUTABLE --session "js-test" --js "document.getElementById('dynamic-element').innerText" 2>/dev/null | tail -n 1)
    verify_value "$DYNAMIC_ELEMENT" "Dynamically created" "Dynamic element created"
    
    echo ""
}

# Test complex JavaScript operations
test_complex_operations() {
    echo "=== Test: Complex JavaScript Operations ==="
    
    # Test JSON operations
    JSON_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "JSON.stringify(window.testData.object)" 2>/dev/null | tail -n 1)
    verify_contains "$JSON_RESULT" "Test Object" "JSON stringify operation"
    
    # Test array operations
    ARRAY_OPERATIONS=$($HWEB_EXECUTABLE --session "js-test" --js "window.testData.array.filter(x => x > 2).length" 2>/dev/null | tail -n 1)
    verify_value "$ARRAY_OPERATIONS" "3" "Array filter operation"
    
    # Test complex DOM query
    TABLE_QUERY=$($HWEB_EXECUTABLE --session "js-test" --js "Array.from(document.querySelectorAll('#test-table tbody tr')).map(row => row.cells[1].innerText).join(',')" 2>/dev/null | tail -n 1)
    verify_value "$TABLE_QUERY" "100,200,300" "Complex DOM query"
    
    # Test multiple operations in one script
    MULTI_OP=$($HWEB_EXECUTABLE --session "js-test" --js "(function() { 
        var count = document.querySelectorAll('li').length; 
        var title = document.title; 
        return count + ' items in ' + title; 
    })()" 2>/dev/null | tail -n 1)
    verify_contains "$MULTI_OP" "4 items" "Multiple operations in one script"
    
    echo ""
}

# Test form data extraction
test_form_data_extraction() {
    echo "=== Test: Form Data Extraction ==="
    
    # Test direct form field access
    USERNAME_VALUE=$($HWEB_EXECUTABLE --session "js-test" --js "document.getElementById('username').value" 2>/dev/null | tail -n 1)
    verify_value "$USERNAME_VALUE" "testuser" "Form username value"
    
    EMAIL_VALUE=$($HWEB_EXECUTABLE --session "js-test" --js "document.getElementById('email').value" 2>/dev/null | tail -n 1)
    verify_value "$EMAIL_VALUE" "test@example.com" "Form email value"
    
    # Test form data function
    FORM_DATA=$($HWEB_EXECUTABLE --session "js-test" --js "JSON.stringify(window.testFunctions.getFormData())" 2>/dev/null | tail -n 1)
    verify_contains "$FORM_DATA" "testuser" "Form data extraction function"
    verify_contains "$FORM_DATA" "test@example.com" "Form data extraction email"
    
    echo ""
}

# Test attribute access
test_attribute_access() {
    echo "=== Test: Attribute Access ==="
    
    # Test data attributes
    DATA_VALUE=$($HWEB_EXECUTABLE --session "js-test" --js "document.getElementById('attr-test').getAttribute('data-value')" 2>/dev/null | tail -n 1)
    verify_value "$DATA_VALUE" "123" "Data attribute access"
    
    DATA_NAME=$($HWEB_EXECUTABLE --session "js-test" --js "document.getElementById('attr-test').getAttribute('data-name')" 2>/dev/null | tail -n 1)
    verify_value "$DATA_NAME" "test-element" "Data name attribute"
    
    # Test class attribute
    CLASS_NAME=$($HWEB_EXECUTABLE --session "js-test" --js "document.getElementById('attr-test').className" 2>/dev/null | tail -n 1)
    verify_value "$CLASS_NAME" "important" "Class attribute access"
    
    # Test attribute modification
    check_command "$HWEB_EXECUTABLE --session 'js-test' --js \"document.getElementById('attr-test').setAttribute('data-modified', 'true'); 'Attribute set'\"" "Set attribute"
    
    MODIFIED_ATTR=$($HWEB_EXECUTABLE --session "js-test" --js "document.getElementById('attr-test').getAttribute('data-modified')" 2>/dev/null | tail -n 1)
    verify_value "$MODIFIED_ATTR" "true" "Modified attribute value"
    
    echo ""
}

# Test event simulation and handling
test_event_handling() {
    echo "=== Test: Event Handling ==="
    
    # Test click event simulation
    check_command "$HWEB_EXECUTABLE --session 'js-test' --js \"document.getElementById('js-trigger').click(); 'Click simulated'\"" "Simulate click event"
    
    # Check if event handler fired
    CLICK_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "document.getElementById('click-result').innerText" 2>/dev/null | tail -n 1)
    verify_value "$CLICK_RESULT" "Button clicked via JS" "Event handler result"
    
    echo ""
}

# Test error handling in JavaScript
test_javascript_errors() {
    echo "=== Test: JavaScript Error Handling ==="
    
    # Test syntax error
    if $HWEB_EXECUTABLE --session 'js-test' --js 'this.is.invalid.syntax(' 2>&1 | grep -q -E "(Error|Failed)"; then
        echo -e "${GREEN}✓ PASS${NC}: Syntax error handled correctly"
    else
        echo -e "${RED}✗ FAIL${NC}: Syntax error not handled properly"
    fi
    
    # Test reference error
    if $HWEB_EXECUTABLE --session 'js-test' --js 'nonExistentVariable.property' 2>&1 | grep -q -E "(Error|Failed)"; then
        echo -e "${GREEN}✓ PASS${NC}: Reference error handled correctly"
    else
        echo -e "${RED}✗ FAIL${NC}: Reference error not handled properly"
    fi
    
    echo ""
}

# Test JavaScript with different return types
test_return_types() {
    echo "=== Test: JavaScript Return Types ==="
    
    # Test undefined return
    UNDEF_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "undefined" 2>/dev/null | tail -n 1)
    verify_value "$UNDEF_RESULT" "undefined" "Undefined return"
    
    # Test null return
    NULL_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "null" 2>/dev/null | tail -n 1)
    verify_value "$NULL_RESULT" "null" "Null return"
    
    # Test number return
    NUMBER_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "42.5" 2>/dev/null | tail -n 1)
    verify_value "$NUMBER_RESULT" "42.5" "Number return"
    
    # Test boolean returns
    TRUE_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "true" 2>/dev/null | tail -n 1)
    verify_value "$TRUE_RESULT" "true" "Boolean true return"
    
    FALSE_RESULT=$($HWEB_EXECUTABLE --session "js-test" --js "false" 2>/dev/null | tail -n 1)
    verify_value "$FALSE_RESULT" "false" "Boolean false return"
    
    echo ""
}

# Cleanup function
cleanup_javascript() {
    echo "=== JavaScript Test Cleanup ==="
    $HWEB_EXECUTABLE --session 'js-test' --end >/dev/null 2>&1 || true
    rm -f "$TEST_FILE"
    echo "JavaScript test files cleaned up"
    echo ""
}

# Main test execution
main() {
    echo "=== JavaScript Execution Test Suite ==="
    echo ""
    
    test_basic_javascript
    test_dom_access
    test_global_objects
    test_function_calls
    test_complex_operations
    test_form_data_extraction
    test_attribute_access
    test_event_handling
    test_javascript_errors
    test_return_types
    
    cleanup_javascript
    
    echo "=== JavaScript Tests Complete ==="
    echo ""
    echo "JavaScript Features Tested:"
    echo "✓ Basic JavaScript expression execution"
    echo "✓ DOM access and manipulation"
    echo "✓ Global object and variable access"
    echo "✓ Function calls with parameters"
    echo "✓ Complex operations (JSON, arrays, queries)"
    echo "✓ Form data extraction"
    echo "✓ Attribute access and modification"
    echo "✓ Event handling and simulation"
    echo "✓ Error handling for invalid JavaScript"
    echo "✓ Different return type handling"
}

# Run tests if called directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main
fi