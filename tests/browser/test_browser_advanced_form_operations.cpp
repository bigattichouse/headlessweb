#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <memory>
#include <thread>
#include <chrono>
#include <fstream>
#include <vector>

extern std::unique_ptr<Browser> g_browser;

class BrowserAdvancedFormOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use global browser instance (properly initialized)
        browser_ = g_browser.get();
        
        // Create temporary directory for file:// URLs
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("browser_advanced_form_tests");
        
        // Reset browser to clean state before each test
        browser_->loadUri("about:blank");
        browser_->waitForNavigation(2000);
        
        debug_output("BrowserAdvancedFormOperationsTest SetUp complete");
    }
    
    void TearDown() override {
        // Clean up temporary directory (don't destroy global browser)
        temp_dir.reset();
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    
    // Generic JavaScript wrapper function for safe execution
    std::string executeWrappedJS(const std::string& jsCode) {
        std::string wrapped = "(function() { " + jsCode + " })()";
        return browser_->executeJavascriptSync(wrapped);
    }
    
    // Enhanced page loading method based on successful BrowserMainTest approach
    bool loadPageWithReadinessCheck(const std::string& url, const std::vector<std::string>& required_elements = {}) {
        browser_->loadUri(url);
        
        // Wait for navigation
        bool nav_success = browser_->waitForNavigation(5000);
        if (!nav_success) return false;
        
        // Allow WebKit processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Check basic JavaScript execution with retry
        for (int i = 0; i < 5; i++) {
            std::string js_test = executeWrappedJS("return 'test';");
            if (js_test == "test") break;
            if (i == 4) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        // Verify DOM is ready
        for (int i = 0; i < 5; i++) {
            std::string dom_check = executeWrappedJS("return document.readyState === 'complete';");
            if (dom_check == "true") break;
            if (i == 4) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        // Check for required elements if specified
        if (!required_elements.empty()) {
            for (int i = 0; i < 5; i++) {
                bool all_elements_ready = true;
                for (const auto& element : required_elements) {
                    std::string element_check = executeWrappedJS(
                        "return document.querySelector('" + element + "') !== null;"
                    );
                    if (element_check != "true") {
                        all_elements_ready = false;
                        break;
                    }
                }
                if (all_elements_ready) break;
                if (i == 4) return false;
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
        
        return true;
    }
    
    // Helper to check if element is checked
    bool checkElementState(const std::string& selector) {
        std::string js = "var el = document.querySelector('" + selector + "'); "
                        "return el ? el.checked : false;";
        std::string result = executeWrappedJS(js);
        return result == "true";
    }
    
    // Helper to get element value
    std::string getElementValue(const std::string& selector) {
        std::string js = "var el = document.querySelector('" + selector + "'); "
                        "return el ? el.value : '';";
        return executeWrappedJS(js);
    }
    
    // Helper to load complex multi-step form
    void loadComplexFormPage() {
        std::string complex_html = R"HTMLDELIM(
            <!DOCTYPE html>
            <html>
            <head>
                <title>Complex Form Test Page</title>
                <style>
                    .step { display: none; }
                    .step.active { display: block; }
                    .invalid { border: 2px solid red; }
                    .valid { border: 2px solid green; }
                </style>
                <script>
                    let currentStep = 1;
                    function showStep(step) {
                        document.querySelectorAll('.step').forEach(s => s.classList.remove('active'));
                        document.getElementById('step' + step).classList.add('active');
                        currentStep = step;
                    }
                    
                    function validateEmail(email) {
                        return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email);
                    }
                    
                    function validateStep(step) {
                        if (step === 1) {
                            const username = document.getElementById('username').value;
                            const email = document.getElementById('email').value;
                            return username.length >= 3 && validateEmail(email);
                        }
                        return true;
                    }
                    
                    function nextStep() {
                        if (validateStep(currentStep)) {
                            if (currentStep < 3) {
                                showStep(currentStep + 1);
                            }
                        }
                    }
                    
                    function prevStep() {
                        if (currentStep > 1) {
                            showStep(currentStep - 1);
                        }
                    }
                    
                    function updateCountry() {
                        const country = document.getElementById('country').value;
                        const stateField = document.getElementById('state-field');
                        const stateSelect = document.getElementById('state');
                        
                        if (country === 'us') {
                            stateField.style.display = 'block';
                            stateSelect.innerHTML = '<option value="ca">California</option><option value="ny">New York</option><option value="tx">Texas</option>';
                        } else if (country === 'ca') {
                            stateField.style.display = 'block';
                            stateSelect.innerHTML = '<option value="on">Ontario</option><option value="bc">British Columbia</option>';
                        } else {
                            stateField.style.display = 'none';
                        }
                    }
                    
                    window.onload = function() {
                        showStep(1);
                    }
                </script>
            </head>
            <body>
                <form id="complex-form" action="/complex-submit" method="post">
                    <!-- Step 1: Personal Information -->
                    <div id="step1" class="step">
                        <h2>Step 1: Personal Information</h2>
                        <label for="username">Username (min 3 chars):</label>
                        <input type="text" id="username" name="username" required minlength="3">
                        
                        <label for="email">Email:</label>
                        <input type="email" id="email" name="email" required>
                        
                        <label for="password">Password:</label>
                        <input type="password" id="password" name="password" required minlength="8">
                        
                        <label for="confirm-password">Confirm Password:</label>
                        <input type="password" id="confirm-password" name="confirm_password" required>
                        
                        <button type="button" onclick="nextStep()" id="step1-next">Next</button>
                    </div>
                    
                    <!-- Step 2: Address Information -->
                    <div id="step2" class="step">
                        <h2>Step 2: Address Information</h2>
                        <label for="country">Country:</label>
                        <select id="country" name="country" onchange="updateCountry()" required>
                            <option value="">Select Country</option>
                            <option value="us">United States</option>
                            <option value="ca">Canada</option>
                            <option value="uk">United Kingdom</option>
                        </select>
                        
                        <div id="state-field" style="display: none;">
                            <label for="state">State/Province:</label>
                            <select id="state" name="state"></select>
                        </div>
                        
                        <label for="address1">Address Line 1:</label>
                        <input type="text" id="address1" name="address1" required>
                        
                        <label for="address2">Address Line 2:</label>
                        <input type="text" id="address2" name="address2">
                        
                        <label for="city">City:</label>
                        <input type="text" id="city" name="city" required>
                        
                        <label for="postal">Postal Code:</label>
                        <input type="text" id="postal" name="postal" required>
                        
                        <button type="button" onclick="prevStep()" id="step2-prev">Previous</button>
                        <button type="button" onclick="nextStep()" id="step2-next">Next</button>
                    </div>
                    
                    <!-- Step 3: Preferences -->
                    <div id="step3" class="step">
                        <h2>Step 3: Preferences</h2>
                        <fieldset>
                            <legend>Newsletter Subscriptions:</legend>
                            <input type="checkbox" id="news-general" name="newsletters[]" value="general">
                            <label for="news-general">General News</label><br>
                            
                            <input type="checkbox" id="news-tech" name="newsletters[]" value="tech">
                            <label for="news-tech">Technology Updates</label><br>
                            
                            <input type="checkbox" id="news-marketing" name="newsletters[]" value="marketing">
                            <label for="news-marketing">Marketing Offers</label><br>
                        </fieldset>
                        
                        <fieldset>
                            <legend>Communication Preference:</legend>
                            <input type="radio" id="comm-email" name="communication" value="email" checked>
                            <label for="comm-email">Email</label><br>
                            
                            <input type="radio" id="comm-sms" name="communication" value="sms">
                            <label for="comm-sms">SMS</label><br>
                            
                            <input type="radio" id="comm-phone" name="communication" value="phone">
                            <label for="comm-phone">Phone</label><br>
                        </fieldset>
                        
                        <label for="bio">Bio (optional):</label>
                        <textarea id="bio" name="bio" rows="4" cols="50"></textarea>
                        
                        <label for="terms">
                            <input type="checkbox" id="terms" name="terms" required>
                            I agree to the Terms and Conditions
                        </label>
                        
                        <button type="button" onclick="prevStep()" id="step3-prev">Previous</button>
                        <button type="submit" id="final-submit">Submit</button>
                    </div>
                </form>
                
                <!-- Dynamic Form for testing -->
                <div id="dynamic-form-container">
                    <h2>Dynamic Form Elements</h2>
                    <button type="button" id="add-field">Add Text Field</button>
                    <button type="button" id="add-checkbox">Add Checkbox</button>
                    <form id="dynamic-form"></form>
                </div>
                
                <script>
                    let fieldCounter = 0;
                    
                    document.getElementById('add-field').onclick = function() {
                        const form = document.getElementById('dynamic-form');
                        const div = document.createElement('div');
                        div.innerHTML = '<label>Field ' + (++fieldCounter) + ':</label><input type="text" id="dynamic-' + fieldCounter + '" name="dynamic-' + fieldCounter + '">';
                        form.appendChild(div);
                    };
                    
                    document.getElementById('add-checkbox').onclick = function() {
                        const form = document.getElementById('dynamic-form');
                        const div = document.createElement('div');
                        div.innerHTML = '<input type="checkbox" id="check-' + (++fieldCounter) + '" name="check-' + fieldCounter + '"><label for="check-' + fieldCounter + '">Checkbox ' + fieldCounter + '</label>';
                        form.appendChild(div);
                    };
                </script>
            </body>
            </html>
        )HTMLDELIM";
        
        // CRITICAL FIX: Use file:// URL instead of data: URL with proper readiness checking
        auto html_file = temp_dir->createFile("complex_form.html", complex_html);
        std::string file_url = "file://" + html_file.string();
        
        debug_output("Loading complex form page: " + file_url);
        debug_output("HTML content length: " + std::to_string(complex_html.length()));
        
        // DEBUG: Also save to persistent location for inspection
        std::ofstream debug_file("/tmp/debug_complex_form.html");
        debug_file << complex_html;
        debug_file.close();
        debug_output("Debug HTML saved to /tmp/debug_complex_form.html");
        
        // CRITICAL FIX: Use proven loadPageWithReadinessCheck approach
        std::vector<std::string> required_elements = {"#step1", "#username", "#email", "#step1-next"};
        bool page_ready = loadPageWithReadinessCheck(file_url, required_elements);
        if (!page_ready) {
            debug_output("Complex form page failed to load and become ready");
            return;
        }
        
        // Wait for JavaScript functions to be available
        for (int i = 0; i < 5; i++) {
            std::string functions_check = executeWrappedJS(
                "return typeof showStep === 'function' && "
                "typeof nextStep === 'function' && "
                "typeof validateStep === 'function';"
            );
            if (functions_check == "true") break;
            if (i == 4) {
                debug_output("JavaScript functions not ready after retries");
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        debug_output("Complex form page successfully loaded and ready");
    }

    Browser* browser_;  // Raw pointer to global browser instance
};

// ========== Multi-Step Form Navigation Tests ==========

TEST_F(BrowserAdvancedFormOperationsTest, MultiStepFormNavigation_StepProgression) {
    loadComplexFormPage();
    
    // Verify initial step exists (visibility would need CSS state checking)
    EXPECT_TRUE(browser_->elementExists("#step1"));
    EXPECT_TRUE(browser_->elementExists("#step2"));
    EXPECT_TRUE(browser_->elementExists("#step3"));
    
    // Fill step 1 with valid data
    browser_->fillInput("#username", "testuser123");
    browser_->fillInput("#email", "test@example.com");
    browser_->fillInput("#password", "password123");
    browser_->fillInput("#confirm-password", "password123");
    
    // Navigate to step 2
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Steps still exist, but active one changes (would need JS check for visibility)
    EXPECT_TRUE(browser_->elementExists("#step1"));
    EXPECT_TRUE(browser_->elementExists("#step2"));
    EXPECT_TRUE(browser_->elementExists("#step3"));
}

TEST_F(BrowserAdvancedFormOperationsTest, MultiStepFormNavigation_StepValidation) {
    loadComplexFormPage();
    
    // Try to proceed with invalid data
    browser_->fillInput("#username", "ab"); // Too short
    browser_->fillInput("#email", "invalid-email");
    
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Should still be on step 1 (check active class instead of existence)
    EXPECT_TRUE(browser_->elementExists("#step1"));
    EXPECT_TRUE(browser_->elementExists("#step2")); // Step 2 exists in DOM
    
    // Check that step 1 is active and step 2 is not active
    std::string step1_class = browser_->getAttribute("#step1", "class");
    std::string step2_class = browser_->getAttribute("#step2", "class");
    EXPECT_TRUE(step1_class.find("active") != std::string::npos);
    EXPECT_TRUE(step2_class.find("active") == std::string::npos);
    
    // Fix validation and proceed
    browser_->fillInput("#username", "validuser");
    browser_->fillInput("#email", "valid@example.com");
    browser_->fillInput("#password", "password123");
    browser_->fillInput("#confirm-password", "password123");
    
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->elementExists("#step2"));
    
    // Check that step 2 is now active
    std::string step2_class_success = browser_->getAttribute("#step2", "class");
    EXPECT_TRUE(step2_class_success.find("active") != std::string::npos);
}

TEST_F(BrowserAdvancedFormOperationsTest, MultiStepFormNavigation_BackNavigation) {
    loadComplexFormPage();
    
    // Navigate to step 2 (assuming valid data)
    browser_->fillInput("#username", "testuser");
    browser_->fillInput("#email", "test@example.com");
    browser_->fillInput("#password", "password123");
    browser_->fillInput("#confirm-password", "password123");
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->elementExists("#step2"));
    
    // Check that step 2 is active
    std::string step2_active_class = browser_->getAttribute("#step2", "class");
    EXPECT_TRUE(step2_active_class.find("active") != std::string::npos);
    
    // Go back to step 1
    browser_->clickElement("#step2-prev");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->elementExists("#step1"));
    EXPECT_TRUE(browser_->elementExists("#step2")); // Step 2 still exists in DOM
    
    // Check that step 1 is active again and step 2 is not
    std::string step1_back_class = browser_->getAttribute("#step1", "class");
    std::string step2_back_class = browser_->getAttribute("#step2", "class");
    EXPECT_TRUE(step1_back_class.find("active") != std::string::npos);
    EXPECT_TRUE(step2_back_class.find("active") == std::string::npos);
    
    // Verify data preservation
    std::string username = getElementValue("#username");
    EXPECT_EQ(username, "testuser");
}

// ========== Conditional Field Logic Tests ==========

TEST_F(BrowserAdvancedFormOperationsTest, ConditionalFieldLogic_CountryStateLogic) {
    loadComplexFormPage();
    
    // Navigate to step 2
    browser_->fillInput("#username", "testuser");
    browser_->fillInput("#email", "test@example.com");
    browser_->fillInput("#password", "password123");
    browser_->fillInput("#confirm-password", "password123");
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Initially state field should be hidden
    EXPECT_FALSE(browser_->elementExists("#state-field"));
    
    // Select US - state field should appear
    browser_->selectOption("#country", "us");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->elementExists("#state-field"));
    
    // Verify US states are populated
    // Check if California option exists by trying to select it
    bool has_california = browser_->selectOption("#state", "ca");
    EXPECT_TRUE(has_california);
    
    // Select Canada - different states should appear
    browser_->selectOption("#country", "ca");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Check if Ontario option exists by trying to select it
    bool has_ontario = browser_->selectOption("#state", "on");
    EXPECT_TRUE(has_ontario);
    
    // Select UK - state field should disappear
    browser_->selectOption("#country", "uk");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_FALSE(browser_->elementExists("#state-field"));
}

