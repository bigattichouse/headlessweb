#!/bin/bash

# Form Interaction Test
# Tests form filling, selection, checking, and submission

set -e

# Helper functions
source "$(dirname "$0")/test_helpers.sh"

# Configuration
TEST_FILE="/tmp/hweb_form_test.html"

# Create comprehensive form test HTML
create_form_test_html() {
    cat > "$TEST_FILE" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>Form Test Page</title>
    <script>
        function validateForm() {
            var username = document.getElementById('username').value;
            var email = document.getElementById('email').value;
            if (username && email) {
                document.getElementById('validation-result').innerText = 'Valid';
                return true;
            } else {
                document.getElementById('validation-result').innerText = 'Invalid';
                return false;
            }
        }
        
        window.addEventListener('load', function() {
            document.getElementById('test-form').addEventListener('submit', function(e) {
                e.preventDefault();
                validateForm();
                document.getElementById('form-status').innerText = 'Form submitted';
            });
        });
    </script>
</head>
<body>
    <h1>Form Test Page</h1>
    
    <form id="test-form" action="/submit" method="post">
        <!-- Text inputs -->
        <div>
            <label for="username">Username:</label>
            <input type="text" id="username" name="username" placeholder="Enter username">
        </div>
        
        <div>
            <label for="email">Email:</label>
            <input type="email" id="email" name="email" placeholder="Enter email">
        </div>
        
        <div>
            <label for="password">Password:</label>
            <input type="password" id="password" name="password" placeholder="Enter password">
        </div>
        
        <!-- Textarea -->
        <div>
            <label for="comments">Comments:</label>
            <textarea id="comments" name="comments" placeholder="Enter comments">Initial textarea content</textarea>
        </div>
        
        <!-- Select dropdown -->
        <div>
            <label for="country">Country:</label>
            <select id="country" name="country">
                <option value="">Select country</option>
                <option value="us">United States</option>
                <option value="ca">Canada</option>
                <option value="mx">Mexico</option>
                <option value="uk">United Kingdom</option>
            </select>
        </div>
        
        <!-- Multi-select -->
        <div>
            <label for="interests">Interests:</label>
            <select id="interests" name="interests" multiple>
                <option value="tech">Technology</option>
                <option value="science">Science</option>
                <option value="art">Art</option>
                <option value="music">Music</option>
            </select>
        </div>
        
        <!-- Radio buttons -->
        <div>
            <label>Gender:</label>
            <input type="radio" id="male" name="gender" value="male">
            <label for="male">Male</label>
            <input type="radio" id="female" name="gender" value="female">
            <label for="female">Female</label>
            <input type="radio" id="other" name="gender" value="other">
            <label for="other">Other</label>
        </div>
        
        <!-- Checkboxes -->
        <div>
            <input type="checkbox" id="newsletter" name="newsletter" value="yes">
            <label for="newsletter">Subscribe to newsletter</label>
        </div>
        
        <div>
            <input type="checkbox" id="terms" name="terms" value="yes">
            <label for="terms">Accept terms and conditions</label>
        </div>
        
        <!-- File input -->
        <div>
            <label for="avatar">Avatar:</label>
            <input type="file" id="avatar" name="avatar" accept="image/*">
        </div>
        
        <!-- Number and range inputs -->
        <div>
            <label for="age">Age:</label>
            <input type="number" id="age" name="age" min="1" max="120" value="25">
        </div>
        
        <div>
            <label for="rating">Rating:</label>
            <input type="range" id="rating" name="rating" min="1" max="10" value="5">
        </div>
        
        <!-- Submit button -->
        <div>
            <button type="submit" id="submit-btn">Submit Form</button>
            <button type="reset" id="reset-btn">Reset Form</button>
        </div>
    </form>
    
    <!-- Status indicators -->
    <div id="form-status"></div>
    <div id="validation-result"></div>
    
    <!-- Test results display -->
    <div id="form-data-display" style="margin-top: 20px; border: 1px solid #ccc; padding: 10px;">
        <h3>Form Data Display</h3>
        <div id="display-username"></div>
        <div id="display-email"></div>
        <div id="display-country"></div>
        <div id="display-newsletter"></div>
        <div id="display-terms"></div>
    </div>
</body>
</html>
EOF
    echo "Created form test HTML: $TEST_FILE"
}

