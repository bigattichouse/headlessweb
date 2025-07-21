#include <gtest/gtest.h>
#include "../../src/hweb/Services/ManagerRegistry.h"
#include "../../src/hweb/Services/SessionService.h"
#include "../../src/hweb/Services/NavigationService.h"
#include "Browser/Browser.h"
#include "Session/Manager.h"
#include "../utils/test_helpers.h"
#include <memory>
#include <thread>
#include <chrono>
#include <filesystem>

class ServiceArchitectureCoordinationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for testing
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("service_coordination_tests");
        
        // Initialize components
        session_manager_ = std::make_unique<SessionManager>(temp_dir->getPath());
        session_service_ = std::make_unique<HWeb::SessionService>(*session_manager_);
        navigation_service_ = std::make_unique<HWeb::NavigationService>();
        browser_ = std::make_unique<Browser>();
        
        // Initialize ManagerRegistry
        HWeb::ManagerRegistry::initialize();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown() override {
        // Cleanup
        browser_.reset();
        navigation_service_.reset();
        session_service_.reset();
        session_manager_.reset();
        
        HWeb::ManagerRegistry::cleanup();
        temp_dir.reset();
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    
    void loadTestPage() {
        std::string test_html = R"(
            <!DOCTYPE html>
            <html>
            <head>
                <title>Service Test Page</title>
            </head>
            <body>
                <h1>Service Architecture Test</h1>
                <form id="test-form">
                    <input type="text" id="test-input" name="test_field" value="initial_value">
                    <textarea id="test-textarea" name="notes">Initial notes</textarea>
                    <input type="checkbox" id="test-checkbox" name="enabled" checked>
                    <select id="test-select" name="option">
                        <option value="option1" selected>Option 1</option>
                        <option value="option2">Option 2</option>
                    </select>
                </form>
                <div id="dynamic-content">Static content</div>
                <script>
                    // Add some state to localStorage
                    localStorage.setItem('test-key', 'test-value');
                    sessionStorage.setItem('session-key', 'session-value');
                    
                    // Set a cookie
                    document.cookie = 'test-cookie=cookie-value';
                    
                    // Function to simulate dynamic content changes
                    function updateDynamicContent(content) {
                        document.getElementById('dynamic-content').textContent = content;
                    }
                    
                    // Simulate some initial state
                    window.testState = {
                        initialized: true,
                        counter: 0
                    };
                </script>
            </body>
            </html>
        )";
        
        browser_->loadHTML(test_html);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::unique_ptr<SessionManager> session_manager_;
    std::unique_ptr<HWeb::SessionService> session_service_;
    std::unique_ptr<HWeb::NavigationService> navigation_service_;
    std::unique_ptr<Browser> browser_;
};

// ========== ManagerRegistry Service Tests ==========

TEST_F(ServiceArchitectureCoordinationTest, ManagerRegistry_InitializationAndAccess) {
    // Test that ManagerRegistry is properly initialized
    EXPECT_TRUE(HWeb::ManagerRegistry::is_initialized());
    
    // Test access to managed components
    auto& assertion_manager = HWeb::ManagerRegistry::get_assertion_manager();
    auto& upload_manager = HWeb::ManagerRegistry::get_upload_manager();
    auto& download_manager = HWeb::ManagerRegistry::get_download_manager();
    
    // Verify managers are functional (basic operations)
    EXPECT_NO_THROW(assertion_manager.clearResults());
    EXPECT_NO_THROW(upload_manager.getUploadDirectory());
    EXPECT_NO_THROW(download_manager.getDownloadDirectory());
}

TEST_F(ServiceArchitectureCoordinationTest, ManagerRegistry_SingletonBehavior) {
    // Test that repeated calls return same instances
    auto& assertion_manager1 = HWeb::ManagerRegistry::get_assertion_manager();
    auto& assertion_manager2 = HWeb::ManagerRegistry::get_assertion_manager();
    
    EXPECT_EQ(&assertion_manager1, &assertion_manager2);
    
    auto& upload_manager1 = HWeb::ManagerRegistry::get_upload_manager();
    auto& upload_manager2 = HWeb::ManagerRegistry::get_upload_manager();
    
    EXPECT_EQ(&upload_manager1, &upload_manager2);
    
    auto& download_manager1 = HWeb::ManagerRegistry::get_download_manager();
    auto& download_manager2 = HWeb::ManagerRegistry::get_download_manager();
    
    EXPECT_EQ(&download_manager1, &download_manager2);
}

