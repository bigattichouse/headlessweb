#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../src/hweb/Commands/Executor.h"
#include "../../src/hweb/Config.h"
#include "../../src/Browser/Browser.h"
#include "../../src/Session/Session.h"
#include "../../src/Session/Manager.h"
#include <memory>
#include <vector>
#include <filesystem>
#include <fstream>

using namespace HWeb;
using ::testing::_;
using ::testing::Return;
using ::testing::Throw;
using ::testing::InSequence;
using ::testing::AtLeast;

class MockBrowser : public Browser {
public:
    MOCK_METHOD(bool, navigate, (const std::string& url), (override));
    MOCK_METHOD(bool, click, (const std::string& selector), (override));
    MOCK_METHOD(bool, type, (const std::string& selector, const std::string& text), (override));
    MOCK_METHOD(std::string, getValue, (const std::string& selector), (const override));
    MOCK_METHOD(bool, waitForElement, (const std::string& selector, int timeout), (override));
    MOCK_METHOD(void, takeScreenshot, (const std::string& filename), (override));
    MOCK_METHOD(bool, executeJavascript, (const std::string& script), (override));
    MOCK_METHOD(void, updateSessionState, (Session& session), (override));
    MOCK_METHOD(void, restoreSessionState, (const Session& session), (override));
    MOCK_METHOD(std::string, getCurrentUrl, (), (const override));
    MOCK_METHOD(std::string, getTitle, (), (const override));
    MOCK_METHOD(bool, elementExists, (const std::string& selector), (const override));
    MOCK_METHOD(bool, assertElement, (const std::string& selector, const std::string& expected), (override));
};

class MockSessionManager : public SessionManager {
public:
    MOCK_METHOD(Session, loadOrCreateSession, (const std::string& sessionName), (override));
    MOCK_METHOD(bool, saveSession, (const Session& session), (override));
    MOCK_METHOD(std::vector<SessionInfo>, listSessions, (), (const override));
    MOCK_METHOD(bool, deleteSession, (const std::string& sessionName), (override));
    MOCK_METHOD(bool, sessionExists, (const std::string& sessionName), (const override));
};

class CommandWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp directory for test data
        temp_dir_ = std::filesystem::temp_directory_path() / "command_workflow_test";
        std::filesystem::create_directories(temp_dir_);
        
        // Initialize mock objects
        mock_browser_ = std::make_shared<MockBrowser>();
        mock_session_manager_ = std::make_unique<MockSessionManager>();
        
        // Create command executor
        command_executor_ = std::make_unique<CommandExecutor>(*mock_browser_, *mock_session_manager_);
        
        // Setup default session
        default_session_.name = "test-workflow-session";
        default_session_.url = "https://example.com";
        default_session_.title = "Test Workflow Page";
        default_session_.timestamp = std::chrono::system_clock::now();
    }
    
    void TearDown() override {
        // Cleanup
        command_executor_.reset();
        mock_session_manager_.reset();
        mock_browser_.reset();
        
        // Remove temp directory
        std::filesystem::remove_all(temp_dir_);
    }
    
    // Helper to create test commands
    std::vector<Command> createBasicWorkflow() {
        std::vector<Command> commands;
        
        Command nav_cmd;
        nav_cmd.type = CommandType::NAVIGATE;
        nav_cmd.url = "https://example.com/form";
        commands.push_back(nav_cmd);
        
        Command type_cmd;
        type_cmd.type = CommandType::TYPE;
        type_cmd.selector = "#username";
        type_cmd.text = "testuser";
        commands.push_back(type_cmd);
        
        Command click_cmd;
        click_cmd.type = CommandType::CLICK;
        click_cmd.selector = "#submit-btn";
        commands.push_back(click_cmd);
        
        return commands;
    }
    
    std::vector<Command> createComplexWorkflow() {
        std::vector<Command> commands;
        
        // Navigation
        Command nav_cmd;
        nav_cmd.type = CommandType::NAVIGATE;
        nav_cmd.url = "https://app.example.com/login";
        commands.push_back(nav_cmd);
        
        // Wait for page load
        Command wait_cmd;
        wait_cmd.type = CommandType::WAIT;
        wait_cmd.selector = "#login-form";
        wait_cmd.timeout = 5000;
        commands.push_back(wait_cmd);
        
        // Fill login form
        Command username_cmd;
        username_cmd.type = CommandType::TYPE;
        username_cmd.selector = "#username";
        username_cmd.text = "workflow@test.com";
        commands.push_back(username_cmd);
        
        Command password_cmd;
        password_cmd.type = CommandType::TYPE;
        password_cmd.selector = "#password";
        password_cmd.text = "testpassword123";
        commands.push_back(password_cmd);
        
        // Submit login
        Command submit_cmd;
        submit_cmd.type = CommandType::CLICK;
        submit_cmd.selector = "#login-submit";
        commands.push_back(submit_cmd);
        
        // Wait for dashboard
        Command dashboard_wait;
        dashboard_wait.type = CommandType::WAIT;
        dashboard_wait.selector = ".dashboard";
        dashboard_wait.timeout = 10000;
        commands.push_back(dashboard_wait);
        
        // Take screenshot
        Command screenshot_cmd;
        screenshot_cmd.type = CommandType::SCREENSHOT;
        screenshot_cmd.filename = (temp_dir_ / "workflow_result.png").string();
        commands.push_back(screenshot_cmd);
        
        // Assert success
        Command assert_cmd;
        assert_cmd.type = CommandType::ASSERT;
        assert_cmd.selector = ".welcome-message";
        assert_cmd.expected = "Welcome, workflow@test.com";
        commands.push_back(assert_cmd);
        
        return commands;
    }

    std::filesystem::path temp_dir_;
    std::shared_ptr<MockBrowser> mock_browser_;
    std::unique_ptr<MockSessionManager> mock_session_manager_;
    std::unique_ptr<CommandExecutor> command_executor_;
    Session default_session_;
};

