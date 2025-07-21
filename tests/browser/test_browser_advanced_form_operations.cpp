#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include <memory>
#include <thread>
#include <chrono>

class BrowserAdvancedFormOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for testing
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("browser_advanced_form_tests");
        browser_ = std::make_unique<Browser>();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown() override {
        browser_.reset();
        temp_dir.reset();
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    
    // Helper to load complex multi-step form
    void loadComplexFormPage() {
        std::string complex_html = R"(
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
        )";
        
        browser_->loadHTML(complex_html);
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }

    std::unique_ptr<Browser> browser_;
};

// ========== Multi-Step Form Navigation Tests ==========

TEST_F(BrowserAdvancedFormOperationsTest, MultiStepFormNavigation_StepProgression) {
    loadComplexFormPage();
    
    // Verify initial step is visible
    EXPECT_TRUE(browser_->isElementVisible("#step1"));
    EXPECT_FALSE(browser_->isElementVisible("#step2"));
    EXPECT_FALSE(browser_->isElementVisible("#step3"));
    
    // Fill step 1 with valid data
    browser_->type("#username", "testuser123");
    browser_->type("#email", "test@example.com");
    browser_->type("#password", "password123");
    browser_->type("#confirm-password", "password123");
    
    // Navigate to step 2
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_FALSE(browser_->isElementVisible("#step1"));
    EXPECT_TRUE(browser_->isElementVisible("#step2"));
    EXPECT_FALSE(browser_->isElementVisible("#step3"));
}

TEST_F(BrowserAdvancedFormOperationsTest, MultiStepFormNavigation_StepValidation) {
    loadComplexFormPage();
    
    // Try to proceed with invalid data
    browser_->type("#username", "ab"); // Too short
    browser_->type("#email", "invalid-email");
    
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Should still be on step 1
    EXPECT_TRUE(browser_->isElementVisible("#step1"));
    EXPECT_FALSE(browser_->isElementVisible("#step2"));
    
    // Fix validation and proceed
    browser_->type("#username", "validuser");
    browser_->type("#email", "valid@example.com");
    browser_->type("#password", "password123");
    browser_->type("#confirm-password", "password123");
    
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->isElementVisible("#step2"));
}

TEST_F(BrowserAdvancedFormOperationsTest, MultiStepFormNavigation_BackNavigation) {
    loadComplexFormPage();
    
    // Navigate to step 2 (assuming valid data)
    browser_->type("#username", "testuser");
    browser_->type("#email", "test@example.com");
    browser_->type("#password", "password123");
    browser_->type("#confirm-password", "password123");
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->isElementVisible("#step2"));
    
    // Go back to step 1
    browser_->click("#step2-prev");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->isElementVisible("#step1"));
    EXPECT_FALSE(browser_->isElementVisible("#step2"));
    
    // Verify data preservation
    std::string username = browser_->getValue("#username");
    EXPECT_EQ(username, "testuser");
}

// ========== Conditional Field Logic Tests ==========

TEST_F(BrowserAdvancedFormOperationsTest, ConditionalFieldLogic_CountryStateLogic) {
    loadComplexFormPage();
    
    // Navigate to step 2
    browser_->type("#username", "testuser");
    browser_->type("#email", "test@example.com");
    browser_->type("#password", "password123");
    browser_->type("#confirm-password", "password123");
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Initially state field should be hidden
    EXPECT_FALSE(browser_->isElementVisible("#state-field"));
    
    // Select US - state field should appear
    browser_->selectOption("#country", "us");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->isElementVisible("#state-field"));
    
    // Verify US states are populated
    bool has_california = browser_->hasOption("#state", "ca");
    EXPECT_TRUE(has_california);
    
    // Select Canada - different states should appear
    browser_->selectOption("#country", "ca");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    bool has_ontario = browser_->hasOption("#state", "on");
    EXPECT_TRUE(has_ontario);
    
    // Select UK - state field should disappear
    browser_->selectOption("#country", "uk");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_FALSE(browser_->isElementVisible("#state-field"));
}

