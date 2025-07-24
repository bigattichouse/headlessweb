#include "PathUtils.h"
#include <cstring>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <pwd.h>
#endif

namespace FileOps {
    
    // ========== Path Normalization ==========
    
    std::string PathUtils::normalizePath(const std::string& path) {
        if (path.empty()) {
            return path;
        }
        
        std::string normalized;
        
        try {
            std::filesystem::path fs_path(path);
            normalized = fs_path.lexically_normal().string();
        } catch (const std::exception& e) {
            // Fallback for invalid paths
            normalized = path;
        }
        
        // Always replace backslashes with forward slashes on non-Windows systems
        // This ensures cross-platform consistency
        #ifndef _WIN32
            std::replace(normalized.begin(), normalized.end(), '\\', '/');
        #endif
        
        return normalized;
    }
    
    std::string PathUtils::toAbsolutePath(const std::string& path) {
        if (path.empty()) {
            return std::filesystem::current_path().string();
        }
        
        try {
            std::filesystem::path fs_path(path);
            return std::filesystem::absolute(fs_path).string();
        } catch (const std::exception& e) {
            std::cerr << "Error converting to absolute path: " << e.what() << std::endl;
            return path;
        }
    }
    
    std::string PathUtils::getFileName(const std::string& path) {
        try {
            return std::filesystem::path(path).filename().string();
        } catch (const std::exception& e) {
            // Fallback implementation
            size_t last_sep = path.find_last_of("/\\");
            if (last_sep != std::string::npos) {
                return path.substr(last_sep + 1);
            }
            return path;
        }
    }
    
    std::string PathUtils::getDirectory(const std::string& path) {
        try {
            return std::filesystem::path(path).parent_path().string();
        } catch (const std::exception& e) {
            // Fallback implementation
            size_t last_sep = path.find_last_of("/\\");
            if (last_sep != std::string::npos) {
                return path.substr(0, last_sep);
            }
            return ".";
        }
    }
    
    std::string PathUtils::getExtension(const std::string& path) {
        try {
            return std::filesystem::path(path).extension().string();
        } catch (const std::exception& e) {
            // Fallback implementation
            std::string filename = getFileName(path);
            size_t dot_pos = filename.find_last_of('.');
            if (dot_pos != std::string::npos && dot_pos != 0) {
                return filename.substr(dot_pos);
            }
            return "";
        }
    }
    
    std::string PathUtils::joinPaths(const std::vector<std::string>& components) {
        if (components.empty()) {
            return "";
        }
        
        std::filesystem::path result;
        for (const auto& component : components) {
            if (!component.empty()) {
                result /= component;
            }
        }
        
        return result.string();
    }
    
    // ========== Platform-Specific Operations ==========
    