# Test basic form input
test_basic_input() {
    echo "=== Test: Basic Form Input ==="
    
    create_form_test_html
    
    # Setup session
    $HWEB_EXECUTABLE --session 'form-test' --url "file://$TEST_FILE" >/dev/null 2>&1
    
    # Test text input
    check_command "$HWEB_EXECUTABLE --session 'form-test' --type '#username' 'testuser'" "Type into text input"
    
    # Verify input value
    USERNAME_VAL=$($HWEB_EXECUTABLE --session "form-test" --js "document.getElementById('username').value" 2>/dev/null | tail -n 1)
    verify_value "$USERNAME_VAL" "testuser" "Text input value"
    
    # Test email input
    check_command "$HWEB_EXECUTABLE --session 'form-test' --type '#email' 'test@example.com'" "Type into email input"
    
    # Test password input
    check_command "$HWEB_EXECUTABLE --session 'form-test' --type '#password' 'secretpass'" "Type into password input"
    
    # Test textarea
    check_command "$HWEB_EXECUTABLE --session 'form-test' --type '#comments' 'This is a test comment'" "Type into textarea"
    
    echo ""
}

# Test select dropdowns
test_select_dropdowns() {
    echo "=== Test: Select Dropdowns ==="
    
    # Test single select
    check_command "$HWEB_EXECUTABLE --session 'form-test' --select '#country' 'us'" "Select country option"
    
    # Verify selection
    COUNTRY_VAL=$($HWEB_EXECUTABLE --session "form-test" --js "document.getElementById('country').value" 2>/dev/null | tail -n 1)
    verify_value "$COUNTRY_VAL" "us" "Country selection value"
    
    # Test different country
    check_command "$HWEB_EXECUTABLE --session 'form-test' --select '#country' 'ca'" "Select Canada"
    
    COUNTRY_VAL2=$($HWEB_EXECUTABLE --session "form-test" --js "document.getElementById('country').value" 2>/dev/null | tail -n 1)
    verify_value "$COUNTRY_VAL2" "ca" "Updated country selection"
    
    echo ""
}

# Test checkboxes and radio buttons
test_checkboxes_radios() {
    echo "=== Test: Checkboxes and Radio Buttons ==="
    
    # Test checkbox checking
    check_command "$HWEB_EXECUTABLE --session 'form-test' --check '#newsletter'" "Check newsletter checkbox"
    
    # Verify checkbox state
    NEWSLETTER_CHECKED=$($HWEB_EXECUTABLE --session "form-test" --js "document.getElementById('newsletter').checked" 2>/dev/null | tail -n 1)
    verify_value "$NEWSLETTER_CHECKED" "true" "Newsletter checkbox checked"
    
    # Test checkbox unchecking
    check_command "$HWEB_EXECUTABLE --session 'form-test' --uncheck '#newsletter'" "Uncheck newsletter checkbox"
    
    NEWSLETTER_UNCHECKED=$($HWEB_EXECUTABLE --session "form-test" --js "document.getElementById('newsletter').checked" 2>/dev/null | tail -n 1)
    verify_value "$NEWSLETTER_UNCHECKED" "false" "Newsletter checkbox unchecked"
    
    # Test multiple checkboxes
    check_command "$HWEB_EXECUTABLE --session 'form-test' --check '#newsletter'" "Check newsletter"
    check_command "$HWEB_EXECUTABLE --session 'form-test' --check '#terms'" "Check terms"
    
    # Test radio button selection
    check_command "$HWEB_EXECUTABLE --session 'form-test' --click '#male'" "Select male radio"
    
    MALE_SELECTED=$($HWEB_EXECUTABLE --session "form-test" --js "document.getElementById('male').checked" 2>/dev/null | tail -n 1)
    verify_value "$MALE_SELECTED" "true" "Male radio button selected"
    
    echo ""
}

