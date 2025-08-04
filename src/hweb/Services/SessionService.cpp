#include "SessionService.h"
#include "../Output.h"

namespace HWeb {

SessionService::SessionService(SessionManager& session_manager) : session_manager_(session_manager) {
}

SessionService::~SessionService() {
}

Session SessionService::initialize_session(const std::string& sessionName) {
    return session_manager_.loadOrCreateSession(sessionName);
}

Session SessionService::initialize_fresh_session(const std::string& sessionName) {
    // Delete existing session if it exists
    try {
        session_manager_.deleteSession(sessionName);
        Output::verbose("Deleted existing session: " + sessionName);
    } catch (const std::exception& e) {
        // Session might not exist, which is fine
        Output::verbose("No existing session to delete: " + sessionName);
    }
    
    // Create a fresh session
    Session session = session_manager_.loadOrCreateSession(sessionName);
    Output::info("Started fresh session: " + sessionName);
    return session;
}

bool SessionService::handle_session_end(const std::string& sessionName) {
    try {
        Session session = session_manager_.loadOrCreateSession(sessionName);
        session_manager_.saveSession(session);
        Output::info("Session '" + sessionName + "' ended.");
        return true;
    } catch (const std::exception& e) {
        Output::error("Failed to end session: " + std::string(e.what()));
        return false;
    }
}

bool SessionService::handle_session_list() {
    try {
        list_sessions(session_manager_);
        return true;
    } catch (const std::exception& e) {
        Output::error("Failed to list sessions: " + std::string(e.what()));
        return false;
    }
}

void SessionService::update_session_state(Browser& browser, Session& session) {
    try {
        browser.updateSessionState(session);
    } catch (const std::exception& e) {
        Output::error("Warning: Failed to update session state: " + std::string(e.what()));
    }
}

bool SessionService::save_session_safely(const Session& session, const std::string& sessionName) {
    try {
        session_manager_.saveSession(session);
        Output::info("Session '" + sessionName + "' saved.");
        return true;
    } catch (const std::exception& e) {
        Output::error("Failed to save session: " + std::string(e.what()));
        return false;
    }
}

void list_sessions(SessionManager& sessionManager) {
    auto sessions = sessionManager.listSessions();
    
    if (sessions.empty()) {
        Output::info("No active sessions.");
        return;
    }
    
    Output::info("Active sessions:");
    for (const auto& info : sessions) {
        Output::info("  " + info.name + " - " + info.url + " (" + info.sizeStr + ", " + info.lastAccessedStr + ")");
    }
}

} // namespace HWeb