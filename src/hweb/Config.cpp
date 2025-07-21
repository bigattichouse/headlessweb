#include "Config.h"
#include "Output.h"
#include "Services/ManagerRegistry.h"
#include <iostream>
#include <sstream>

namespace HWeb {

HWebConfig ConfigParser::parseArguments(const std::vector<std::string>& args) {
    HWebConfig config;
    
    // Current assertion being built
    Assertion::Command current_assertion;
    bool has_pending_assertion = false;
    
    // Parse arguments
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--session" && i + 1 < args.size()) {
            config.sessionName = args[++i];
        } else if (args[i] == "--url" && i + 1 < args.size()) {
            config.url = args[++i];
        } else if (args[i] == "--end") {
            config.endSession = true;
        } else if (args[i] == "--list") {
            config.listSessions = true;
        } else if (args[i] == "--json") {
            config.json_mode = true;
        } else if (args[i] == "--silent") {
            config.silent_mode = true;
        } else if (args[i] == "--width" && i + 1 < args.size()) {
            config.browser_width = std::stoi(args[++i]);
        } else if (args[i] == "--user-agent" && i + 1 < args.size()) {
            config.commands.push_back({"user-agent", "", args[++i]});
        }
        // Test Suite Management
        else if (args[i] == "--test-suite") {
            parse_test_suite_command(args, i, config);
        }
        // Assertion Commands
        else if (args[i].substr(0, 8) == "--assert") {
            parse_assertion_command(args, i, config, current_assertion, has_pending_assertion);
        }
        // File Operation Commands  
        else if (args[i] == "--upload" || args[i] == "--upload-multiple" || 
                 args[i] == "--download-wait" || args[i] == "--download-wait-multiple") {
            parse_file_operation_command(args, i, config);
        }
        // File Operation Options
        else if (args[i] == "--max-file-size" || args[i] == "--allowed-types" || 
                 args[i] == "--download-dir" || args[i] == "--upload-timeout" || 
                 args[i] == "--download-timeout") {
            parse_file_operation_options(args, i, config);
        }
        // Advanced Waiting Commands
        else if (args[i].substr(0, 6) == "--wait" && args[i] != "--wait" && args[i] != "--wait-nav" && args[i] != "--wait-ready") {
            parse_advanced_wait_command(args, i, config);
        }
        // Regular Commands
        else {
            parse_regular_command(args, i, config);
        }
    }
    
    // Add any final pending assertion
    if (has_pending_assertion) {
        config.assertions.push_back(current_assertion);
    }
    
    validate_config(config);
    return config;
}

void ConfigParser::parse_assertion_command(const std::vector<std::string>& args, size_t& i, 
                                          HWebConfig& config, Assertion::Command& current_assertion, 
                                          bool& has_pending_assertion) {
    if (args[i] == "--assert-exists" && i + 1 < args.size()) {
        if (has_pending_assertion) {
            config.assertions.push_back(current_assertion);
        }
        
        current_assertion = {};
        current_assertion.type = "exists";
        current_assertion.selector = args[++i];
        current_assertion.expected_value = "true";
        current_assertion.op = Assertion::ComparisonOperator::EQUALS;
        current_assertion.json_output = config.json_mode;
        current_assertion.silent = config.silent_mode;
        current_assertion.case_sensitive = true;
        current_assertion.timeout_ms = 5000;
        has_pending_assertion = true;
        
        if (i + 1 < args.size() && args[i + 1][0] != '-') {
            current_assertion.expected_value = args[++i];
        }
    } else if (args[i] == "--assert-text" && i + 2 < args.size()) {
        if (has_pending_assertion) {
            config.assertions.push_back(current_assertion);
        }
        
        current_assertion = {};
        current_assertion.type = "text";
        current_assertion.selector = args[++i];
        current_assertion.expected_value = args[++i];
        current_assertion.op = Assertion::ComparisonOperator::EQUALS;
        current_assertion.json_output = config.json_mode;
        current_assertion.silent = config.silent_mode;
        current_assertion.case_sensitive = true;
        current_assertion.timeout_ms = 5000;
        has_pending_assertion = true;
    } else if (args[i] == "--assert-count" && i + 2 < args.size()) {
        if (has_pending_assertion) {
            config.assertions.push_back(current_assertion);
        }
        
        current_assertion = {};
        current_assertion.type = "count";
        current_assertion.selector = args[++i];
        current_assertion.expected_value = args[++i];
        current_assertion.op = Assertion::ComparisonOperator::EQUALS;
        current_assertion.json_output = config.json_mode;
        current_assertion.silent = config.silent_mode;
        current_assertion.case_sensitive = true;
        current_assertion.timeout_ms = 5000;
        has_pending_assertion = true;
    } else if (args[i] == "--assert-js" && i + 1 < args.size()) {
        if (has_pending_assertion) {
            config.assertions.push_back(current_assertion);
        }
        
        current_assertion = {};
        current_assertion.type = "js";
        current_assertion.selector = args[++i];
        current_assertion.expected_value = "true";
        current_assertion.op = Assertion::ComparisonOperator::EQUALS;
        current_assertion.json_output = config.json_mode;
        current_assertion.silent = config.silent_mode;
        current_assertion.case_sensitive = true;
        current_assertion.timeout_ms = 5000;
        has_pending_assertion = true;
        
        if (i + 1 < args.size() && args[i + 1][0] != '-') {
            current_assertion.expected_value = args[++i];
        }
    } else if (args[i] == "--message" && i + 1 < args.size()) {
        if (has_pending_assertion) {
            current_assertion.custom_message = args[++i];
        } else {
            Output::error("--message must follow an assertion command");
            throw std::runtime_error("Invalid argument sequence");
        }
    } else if (args[i] == "--timeout" && i + 1 < args.size()) {
        int timeout = std::stoi(args[++i]);
        if (has_pending_assertion) {
            current_assertion.timeout_ms = timeout;
        } else {
            Output::error("--timeout must follow an assertion command");
            throw std::runtime_error("Invalid argument sequence");
        }
    }
}

