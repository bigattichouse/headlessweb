#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <glib.h>

#include "Types.h"
#include "Config.h"
#include "Output.h"
#include "Commands/Executor.h"
#include "Services/ManagerRegistry.h"
#include "Services/NavigationService.h"
#include "Services/SessionService.h"
#include "Handlers/FileOperations.h"
#include "../Browser/Browser.h"
#include "../Session/Manager.h"
#include "../Debug.h"

namespace HWeb {

// Forward declaration for the main entry point
int main(int argc, char* argv[]);

void initialize_application() {
    ManagerRegistry::initialize();
}

void cleanup_application() {
    ManagerRegistry::cleanup();
}

int run_application(const HWebConfig& config) {
    // Initialize services
    std::string home = std::getenv("HOME");
    SessionManager sessionManager(home + "/.hweb/sessions");
    SessionService sessionService(sessionManager);
    NavigationService navigationService;
    CommandExecutor commandExecutor;
    
    // Configure output modes
    Output::set_json_mode(config.json_mode);
    Output::set_silent_mode(config.silent_mode);
    
    // Configure managers
    auto& assertionManager = ManagerRegistry::get_assertion_manager();
    assertionManager.setSilentMode(config.silent_mode);
    assertionManager.setJsonOutput(config.json_mode);
    
    // Handle list sessions
    if (config.listSessions) {
        return sessionService.handle_session_list() ? 0 : 1;
    }
    
    // Handle help
    if (config.showHelp) {
        ConfigParser parser;
        parser.print_usage();
        return 0;
    }
    
    // Determine session name
    std::string sessionName = config.sessionName.empty() ? "default" : config.sessionName;
    
    // Handle end session
    if (config.endSession) {
        return sessionService.handle_session_end(sessionName) ? 0 : 1;
    }
    
    // Initialize session
    Session session = sessionService.initialize_session(sessionName);
    
    // Check if we need browser
    if (config.url.empty() && config.commands.empty() && config.assertions.empty() && session.getCurrentUrl().empty()) {
        Output::error("No URL in session. Use --url to navigate.");
        return 1;
    }
    
    // Create and configure browser
    Browser browser(config);
    browser.setViewport(config.browser_width, 800);
    
    // Configure file operation handlers
    FileOperationHandler fileHandler;
    fileHandler.configure_managers(config.file_settings);
    
    // Plan and execute navigation
    auto navigationPlan = navigationService.create_navigation_plan(config, session);
    if (!navigationService.execute_navigation_plan(browser, session, navigationPlan)) {
        return 1;
    }
    
    int exit_code = 0;
    bool state_modified = false;
    
    // Execute commands
    if (!config.commands.empty()) {
        int cmd_result = commandExecutor.execute_commands(browser, session, config.commands);
        if (cmd_result != 0) {
            exit_code = cmd_result;
        }
        state_modified = true;
    }
    
    // Execute assertions
    if (!config.assertions.empty()) {
        int assertion_result = commandExecutor.execute_assertions(browser, config.assertions);
        if (assertion_result != 0) {
            exit_code = assertion_result;
        }
    }
    
    // Update session state if needed
    if (state_modified || navigationPlan.should_navigate) {
        sessionService.update_session_state(browser, session);
    }
    
    // Save session
    if (!config.commands.empty() || !config.assertions.empty() || state_modified || navigationPlan.should_navigate) {
        if (!sessionService.save_session_safely(session, sessionName)) {
            exit_code = 1;
        }
    }
    
    // Ensure all GTK events are processed before exit
    for (int i = 0; i < 10; i++) {
        while (g_main_context_pending(g_main_context_default())) {
            g_main_context_iteration(g_main_context_default(), FALSE);
        }
        browser.wait(10);
    }
    
    return exit_code;
}

} // namespace HWeb

int HWeb::main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    
    HWeb::initialize_application();
    
    try {
        HWeb::ConfigParser parser;
        HWeb::HWebConfig config = parser.parseArguments(args);
        
        // Handle debug flag  
        for (const auto& arg : args) {
            if (arg == "--debug") {
                g_debug = true;
                break;
            }
        }
        
        int result = HWeb::run_application(config);
        
        HWeb::cleanup_application();
        return result;
        
    } catch (const std::exception& e) {
        HWeb::Output::error("Application error: " + std::string(e.what()));
        HWeb::cleanup_application();
        return 1;
    } catch (...) {
        HWeb::Output::error("Unknown application error occurred");
        HWeb::cleanup_application();
        return 1;
    }
}