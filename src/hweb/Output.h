#pragma once

#include <string>

namespace HWeb {

class Output {
private:
    static bool json_mode_;
    static bool silent_mode_;

public:
    static void set_json_mode(bool enabled);
    static void set_silent_mode(bool enabled);
    static bool is_json_mode();
    static bool is_silent_mode();
    
    static void info(const std::string& message);
    static void error(const std::string& message);
    static void format_error(const std::string& context, const std::string& error);
};

} // namespace HWeb