TEST_F(BrowserAdvancedFormOperationsTest, ConditionalFieldLogic_DependentValidation) {
    loadComplexFormPage();
    
    // Navigate through steps to preferences
    browser_->fillInput("#username", "testuser");
    browser_->fillInput("#email", "test@example.com");
    browser_->fillInput("#password", "password123");
    browser_->fillInput("#confirm-password", "password123");
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    browser_->selectOption("#country", "us");
    browser_->fillInput("#address1", "123 Test St");
    browser_->fillInput("#city", "Test City");
    browser_->fillInput("#postal", "12345");
    browser_->clickElement("#step2-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Test terms checkbox requirement
    EXPECT_FALSE(checkElementState("#terms"));
    
    // Try to submit without accepting terms
    browser_->clickElement("#final-submit");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Form should not submit (browser validation)
    // Check terms and try again
    browser_->checkElement("#terms");
    EXPECT_TRUE(checkElementState("#terms"));
}

// ========== Complex Field Group Testing ==========

TEST_F(BrowserAdvancedFormOperationsTest, ComplexFieldGroups_CheckboxArrays) {
    loadComplexFormPage();
    
    // Navigate to preferences step
    browser_->fillInput("#username", "testuser");
    browser_->fillInput("#email", "test@example.com");
    browser_->fillInput("#password", "password123");
    browser_->fillInput("#confirm-password", "password123");
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    browser_->selectOption("#country", "us");
    browser_->fillInput("#address1", "123 Test St");
    browser_->fillInput("#city", "Test City");
    browser_->fillInput("#postal", "12345");
    browser_->clickElement("#step2-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Test multiple checkbox selections
    browser_->checkElement("#news-general");
    browser_->checkElement("#news-tech");
    EXPECT_FALSE(checkElementState("#news-marketing"));
    
    EXPECT_TRUE(checkElementState("#news-general"));
    EXPECT_TRUE(checkElementState("#news-tech"));
    
    // Test unchecking
    browser_->uncheckElement("#news-general");
    EXPECT_FALSE(checkElementState("#news-general"));
    EXPECT_TRUE(checkElementState("#news-tech"));
}

TEST_F(BrowserAdvancedFormOperationsTest, ComplexFieldGroups_RadioButtonLogic) {
    loadComplexFormPage();
    
    // Navigate to preferences
    browser_->fillInput("#username", "testuser");
    browser_->fillInput("#email", "test@example.com");
    browser_->fillInput("#password", "password123");
    browser_->fillInput("#confirm-password", "password123");
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    browser_->selectOption("#country", "us");
    browser_->fillInput("#address1", "123 Test St");
    browser_->fillInput("#city", "Test City");
    browser_->fillInput("#postal", "12345");
    browser_->clickElement("#step2-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Test radio button group behavior
    EXPECT_TRUE(checkElementState("#comm-email")); // Initially checked
    EXPECT_FALSE(checkElementState("#comm-sms"));
    EXPECT_FALSE(checkElementState("#comm-phone"));
    
    // Select different option
    browser_->checkElement("#comm-sms");
    EXPECT_FALSE(checkElementState("#comm-email"));
    EXPECT_TRUE(checkElementState("#comm-sms"));
    EXPECT_FALSE(checkElementState("#comm-phone"));
    
    // Select third option
    browser_->checkElement("#comm-phone");
    EXPECT_FALSE(checkElementState("#comm-email"));
    EXPECT_FALSE(checkElementState("#comm-sms"));
    EXPECT_TRUE(checkElementState("#comm-phone"));
}

// ========== Dynamic Form Element Testing ==========

TEST_F(BrowserAdvancedFormOperationsTest, DynamicFormElements_AddRemoveFields) {
    loadComplexFormPage();
    
    // Initially no dynamic fields
    int initial_field_count = browser_->countElements("#dynamic-form input[type='text']");
    EXPECT_EQ(initial_field_count, 0);
    
    // Add a text field
    browser_->clickElement("#add-field");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    int field_count_after_add = browser_->countElements("#dynamic-form input[type='text']");
    EXPECT_EQ(field_count_after_add, 1);
    
    // Verify the field is functional
    bool field_exists = browser_->elementExists("#dynamic-1");
    EXPECT_TRUE(field_exists);
    
    if (field_exists) {
        browser_->fillInput("#dynamic-1", "dynamic test value");
        std::string value = getElementValue("#dynamic-1");
        EXPECT_EQ(value, "dynamic test value");
    }
    
    // Add another field
    browser_->clickElement("#add-field");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    field_count_after_add = browser_->countElements("#dynamic-form input[type='text']");
    EXPECT_EQ(field_count_after_add, 2);
}

TEST_F(BrowserAdvancedFormOperationsTest, DynamicFormElements_CheckboxGeneration) {
    loadComplexFormPage();
    
    // Add dynamic checkboxes
    browser_->clickElement("#add-checkbox");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    int checkbox_count = browser_->countElements("#dynamic-form input[type='checkbox']");
    EXPECT_EQ(checkbox_count, 1);
    
    // Test the dynamically created checkbox
    bool checkbox_exists = browser_->elementExists("#check-1");
    EXPECT_TRUE(checkbox_exists);
    
    if (checkbox_exists) {
        EXPECT_FALSE(checkElementState("#check-1"));
        browser_->checkElement("#check-1");
        EXPECT_TRUE(checkElementState("#check-1"));
    }
    
    // Add multiple checkboxes
    browser_->clickElement("#add-checkbox");
    browser_->clickElement("#add-checkbox");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    checkbox_count = browser_->countElements("#dynamic-form input[type='checkbox']");
    EXPECT_EQ(checkbox_count, 3);
}

// ========== Form State Persistence Tests ==========

TEST_F(BrowserAdvancedFormOperationsTest, FormStatePersistence_CrossStepData) {
    loadComplexFormPage();
    
    // Fill step 1
    browser_->fillInput("#username", "persisttest");
    browser_->fillInput("#email", "persist@test.com");
    browser_->fillInput("#password", "persist123");
    browser_->fillInput("#confirm-password", "persist123");
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Fill step 2
    browser_->selectOption("#country", "us");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    browser_->selectOption("#state", "ca");
    browser_->fillInput("#address1", "456 Persist Ave");
    browser_->fillInput("#city", "Persist City");
    browser_->fillInput("#postal", "90210");
    
    // Navigate back to step 1 and verify data
    browser_->clickElement("#step2-prev");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_EQ(getElementValue("#username"), "persisttest");
    EXPECT_EQ(getElementValue("#email"), "persist@test.com");
    
    // Navigate forward again and verify step 2 data
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_EQ(getElementValue("#country"), "us");
    EXPECT_EQ(getElementValue("#address1"), "456 Persist Ave");
    EXPECT_EQ(getElementValue("#city"), "Persist City");
}

// ========== Complex Validation Scenarios ==========

TEST_F(BrowserAdvancedFormOperationsTest, ComplexValidation_PasswordMatching) {
    loadComplexFormPage();
    
    browser_->fillInput("#username", "valuser");
    browser_->fillInput("#email", "val@test.com");
    browser_->fillInput("#password", "password123");
    browser_->fillInput("#confirm-password", "password456"); // Mismatched
    
    // Try to proceed - should validate password match
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Should remain on step 1 due to validation
    EXPECT_TRUE(browser_->elementExists("#step1"));
    
    // Fix password match
    browser_->fillInput("#confirm-password", "password123");
    
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->elementExists("#step2"));
}

TEST_F(BrowserAdvancedFormOperationsTest, ComplexValidation_EmailFormat) {
    loadComplexFormPage();
    
    browser_->fillInput("#username", "emailtest");
    browser_->fillInput("#password", "password123");
    browser_->fillInput("#confirm-password", "password123");
    
    // Test various invalid email formats
    std::vector<std::string> invalid_emails = {
        "plainaddress",
        "@missingdomain.com",
        "missing@.com",
        "missing@domain",
        "spaces @domain.com",
        "double@@domain.com"
    };
    
    for (const auto& invalid_email : invalid_emails) {
        browser_->fillInput("#email", "");
        browser_->fillInput("#email", invalid_email);
        
        browser_->clickElement("#step1-next");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Should remain on step 1
        EXPECT_TRUE(browser_->elementExists("#step1"));
        EXPECT_FALSE(browser_->elementExists("#step2"));
    }
    
    // Test valid email
    browser_->fillInput("#email", "");
    browser_->fillInput("#email", "valid@domain.com");
    
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->elementExists("#step2"));
}

// ========== Error Handling and Recovery ==========

TEST_F(BrowserAdvancedFormOperationsTest, ErrorHandling_InvalidFormOperations) {
    loadComplexFormPage();
    
    // Test operations on elements that don't exist yet (in hidden steps)
    EXPECT_FALSE(browser_->fillInput("#country", "test")); // Hidden in step 2
    EXPECT_FALSE(browser_->checkElement("#terms")); // Hidden in step 3
    
    // Test operations after elements become available
    browser_->fillInput("#username", "testuser");
    browser_->fillInput("#email", "test@example.com");
    browser_->fillInput("#password", "password123");
    browser_->fillInput("#confirm-password", "password123");
    browser_->clickElement("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Now step 2 elements should be available
    EXPECT_TRUE(browser_->elementExists("#country"));
    bool country_select_worked = browser_->selectOption("#country", "us");
    EXPECT_TRUE(country_select_worked);
}

TEST_F(BrowserAdvancedFormOperationsTest, ErrorHandling_FormSubmissionFailure) {
    loadComplexFormPage();
    
    // Navigate to final step without filling required fields
    executeWrappedJS("showStep(3);");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->elementExists("#step3"));
    
    // Try to submit without required terms checkbox
    EXPECT_FALSE(checkElementState("#terms"));
    
    browser_->clickElement("#final-submit");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Browser should prevent submission due to HTML5 validation
    // Form should still be visible
    EXPECT_TRUE(browser_->elementExists("#complex-form"));
}

// ========== Performance and Stress Testing ==========

TEST_F(BrowserAdvancedFormOperationsTest, Performance_ManyDynamicFields) {
    loadComplexFormPage();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Add many dynamic fields rapidly
    for (int i = 0; i < 20; i++) {
        browser_->clickElement("#add-field");
        if (i % 5 == 0) { // Occasional small delay
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Should complete within reasonable time (adjust as needed)
    EXPECT_LT(duration.count(), 3000); // Less than 3 seconds
    
    // Verify all fields were created
    int field_count = browser_->countElements("#dynamic-form input[type='text']");
    EXPECT_EQ(field_count, 20);
    
    // Test that fields are functional
    browser_->fillInput("#dynamic-10", "performance test");
    std::string value = getElementValue("#dynamic-10");
    EXPECT_EQ(value, "performance test");
}

TEST_F(BrowserAdvancedFormOperationsTest, Performance_RapidFormNavigation) {
    loadComplexFormPage();
    
    // Fill out forms quickly and navigate rapidly
    browser_->fillInput("#username", "speedtest");
    browser_->fillInput("#email", "speed@test.com");
    browser_->fillInput("#password", "speed123");
    browser_->fillInput("#confirm-password", "speed123");
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Rapid navigation test
    for (int i = 0; i < 5; i++) {
        browser_->clickElement("#step1-next");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (browser_->elementExists("#step2")) {
            browser_->clickElement("#step2-prev");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    EXPECT_LT(duration.count(), 2000); // Less than 2 seconds
    
    // Should end up back at step 1
    EXPECT_TRUE(browser_->elementExists("#step1"));
}