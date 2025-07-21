#include <gtest/gtest.h>
#include "../../src/Browser/Browser.h"
#include <memory>
#include <thread>
#include <chrono>

extern bool g_debug;

class BrowserFormOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Enable debug output for tests
        g_debug = true;
        
        // Initialize browser
        browser_ = std::make_unique<Browser>();
        
        // Small delay to ensure proper initialization
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown() override {
        // Cleanup
        browser_.reset();
    }
    
    // Helper to load test form page
    void loadTestFormPage() {
        std::string form_html = R"(
            <!DOCTYPE html>
            <html>
            <head>
                <title>Form Test Page</title>
            </head>
            <body>
                <form id="test-form" action="/submit" method="post">
                    <!-- Text inputs -->
                    <input type="text" id="text-input" name="text-field" placeholder="Enter text"/>
                    <input type="password" id="password-input" name="password-field"/>
                    <input type="email" id="email-input" name="email-field"/>
                    
                    <!-- Checkboxes -->
                    <input type="checkbox" id="checkbox1" name="checkbox-group" value="option1"/>
                    <label for="checkbox1">Option 1</label>
                    <input type="checkbox" id="checkbox2" name="checkbox-group" value="option2" checked/>
                    <label for="checkbox2">Option 2</label>
                    <input type="checkbox" id="checkbox3" name="checkbox-group" value="option3"/>
                    <label for="checkbox3">Option 3</label>
                    
                    <!-- Radio buttons -->
                    <input type="radio" id="radio1" name="radio-group" value="choice1"/>
                    <label for="radio1">Choice 1</label>
                    <input type="radio" id="radio2" name="radio-group" value="choice2" checked/>
                    <label for="radio2">Choice 2</label>
                    <input type="radio" id="radio3" name="radio-group" value="choice3"/>
                    <label for="radio3">Choice 3</label>
                    
                    <!-- Select dropdowns -->
                    <select id="dropdown1" name="dropdown-field">
                        <option value="">Select option...</option>
                        <option value="option1">Option 1</option>
                        <option value="option2" selected>Option 2</option>
                        <option value="option3">Option 3</option>
                    </select>
                    
                    <select id="dropdown2" name="multi-dropdown" multiple>
                        <option value="multi1">Multi Option 1</option>
                        <option value="multi2" selected>Multi Option 2</option>
                        <option value="multi3">Multi Option 3</option>
                        <option value="multi4" selected>Multi Option 4</option>
                    </select>
                    
                    <!-- Textarea -->
                    <textarea id="textarea1" name="textarea-field" placeholder="Enter long text"></textarea>
                    
                    <!-- Submit buttons -->
                    <input type="submit" id="submit-btn" value="Submit Form"/>
                    <button type="button" id="reset-btn">Reset</button>
                    <button type="button" id="cancel-btn">Cancel</button>
                </form>
                
                <!-- Second form for multi-form testing -->
                <form id="second-form" action="/submit2" method="get">
                    <input type="text" id="second-text" name="second-field"/>
                    <input type="submit" value="Submit Second"/>
                </form>
            </body>
            </html>
        )";
        
        browser_->loadHTML(form_html);
        // Wait for form to load
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::unique_ptr<Browser> browser_;
};

// ========== Checkbox Interaction Tests ==========

TEST_F(BrowserFormOperationsTest, CheckboxInteractionLogic_BasicOperations) {
    loadTestFormPage();
    
    // Test initial checkbox states
    EXPECT_FALSE(browser_->isChecked("#checkbox1"));
    EXPECT_TRUE(browser_->isChecked("#checkbox2"));  // Initially checked
    EXPECT_FALSE(browser_->isChecked("#checkbox3"));
    
    // Test checking unchecked checkbox
    browser_->check("#checkbox1");
    EXPECT_TRUE(browser_->isChecked("#checkbox1"));
    
    // Test unchecking checked checkbox
    browser_->uncheck("#checkbox2");
    EXPECT_FALSE(browser_->isChecked("#checkbox2"));
    
    // Test checking already checked checkbox (should remain checked)
    browser_->check("#checkbox1");
    EXPECT_TRUE(browser_->isChecked("#checkbox1"));
    
    // Test unchecking already unchecked checkbox (should remain unchecked)
    browser_->uncheck("#checkbox3");
    EXPECT_FALSE(browser_->isChecked("#checkbox3"));
}