    std::string PathUtils::getDefaultDownloadDirectory() {
        std::string downloads_dir;
        
        // Check environment variable first
        const char* hweb_download_dir = std::getenv("HWEB_DOWNLOAD_DIR");
        if (hweb_download_dir && strlen(hweb_download_dir) > 0) {
            downloads_dir = hweb_download_dir;
            if (isDirectory(downloads_dir)) {
                return normalizePath(downloads_dir);
            }
        }
        
        #ifdef _WIN32
            // Windows: Use SHGetFolderPath for Downloads folder
            CHAR path[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
                downloads_dir = std::string(path) + "\\Downloads";
            } else {
                downloads_dir = getHomeDirectory() + "\\Downloads";
            }
        #elif defined(__APPLE__)
            // macOS: ~/Downloads
            downloads_dir = getHomeDirectory() + "/Downloads";
        #else
            // Linux: Check XDG directories and common locations
            const char* xdg_download = std::getenv("XDG_DOWNLOAD_DIR");
            if (xdg_download && strlen(xdg_download) > 0 && isDirectory(xdg_download)) {
                downloads_dir = xdg_download;
            } else {
                // Try to read from ~/.config/user-dirs.dirs
                std::string home = getHomeDirectory();
                std::string user_dirs_file = home + "/.config/user-dirs.dirs";
                std::ifstream dirs_file(user_dirs_file);
                
                if (dirs_file.is_open()) {
                    std::string line;
                    while (std::getline(dirs_file, line)) {
                        if (line.find("XDG_DOWNLOAD_DIR=") == 0) {
                            // Extract path from XDG_DOWNLOAD_DIR="$HOME/Downloads"
                            size_t start = line.find('"');
                            size_t end = line.find('"', start + 1);
                            if (start != std::string::npos && end != std::string::npos) {
                                std::string xdg_path = line.substr(start + 1, end - start - 1);
                                // Replace $HOME with actual home directory
                                if (xdg_path.find("$HOME") == 0) {
                                    xdg_path = home + xdg_path.substr(5);
                                }
                                if (isDirectory(xdg_path)) {
                                    downloads_dir = xdg_path;
                                    break;
                                }
                            }
                        }
                    }
                    dirs_file.close();
                }
                
                // Fallback to common locations
                if (downloads_dir.empty()) {
                    std::vector<std::string> candidates = {
                        home + "/Downloads",
                        home + "/downloads", 
                        home + "/Download",
                        home + "/下载",  // Chinese
                        home + "/Téléchargements",  // French
                        home + "/Descargas"  // Spanish
                    };
                    
                    for (const auto& candidate : candidates) {
                        if (isDirectory(candidate)) {
                            downloads_dir = candidate;
                            break;
                        }
                    }
                    
                    // If none found, use ~/Downloads as default
                    if (downloads_dir.empty()) {
                        downloads_dir = home + "/Downloads";
                    }
                }
            }
        #endif
        
        // Fallback to current directory + downloads if Downloads doesn't exist
        if (!isDirectory(downloads_dir)) {
            downloads_dir = std::filesystem::current_path().string() + "/downloads";
            createDirectoriesIfNeeded(downloads_dir);
        }
        
        return normalizePath(downloads_dir);
    }
    
    std::string PathUtils::getHomeDirectory() {
        #ifdef _WIN32
            const char* home = std::getenv("USERPROFILE");
            if (!home) {
                home = std::getenv("HOMEDRIVE");
                const char* homepath = std::getenv("HOMEPATH");
                if (home && homepath) {
                    return std::string(home) + std::string(homepath);
                }
            }
            return home ? std::string(home) : "C:\\";
        #else
            const char* home = std::getenv("HOME");
            if (home) {
                return std::string(home);
            }
            
            // Fallback: get from passwd
            struct passwd* pw = getpwuid(getuid());
            if (pw && pw->pw_dir) {
                return std::string(pw->pw_dir);
            }
            
            return "/tmp";
        #endif
    }
    
    std::string PathUtils::getTempDirectory() {
        std::string temp_base;
        
        #ifdef _WIN32
            const char* temp = std::getenv("TEMP");
            if (!temp) temp = std::getenv("TMP");
            temp_base = temp ? std::string(temp) : "C:\\Windows\\Temp";
        #else
            temp_base = "/tmp";
        #endif
        
        std::string hweb_temp = joinPaths({temp_base, "hweb-fileops"});
        createDirectoriesIfNeeded(hweb_temp);
        
        return hweb_temp;
    }
    