void ConfigParser::parse_file_operation_command(const std::vector<std::string>& args, size_t& i, 
                                               HWebConfig& config) {
    if (args[i] == "--upload" && i + 2 < args.size()) {
        Command cmd;
        cmd.type = "upload";
        cmd.selector = args[++i];
        cmd.value = args[++i];
        cmd.timeout = config.file_settings.upload_timeout;
        config.commands.push_back(cmd);
    } else if (args[i] == "--upload-multiple" && i + 2 < args.size()) {
        Command cmd;
        cmd.type = "upload-multiple";
        cmd.selector = args[++i];
        cmd.value = args[++i];
        cmd.timeout = config.file_settings.upload_timeout;
        config.commands.push_back(cmd);
    } else if (args[i] == "--download-wait" && i + 1 < args.size()) {
        Command cmd;
        cmd.type = "download-wait";
        cmd.selector = args[++i];
        cmd.value = "";
        cmd.timeout = config.file_settings.download_timeout;
        config.commands.push_back(cmd);
    } else if (args[i] == "--download-wait-multiple" && i + 1 < args.size()) {
        Command cmd;
        cmd.type = "download-wait-multiple";
        cmd.selector = "";
        cmd.value = args[++i];
        cmd.timeout = config.file_settings.download_timeout;
        config.commands.push_back(cmd);
    }
}

void ConfigParser::parse_file_operation_options(const std::vector<std::string>& args, size_t& i, 
                                               HWebConfig& config) {
    if (args[i] == "--max-file-size" && i + 1 < args.size()) {
        config.file_settings.max_file_size = std::stoull(args[++i]);
    } else if (args[i] == "--allowed-types" && i + 1 < args.size()) {
        std::string types_str = args[++i];
        config.file_settings.allowed_types.clear();
        std::stringstream ss(types_str);
        std::string type;
        while (std::getline(ss, type, ',')) {
            type.erase(0, type.find_first_not_of(" \t"));
            type.erase(type.find_last_not_of(" \t") + 1);
            if (!type.empty()) {
                config.file_settings.allowed_types.push_back(type);
            }
        }
    } else if (args[i] == "--download-dir" && i + 1 < args.size()) {
        config.file_settings.download_dir = args[++i];
    } else if (args[i] == "--upload-timeout" && i + 1 < args.size()) {
        config.file_settings.upload_timeout = std::stoi(args[++i]);
    } else if (args[i] == "--download-timeout" && i + 1 < args.size()) {
        config.file_settings.download_timeout = std::stoi(args[++i]);
    }
}