TEST_F(BrowserFormOperationsTest, CheckboxInteractionLogic_MultipleCheckboxes) {
    loadTestFormPage();
    
    // Test checking multiple checkboxes in same group
    browser_->check("#checkbox1");
    browser_->check("#checkbox3");
    
    EXPECT_TRUE(browser_->isChecked("#checkbox1"));
    EXPECT_TRUE(browser_->isChecked("#checkbox2"));  // Initially checked
    EXPECT_TRUE(browser_->isChecked("#checkbox3"));
    
    // Test unchecking all
    browser_->uncheck("#checkbox1");
    browser_->uncheck("#checkbox2");
    browser_->uncheck("#checkbox3");
    
    EXPECT_FALSE(browser_->isChecked("#checkbox1"));
    EXPECT_FALSE(browser_->isChecked("#checkbox2"));
    EXPECT_FALSE(browser_->isChecked("#checkbox3"));
}

TEST_F(BrowserFormOperationsTest, CheckboxInteractionLogic_ValueExtraction) {
    loadTestFormPage();
    
    // Test extracting values from checked checkboxes
    browser_->check("#checkbox1");
    browser_->check("#checkbox3");
    
    std::string checkbox1_value = browser_->getAttribute("#checkbox1", "value");
    std::string checkbox3_value = browser_->getAttribute("#checkbox3", "value");
    
    EXPECT_EQ(checkbox1_value, "option1");
    EXPECT_EQ(checkbox3_value, "option3");
}

TEST_F(BrowserFormOperationsTest, CheckboxInteractionLogic_ErrorHandling) {
    loadTestFormPage();
    
    // Test operations on non-existent checkbox
    EXPECT_FALSE(browser_->check("#nonexistent-checkbox"));
    EXPECT_FALSE(browser_->uncheck("#nonexistent-checkbox"));
    EXPECT_FALSE(browser_->isChecked("#nonexistent-checkbox"));
    
    // Test operations on non-checkbox elements
    EXPECT_FALSE(browser_->check("#text-input"));
    EXPECT_FALSE(browser_->isChecked("#submit-btn"));
}

// ========== Radio Button Tests ==========

TEST_F(BrowserFormOperationsTest, RadioButtonGroupManagement_BasicOperations) {
    loadTestFormPage();
    
    // Test initial radio button state
    EXPECT_FALSE(browser_->isChecked("#radio1"));
    EXPECT_TRUE(browser_->isChecked("#radio2"));  // Initially checked
    EXPECT_FALSE(browser_->isChecked("#radio3"));
    
    // Test selecting different radio button
    browser_->check("#radio1");
    
    EXPECT_TRUE(browser_->isChecked("#radio1"));
    EXPECT_FALSE(browser_->isChecked("#radio2"));  // Should be unchecked
    EXPECT_FALSE(browser_->isChecked("#radio3"));
    
    // Test selecting another radio button
    browser_->check("#radio3");
    
    EXPECT_FALSE(browser_->isChecked("#radio1"));  // Should be unchecked
    EXPECT_FALSE(browser_->isChecked("#radio2"));
    EXPECT_TRUE(browser_->isChecked("#radio3"));
}

TEST_F(BrowserFormOperationsTest, RadioButtonGroupManagement_MutualExclusion) {
    loadTestFormPage();
    
    // Ensure only one radio button can be selected at a time
    browser_->check("#radio1");
    EXPECT_TRUE(browser_->isChecked("#radio1"));
    EXPECT_FALSE(browser_->isChecked("#radio2"));
    EXPECT_FALSE(browser_->isChecked("#radio3"));
    
    browser_->check("#radio2");
    EXPECT_FALSE(browser_->isChecked("#radio1"));
    EXPECT_TRUE(browser_->isChecked("#radio2"));
    EXPECT_FALSE(browser_->isChecked("#radio3"));
    
    browser_->check("#radio3");
    EXPECT_FALSE(browser_->isChecked("#radio1"));
    EXPECT_FALSE(browser_->isChecked("#radio2"));
    EXPECT_TRUE(browser_->isChecked("#radio3"));
}

TEST_F(BrowserFormOperationsTest, RadioButtonGroupManagement_ValueExtraction) {
    loadTestFormPage();
    
    // Test getting selected radio button value
    browser_->check("#radio1");
    std::string selected_value = browser_->getAttribute("#radio1", "value");
    EXPECT_EQ(selected_value, "choice1");
    
    browser_->check("#radio3");
    selected_value = browser_->getAttribute("#radio3", "value");
    EXPECT_EQ(selected_value, "choice3");
}

