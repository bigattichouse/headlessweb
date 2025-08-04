#pragma once

#include "../Types.h"
#include "../../Browser/Browser.h"
#include "../../Session/Session.h"
#include "../../Session/Manager.h"

namespace HWeb {

class SessionService {
public:
    SessionService(SessionManager& session_manager);
    ~SessionService();
    
    Session initialize_session(const std::string& sessionName);
    Session initialize_fresh_session(const std::string& sessionName);
    bool handle_session_end(const std::string& sessionName);
    bool handle_session_list();
    void update_session_state(Browser& browser, Session& session);
    bool save_session_safely(const Session& session, const std::string& sessionName);
    
private:
    SessionManager& session_manager_;
};

void list_sessions(SessionManager& sessionManager);

} // namespace HWeb