void ConfigParser::parse_test_suite_command(const std::vector<std::string>& args, size_t& i, 
                                           const HWebConfig& config) {
    if (i + 1 < args.size()) {
        std::string action = args[++i];
        if (action == "start" && i + 1 < args.size()) {
            std::string suite_name = args[++i];
            ManagerRegistry::get_assertion_manager().startSuite(suite_name);
        } else if (action == "end") {
            std::string format = "text";
            if (i + 1 < args.size() && args[i + 1][0] != '-') {
                format = args[++i];
            }
            ManagerRegistry::get_assertion_manager().endSuite(config.json_mode, format);
        }
    }
}

void ConfigParser::parse_advanced_wait_command(const std::vector<std::string>& args, size_t& i, 
                                              HWebConfig& config) {
    Command cmd;
    cmd.type = args[i].substr(2); // Remove "--" prefix
    cmd.timeout = 10000;
    
    if (args[i] == "--wait-text-advanced" && i + 1 < args.size()) {
        cmd.selector = "";
        cmd.value = args[++i];
        cmd.timeout = 10000;
    } else if (args[i] == "--wait-network-idle") {
        cmd.selector = "";
        cmd.value = "500";
        cmd.timeout = 30000;
        if (i + 1 < args.size() && args[i + 1][0] != '-') {
            cmd.value = args[++i];
        }
    } else if (args[i] == "--wait-network-request" && i + 1 < args.size()) {
        cmd.selector = "";
        cmd.value = args[++i];
        cmd.timeout = 15000;
    } else if (args[i] == "--wait-element-visible" && i + 1 < args.size()) {
        cmd.selector = args[++i];
        cmd.value = "";
        cmd.timeout = 10000;
    } else if (args[i] == "--wait-element-count" && i + 3 < args.size()) {
        cmd.selector = args[++i];
        cmd.value = args[++i] + " " + args[++i];
        cmd.timeout = 10000;
    } else if (args[i] == "--wait-attribute" && i + 3 < args.size()) {
        cmd.selector = args[++i];
        cmd.value = args[++i] + " " + args[++i];
        cmd.timeout = 10000;
    } else if (args[i] == "--wait-url-change" && i + 1 < args.size()) {
        cmd.selector = "";
        cmd.value = args[++i];
        cmd.timeout = 10000;
    } else if (args[i] == "--wait-title-change" && i + 1 < args.size()) {
        cmd.selector = "";
        cmd.value = args[++i];
        cmd.timeout = 10000;
    } else if (args[i] == "--wait-spa-navigation") {
        cmd.selector = "";
        cmd.value = "";
        cmd.timeout = 10000;
        if (i + 1 < args.size() && args[i + 1][0] != '-') {
            cmd.value = args[++i];
        }
    } else if (args[i] == "--wait-framework-ready") {
        cmd.selector = "";
        cmd.value = "auto";
        cmd.timeout = 15000;
        if (i + 1 < args.size() && args[i + 1][0] != '-') {
            cmd.value = args[++i];
        }
    } else if (args[i] == "--wait-dom-change" && i + 1 < args.size()) {
        cmd.selector = args[++i];
        cmd.value = "";
        cmd.timeout = 10000;
    } else if (args[i] == "--wait-content-change" && i + 2 < args.size()) {
        cmd.selector = args[++i];
        cmd.value = args[++i];
        cmd.timeout = 10000;
    }
    
    config.commands.push_back(cmd);
}

