#pragma once

#include "../Types.h"
#include "../../Browser/Browser.h"
#include "../../Session/Session.h"

namespace HWeb {

class BasicCommandHandler {
public:
    BasicCommandHandler();
    ~BasicCommandHandler();
    
    int handle_command(Browser& browser, Session& session, const Command& cmd);
    
private:
    int handle_navigation_command(Browser& browser, Session& session, const Command& cmd);
    int handle_interaction_command(Browser& browser, const Command& cmd);
    int handle_data_extraction_command(Browser& browser, const Command& cmd);
    int handle_session_command(Session& session, const Command& cmd);
    int handle_history_navigation(Browser& browser, Session& session, const Command& cmd);
    
    bool wait_for_navigation_complete(Browser& browser, int timeout_ms);
};

} // namespace HWeb