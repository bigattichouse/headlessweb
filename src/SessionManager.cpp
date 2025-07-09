#include "SessionManager.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

SessionManager::SessionManager(const std::string& sessionPath) : sessionPath(sessionPath) {
    fs::create_directories(sessionPath);
}

Session SessionManager::loadOrCreateSession(const std::string& name) {
    std::string filePath = getSessionFilePath(name);
    if (fs::exists(filePath)) {
        std::ifstream file(filePath);
        std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return Session::deserialize(data);
    }
    return Session(name);
}

void SessionManager::saveSession(const Session& session) {
    std::string filePath = getSessionFilePath(session.getName());
    std::ofstream file(filePath);
    file << session.serialize();
}

void SessionManager::deleteSession(const std::string& name) {
    std::string filePath = getSessionFilePath(name);
    if (fs::exists(filePath)) {
        fs::remove(filePath);
    }
}

std::string SessionManager::getSessionFilePath(const std::string& name) const {
    return fs::path(sessionPath) / (name + ".json");
}
