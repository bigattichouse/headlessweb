#pragma once

#include "Session.h"
#include <string>
#include <optional>

class SessionManager {
public:
    SessionManager(const std::string& sessionPath);

    Session loadOrCreateSession(const std::string& name);
    void saveSession(const Session& session);
    void deleteSession(const std::string& name);

private:
    std::string getSessionFilePath(const std::string& name) const;

    std::string sessionPath;
};