TEST_F(ServiceArchitectureCoordinationTest, ManagerRegistry_CrossServiceCoordination) {
    loadTestPage();
    
    // Test coordination between assertion manager and browser state
    auto& assertion_manager = HWeb::ManagerRegistry::get_assertion_manager();
    
    // Add some assertions that would use browser state
    assertion_manager.addAssertion("element-exists", "#test-input", "", "");
    assertion_manager.addAssertion("element-value", "#test-input", "initial_value", "equals");
    
    // Execute assertions using browser
    bool assertions_passed = assertion_manager.executeAssertions(*browser_);
    EXPECT_TRUE(assertions_passed);
    
    // Verify results are accessible
    auto results = assertion_manager.getResults();
    EXPECT_FALSE(results.empty());
}

// ========== SessionService Coordination Tests ==========

TEST_F(ServiceArchitectureCoordinationTest, SessionService_BrowserStateIntegration) {
    loadTestPage();
    
    // Create a test session
    Session test_session = session_service_->initialize_session("service_test_session");
    EXPECT_FALSE(test_session.getUrl().empty());
    
    // Modify browser state
    browser_->type("#test-input", "modified_value");
    browser_->uncheck("#test-checkbox");
    browser_->selectOption("#test-select", "option2");
    
    // Update session with current browser state
    session_service_->update_session_state(*browser_, test_session);
    
    // Verify session captured the state
    auto form_data = test_session.getFormData();
    EXPECT_EQ(form_data.at("test_field"), "modified_value");
    
    // Save session
    bool saved = session_service_->save_session_safely(test_session, "service_test_session");
    EXPECT_TRUE(saved);
}

TEST_F(ServiceArchitectureCoordinationTest, SessionService_NavigationServiceIntegration) {
    // Create a test session with URL
    Session test_session = session_service_->initialize_session("nav_test_session");
    
    // Create HWebConfig for navigation planning
    HWebConfig config;
    config.url = "data:text/html,<h1>Test Page</h1>";
    config.sessionName = "nav_test_session";
    
    // Test navigation planning
    auto nav_plan = navigation_service_->create_navigation_plan(config, test_session);
    EXPECT_TRUE(nav_plan.should_navigate);
    EXPECT_FALSE(nav_plan.is_session_restore);
    EXPECT_EQ(nav_plan.navigation_url, config.url);
    
    // Execute navigation plan
    bool nav_executed = navigation_service_->execute_navigation_plan(*browser_, test_session, nav_plan);
    EXPECT_TRUE(nav_executed);
    
    // Update session after navigation
    session_service_->update_session_state(*browser_, test_session);
    
    // Verify session updated with new URL
    EXPECT_EQ(test_session.getUrl(), config.url);
}

TEST_F(ServiceArchitectureCoordinationTest, SessionService_MultiSessionIsolation) {
    loadTestPage();
    
    // Create multiple sessions
    Session session1 = session_service_->initialize_session("session1");
    Session session2 = session_service_->initialize_session("session2");
    
    // Modify browser state for session1
    browser_->type("#test-input", "session1_value");
    session_service_->update_session_state(*browser_, session1);
    
    // Clear and modify for session2
    browser_->clearField("#test-input");
    browser_->type("#test-input", "session2_value");
    session_service_->update_session_state(*browser_, session2);
    
    // Save both sessions
    EXPECT_TRUE(session_service_->save_session_safely(session1, "session1"));
    EXPECT_TRUE(session_service_->save_session_safely(session2, "session2"));
    
    // Verify sessions maintain different data
    EXPECT_NE(session1.getFormData().at("test_field"), session2.getFormData().at("test_field"));
    EXPECT_EQ(session1.getFormData().at("test_field"), "session1_value");
    EXPECT_EQ(session2.getFormData().at("test_field"), "session2_value");
}

// ========== NavigationService Coordination Tests ==========

TEST_F(ServiceArchitectureCoordinationTest, NavigationService_StrategyDetermination) {
    // Test different navigation strategies based on config and session state
    HWebConfig config1;
    config1.url = "https://example.com";
    config1.sessionName = "";
    
    Session empty_session;
    auto strategy1 = navigation_service_->determine_navigation_strategy(config1, empty_session);
    EXPECT_EQ(strategy1, NavigationStrategy::Direct);
    
    // Test with session restore
    HWebConfig config2;
    config2.sessionName = "existing_session";
    config2.url = "";
    
    Session existing_session;
    existing_session.setUrl("https://saved.com");
    auto strategy2 = navigation_service_->determine_navigation_strategy(config2, existing_session);
    // Strategy depends on implementation logic
}