TEST_F(BrowserAdvancedFormOperationsTest, ConditionalFieldLogic_DependentValidation) {
    loadComplexFormPage();
    
    // Navigate through steps to preferences
    browser_->type("#username", "testuser");
    browser_->type("#email", "test@example.com");
    browser_->type("#password", "password123");
    browser_->type("#confirm-password", "password123");
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    browser_->selectOption("#country", "us");
    browser_->type("#address1", "123 Test St");
    browser_->type("#city", "Test City");
    browser_->type("#postal", "12345");
    browser_->click("#step2-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Test terms checkbox requirement
    EXPECT_FALSE(browser_->isChecked("#terms"));
    
    // Try to submit without accepting terms
    browser_->click("#final-submit");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Form should not submit (browser validation)
    // Check terms and try again
    browser_->check("#terms");
    EXPECT_TRUE(browser_->isChecked("#terms"));
}

// ========== Complex Field Group Testing ==========

TEST_F(BrowserAdvancedFormOperationsTest, ComplexFieldGroups_CheckboxArrays) {
    loadComplexFormPage();
    
    // Navigate to preferences step
    browser_->type("#username", "testuser");
    browser_->type("#email", "test@example.com");
    browser_->type("#password", "password123");
    browser_->type("#confirm-password", "password123");
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    browser_->selectOption("#country", "us");
    browser_->type("#address1", "123 Test St");
    browser_->type("#city", "Test City");
    browser_->type("#postal", "12345");
    browser_->click("#step2-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Test multiple checkbox selections
    browser_->check("#news-general");
    browser_->check("#news-tech");
    EXPECT_FALSE(browser_->isChecked("#news-marketing"));
    
    EXPECT_TRUE(browser_->isChecked("#news-general"));
    EXPECT_TRUE(browser_->isChecked("#news-tech"));
    
    // Test unchecking
    browser_->uncheck("#news-general");
    EXPECT_FALSE(browser_->isChecked("#news-general"));
    EXPECT_TRUE(browser_->isChecked("#news-tech"));
}

TEST_F(BrowserAdvancedFormOperationsTest, ComplexFieldGroups_RadioButtonLogic) {
    loadComplexFormPage();
    
    // Navigate to preferences
    browser_->type("#username", "testuser");
    browser_->type("#email", "test@example.com");
    browser_->type("#password", "password123");
    browser_->type("#confirm-password", "password123");
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    browser_->selectOption("#country", "us");
    browser_->type("#address1", "123 Test St");
    browser_->type("#city", "Test City");
    browser_->type("#postal", "12345");
    browser_->click("#step2-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Test radio button group behavior
    EXPECT_TRUE(browser_->isChecked("#comm-email")); // Initially checked
    EXPECT_FALSE(browser_->isChecked("#comm-sms"));
    EXPECT_FALSE(browser_->isChecked("#comm-phone"));
    
    // Select different option
    browser_->check("#comm-sms");
    EXPECT_FALSE(browser_->isChecked("#comm-email"));
    EXPECT_TRUE(browser_->isChecked("#comm-sms"));
    EXPECT_FALSE(browser_->isChecked("#comm-phone"));
    
    // Select third option
    browser_->check("#comm-phone");
    EXPECT_FALSE(browser_->isChecked("#comm-email"));
    EXPECT_FALSE(browser_->isChecked("#comm-sms"));
    EXPECT_TRUE(browser_->isChecked("#comm-phone"));
}

// ========== Dynamic Form Element Testing ==========

TEST_F(BrowserAdvancedFormOperationsTest, DynamicFormElements_AddRemoveFields) {
    loadComplexFormPage();
    
    // Initially no dynamic fields
    int initial_field_count = browser_->countElements("#dynamic-form input[type='text']");
    EXPECT_EQ(initial_field_count, 0);
    
    // Add a text field
    browser_->click("#add-field");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    int field_count_after_add = browser_->countElements("#dynamic-form input[type='text']");
    EXPECT_EQ(field_count_after_add, 1);
    
    // Verify the field is functional
    bool field_exists = browser_->elementExists("#dynamic-1");
    EXPECT_TRUE(field_exists);
    
    if (field_exists) {
        browser_->type("#dynamic-1", "dynamic test value");
        std::string value = browser_->getValue("#dynamic-1");
        EXPECT_EQ(value, "dynamic test value");
    }
    
    // Add another field
    browser_->click("#add-field");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    field_count_after_add = browser_->countElements("#dynamic-form input[type='text']");
    EXPECT_EQ(field_count_after_add, 2);
}

TEST_F(BrowserAdvancedFormOperationsTest, DynamicFormElements_CheckboxGeneration) {
    loadComplexFormPage();
    
    // Add dynamic checkboxes
    browser_->click("#add-checkbox");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    int checkbox_count = browser_->countElements("#dynamic-form input[type='checkbox']");
    EXPECT_EQ(checkbox_count, 1);
    
    // Test the dynamically created checkbox
    bool checkbox_exists = browser_->elementExists("#check-1");
    EXPECT_TRUE(checkbox_exists);
    
    if (checkbox_exists) {
        EXPECT_FALSE(browser_->isChecked("#check-1"));
        browser_->check("#check-1");
        EXPECT_TRUE(browser_->isChecked("#check-1"));
    }
    
    // Add multiple checkboxes
    browser_->click("#add-checkbox");
    browser_->click("#add-checkbox");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    checkbox_count = browser_->countElements("#dynamic-form input[type='checkbox']");
    EXPECT_EQ(checkbox_count, 3);
}

// ========== Form State Persistence Tests ==========

TEST_F(BrowserAdvancedFormOperationsTest, FormStatePersistence_CrossStepData) {
    loadComplexFormPage();
    
    // Fill step 1
    browser_->type("#username", "persisttest");
    browser_->type("#email", "persist@test.com");
    browser_->type("#password", "persist123");
    browser_->type("#confirm-password", "persist123");
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Fill step 2
    browser_->selectOption("#country", "us");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    browser_->selectOption("#state", "ca");
    browser_->type("#address1", "456 Persist Ave");
    browser_->type("#city", "Persist City");
    browser_->type("#postal", "90210");
    
    // Navigate back to step 1 and verify data
    browser_->click("#step2-prev");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_EQ(browser_->getValue("#username"), "persisttest");
    EXPECT_EQ(browser_->getValue("#email"), "persist@test.com");
    
    // Navigate forward again and verify step 2 data
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_EQ(browser_->getValue("#country"), "us");
    EXPECT_EQ(browser_->getValue("#address1"), "456 Persist Ave");
    EXPECT_EQ(browser_->getValue("#city"), "Persist City");
}

// ========== Complex Validation Scenarios ==========

TEST_F(BrowserAdvancedFormOperationsTest, ComplexValidation_PasswordMatching) {
    loadComplexFormPage();
    
    browser_->type("#username", "valuser");
    browser_->type("#email", "val@test.com");
    browser_->type("#password", "password123");
    browser_->type("#confirm-password", "password456"); // Mismatched
    
    // Try to proceed - should validate password match
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Should remain on step 1 due to validation
    EXPECT_TRUE(browser_->isElementVisible("#step1"));
    
    // Fix password match
    browser_->clearField("#confirm-password");
    browser_->type("#confirm-password", "password123");
    
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->isElementVisible("#step2"));
}

TEST_F(BrowserAdvancedFormOperationsTest, ComplexValidation_EmailFormat) {
    loadComplexFormPage();
    
    browser_->type("#username", "emailtest");
    browser_->type("#password", "password123");
    browser_->type("#confirm-password", "password123");
    
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
        browser_->clearField("#email");
        browser_->type("#email", invalid_email);
        
        browser_->click("#step1-next");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Should remain on step 1
        EXPECT_TRUE(browser_->isElementVisible("#step1"));
        EXPECT_FALSE(browser_->isElementVisible("#step2"));
    }
    
    // Test valid email
    browser_->clearField("#email");
    browser_->type("#email", "valid@domain.com");
    
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->isElementVisible("#step2"));
}