TEST_F(BrowserFormOperationsTest, RadioButtonGroupManagement_GroupValidation) {
    loadTestFormPage();
    
    // Test that radio buttons with same name attribute work as a group
    std::string name1 = browser_->getAttribute("#radio1", "name");
    std::string name2 = browser_->getAttribute("#radio2", "name");
    std::string name3 = browser_->getAttribute("#radio3", "name");
    
    EXPECT_EQ(name1, "radio-group");
    EXPECT_EQ(name2, "radio-group");
    EXPECT_EQ(name3, "radio-group");
}

// ========== Dropdown Selection Tests ==========

TEST_F(BrowserFormOperationsTest, DropdownSelectionValidation_BasicOperations) {
    loadTestFormPage();
    
    // Test initial dropdown value
    std::string initial_value = browser_->getValue("#dropdown1");
    EXPECT_EQ(initial_value, "option2");  // Initially selected
    
    // Test selecting different option
    browser_->selectOption("#dropdown1", "option1");
    std::string new_value = browser_->getValue("#dropdown1");
    EXPECT_EQ(new_value, "option1");
    
    // Test selecting by index
    browser_->selectOption("#dropdown1", 3);  // "option3"
    new_value = browser_->getValue("#dropdown1");
    EXPECT_EQ(new_value, "option3");
}

TEST_F(BrowserFormOperationsTest, DropdownSelectionValidation_MultipleSelection) {
    loadTestFormPage();
    
    // Test multiple select dropdown
    browser_->selectOption("#dropdown2", "multi1");
    browser_->selectOption("#dropdown2", "multi3");
    
    // Verify multiple selections
    std::vector<std::string> selected_values = browser_->getSelectedOptions("#dropdown2");
    EXPECT_GE(selected_values.size(), 1);
    
    // Check if specific option is selected
    bool multi1_selected = browser_->isOptionSelected("#dropdown2", "multi1");
    bool multi3_selected = browser_->isOptionSelected("#dropdown2", "multi3");
    
    EXPECT_TRUE(multi1_selected);
    EXPECT_TRUE(multi3_selected);
}

TEST_F(BrowserFormOperationsTest, DropdownSelectionValidation_OptionValidation) {
    loadTestFormPage();
    
    // Test selecting invalid option
    bool result = browser_->selectOption("#dropdown1", "nonexistent-option");
    EXPECT_FALSE(result);
    
    // Test selecting valid option
    result = browser_->selectOption("#dropdown1", "option1");
    EXPECT_TRUE(result);
    
    // Verify option exists
    bool option_exists = browser_->hasOption("#dropdown1", "option2");
    EXPECT_TRUE(option_exists);
    
    option_exists = browser_->hasOption("#dropdown1", "nonexistent");
    EXPECT_FALSE(option_exists);
}

TEST_F(BrowserFormOperationsTest, DropdownSelectionValidation_OptionCount) {
    loadTestFormPage();
    
    // Test counting options
    int option_count = browser_->getOptionCount("#dropdown1");
    EXPECT_EQ(option_count, 4);  // Including empty option
    
    int multi_option_count = browser_->getOptionCount("#dropdown2");
    EXPECT_EQ(multi_option_count, 4);
}

// ========== Form Submission Tests ==========

TEST_F(BrowserFormOperationsTest, FormSubmissionWorkflow_BasicSubmission) {
    loadTestFormPage();
    
    // Fill form fields
    browser_->type("#text-input", "test value");
    browser_->type("#email-input", "test@example.com");
    browser_->check("#checkbox1");
    browser_->check("#radio3");
    browser_->selectOption("#dropdown1", "option1");
    browser_->type("#textarea1", "Long text content");
    
    // Test form submission
    bool submitted = browser_->submitForm("#test-form");
    EXPECT_TRUE(submitted);
}

TEST_F(BrowserFormOperationsTest, FormSubmissionWorkflow_ValidationBeforeSubmit) {
    loadTestFormPage();
    
    // Test form validation before submission
    bool is_form_valid = browser_->isFormValid("#test-form");
    
    // Fill required fields if any
    browser_->type("#text-input", "required value");
    browser_->type("#email-input", "valid@email.com");
    
    // Test submission
    bool submitted = browser_->submitForm("#test-form");
    EXPECT_TRUE(submitted);
}