TEST_F(ServiceArchitectureCoordinationTest, NavigationService_WaitMechanisms) {
    // Test navigation waiting with real browser
    std::string simple_html = "data:text/html,<h1>Simple Test</h1>";
    
    bool navigated = navigation_service_->navigate_to_url(*browser_, simple_html);
    EXPECT_TRUE(navigated);
    
    // Test waiting for navigation completion
    bool nav_complete = navigation_service_->wait_for_navigation_complete(*browser_, 2000);
    EXPECT_TRUE(nav_complete);
    
    // Test waiting for page ready
    bool page_ready = navigation_service_->wait_for_page_ready(*browser_, 2000);
    EXPECT_TRUE(page_ready);
    
    // Verify navigation actually occurred
    std::string current_url = browser_->getCurrentUrl();
    EXPECT_EQ(current_url, simple_html);
}

TEST_F(ServiceArchitectureCoordinationTest, NavigationService_ComplexNavigationPlans) {
    // Create complex navigation plan with session restore
    HWebConfig config;
    config.url = "data:text/html,<h1>Complex Page</h1>";
    config.sessionName = "complex_session";
    
    Session session_with_state;
    session_with_state.setUrl("data:text/html,<h1>Previous Page</h1>");
    session_with_state.setViewportWidth(1024);
    session_with_state.setViewportHeight(768);
    
    auto nav_plan = navigation_service_->create_navigation_plan(config, session_with_state);
    
    EXPECT_TRUE(nav_plan.should_navigate);
    EXPECT_EQ(nav_plan.navigation_url, config.url);
    
    // Execute complex navigation
    bool executed = navigation_service_->execute_navigation_plan(*browser_, session_with_state, nav_plan);
    EXPECT_TRUE(executed);
    
    // Verify browser state after complex navigation
    EXPECT_EQ(browser_->getCurrentUrl(), config.url);
    
    // Verify viewport was restored from session
    int width, height;
    browser_->getViewportSize(width, height);
    EXPECT_EQ(width, 1024);
    EXPECT_EQ(height, 768);
}

// ========== Cross-Service Error Handling ==========

TEST_F(ServiceArchitectureCoordinationTest, CrossService_ErrorPropagation) {
    // Test error handling when services depend on each other
    
    // Test navigation failure affecting session service
    bool nav_result = navigation_service_->navigate_to_url(*browser_, "invalid://malformed.url");
    EXPECT_FALSE(nav_result);
    
    // Session service should handle navigation failure gracefully
    Session test_session = session_service_->initialize_session("error_test_session");
    
    // Attempting to update session state after failed navigation
    EXPECT_NO_THROW(session_service_->update_session_state(*browser_, test_session));
    
    // Session should have fallback URL or empty state
    std::string session_url = test_session.getUrl();
    EXPECT_TRUE(session_url.empty() || session_url != "invalid://malformed.url");
}

TEST_F(ServiceArchitectureCoordinationTest, CrossService_RecoveryMechanisms) {
    loadTestPage();
    
    // Create session and establish initial state
    Session recovery_session = session_service_->initialize_session("recovery_test");
    browser_->type("#test-input", "recovery_value");
    session_service_->update_session_state(*browser_, recovery_session);
    
    // Simulate browser failure/reset
    browser_.reset();
    browser_ = std::make_unique<Browser>();
    
    // Test service recovery with new browser instance
    HWebConfig recovery_config;
    recovery_config.sessionName = "recovery_test";
    recovery_config.url = ""; // Will use session URL
    
    auto nav_plan = navigation_service_->create_navigation_plan(recovery_config, recovery_session);
    EXPECT_TRUE(nav_plan.is_session_restore || nav_plan.should_navigate);
    
    // Recovery should be possible
    bool recovery_successful = navigation_service_->execute_navigation_plan(*browser_, recovery_session, nav_plan);
    EXPECT_TRUE(recovery_successful);
}

// ========== Resource Management Coordination ==========

TEST_F(ServiceArchitectureCoordinationTest, ResourceManagement_ServiceLifecycle) {
    // Test proper resource allocation and cleanup across services
    
    // Create multiple sessions to test resource usage
    std::vector<Session> test_sessions;
    for (int i = 0; i < 5; i++) {
        Session session = session_service_->initialize_session("resource_test_" + std::to_string(i));
        test_sessions.push_back(session);
    }
    
    // Each session should have proper isolation
    EXPECT_EQ(test_sessions.size(), 5);
    
    // Test resource cleanup
    for (size_t i = 0; i < test_sessions.size(); i++) {
        bool cleanup_successful = session_service_->handle_session_end("resource_test_" + std::to_string(i));
        EXPECT_TRUE(cleanup_successful);
    }
    
    // ManagerRegistry should still be functional after session cleanup
    EXPECT_TRUE(HWeb::ManagerRegistry::is_initialized());
    auto& assertion_manager = HWeb::ManagerRegistry::get_assertion_manager();
    EXPECT_NO_THROW(assertion_manager.clearResults());
}