// ========== Command Chaining Tests ==========

TEST_F(CommandWorkflowTest, CommandChainingSequence_BasicSequence) {
    auto commands = createBasicWorkflow();
    
    InSequence seq;
    
    // Expect commands in order
    EXPECT_CALL(*mock_browser_, navigate("https://example.com/form"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, type("#username", "testuser"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, click("#submit-btn"))
        .WillOnce(Return(true));
    
    // Execute workflow
    bool result = command_executor_->executeWorkflow(commands, default_session_);
    EXPECT_TRUE(result);
}

TEST_F(CommandWorkflowTest, CommandChainingSequence_ComplexSequence) {
    auto commands = createComplexWorkflow();
    
    InSequence seq;
    
    // Setup expectations for complex workflow
    EXPECT_CALL(*mock_browser_, navigate("https://app.example.com/login"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, waitForElement("#login-form", 5000))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, type("#username", "workflow@test.com"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, type("#password", "testpassword123"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, click("#login-submit"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, waitForElement(".dashboard", 10000))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, takeScreenshot(_))
        .Times(1);
    
    EXPECT_CALL(*mock_browser_, assertElement(".welcome-message", "Welcome, workflow@test.com"))
        .WillOnce(Return(true));
    
    // Execute complex workflow
    bool result = command_executor_->executeWorkflow(commands, default_session_);
    EXPECT_TRUE(result);
}

TEST_F(CommandWorkflowTest, CommandChainingSequence_ConditionalExecution) {
    std::vector<Command> conditional_commands;
    
    // Navigation command
    Command nav_cmd;
    nav_cmd.type = CommandType::NAVIGATE;
    nav_cmd.url = "https://conditional.example.com";
    conditional_commands.push_back(nav_cmd);
    
    // Conditional command - only execute if element exists
    Command conditional_cmd;
    conditional_cmd.type = CommandType::CONDITIONAL;
    conditional_cmd.selector = "#optional-element";
    conditional_cmd.condition = "exists";
    conditional_commands.push_back(conditional_cmd);
    
    Command action_cmd;
    action_cmd.type = CommandType::CLICK;
    action_cmd.selector = "#conditional-button";
    conditional_commands.push_back(action_cmd);
    
    InSequence seq;
    
    EXPECT_CALL(*mock_browser_, navigate("https://conditional.example.com"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, elementExists("#optional-element"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, click("#conditional-button"))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflow(conditional_commands, default_session_);
    EXPECT_TRUE(result);
}

TEST_F(CommandWorkflowTest, CommandChainingSequence_ParameterPassing) {
    std::vector<Command> param_commands;
    
    // Extract value command
    Command extract_cmd;
    extract_cmd.type = CommandType::EXTRACT;
    extract_cmd.selector = "#extracted-value";
    extract_cmd.variable_name = "extracted_data";
    param_commands.push_back(extract_cmd);
    
    // Use extracted value
    Command use_cmd;
    use_cmd.type = CommandType::TYPE;
    use_cmd.selector = "#target-field";
    use_cmd.text = "{{extracted_data}}";  // Variable substitution
    param_commands.push_back(use_cmd);
    
    EXPECT_CALL(*mock_browser_, getValue("#extracted-value"))
        .WillOnce(Return("test_extracted_value"));
    
    EXPECT_CALL(*mock_browser_, type("#target-field", "test_extracted_value"))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflow(param_commands, default_session_);
    EXPECT_TRUE(result);
}

// ========== Error Recovery Tests ==========

TEST_F(CommandWorkflowTest, ErrorRecoveryBetweenCommands_SingleCommandFailure) {
    auto commands = createBasicWorkflow();
    
    InSequence seq;
    
    // First command succeeds
    EXPECT_CALL(*mock_browser_, navigate("https://example.com/form"))
        .WillOnce(Return(true));
    
    // Second command fails
    EXPECT_CALL(*mock_browser_, type("#username", "testuser"))
        .WillOnce(Return(false));
    
    // Third command should not be executed due to failure
    EXPECT_CALL(*mock_browser_, click("#submit-btn"))
        .Times(0);
    
    bool result = command_executor_->executeWorkflow(commands, default_session_);
    EXPECT_FALSE(result);
}

TEST_F(CommandWorkflowTest, ErrorRecoveryBetweenCommands_RetryMechanism) {
    std::vector<Command> retry_commands;
    
    Command retry_cmd;
    retry_cmd.type = CommandType::CLICK;
    retry_cmd.selector = "#unreliable-button";
    retry_cmd.max_retries = 3;
    retry_cmd.retry_delay_ms = 100;
    retry_commands.push_back(retry_cmd);
    
    // Fail first two attempts, succeed on third
    EXPECT_CALL(*mock_browser_, click("#unreliable-button"))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflow(retry_commands, default_session_);
    EXPECT_TRUE(result);
}

TEST_F(CommandWorkflowTest, ErrorRecoveryBetweenCommands_ContinueOnError) {
    std::vector<Command> continue_commands;
    
    Command nav_cmd;
    nav_cmd.type = CommandType::NAVIGATE;
    nav_cmd.url = "https://example.com";
    continue_commands.push_back(nav_cmd);
    
    Command optional_cmd;
    optional_cmd.type = CommandType::CLICK;
    optional_cmd.selector = "#optional-element";
    optional_cmd.continue_on_error = true;
    continue_commands.push_back(optional_cmd);
    
    Command final_cmd;
    final_cmd.type = CommandType::SCREENSHOT;
    final_cmd.filename = (temp_dir_ / "continue_on_error.png").string();
    continue_commands.push_back(final_cmd);
    
    InSequence seq;
    
    EXPECT_CALL(*mock_browser_, navigate("https://example.com"))
        .WillOnce(Return(true));
    
    // This command fails but workflow continues
    EXPECT_CALL(*mock_browser_, click("#optional-element"))
        .WillOnce(Return(false));
    
    // Final command still executes
    EXPECT_CALL(*mock_browser_, takeScreenshot(_))
        .Times(1);
    
    bool result = command_executor_->executeWorkflow(continue_commands, default_session_);
    EXPECT_TRUE(result);
}

TEST_F(CommandWorkflowTest, ErrorRecoveryBetweenCommands_ErrorHandlerCommands) {
    std::vector<Command> error_handler_commands;
    
    Command main_cmd;
    main_cmd.type = CommandType::CLICK;
    main_cmd.selector = "#main-action";
    error_handler_commands.push_back(main_cmd);
    
    Command error_handler;
    error_handler.type = CommandType::ON_ERROR;
    error_handler.error_action = "screenshot";
    error_handler.filename = (temp_dir_ / "error_screenshot.png").string();
    error_handler_commands.push_back(error_handler);
    
    Command cleanup_cmd;
    cleanup_cmd.type = CommandType::NAVIGATE;
    cleanup_cmd.url = "https://example.com/safe-page";
    error_handler_commands.push_back(cleanup_cmd);
    
    // Main command fails
    EXPECT_CALL(*mock_browser_, click("#main-action"))
        .WillOnce(Return(false));
    
    // Error handler executes
    EXPECT_CALL(*mock_browser_, takeScreenshot(_))
        .Times(1);
    
    // Cleanup command executes
    EXPECT_CALL(*mock_browser_, navigate("https://example.com/safe-page"))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflow(error_handler_commands, default_session_);
    EXPECT_TRUE(result);  // Should succeed due to error handling
}

// ========== State Persistence Tests ==========

TEST_F(CommandWorkflowTest, StatePersistenceAcrossOperations_SessionStateUpdates) {
    auto commands = createBasicWorkflow();
    
    // Expect session state updates during workflow
    EXPECT_CALL(*mock_session_manager_, saveSession(_))
        .Times(AtLeast(1));
    
    EXPECT_CALL(*mock_browser_, updateSessionState(_))
        .Times(AtLeast(1));
    
    EXPECT_CALL(*mock_browser_, navigate("https://example.com/form"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, type("#username", "testuser"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, click("#submit-btn"))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflow(commands, default_session_);
    EXPECT_TRUE(result);
}

TEST_F(CommandWorkflowTest, StatePersistenceAcrossOperations_VariableStorage) {
    std::vector<Command> variable_commands;
    
    // Store variable
    Command store_cmd;
    store_cmd.type = CommandType::STORE;
    store_cmd.variable_name = "user_id";
    store_cmd.value = "12345";
    variable_commands.push_back(store_cmd);
    
    // Use variable
    Command use_cmd;
    use_cmd.type = CommandType::TYPE;
    use_cmd.selector = "#user-id-field";
    use_cmd.text = "{{user_id}}";
    variable_commands.push_back(use_cmd);
    
    // Retrieve variable
    Command get_cmd;
    get_cmd.type = CommandType::GET;
    get_cmd.variable_name = "user_id";
    variable_commands.push_back(get_cmd);
    
    EXPECT_CALL(*mock_browser_, type("#user-id-field", "12345"))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflow(variable_commands, default_session_);
    EXPECT_TRUE(result);
    
    // Verify variable persistence
    std::string stored_value = command_executor_->getStoredVariable("user_id");
    EXPECT_EQ(stored_value, "12345");
}

TEST_F(CommandWorkflowTest, StatePersistenceAcrossOperations_SessionContinuation) {
    // First workflow
    auto first_commands = createBasicWorkflow();
    
    EXPECT_CALL(*mock_browser_, navigate("https://example.com/form"))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_browser_, type("#username", "testuser"))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_browser_, click("#submit-btn"))
        .WillOnce(Return(true));
    
    bool first_result = command_executor_->executeWorkflow(first_commands, default_session_);
    EXPECT_TRUE(first_result);
    
    // Second workflow continuing from same session
    std::vector<Command> continuation_commands;
    
    Command wait_cmd;
    wait_cmd.type = CommandType::WAIT;
    wait_cmd.selector = "#success-message";
    wait_cmd.timeout = 5000;
    continuation_commands.push_back(wait_cmd);
    
    Command assert_cmd;
    assert_cmd.type = CommandType::ASSERT;
    assert_cmd.selector = "#username-display";
    assert_cmd.expected = "testuser";
    continuation_commands.push_back(assert_cmd);
    
    EXPECT_CALL(*mock_browser_, waitForElement("#success-message", 5000))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_browser_, assertElement("#username-display", "testuser"))
        .WillOnce(Return(true));
    
    bool continuation_result = command_executor_->executeWorkflow(continuation_commands, default_session_);
    EXPECT_TRUE(continuation_result);
}

// ========== Workflow Timeout Tests ==========

TEST_F(CommandWorkflowTest, WorkflowTimeoutHandling_GlobalTimeout) {
    auto commands = createComplexWorkflow();
    WorkflowConfig config;
    config.global_timeout_ms = 1000;  // Very short timeout
    config.abort_on_timeout = true;
    
    // Setup slow operation that will timeout
    EXPECT_CALL(*mock_browser_, navigate("https://app.example.com/login"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, waitForElement("#login-form", 5000))
        .WillOnce([](const std::string&, int) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));  // Longer than global timeout
            return true;
        });
    
    bool result = command_executor_->executeWorkflowWithConfig(commands, default_session_, config);
    EXPECT_FALSE(result);  // Should timeout
}

TEST_F(CommandWorkflowTest, WorkflowTimeoutHandling_CommandTimeout) {
    std::vector<Command> timeout_commands;
    
    Command timeout_cmd;
    timeout_cmd.type = CommandType::WAIT;
    timeout_cmd.selector = "#slow-element";
    timeout_cmd.timeout = 500;  // Short timeout
    timeout_commands.push_back(timeout_cmd);
    
    EXPECT_CALL(*mock_browser_, waitForElement("#slow-element", 500))
        .WillOnce(Return(false));  // Timeout
    
    bool result = command_executor_->executeWorkflow(timeout_commands, default_session_);
    EXPECT_FALSE(result);
}

TEST_F(CommandWorkflowTest, WorkflowTimeoutHandling_TimeoutRecovery) {
    std::vector<Command> recovery_commands;
    
    Command timeout_cmd;
    timeout_cmd.type = CommandType::WAIT;
    timeout_cmd.selector = "#timeout-element";
    timeout_cmd.timeout = 100;
    timeout_cmd.on_timeout = "continue";
    recovery_commands.push_back(timeout_cmd);
    
    Command recovery_cmd;
    recovery_cmd.type = CommandType::CLICK;
    recovery_cmd.selector = "#fallback-button";
    recovery_commands.push_back(recovery_cmd);
    
    EXPECT_CALL(*mock_browser_, waitForElement("#timeout-element", 100))
        .WillOnce(Return(false));
    
    EXPECT_CALL(*mock_browser_, click("#fallback-button"))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflow(recovery_commands, default_session_);
    EXPECT_TRUE(result);
}

// ========== Workflow Rollback Tests ==========

TEST_F(CommandWorkflowTest, WorkflowRollbackMechanism_BasicRollback) {
    std::vector<Command> rollback_commands;
    
    Command setup_cmd;
    setup_cmd.type = CommandType::TYPE;
    setup_cmd.selector = "#setup-field";
    setup_cmd.text = "setup-data";
    setup_cmd.rollback_action = "clear_field";
    rollback_commands.push_back(setup_cmd);
    
    Command failing_cmd;
    failing_cmd.type = CommandType::CLICK;
    failing_cmd.selector = "#failing-button";
    rollback_commands.push_back(failing_cmd);
    
    InSequence seq;
    
    EXPECT_CALL(*mock_browser_, type("#setup-field", "setup-data"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, click("#failing-button"))
        .WillOnce(Return(false));
    
    // Rollback action
    EXPECT_CALL(*mock_browser_, type("#setup-field", ""))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflowWithRollback(rollback_commands, default_session_);
    EXPECT_FALSE(result);  // Workflow failed but rolled back cleanly
}

TEST_F(CommandWorkflowTest, WorkflowRollbackMechanism_TransactionRollback) {
    std::vector<Command> transaction_commands;
    
    Command begin_cmd;
    begin_cmd.type = CommandType::BEGIN_TRANSACTION;
    transaction_commands.push_back(begin_cmd);
    
    Command action1_cmd;
    action1_cmd.type = CommandType::TYPE;
    action1_cmd.selector = "#field1";
    action1_cmd.text = "data1";
    transaction_commands.push_back(action1_cmd);
    
    Command action2_cmd;
    action2_cmd.type = CommandType::TYPE;
    action2_cmd.selector = "#field2";
    action2_cmd.text = "data2";
    transaction_commands.push_back(action2_cmd);
    
    Command failing_cmd;
    failing_cmd.type = CommandType::CLICK;
    failing_cmd.selector = "#commit-button";
    transaction_commands.push_back(failing_cmd);
    
    Command commit_cmd;
    commit_cmd.type = CommandType::COMMIT_TRANSACTION;
    transaction_commands.push_back(commit_cmd);
    
    InSequence seq;
    
    EXPECT_CALL(*mock_browser_, type("#field1", "data1"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, type("#field2", "data2"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, click("#commit-button"))
        .WillOnce(Return(false));
    
    // Transaction rollback
    EXPECT_CALL(*mock_browser_, type("#field2", ""))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, type("#field1", ""))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflow(transaction_commands, default_session_);
    EXPECT_FALSE(result);
}

// ========== Conditional Workflow Tests ==========

TEST_F(CommandWorkflowTest, ConditionalWorkflowExecution_IfElseBranching) {
    std::vector<Command> conditional_commands;
    
    Command condition_cmd;
    condition_cmd.type = CommandType::IF;
    condition_cmd.condition = "element_exists";
    condition_cmd.selector = "#login-form";
    conditional_commands.push_back(condition_cmd);
    
    Command then_cmd;
    then_cmd.type = CommandType::CLICK;
    then_cmd.selector = "#login-submit";
    conditional_commands.push_back(then_cmd);
    
    Command else_cmd;
    else_cmd.type = CommandType::ELSE;
    conditional_commands.push_back(else_cmd);
    
    Command else_action;
    else_action.type = CommandType::NAVIGATE;
    else_action.url = "https://example.com/login";
    conditional_commands.push_back(else_action);
    
    Command endif_cmd;
    endif_cmd.type = CommandType::ENDIF;
    conditional_commands.push_back(endif_cmd);
    
    // Element exists - take then branch
    EXPECT_CALL(*mock_browser_, elementExists("#login-form"))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mock_browser_, click("#login-submit"))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflow(conditional_commands, default_session_);
    EXPECT_TRUE(result);
}

// ========== Workflow Parameter Validation Tests ==========

TEST_F(CommandWorkflowTest, WorkflowParameterValidation_RequiredParameters) {
    std::vector<Command> param_commands;
    
    Command param_cmd;
    param_cmd.type = CommandType::TYPE;
    param_cmd.selector = "#required-field";
    param_cmd.text = "{{required_param}}";
    param_cmd.required_params = {"required_param"};
    param_commands.push_back(param_cmd);
    
    WorkflowConfig config;
    config.parameters["required_param"] = "provided_value";
    
    EXPECT_CALL(*mock_browser_, type("#required-field", "provided_value"))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflowWithConfig(param_commands, default_session_, config);
    EXPECT_TRUE(result);
}

TEST_F(CommandWorkflowTest, WorkflowParameterValidation_MissingParameters) {
    std::vector<Command> param_commands;
    
    Command param_cmd;
    param_cmd.type = CommandType::TYPE;
    param_cmd.selector = "#required-field";
    param_cmd.text = "{{missing_param}}";
    param_cmd.required_params = {"missing_param"};
    param_commands.push_back(param_cmd);
    
    WorkflowConfig config;
    // missing_param not provided
    
    bool result = command_executor_->executeWorkflowWithConfig(param_commands, default_session_, config);
    EXPECT_FALSE(result);  // Should fail due to missing parameter
}

// ========== Workflow Logging Tests ==========

TEST_F(CommandWorkflowTest, WorkflowLoggingAndTracing_ExecutionLog) {
    auto commands = createBasicWorkflow();
    
    WorkflowConfig config;
    config.enable_logging = true;
    config.log_level = "DEBUG";
    config.log_file = (temp_dir_ / "workflow.log").string();
    
    EXPECT_CALL(*mock_browser_, navigate("https://example.com/form"))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_browser_, type("#username", "testuser"))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_browser_, click("#submit-btn"))
        .WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflowWithConfig(commands, default_session_, config);
    EXPECT_TRUE(result);
    
    // Verify log file was created
    EXPECT_TRUE(std::filesystem::exists(config.log_file));
    
    // Verify log content
    std::ifstream log_file(config.log_file);
    std::string log_content((std::istreambuf_iterator<char>(log_file)),
                           std::istreambuf_iterator<char>());
    EXPECT_TRUE(log_content.find("NAVIGATE") != std::string::npos);
    EXPECT_TRUE(log_content.find("TYPE") != std::string::npos);
    EXPECT_TRUE(log_content.find("CLICK") != std::string::npos);
}

TEST_F(CommandWorkflowTest, WorkflowLoggingAndTracing_PerformanceMetrics) {
    auto commands = createComplexWorkflow();
    
    WorkflowConfig config;
    config.enable_performance_tracking = true;
    config.performance_log = (temp_dir_ / "performance.log").string();
    
    // Setup expectations (using shortened expectations for brevity)
    EXPECT_CALL(*mock_browser_, navigate(_)).WillOnce(Return(true));
    EXPECT_CALL(*mock_browser_, waitForElement(_, _)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_browser_, type(_, _)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_browser_, click(_)).WillOnce(Return(true));
    EXPECT_CALL(*mock_browser_, takeScreenshot(_)).Times(1);
    EXPECT_CALL(*mock_browser_, assertElement(_, _)).WillOnce(Return(true));
    
    bool result = command_executor_->executeWorkflowWithConfig(commands, default_session_, config);
    EXPECT_TRUE(result);
    
    // Verify performance log
    EXPECT_TRUE(std::filesystem::exists(config.performance_log));
}

} // namespace