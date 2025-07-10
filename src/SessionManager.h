#pragma once

#include "Session.h"
#include <string>
#include <vector>
#include <optional>

struct SessionInfo {
    std::string name;
    std::string url;
    std::string sizeStr;
    std::string lastAccessedStr;
};

class SessionManager {
public:
    SessionManager(const std::string& sessionPath);

    Session loadOrCreateSession(const std::string& name);
    void saveSession(const Session& session);
    void deleteSession(const std::string& name);
    std::vector<SessionInfo> listSessions();

private:
    std::string getSessionFilePath(const std::string& name) const;

    std::string sessionPath;
};