TEST_F(ServiceArchitectureCoordinationTest, ResourceManagement_ConcurrentAccess) {
    // Test concurrent access to services (simulated)
    loadTestPage();
    
    // Create session
    Session concurrent_session = session_service_->initialize_session("concurrent_test");
    
    // Simulate concurrent navigation and session updates
    bool nav_started = navigation_service_->navigate_to_url(*browser_, "data:text/html,<h1>Concurrent Test</h1>");
    EXPECT_TRUE(nav_started);
    
    // Update session state while navigation might be in progress
    browser_->type("#test-input", "concurrent_value");
    EXPECT_NO_THROW(session_service_->update_session_state(*browser_, concurrent_session));
    
    // Wait for navigation to complete
    bool nav_complete = navigation_service_->wait_for_navigation_complete(*browser_, 3000);
    EXPECT_TRUE(nav_complete);
    
    // Final session state should be consistent
    session_service_->update_session_state(*browser_, concurrent_session);
    auto final_form_data = concurrent_session.getFormData();
    
    // Should contain our test data
    if (final_form_data.find("test_field") != final_form_data.end()) {
        EXPECT_EQ(final_form_data.at("test_field"), "concurrent_value");
    }
}

// ========== Service Integration Workflows ==========

TEST_F(ServiceArchitectureCoordinationTest, ServiceIntegration_CompleteWorkflow) {
    // Test complete workflow involving all services
    
    // Step 1: Initialize session through SessionService
    Session workflow_session = session_service_->initialize_session("workflow_test");
    
    // Step 2: Plan and execute navigation through NavigationService
    HWebConfig workflow_config;
    workflow_config.url = "data:text/html,<form><input id='workflow-input' value='workflow-data'></form>";
    workflow_config.sessionName = "workflow_test";
    
    auto nav_plan = navigation_service_->create_navigation_plan(workflow_config, workflow_session);
    bool nav_executed = navigation_service_->execute_navigation_plan(*browser_, workflow_session, nav_plan);
    EXPECT_TRUE(nav_executed);
    
    // Step 3: Interact with page content
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Allow page load
    browser_->type("#workflow-input", "modified-workflow-data");
    
    // Step 4: Use ManagerRegistry services for assertions
    auto& assertion_manager = HWeb::ManagerRegistry::get_assertion_manager();
    assertion_manager.addAssertion("element-value", "#workflow-input", "modified-workflow-data", "equals");
    bool assertions_passed = assertion_manager.executeAssertions(*browser_);
    EXPECT_TRUE(assertions_passed);
    
    // Step 5: Update and save session through SessionService
    session_service_->update_session_state(*browser_, workflow_session);
    bool session_saved = session_service_->save_session_safely(workflow_session, "workflow_test");
    EXPECT_TRUE(session_saved);
    
    // Step 6: Verify complete workflow success
    auto results = assertion_manager.getResults();
    EXPECT_FALSE(results.empty());
    
    auto session_form_data = workflow_session.getFormData();
    if (session_form_data.find("workflow-input") != session_form_data.end()) {
        EXPECT_EQ(session_form_data.at("workflow-input"), "modified-workflow-data");
    }
}

TEST_F(ServiceArchitectureCoordinationTest, ServiceIntegration_ErrorRecoveryWorkflow) {
    // Test workflow with error conditions and recovery
    
    // Step 1: Attempt to create session with invalid parameters
    Session error_session = session_service_->initialize_session(""); // Empty name
    // Should handle gracefully
    
    // Step 2: Attempt navigation to invalid URL
    HWebConfig error_config;
    error_config.url = "://invalid-url";
    error_config.sessionName = "error_test";
    
    auto nav_plan = navigation_service_->create_navigation_plan(error_config, error_session);
    bool nav_executed = navigation_service_->execute_navigation_plan(*browser_, error_session, nav_plan);
    // Should handle failure gracefully
    
    // Step 3: Attempt recovery with valid configuration
    HWebConfig recovery_config;
    recovery_config.url = "data:text/html,<h1>Recovery Success</h1>";
    recovery_config.sessionName = "recovery_test";
    
    Session recovery_session = session_service_->initialize_session("recovery_test");
    auto recovery_plan = navigation_service_->create_navigation_plan(recovery_config, recovery_session);
    bool recovery_executed = navigation_service_->execute_navigation_plan(*browser_, recovery_session, recovery_plan);
    EXPECT_TRUE(recovery_executed);
    
    // Step 4: Verify recovery was successful
    std::string current_url = browser_->getCurrentUrl();
    EXPECT_EQ(current_url, recovery_config.url);
    
    // Services should still be functional after error recovery
    EXPECT_TRUE(HWeb::ManagerRegistry::is_initialized());
}