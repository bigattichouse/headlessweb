#include "Executor.h"
#include "../Output.h"
#include "../Services/ManagerRegistry.h"
#include "../Handlers/FileOperations.h"
#include "../Handlers/AdvancedWait.h"
#include "../Handlers/BasicCommands.h"

namespace HWeb {

CommandExecutor::CommandExecutor() {
}

CommandExecutor::~CommandExecutor() {
}

int CommandExecutor::execute_commands(Browser& browser, Session& session, const std::vector<Command>& commands) {
    int exit_code = 0;
    bool state_modified = false;
    bool isHistoryNavigation = false;
    
    FileOperationHandler file_handler;
    AdvancedWaitHandler wait_handler;
    BasicCommandHandler basic_handler;
    
    for (const auto& cmd : commands) {
        bool navigation_expected = is_navigation_command(cmd.type);
        
        if (is_state_modifying_command(cmd.type)) {
            state_modified = true;
        }
        
        // Record action if recording is enabled
        if (session.isRecording() && 
            (cmd.type == "type" || cmd.type == "click" || cmd.type == "submit" || 
             cmd.type == "select" || cmd.type == "check" || cmd.type == "uncheck")) {
            Session::RecordedAction action;
            action.type = cmd.type;
            action.selector = cmd.selector;
            action.value = cmd.value;
            action.delay = 500;
            session.recordAction(action);
        }
        
        // Execute command based on type
        int cmd_result = 0;
        
        // File operations
        if (cmd.type == "upload" || cmd.type == "upload-multiple" || 
            cmd.type == "download-wait" || cmd.type == "download-wait-multiple") {
            cmd_result = file_handler.handle_command(browser, cmd);
        }
        // Advanced waiting commands
        else if (cmd.type.substr(0, 5) == "wait-" && cmd.type != "wait" && 
                 cmd.type != "wait-nav" && cmd.type != "wait-ready") {
            cmd_result = wait_handler.handle_command(browser, cmd);
        }
        // Basic commands and navigation
        else {
            if (cmd.type == "back" || cmd.type == "forward") {
                isHistoryNavigation = true;
            }
            cmd_result = basic_handler.handle_command(browser, session, cmd);
        }
        
        if (cmd_result != 0) {
            exit_code = cmd_result;
        }
        
        // Handle navigation updates
        if (navigation_expected && !isHistoryNavigation) {
            handle_navigation_update(browser, session, navigation_expected, isHistoryNavigation);
        }
        
        isHistoryNavigation = false; // Reset for next iteration
    }
    
    return exit_code;
}

int CommandExecutor::execute_assertions(Browser& browser, const std::vector<Assertion::Command>& assertions) {
    int exit_code = 0;
    auto& assertion_manager = ManagerRegistry::get_assertion_manager();
    
    for (const auto& assertion : assertions) {
        Assertion::Result result = assertion_manager.executeAssertion(browser, assertion);
        
        if (result == Assertion::Result::FAIL || result == Assertion::Result::ERROR) {
            exit_code = static_cast<int>(result);
        }
    }
    
    return exit_code;
}

bool CommandExecutor::execute_single_command(Browser& browser, Session& session, const Command& cmd) {
    // This is handled by the specific handlers now
    return true;
}

void CommandExecutor::handle_navigation_update(Browser& browser, Session& session, 
                                             bool navigation_expected, bool isHistoryNavigation) {
    if (navigation_expected && !isHistoryNavigation) {
        browser.waitForPageStabilization(2000);
        std::string new_url = browser.getCurrentUrl();
        if (new_url != session.getCurrentUrl() && !new_url.empty()) {
            session.addToHistory(new_url);
            session.setCurrentUrl(new_url);
            Output::info("Navigation detected: " + new_url);
        }
    }
}

bool CommandExecutor::is_state_modifying_command(const std::string& command_type) {
    return (command_type == "type" || command_type == "click" || command_type == "submit" || 
            command_type == "select" || command_type == "check" || command_type == "uncheck" ||
            command_type == "js" || command_type == "scroll" || command_type == "user-agent");
}

bool CommandExecutor::is_navigation_command(const std::string& command_type) {
    return (command_type == "submit" || command_type == "click" || command_type == "back" || 
            command_type == "forward" || command_type == "reload");
}

} // namespace HWeb