void ConfigParser::parse_regular_command(const std::vector<std::string>& args, size_t& i, 
                                        HWebConfig& config) {
    // Form interaction commands
    if (args[i] == "--type" && i + 2 < args.size()) {
        config.commands.push_back({"type", args[i+1], args[i+2]});
        i += 2;
    } else if (args[i] == "--click" && i + 1 < args.size()) {
        config.commands.push_back({"click", args[++i], ""});
    } else if (args[i] == "--submit") {
        std::string form_sel = (i + 1 < args.size() && args[i+1][0] != '-') ? args[++i] : "form";
        config.commands.push_back({"submit", form_sel, ""});
    } else if (args[i] == "--select" && i + 2 < args.size()) {
        config.commands.push_back({"select", args[i+1], args[i+2]});
        i += 2;
    } else if (args[i] == "--check" && i + 1 < args.size()) {
        config.commands.push_back({"check", args[++i], ""});
    } else if (args[i] == "--uncheck" && i + 1 < args.size()) {
        config.commands.push_back({"uncheck", args[++i], ""});
    } else if (args[i] == "--focus" && i + 1 < args.size()) {
        config.commands.push_back({"focus", args[++i], ""});
    }
    
    // Navigation commands
    else if (args[i] == "--back") {
        config.commands.push_back({"back", "", ""});
    } else if (args[i] == "--forward") {
        config.commands.push_back({"forward", "", ""});
    } else if (args[i] == "--reload") {
        config.commands.push_back({"reload", "", ""});
    }
    
    // Data extraction commands
    else if (args[i] == "--text" && i + 1 < args.size()) {
        config.commands.push_back({"text", args[++i], ""});
    } else if (args[i] == "--html" && i + 1 < args.size()) {
        config.commands.push_back({"html", args[++i], ""});
    } else if (args[i] == "--attr" && i + 2 < args.size()) {
        if (i + 3 < args.size() && args[i+3][0] != '-') {
            config.commands.push_back({"set-attr", args[i+1], args[i+2] + " " + args[i+3]});
            i += 3;
        } else {
            config.commands.push_back({"attr", args[i+1], args[i+2]});
            i += 2;
        }
    } else if (args[i] == "--exists" && i + 1 < args.size()) {
        config.commands.push_back({"exists", args[++i], ""});
    } else if (args[i] == "--count" && i + 1 < args.size()) {
        config.commands.push_back({"count", args[++i], ""});
    }
    
    // JavaScript and search commands
    else if (args[i] == "--js" && i + 1 < args.size()) {
        config.commands.push_back({"js", "", args[++i]});
    } else if (args[i] == "--search" && i + 1 < args.size()) {
        config.commands.push_back({"search", "", args[++i]});
    }
    
    // Data storage commands
    else if (args[i] == "--store" && i + 2 < args.size()) {
        config.commands.push_back({"store", args[i+1], args[i+2]});
        i += 2;
    } else if (args[i] == "--get" && i + 1 < args.size()) {
        config.commands.push_back({"get", args[++i], ""});
    }
    
    // Screenshot commands
    else if (args[i] == "--screenshot") {
        std::string filename = (i + 1 < args.size() && args[i+1][0] != '-') ? args[++i] : "screenshot.png";
        config.commands.push_back({"screenshot", filename, ""});
    } else if (args[i] == "--screenshot-full") {
        std::string filename = (i + 1 < args.size() && args[i+1][0] != '-') ? args[++i] : "screenshot-full.png";
        config.commands.push_back({"screenshot-full", filename, ""});
    }
    
    // Recording/replay commands
    else if (args[i] == "--record-start") {
        config.commands.push_back({"record-start", "", ""});
    } else if (args[i] == "--record-stop") {
        config.commands.push_back({"record-stop", "", ""});
    } else if (args[i] == "--replay" && i + 1 < args.size()) {
        config.commands.push_back({"replay", args[++i], ""});
    }
    
    // Extract command
    else if (args[i] == "--extract" && i + 2 < args.size()) {
        config.commands.push_back({"extract", args[i+1], args[i+2]});
        i += 2;
    }
    
    // Basic waiting commands
    else if (args[i] == "--wait" && i + 1 < args.size()) {
        config.commands.push_back({"wait", args[++i], ""});
    } else if (args[i] == "--wait-nav") {
        config.commands.push_back({"wait-nav", "", ""});
    } else if (args[i] == "--wait-ready" && i + 1 < args.size()) {
        config.commands.push_back({"wait-ready", args[++i], ""});
    }
}

void ConfigParser::validate_config(const HWebConfig& config) {
    // Add validation logic here if needed
}

void ConfigParser::print_usage() {
    std::cerr << "Usage: hweb [options] [commands...]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --session <n>        Use named session (default: 'default')" << std::endl;
    std::cerr << "  --url <url>          Navigate to URL" << std::endl;
    std::cerr << "  --end                End session" << std::endl;
    std::cerr << "  --list               List all sessions" << std::endl;
    std::cerr << "  --debug              Enable debug output" << std::endl;
    std::cerr << "  --user-agent <ua>    Set custom user agent" << std::endl;
    std::cerr << "  --width <px>         Set browser width (default: 1000)" << std::endl;
    std::cerr << "  --json               Enable JSON output mode" << std::endl;
    std::cerr << "  --silent             Silent mode (exit codes only)" << std::endl;
    // ... rest of usage text from original hweb.cpp
}

} // namespace HWeb