TEST_F(BrowserFormOperationsTest, FormSubmissionWorkflow_FormDataExtraction) {
    loadTestFormPage();
    
    // Fill form with test data
    browser_->type("#text-input", "test text");
    browser_->type("#password-input", "password123");
    browser_->type("#email-input", "user@test.com");
    browser_->check("#checkbox1");
    browser_->uncheck("#checkbox2");
    browser_->check("#radio1");
    browser_->selectOption("#dropdown1", "option3");
    browser_->type("#textarea1", "textarea content");
    
    // Extract form data
    std::map<std::string, std::string> form_data = browser_->getFormData("#test-form");
    
    EXPECT_EQ(form_data["text-field"], "test text");
    EXPECT_EQ(form_data["email-field"], "user@test.com");
    EXPECT_EQ(form_data["dropdown-field"], "option3");
    EXPECT_EQ(form_data["textarea-field"], "textarea content");
}

TEST_F(BrowserFormOperationsTest, FormSubmissionWorkflow_SubmitButtonHandling) {
    loadTestFormPage();
    
    // Test different ways to submit form
    browser_->type("#text-input", "submit test");
    
    // Submit by clicking submit button
    bool clicked = browser_->click("#submit-btn");
    EXPECT_TRUE(clicked);
    
    // Test form submission event
    bool form_submitted = browser_->isFormSubmitted("#test-form");
    // Note: This would depend on implementation to track submission state
}

// ========== Form Field Validation Tests ==========

TEST_F(BrowserFormOperationsTest, FormFieldValidation_InputTypes) {
    loadTestFormPage();
    
    // Test email field validation
    browser_->type("#email-input", "invalid-email");
    bool is_valid_email = browser_->isFieldValid("#email-input");
    EXPECT_FALSE(is_valid_email);
    
    browser_->type("#email-input", "valid@email.com");
    is_valid_email = browser_->isFieldValid("#email-input");
    EXPECT_TRUE(is_valid_email);
    
    // Test required field validation
    browser_->clearField("#text-input");
    bool is_text_valid = browser_->isFieldValid("#text-input");
    // Depends on whether field is marked as required
}

TEST_F(BrowserFormOperationsTest, FormFieldValidation_FieldStates) {
    loadTestFormPage();
    
    // Test field focus
    browser_->focus("#text-input");
    bool is_focused = browser_->hasFocus("#text-input");
    EXPECT_TRUE(is_focused);
    
    // Test field blur
    browser_->blur("#text-input");
    is_focused = browser_->hasFocus("#text-input");
    EXPECT_FALSE(is_focused);
    
    // Test field enabled/disabled state
    bool is_enabled = browser_->isFieldEnabled("#text-input");
    EXPECT_TRUE(is_enabled);
    
    browser_->disableField("#text-input");
    is_enabled = browser_->isFieldEnabled("#text-input");
    EXPECT_FALSE(is_enabled);
}

// ========== Form Reset Tests ==========

TEST_F(BrowserFormOperationsTest, FormResetFunctionality_BasicReset) {
    loadTestFormPage();
    
    // Fill form with data
    browser_->type("#text-input", "test data");
    browser_->check("#checkbox1");
    browser_->uncheck("#checkbox2");
    browser_->check("#radio1");
    browser_->selectOption("#dropdown1", "option1");
    
    // Reset form
    bool reset_successful = browser_->resetForm("#test-form");
    EXPECT_TRUE(reset_successful);
    
    // Verify fields are reset to initial state
    std::string text_value = browser_->getValue("#text-input");
    EXPECT_TRUE(text_value.empty());
    
    EXPECT_FALSE(browser_->isChecked("#checkbox1"));
    EXPECT_TRUE(browser_->isChecked("#checkbox2"));   // Back to initial state
    EXPECT_FALSE(browser_->isChecked("#radio1"));
    EXPECT_TRUE(browser_->isChecked("#radio2"));      // Back to initial state
    
    std::string dropdown_value = browser_->getValue("#dropdown1");
    EXPECT_EQ(dropdown_value, "option2");  // Back to initial state
}

