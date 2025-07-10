#include "SessionManager.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace fs = std::filesystem;

SessionManager::SessionManager(const std::string& sessionPath) : sessionPath(sessionPath) {
    try {
        fs::create_directories(sessionPath);
        
        // Verify directory is writable
        std::string testFile = sessionPath + "/.write_test";
        std::ofstream test(testFile);
        if (test.is_open()) {
            test << "test";
            test.close();
            fs::remove(testFile);
        } else {
            std::cerr << "Warning: Session directory may not be writable: " << sessionPath << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating session directory: " << e.what() << std::endl;
        // Try to continue anyway
    }
}

Session SessionManager::loadOrCreateSession(const std::string& name) {
    std::string filePath = getSessionFilePath(name);
    
    if (fs::exists(filePath)) {
        try {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                std::cerr << "Warning: Could not open session file: " << filePath << std::endl;
                return Session(name);
            }
            
            std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            
            if (data.empty()) {
                std::cerr << "Warning: Session file is empty: " << filePath << std::endl;
                return Session(name);
            }
            
            try {
                Session session = Session::deserialize(data);
                // Update the last accessed time when loading
                session.updateLastAccessed();
                return session;
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to deserialize session (creating new): " << e.what() << std::endl;
                // Backup corrupted session file
                std::string backupPath = filePath + ".corrupted." + std::to_string(std::time(nullptr));
                try {
                    fs::copy_file(filePath, backupPath);
                    std::cerr << "Corrupted session backed up to: " << backupPath << std::endl;
                } catch (...) {
                    // Ignore backup errors
                }
                return Session(name);
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Error loading session file (creating new): " << e.what() << std::endl;
            return Session(name);
        }
    }
    
    return Session(name);
}

void SessionManager::saveSession(const Session& session) {
    std::string filePath = getSessionFilePath(session.getName());
    
    try {
        // Create directory if it doesn't exist
        fs::create_directories(fs::path(filePath).parent_path());
        
        // Write to temporary file first, then rename (atomic operation)
        std::string tempPath = filePath + ".tmp";
        {
            std::ofstream file(tempPath);
            if (!file.is_open()) {
                std::cerr << "Error: Could not create temporary session file: " << tempPath << std::endl;
                return;
            }
            
            std::string serialized = session.serialize();
            file << serialized;
            file.close();
            
            if (file.fail()) {
                std::cerr << "Error: Failed to write session data to: " << tempPath << std::endl;
                try {
                    fs::remove(tempPath);
                } catch (...) {
                    // Ignore removal errors
                }
                return;
            }
        }
        
        // Verify the temp file was created and has content
        if (!fs::exists(tempPath) || fs::file_size(tempPath) == 0) {
            std::cerr << "Error: Temporary session file is empty or missing: " << tempPath << std::endl;
            return;
        }
        
        // Atomically replace the old session file
        try {
            // On POSIX systems, rename is atomic
            fs::rename(tempPath, filePath);
        } catch (const fs::filesystem_error& e) {
            // If rename fails, try remove + rename
            std::cerr << "Warning: Atomic rename failed, trying alternative: " << e.what() << std::endl;
            try {
                if (fs::exists(filePath)) {
                    fs::remove(filePath);
                }
                fs::rename(tempPath, filePath);
            } catch (const fs::filesystem_error& e2) {
                std::cerr << "Error: Failed to save session file: " << e2.what() << std::endl;
                // Try to clean up temp file
                try {
                    fs::remove(tempPath);
                } catch (...) {}
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving session: " << e.what() << std::endl;
    }
}

void SessionManager::deleteSession(const std::string& name) {
    std::string filePath = getSessionFilePath(name);
    try {
        if (fs::exists(filePath)) {
            fs::remove(filePath);
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Warning: Could not delete session file: " << e.what() << std::endl;
    }
}

std::vector<SessionInfo> SessionManager::listSessions() {
    std::vector<SessionInfo> sessions;
    
    try {
        if (!fs::exists(sessionPath) || !fs::is_directory(sessionPath)) {
            return sessions;
        }
        
        for (const auto& entry : fs::directory_iterator(sessionPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                try {
                    // Load session to get details
                    std::string sessionName = entry.path().stem().string();
                    
                    // Quick load just to get basic info without full deserialization
                    std::ifstream file(entry.path());
                    if (!file.is_open()) {
                        continue;
                    }
                    
                    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    file.close();
                    
                    if (data.empty()) {
                        continue;
                    }
                    
                    // Parse just the fields we need
                    Json::Value root;
                    Json::Reader reader;
                    if (!reader.parse(data, root)) {
                        continue;
                    }
                    
                    SessionInfo info;
                    info.name = sessionName;
                    info.url = root.get("currentUrl", "").asString();
                    if (info.url.empty()) {
                        info.url = root.get("url", "").asString(); // Backward compatibility
                    }
                    
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
                    auto lastAccessed = root.get("lastAccessed", 0).asInt64();
                    if (lastAccessed == 0) {
                        // Use file modification time as fallback
                        auto ftime = fs::last_write_time(entry.path());
                        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                        );
                        lastAccessed = std::chrono::duration_cast<std::chrono::seconds>(
                            sctp.time_since_epoch()
                        ).count();
                    }
                    
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
                    // Skip sessions that can't be loaded, but don't spam errors
                    std::cerr << "Warning: Skipping unreadable session file: " << entry.path().filename() << std::endl;
                    continue;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Warning: Error listing sessions: " << e.what() << std::endl;
        return sessions;
    }
    
    return sessions;
}

std::string SessionManager::getSessionFilePath(const std::string& name) const {
    return fs::path(sessionPath) / (name + ".json");
}
