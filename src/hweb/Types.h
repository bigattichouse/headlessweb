#pragma once

#include <string>
#include <vector>
#include "../Assertion/Types.h"

namespace HWeb {

struct Command {
    std::string type;
    std::string selector;
    std::string value;
    int timeout = 10000;
};

struct FileOperationSettings {
    size_t max_file_size = 104857600; // 100MB default
    std::vector<std::string> allowed_types = {"*"};
    std::string download_dir = "";
    int upload_timeout = 30000;
    int download_timeout = 30000;
};

struct HWebConfig {
    std::string sessionName;
    std::string url;
    bool endSession = false;
    bool listSessions = false;
    bool showHelp = false;
    bool json_mode = false;
    bool silent_mode = false;
    bool allow_data_uri = false;
    int browser_width = 1000;
    std::vector<Command> commands;
    std::vector<Assertion::Command> assertions;
    FileOperationSettings file_settings;
};

enum class NavigationStrategy {
    NEW_URL,
    SESSION_RESTORE,
    CONTINUE_SESSION,
    SESSION_ONLY,
    NO_NAVIGATION
};

} // namespace HWeb