TEST_F(BrowserFormOperationsTest, FormResetFunctionality_ResetButton) {
    loadTestFormPage();
    
    // Fill form
    browser_->type("#text-input", "data to reset");
    browser_->check("#checkbox3");
    
    // Click reset button
    browser_->click("#reset-btn");
    
    // Verify reset occurred
    std::string text_value = browser_->getValue("#text-input");
    EXPECT_TRUE(text_value.empty());
}

// ========== Multiple Form Handling Tests ==========

TEST_F(BrowserFormOperationsTest, MultipleFormHandling_FormIdentification) {
    loadTestFormPage();
    
    // Test identifying different forms
    bool form1_exists = browser_->elementExists("#test-form");
    bool form2_exists = browser_->elementExists("#second-form");
    
    EXPECT_TRUE(form1_exists);
    EXPECT_TRUE(form2_exists);
    
    // Test form count
    int form_count = browser_->countElements("form");
    EXPECT_EQ(form_count, 2);
}

TEST_F(BrowserFormOperationsTest, MultipleFormHandling_IndependentOperation) {
    loadTestFormPage();
    
    // Fill first form
    browser_->type("#text-input", "form1 data");
    browser_->check("#checkbox1");
    
    // Fill second form  
    browser_->type("#second-text", "form2 data");
    
    // Verify forms maintain independent data
    std::string form1_data = browser_->getValue("#text-input");
    std::string form2_data = browser_->getValue("#second-text");
    
    EXPECT_EQ(form1_data, "form1 data");
    EXPECT_EQ(form2_data, "form2 data");
    
    // Reset one form shouldn't affect the other
    browser_->resetForm("#test-form");
    
    std::string form1_after_reset = browser_->getValue("#text-input");
    std::string form2_after_reset = browser_->getValue("#second-text");
    
    EXPECT_TRUE(form1_after_reset.empty());
    EXPECT_EQ(form2_after_reset, "form2 data");
}

// ========== Focus Management Tests ==========

TEST_F(BrowserFormOperationsTest, FormElementFocusManagement_TabOrder) {
    loadTestFormPage();
    
    // Test tab navigation through form elements
    browser_->focus("#text-input");
    EXPECT_TRUE(browser_->hasFocus("#text-input"));
    
    browser_->simulateTab();
    // Next focusable element should be focused
    
    browser_->simulateShiftTab();
    // Should go back to previous element
}

TEST_F(BrowserFormOperationsTest, FormElementFocusManagement_FocusEvents) {
    loadTestFormPage();
    
    // Test focus event handling
    browser_->focus("#text-input");
    bool focus_event_fired = browser_->wasEventFired("#text-input", "focus");
    EXPECT_TRUE(focus_event_fired);
    
    browser_->blur("#text-input");
    bool blur_event_fired = browser_->wasEventFired("#text-input", "blur");
    EXPECT_TRUE(blur_event_fired);
}

// ========== Error Handling Tests ==========

TEST_F(BrowserFormOperationsTest, FormErrorHandling_InvalidSelectors) {
    loadTestFormPage();
    
    // Test operations with invalid selectors
    EXPECT_FALSE(browser_->type("#nonexistent", "test"));
    EXPECT_FALSE(browser_->check("#invalid-checkbox"));
    EXPECT_FALSE(browser_->selectOption("#invalid-dropdown", "option"));
    EXPECT_FALSE(browser_->submitForm("#invalid-form"));
    EXPECT_FALSE(browser_->resetForm("#invalid-form"));
}

TEST_F(BrowserFormOperationsTest, FormErrorHandling_WrongElementTypes) {
    loadTestFormPage();
    
    // Test checkbox operations on non-checkbox elements
    EXPECT_FALSE(browser_->check("#text-input"));
    EXPECT_FALSE(browser_->isChecked("#dropdown1"));
    
    // Test dropdown operations on non-select elements
    EXPECT_FALSE(browser_->selectOption("#text-input", "value"));
    
    // Test typing on non-input elements
    EXPECT_FALSE(browser_->type("#submit-btn", "text"));
}

TEST_F(BrowserFormOperationsTest, FormErrorHandling_FormStateErrors) {
    loadTestFormPage();
    
    // Test operations on disabled elements
    browser_->disableField("#text-input");
    bool typing_successful = browser_->type("#text-input", "disabled test");
    EXPECT_FALSE(typing_successful);
    
    // Test submission of invalid form
    browser_->type("#email-input", "invalid-email-format");
    bool submission_successful = browser_->submitForm("#test-form");
    // Depends on browser validation behavior
}

} // namespace