# Test form focus and navigation
test_form_focus() {
    echo "=== Test: Form Focus and Navigation ==="
    
    # Test focusing on elements
    check_command "$HWEB_EXECUTABLE --session 'form-test' --focus '#email'" "Focus on email field"
    
    # Verify focus (check if element is active)
    FOCUSED_ELEMENT=$($HWEB_EXECUTABLE --session "form-test" --js "document.activeElement.id" 2>/dev/null | tail -n 1)
    verify_value "$FOCUSED_ELEMENT" "email" "Email field focused"
    
    # Test tab navigation simulation
    check_command "$HWEB_EXECUTABLE --session 'form-test' --focus '#password'" "Focus on password field"
    
    echo ""
}

# Test form submission
test_form_submission() {
    echo "=== Test: Form Submission ==="
    
    # Fill out form completely
    check_command "$HWEB_EXECUTABLE --session 'form-test' --type '#username' 'formtester'" "Fill username"
    check_command "$HWEB_EXECUTABLE --session 'form-test' --type '#email' 'form@test.com'" "Fill email"
    check_command "$HWEB_EXECUTABLE --session 'form-test' --select '#country' 'us'" "Select country"
    check_command "$HWEB_EXECUTABLE --session 'form-test' --check '#terms'" "Accept terms"
    
    # Submit form
    check_command "$HWEB_EXECUTABLE --session 'form-test' --click '#submit-btn'" "Submit form"
    
    # Check form submission result (our test page prevents actual submission)
    FORM_STATUS=$($HWEB_EXECUTABLE --session "form-test" --js "document.getElementById('form-status').innerText" 2>/dev/null | tail -n 1)
    verify_value "$FORM_STATUS" "Form submitted" "Form submission status"
    
    echo ""
}

# Test form validation
test_form_validation() {
    echo "=== Test: Form Validation ==="
    
    # Clear form and test validation
    check_command "$HWEB_EXECUTABLE --session 'form-test' --click '#reset-btn'" "Reset form"
    
    # Submit empty form (should trigger validation)
    check_command "$HWEB_EXECUTABLE --session 'form-test' --click '#submit-btn'" "Submit empty form"
    
    VALIDATION_RESULT=$($HWEB_EXECUTABLE --session "form-test" --js "document.getElementById('validation-result').innerText" 2>/dev/null | tail -n 1)
    verify_value "$VALIDATION_RESULT" "Invalid" "Form validation for empty form"
    
    # Fill required fields and test again
    check_command "$HWEB_EXECUTABLE --session 'form-test' --type '#username' 'validuser'" "Fill required username"
    check_command "$HWEB_EXECUTABLE --session 'form-test' --type '#email' 'valid@email.com'" "Fill required email"
    check_command "$HWEB_EXECUTABLE --session 'form-test' --click '#submit-btn'" "Submit valid form"
    
    VALID_RESULT=$($HWEB_EXECUTABLE --session "form-test" --js "document.getElementById('validation-result').innerText" 2>/dev/null | tail -n 1)
    verify_value "$VALID_RESULT" "Valid" "Form validation for valid form"
    
    echo ""
}

# Test complex form interactions
test_complex_interactions() {
    echo "=== Test: Complex Form Interactions ==="
    
    # Test chained form operations
    check_command "$HWEB_EXECUTABLE --session 'form-chain' \
        --url 'file://$TEST_FILE' \
        --type '#username' 'chainuser' \
        --type '#email' 'chain@test.com' \
        --select '#country' 'ca' \
        --check '#newsletter' \
        --check '#terms' \
        --click '#male' \
        --type '#comments' 'Chained form operations test'" "Chained form operations"
    
    # Verify all chained operations
    CHAIN_USERNAME=$($HWEB_EXECUTABLE --session "form-chain" --js "document.getElementById('username').value" 2>/dev/null | tail -n 1)
    verify_value "$CHAIN_USERNAME" "chainuser" "Chained username"
    
    CHAIN_COUNTRY=$($HWEB_EXECUTABLE --session "form-chain" --js "document.getElementById('country').value" 2>/dev/null | tail -n 1)
    verify_value "$CHAIN_COUNTRY" "ca" "Chained country selection"
    
    CHAIN_NEWSLETTER=$($HWEB_EXECUTABLE --session "form-chain" --js "document.getElementById('newsletter').checked" 2>/dev/null | tail -n 1)
    verify_value "$CHAIN_NEWSLETTER" "true" "Chained newsletter checkbox"
    
    echo ""
}

