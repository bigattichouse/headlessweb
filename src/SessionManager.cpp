#include "SessionManager.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

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

std::vector<SessionInfo> SessionManager::listSessions() {
    std::vector<SessionInfo> sessions;
    
    try {
        for (const auto& entry : fs::directory_iterator(sessionPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                try {
                    // Load session to get details
                    std::string sessionName = entry.path().stem().string();
                    Session session = loadOrCreateSession(sessionName);
                    
                    SessionInfo info;
                    info.name = sessionName;
                    info.url = session.getCurrentUrl();
                    
                    // Format size
                    auto fileSize = fs::file_size(entry.path());
                    if (fileSize < 1024) {
                        info.sizeStr = std::to_string(fileSize) + "B";
                    } else if (fileSize < 1024 * 1024) {
                        info.sizeStr = std::to_string(fileSize / 1024) + "K";
                    } else {
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(1) << (fileSize / (1024.0 * 1024.0)) << "M";
                        info.sizeStr = oss.str();
                    }
                    
                    // Format last accessed time
                    auto lastAccessed = session.getLastAccessed();
                    auto now = std::chrono::system_clock::now();
                    auto nowTime = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                    auto diff = nowTime - lastAccessed;
                    
                    if (diff < 60) {
                        info.lastAccessedStr = std::to_string(diff) + " sec ago";
                    } else if (diff < 3600) {
                        info.lastAccessedStr = std::to_string(diff / 60) + " min ago";
                    } else if (diff < 86400) {
                        info.lastAccessedStr = std::to_string(diff / 3600) + " hours ago";
                    } else {
                        info.lastAccessedStr = std::to_string(diff / 86400) + " days ago";
                    }
                    
                    sessions.push_back(info);
                } catch (const std::exception& e) {
                    // Skip sessions that can't be loaded
                    continue;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        // Directory doesn't exist or can't be accessed
        return sessions;
    }
    
    return sessions;
}

std::string SessionManager::getSessionFilePath(const std::string& name) const {
    return fs::path(sessionPath) / (name + ".json");
}