    bool PathUtils::createDirectoriesIfNeeded(const std::string& path) {
        try {
            // Check if already exists first
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                return true;
            }
            
            return std::filesystem::create_directories(path);
        } catch (const std::exception& e) {
            std::cerr << "Error creating directories: " << e.what() << std::endl;
            return false;
        }
    }
    
    // ========== File System Queries ==========
    
    bool PathUtils::exists(const std::string& path) {
        try {
            return std::filesystem::exists(path);
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    bool PathUtils::isFile(const std::string& path) {
        try {
            return std::filesystem::is_regular_file(path);
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    bool PathUtils::isDirectory(const std::string& path) {
        try {
            return std::filesystem::is_directory(path);
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    bool PathUtils::isReadable(const std::string& path) {
        #ifdef _WIN32
            return _access(path.c_str(), 4) == 0; // R_OK = 4
        #else
            return access(path.c_str(), R_OK) == 0;
        #endif
    }
    
    bool PathUtils::isWritable(const std::string& path) {
        #ifdef _WIN32
            return _access(path.c_str(), 2) == 0; // W_OK = 2
        #else
            return access(path.c_str(), W_OK) == 0;
        #endif
    }
    
    size_t PathUtils::getFileSize(const std::string& path) {
        try {
            return std::filesystem::file_size(path);
        } catch (const std::exception& e) {
            return 0;
        }
    }
    
    std::chrono::system_clock::time_point PathUtils::getModificationTime(const std::string& path) {
        try {
            auto ftime = std::filesystem::last_write_time(path);
            // Convert to system_clock time_point
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            return sctp;
        } catch (const std::exception& e) {
            return std::chrono::system_clock::time_point{};
        }
    }
    
    // ========== Security and Validation ==========
    
    bool PathUtils::isSecurePath(const std::string& path) {
        if (path.empty()) {
            return false;
        }
        
        // Check for null bytes
        if (path.find('\0') != std::string::npos) {
            return false;
        }
        
        // Check for directory traversal patterns
        if (path.find("..") != std::string::npos) {
            // Allow .. only if it's part of a legitimate path component
            std::string normalized = normalizePath(path);
            if (normalized.find("..") != std::string::npos) {
                return false;
            }
        }
        
        // Check path length
        if (!isValidPathLength(path)) {
            return false;
        }
        
        // Check for forbidden characters in paths (less restrictive than filenames)
        std::vector<char> path_forbidden;
        #ifdef _WIN32
            path_forbidden = {'<', '>', '"', '|', '?', '*'};  // Allow : for drive letters
        #else
            path_forbidden = {'\0', '<', '>'};  // Allow : in paths
        #endif
        
        for (char c : path) {
            if (std::find(path_forbidden.begin(), path_forbidden.end(), c) != path_forbidden.end()) {
                return false;
            }
        }
        
        return true;
    }
    
    std::string PathUtils::sanitizeFileName(const std::string& filename) {
        std::string sanitized = filename;
        auto forbidden = getForbiddenChars();
        
        for (char& c : sanitized) {
            if (std::find(forbidden.begin(), forbidden.end(), c) != forbidden.end()) {
                c = '_';
            }
        }
        
        // Remove leading/trailing whitespace and dots
        sanitized.erase(0, sanitized.find_first_not_of(" \t."));
        sanitized.erase(sanitized.find_last_not_of(" \t.") + 1);
        
        // Ensure filename isn't empty after sanitization
        if (sanitized.empty()) {
            sanitized = "file";
        }
        
        return sanitized;
    }
    
    bool PathUtils::isValidPathLength(const std::string& path) {
        #ifdef _WIN32
            return path.length() < MAX_PATH; // 260 characters on Windows
        #else
            return path.length() < 4096; // Typical Linux limit
        #endif
    }
    
    bool PathUtils::isAllowedFileType(const std::string& path, const std::vector<std::string>& allowed_types) {
        if (allowed_types.empty() || 
            (allowed_types.size() == 1 && allowed_types[0] == "*")) {
            return true; // Allow all types
        }
        
        std::string extension = getExtension(path);
        if (extension.empty()) {
            extension = ""; // Files without extension
        }
        
        // Convert to lowercase for case-insensitive comparison
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        for (const auto& allowed : allowed_types) {
            std::string allowed_lower = allowed;
            std::transform(allowed_lower.begin(), allowed_lower.end(), allowed_lower.begin(), ::tolower);
            
            if (allowed_lower == "*" || 
                allowed_lower == extension ||
                (allowed_lower.front() == '.' && allowed_lower == extension) ||
                (extension.front() == '.' && allowed_lower == extension.substr(1))) {
                return true;
            }
        }
        
        return false;
    }
    
    // ========== Pattern Matching ==========
    
    std::vector<std::string> PathUtils::findFilesMatchingPattern(
        const std::string& directory, 
        const std::string& pattern) {
        
        std::vector<std::string> matches;
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    
                    if (isRegexPattern(pattern)) {
                        if (matchesRegexPattern(filename, pattern)) {
                            matches.push_back(entry.path().string());
                        }
                    } else {
                        if (matchesGlobPattern(filename, pattern)) {
                            matches.push_back(entry.path().string());
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error searching directory: " << e.what() << std::endl;
        }
        
        return matches;
    }
    
    bool PathUtils::matchesGlobPattern(const std::string& filename, const std::string& pattern) {
        try {
            std::string regex_pattern = globToRegex(pattern);
            std::regex regex(regex_pattern, std::regex_constants::icase);
            return std::regex_match(filename, regex);
        } catch (const std::exception& e) {
            // Fallback to simple wildcard matching
            return filename.find(pattern.substr(0, pattern.find('*'))) != std::string::npos;
        }
    }
    
    bool PathUtils::matchesRegexPattern(const std::string& filename, const std::string& pattern) {
        if (!isRegexPattern(pattern)) {
            return false;
        }
        
        try {
            // Extract regex from /pattern/ format
            std::string regex_str = pattern.substr(1, pattern.length() - 2);
            std::regex regex(regex_str, std::regex_constants::icase);
            return std::regex_match(filename, regex);
        } catch (const std::exception& e) {
            std::cerr << "Invalid regex pattern: " << pattern << std::endl;
            return false;
        }
    }
    
    bool PathUtils::isGlobPattern(const std::string& pattern) {
        return pattern.find('*') != std::string::npos || 
               pattern.find('?') != std::string::npos ||
               pattern.find('[') != std::string::npos;
    }
    
    bool PathUtils::isRegexPattern(const std::string& pattern) {
        return pattern.length() >= 3 && 
               pattern.front() == '/' && 
               pattern.back() == '/';
    }
    
    // ========== File Operations ==========
    
    bool PathUtils::copyFile(const std::string& source, const std::string& destination) {
        try {
            // Create destination directory if needed
            createDirectoriesIfNeeded(getDirectory(destination));
            
            std::filesystem::copy_file(source, destination, 
                std::filesystem::copy_options::overwrite_existing);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error copying file: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool PathUtils::moveFile(const std::string& source, const std::string& destination) {
        try {
            // Create destination directory if needed
            createDirectoriesIfNeeded(getDirectory(destination));
            
            std::filesystem::rename(source, destination);
            return true;
        } catch (const std::exception& e) {
            // Try copy + delete as fallback for cross-filesystem moves
            if (copyFile(source, destination)) {
                return deleteFile(source);
            }
            std::cerr << "Error moving file: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool PathUtils::deleteFile(const std::string& path) {
        try {
            return std::filesystem::remove(path);
        } catch (const std::exception& e) {
            std::cerr << "Error deleting file: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool PathUtils::createEmptyFile(const std::string& path) {
        try {
            createDirectoriesIfNeeded(getDirectory(path));
            
            std::ofstream file(path);
            return file.good();
        } catch (const std::exception& e) {
            std::cerr << "Error creating file: " << e.what() << std::endl;
            return false;
        }
    }
    
    // ========== Utility Functions ==========
    
    std::string PathUtils::formatFileSize(size_t bytes) {
        const char* sizes[] = {"B", "KB", "MB", "GB", "TB"};
        int order = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024 && order < 4) {
            order++;
            size /= 1024;
        }
        
        std::ostringstream oss;
        if (order == 0) {
            oss << static_cast<size_t>(size) << " " << sizes[order];
        } else {
            oss.precision(1);
            oss << std::fixed << size << " " << sizes[order];
        }
        
        return oss.str();
    }
    
    std::string PathUtils::pathToUri(const std::string& path) {
        std::string absolute = toAbsolutePath(path);
        
        #ifdef _WIN32
            // Convert Windows path to file URI
            std::string uri = "file:///";
            for (char c : absolute) {
                if (c == '\\') {
                    uri += '/';
                } else {
                    uri += c;
                }
            }
            return uri;
        #else
            return "file://" + absolute;
        #endif
    }
    
    std::string PathUtils::uriToPath(const std::string& uri) {
        if (uri.substr(0, 7) == "file://") {
            std::string path = uri.substr(7);
            
            #ifdef _WIN32
                // Handle Windows file URIs (file:///C:/path)
                if (path.front() == '/') {
                    path = path.substr(1);
                }
                std::replace(path.begin(), path.end(), '/', '\\');
            #endif
            
            return path;
        }
        
        return uri; // Return as-is if not a file URI
    }
    
    std::string PathUtils::generateUniqueFileName(const std::string& path) {
        std::string directory = getDirectory(path);
        std::string filename = getFileName(path);
        std::string extension = getExtension(path);
        std::string base = filename.substr(0, filename.length() - extension.length());
        
        // Always generate a unique name, even if original doesn't exist
        static int global_counter = 0;
        global_counter++;
        
        int counter = global_counter;
        std::string unique_path;
        
        do {
            std::ostringstream oss;
            oss << base << "_" << counter << extension;
            unique_path = joinPaths({directory, oss.str()});
            counter++;
        } while (exists(unique_path));
        
        return unique_path;
    }
    
    std::string PathUtils::escapeForShell(const std::string& path) {
        #ifdef _WIN32
            // Windows: wrap in quotes if contains spaces
            if (path.find(' ') != std::string::npos) {
                return "\"" + path + "\"";
            }
            return path;
        #else
            // Unix: escape special characters
            std::string escaped;
            for (char c : path) {
                if (c == ' ' || c == '"' || c == '\'' || c == '\\' || 
                    c == '$' || c == '`' || c == '(' || c == ')') {
                    escaped += '\\';
                }
                escaped += c;
            }
            return escaped;
        #endif
    }
    
    // ========== Internal Helper Methods ==========
    
    char PathUtils::getPathSeparator() {
        #ifdef _WIN32
            return '\\';
        #else
            return '/';
        #endif
    }
    
    bool PathUtils::isValidFileNameChar(char c) {
        auto forbidden = getForbiddenChars();
        return std::find(forbidden.begin(), forbidden.end(), c) == forbidden.end();
    }
    
    std::vector<char> PathUtils::getForbiddenChars() {
        #ifdef _WIN32
            return {'<', '>', ':', '"', '|', '?', '*'};
        #else
            return {'\0', '<', '>', ':'};
        #endif
    }
    
    std::string PathUtils::globToRegex(const std::string& glob) {
        std::string regex;
        regex.reserve(glob.length() * 2);
        
        for (size_t i = 0; i < glob.length(); ++i) {
            char c = glob[i];
            switch (c) {
                case '*':
                    regex += ".*";
                    break;
                case '?':
                    regex += ".";
                    break;
                case '.':
                    regex += "\\.";
                    break;
                case '[': {
                    // Handle character classes - find the closing ]
                    size_t end = i + 1;
                    while (end < glob.length() && glob[end] != ']') {
                        end++;
                    }
                    if (end < glob.length()) {
                        // Found closing ], keep the character class as-is
                        regex += glob.substr(i, end - i + 1);
                        i = end; // Skip to after the ]
                    } else {
                        // No closing ], escape the [
                        regex += "\\[";
                    }
                    break;
                }
                case '^':
                case '$':
                case '(':
                case ')':
                case ']':  // ] outside of character class should be escaped
                case '{':
                case '}':
                case '+':
                case '|':
                case '\\':
                    regex += '\\';
                    regex += c;
                    break;
                default:
                    regex += c;
                    break;
            }
        }
        
        return regex;
    }
    
    std::string PathUtils::getPlatformType() {
        #ifdef _WIN32
            return "windows";
        #elif defined(__APPLE__)
            return "macos";
        #else
            return "linux";
        #endif
    }
    
} // namespace FileOps
