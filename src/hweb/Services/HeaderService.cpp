#include "HeaderService.h"
#include "../Output.h"
#include <fstream>
#include <filesystem>
#include <set>
#include <algorithm>
#include <regex>
#include <sstream>
#include <climits>

namespace fs = std::filesystem;

namespace HWeb {

bool HeaderService::exportHeadersToFile(
    const Session& session,
    const std::string& filePath,
    const std::string& filter
) {
    return exportHeadersToFile(
        session.getHttpHeaders(),
        filePath,
        [&filter](const HttpHeaders& header) {
            return matchesFilter(header, filter);
        }
    );
}

bool HeaderService::exportHeadersToFile(
    const std::vector<HttpHeaders>& headers,
    const std::string& filePath,
    std::function<bool(const HttpHeaders&)> filter
) {
    try {
        Json::Value root(Json::arrayValue);
        
        for (const auto& header : headers) {
            // Apply filter if provided
            if (filter && !filter(header)) {
                continue;
            }
            
            Json::Value headerObj;
            headerObj["url"] = header.url;
            headerObj["method"] = header.method;
            headerObj["statusCode"] = header.statusCode;
            headerObj["timestamp"] = static_cast<Json::Int64>(header.timestamp);
            
            Json::Value reqHeaders(Json::objectValue);
            for (const auto& [key, value] : header.requestHeaders) {
                reqHeaders[key] = value;
            }
            headerObj["requestHeaders"] = reqHeaders;
            
            Json::Value respHeaders(Json::objectValue);
            for (const auto& [key, value] : header.responseHeaders) {
                respHeaders[key] = value;
            }
            headerObj["responseHeaders"] = respHeaders;
            
            root.append(headerObj);
        }
        
        // Create parent directories if needed
        fs::path pathObj(filePath);
        if (pathObj.has_parent_path()) {
            fs::create_directories(pathObj.parent_path());
        }
        
        // Write to file
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::string jsonStr = Json::writeString(builder, root);
        file << jsonStr;
        file.close();
        
        return !file.fail();
        
    } catch (const std::exception& e) {
        return false;
    }
}

int HeaderService::importHeadersFromFile(
    Session& session,
    const std::string& filePath
) {
    try {
        // Validate file structure first so we can distinguish errors from empty files
        std::string validationError = validateHeadersFile(filePath);
        if (!validationError.empty()) {
            return -1;
        }

        auto headers = loadHeadersFromFile(filePath);

        for (const auto& header : headers) {
            session.addHttpHeader(header);
        }

        return static_cast<int>(headers.size());

    } catch (const std::exception& e) {
        return -1;
    }
}

std::vector<HttpHeaders> HeaderService::loadHeadersFromFile(
    const std::string& filePath
) {
    std::vector<HttpHeaders> headers;
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return headers;
        }
        
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(file, root)) {
            return headers;
        }
        
        if (!root.isArray()) {
            return headers;
        }
        
        Session dummySession("dummy");
        for (const auto& headerJson : root) {
            auto header = dummySession.jsonToHttpHeaders(headerJson);
            headers.push_back(header);
        }
        
        return headers;
        
    } catch (const std::exception& e) {
        return headers;
    }
}

std::string HeaderService::validateHeadersFile(const std::string& filePath) {
    try {
        // Check file exists
        if (!fs::exists(filePath)) {
            return "File does not exist: " + filePath;
        }
        
        // Check file is readable
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return "Cannot open file: " + filePath;
        }
        
        // Parse JSON
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(file, root)) {
            return "Invalid JSON format: " + filePath;
        }
        
        // Check it's an array
        if (!root.isArray()) {
            return "Headers file must contain a JSON array";
        }
        
        // Validate each header entry
        size_t index = 0;
        for (const auto& headerJson : root) {
            if (!headerJson.isObject()) {
                return "Header entry at index " + std::to_string(index) + " is not an object";
            }
            
            if (!headerJson.isMember("url")) {
                return "Header entry at index " + std::to_string(index) + " missing 'url' field";
            }
            
            if (!headerJson["url"].isString()) {
                return "Header entry at index " + std::to_string(index) + " has non-string 'url'";
            }
            
            index++;
        }
        
        return ""; // Valid
        
    } catch (const std::exception& e) {
        return "Error validating file: " + std::string(e.what());
    }
}

HeaderService::HeadersStats HeaderService::getHeadersFileStats(const std::string& filePath) {
    HeadersStats stats;
    stats.totalHeaders = 0;
    stats.uniqueUrls = 0;
    stats.uniqueDomains = 0;
    stats.oldestTimestamp = INT64_MAX;
    stats.newestTimestamp = 0;
    
    try {
        auto headers = loadHeadersFromFile(filePath);
        
        std::set<std::string> uniqueUrls;
        std::set<std::string> uniqueDomains;
        std::set<std::string> methods;
        
        for (const auto& header : headers) {
            stats.totalHeaders++;
            uniqueUrls.insert(header.url);
            uniqueDomains.insert(extractDomain(header.url));
            methods.insert(header.method);
            
            if (header.timestamp > 0) {
                stats.oldestTimestamp = std::min(stats.oldestTimestamp, header.timestamp);
                stats.newestTimestamp = std::max(stats.newestTimestamp, header.timestamp);
            }
        }
        
        stats.uniqueUrls = uniqueUrls.size();
        stats.uniqueDomains = uniqueDomains.size();
        stats.methods = std::vector<std::string>(methods.begin(), methods.end());
        
        if (stats.totalHeaders == 0) {
            stats.oldestTimestamp = 0;
        }
        
    } catch (...) {
        // Return empty stats on error
    }
    
    return stats;
}

bool HeaderService::matchesFilter(
    const HttpHeaders& header,
    const std::string& filter
) {
    if (filter.empty()) {
        return true; // No filter = match all
    }
    
    // First check if it matches as a URL pattern
    if (header.url.find(filter) != std::string::npos) {
        return true;
    }
    
    // Check if filter is a comma-separated list of header names
    if (filter.find(',') != std::string::npos) {
        std::stringstream ss(filter);
        std::string name;

        while (std::getline(ss, name, ',')) {
            // Trim leading whitespace
            auto start = name.find_first_not_of(" \t");
            if (start == std::string::npos) {
                continue; // all-whitespace token — skip
            }
            auto end = name.find_last_not_of(" \t");
            name = name.substr(start, end - start + 1);

            // Check if this header name exists in request or response headers
            if (header.requestHeaders.count(name) > 0 ||
                header.responseHeaders.count(name) > 0) {
                return true;
            }
        }

        return false;
    }
    
    // Single header name check (no comma)
    if (header.requestHeaders.count(filter) > 0 ||
        header.responseHeaders.count(filter) > 0) {
        return true;
    }

    return false;
}

std::string HeaderService::extractDomain(const std::string& url) {
    try {
        // Simple URL parsing: extract domain from http(s)://domain/path
        size_t start = url.find("://");
        if (start == std::string::npos) {
            start = 0;
        } else {
            start += 3; // Skip "://"
        }
        
        size_t end = url.find('/', start);
        if (end == std::string::npos) {
            end = url.length();
        }
        
        // Remove port if present
        std::string domain = url.substr(start, end - start);
        size_t portPos = domain.find(':');
        if (portPos != std::string::npos) {
            domain = domain.substr(0, portPos);
        }
        
        return domain;
        
    } catch (...) {
        return url;
    }
}

} // namespace HWeb
