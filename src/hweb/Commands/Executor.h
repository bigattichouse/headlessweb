#pragma once

#include "../Types.h"
#include "../../Browser/Browser.h"
#include "../../Session/Session.h"
#include <vector>

namespace HWeb {

class CommandExecutor {
public:
    CommandExecutor();
    ~CommandExecutor();
    
    int execute_commands(Browser& browser, Session& session, const std::vector<Command>& commands);
    int execute_assertions(Browser& browser, const std::vector<Assertion::Command>& assertions);
    
private:
    bool execute_single_command(Browser& browser, Session& session, const Command& cmd);
    void handle_navigation_update(Browser& browser, Session& session, bool navigation_expected, bool isHistoryNavigation);
    bool is_state_modifying_command(const std::string& command_type);
    bool is_navigation_command(const std::string& command_type);
};

} // namespace HWeb