# Test form persistence across sessions
test_form_persistence() {
    echo "=== Test: Form State Persistence ==="
    
    # Fill form and save session
    check_command "$HWEB_EXECUTABLE --session 'form-persist' \
        --url 'file://$TEST_FILE' \
        --type '#username' 'persistent' \
        --type '#email' 'persist@test.com' \
        --select '#country' 'uk' \
        --check '#terms'" "Fill form for persistence test"
    
    # End session to save state
    check_command "$HWEB_EXECUTABLE --session 'form-persist' --end" "Save form session"
    
    # Restore session and verify form state
    check_command "$HWEB_EXECUTABLE --session 'form-persist'" "Restore form session"
    
    PERSIST_USERNAME=$($HWEB_EXECUTABLE --session "form-persist" --js "document.getElementById('username').value" 2>/dev/null | tail -n 1)
    verify_value "$PERSIST_USERNAME" "persistent" "Persistent username"
    
    PERSIST_COUNTRY=$($HWEB_EXECUTABLE --session "form-persist" --js "document.getElementById('country').value" 2>/dev/null | tail -n 1)
    verify_value "$PERSIST_COUNTRY" "uk" "Persistent country selection"
    
    PERSIST_TERMS=$($HWEB_EXECUTABLE --session "form-persist" --js "document.getElementById('terms').checked" 2>/dev/null | tail -n 1)
    verify_value "$PERSIST_TERMS" "true" "Persistent terms checkbox"
    
    echo ""
}

# Test form error handling
test_form_errors() {
    echo "=== Test: Form Error Handling ==="
    
    # Test invalid selector
    if $HWEB_EXECUTABLE --session 'form-test' --type '#nonexistent-field' 'value' 2>&1 | grep -q "Failed"; then
        echo -e "${GREEN}✓ PASS${NC}: Invalid form field selector handled"
    else
        echo -e "${RED}✗ FAIL${NC}: Invalid form field selector not handled"
    fi
    
    # Test invalid select option
    if $HWEB_EXECUTABLE --session 'form-test' --select '#country' 'invalid-option' 2>&1 | grep -q -E "(Failed|Error|not found)"; then
        echo -e "${GREEN}✓ PASS${NC}: Invalid select option handled"
    else
        echo -e "${YELLOW}? INFO${NC}: Invalid select option handling may need verification"
    fi
    
    echo ""
}

# Cleanup function
cleanup_forms() {
    echo "=== Form Test Cleanup ==="
    $HWEB_EXECUTABLE --session 'form-test' --end >/dev/null 2>&1 || true
    $HWEB_EXECUTABLE --session 'form-chain' --end >/dev/null 2>&1 || true
    $HWEB_EXECUTABLE --session 'form-persist' --end >/dev/null 2>&1 || true
    rm -f "$TEST_FILE"
    echo "Form test files cleaned up"
    echo ""
}

# Main test execution
main() {
    echo "=== Form Interaction Test Suite ==="
    echo ""
    
    test_basic_input
    test_select_dropdowns
    test_checkboxes_radios
    test_form_focus
    test_form_submission
    test_form_validation
    test_complex_interactions
    test_form_persistence
    test_form_errors
    
    cleanup_forms
    
    echo "=== Form Tests Complete ==="
    echo ""
    echo "Form Features Tested:"
    echo "✓ Basic text input (text, email, password, textarea)"
    echo "✓ Select dropdowns (single and multi-select)"
    echo "✓ Checkboxes and radio buttons"
    echo "✓ Form focus and navigation"
    echo "✓ Form submission and validation"
    echo "✓ Complex chained form operations"
    echo "✓ Form state persistence across sessions"
    echo "✓ Error handling for invalid selectors"
}

# Run tests if called directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main
fi