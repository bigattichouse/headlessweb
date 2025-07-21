#include "Output.h"
#include <iostream>

namespace HWeb {

bool Output::json_mode_ = false;
bool Output::silent_mode_ = false;

void Output::set_json_mode(bool enabled) {
    json_mode_ = enabled;
}

void Output::set_silent_mode(bool enabled) {
    silent_mode_ = enabled;
}

bool Output::is_json_mode() {
    return json_mode_;
}

bool Output::is_silent_mode() {
    return silent_mode_;
}

void Output::info(const std::string& message) {
    if (!silent_mode_) {
        std::cerr << message << std::endl;
    }
}

void Output::error(const std::string& message) {
    std::cerr << message << std::endl;
}

void Output::format_error(const std::string& context, const std::string& error) {
    if (json_mode_) {
        std::cerr << "{\"error\": \"" << context << ": " << error << "\"}" << std::endl;
    } else {
        std::cerr << "Error: " << context << ": " << error << std::endl;
    }
}

} // namespace HWeb