// ========== Error Handling and Recovery ==========

TEST_F(BrowserAdvancedFormOperationsTest, ErrorHandling_InvalidFormOperations) {
    loadComplexFormPage();
    
    // Test operations on elements that don't exist yet (in hidden steps)
    EXPECT_FALSE(browser_->type("#country", "test")); // Hidden in step 2
    EXPECT_FALSE(browser_->check("#terms")); // Hidden in step 3
    
    // Test operations after elements become available
    browser_->type("#username", "testuser");
    browser_->type("#email", "test@example.com");
    browser_->type("#password", "password123");
    browser_->type("#confirm-password", "password123");
    browser_->click("#step1-next");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Now step 2 elements should be available
    EXPECT_TRUE(browser_->elementExists("#country"));
    bool country_select_worked = browser_->selectOption("#country", "us");
    EXPECT_TRUE(country_select_worked);
}

TEST_F(BrowserAdvancedFormOperationsTest, ErrorHandling_FormSubmissionFailure) {
    loadComplexFormPage();
    
    // Navigate to final step without filling required fields
    browser_->executeJS("showStep(3)");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_TRUE(browser_->isElementVisible("#step3"));
    
    // Try to submit without required terms checkbox
    EXPECT_FALSE(browser_->isChecked("#terms"));
    
    browser_->click("#final-submit");
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
        browser_->click("#add-field");
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
    browser_->type("#dynamic-10", "performance test");
    std::string value = browser_->getValue("#dynamic-10");
    EXPECT_EQ(value, "performance test");
}

TEST_F(BrowserAdvancedFormOperationsTest, Performance_RapidFormNavigation) {
    loadComplexFormPage();
    
    // Fill out forms quickly and navigate rapidly
    browser_->type("#username", "speedtest");
    browser_->type("#email", "speed@test.com");
    browser_->type("#password", "speed123");
    browser_->type("#confirm-password", "speed123");
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Rapid navigation test
    for (int i = 0; i < 5; i++) {
        browser_->click("#step1-next");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (browser_->isElementVisible("#step2")) {
            browser_->click("#step2-prev");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    EXPECT_LT(duration.count(), 2000); // Less than 2 seconds
    
    // Should end up back at step 1
    EXPECT_TRUE(browser_->isElementVisible("#step1"));
}