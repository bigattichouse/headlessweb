#pragma once

#include "Types.h"
#include <vector>
#include <string>

namespace HWeb {

class ConfigParser {
public:
    HWebConfig parseArguments(const std::vector<std::string>& args);
    void print_usage();
    
private:
    void validate_config(const HWebConfig& config);
    bool isUrl(const std::string& str);
    void parse_assertion_command(const std::vector<std::string>& args, size_t& i, 
                                HWebConfig& config, Assertion::Command& current_assertion, 
                                bool& has_pending_assertion);
    void parse_file_operation_command(const std::vector<std::string>& args, size_t& i, 
                                     HWebConfig& config);
    void parse_advanced_wait_command(const std::vector<std::string>& args, size_t& i, 
                                    HWebConfig& config);
    void parse_regular_command(const std::vector<std::string>& args, size_t& i, 
                              HWebConfig& config);
    void parse_file_operation_options(const std::vector<std::string>& args, size_t& i, 
                                     HWebConfig& config);
    void parse_test_suite_command(const std::vector<std::string>& args, size_t& i, 
                                 const HWebConfig& config);
};

} // namespace HWeb