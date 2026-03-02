#pragma once

#include "../../Session/Session.h"
#include <string>
#include <vector>
#include <functional>

namespace HWeb {

/**
 * Service for exporting and importing HTTP headers to/from files
 * 
 * Provides functionality to:
 * - Export session headers to standalone JSON files
 * - Import headers from JSON files into sessions
 * - Filter headers by URL pattern or header names
 * - Apply imported headers to outgoing requests
 */
class HeaderService {
public:
    /**
     * Export headers from a session to a JSON file
     * 
     * @param session The session containing headers to export
     * @param filePath Path to the output JSON file
     * @param filter Optional filter (URL pattern or comma-separated header names)
     * @return true if export was successful
     */
    static bool exportHeadersToFile(
        const Session& session,
        const std::string& filePath,
        const std::string& filter = ""
    );

    /**
     * Import headers from a JSON file into a session
     * 
     * @param session The session to import headers into
     * @param filePath Path to the input JSON file
     * @return Number of headers imported, or -1 on error
     */
    static int importHeadersFromFile(
        Session& session,
        const std::string& filePath
    );

    /**
     * Export specific headers to a JSON file
     * 
     * @param headers The headers to export
     * @param filePath Path to the output JSON file
     * @param filter Optional filter function
     * @return true if export was successful
     */
    static bool exportHeadersToFile(
        const std::vector<HttpHeaders>& headers,
        const std::string& filePath,
        std::function<bool(const HttpHeaders&)> filter = nullptr
    );

    /**
     * Load headers from a JSON file without importing to session
     * 
     * @param filePath Path to the input JSON file
     * @return Vector of loaded headers, or empty vector on error
     */
    static std::vector<HttpHeaders> loadHeadersFromFile(
        const std::string& filePath
    );

    /**
     * Validate a headers file
     * 
     * @param filePath Path to the file to validate
     * @return Error message if invalid, empty string if valid
     */
    static std::string validateHeadersFile(const std::string& filePath);

    /**
     * Get statistics about headers in a file
     */
    struct HeadersStats {
        size_t totalHeaders = 0;
        size_t uniqueUrls = 0;
        size_t uniqueDomains = 0;
        std::vector<std::string> methods;
        int64_t oldestTimestamp = 0;
        int64_t newestTimestamp = 0;
    };

    static HeadersStats getHeadersFileStats(const std::string& filePath);

private:
    /**
     * Check if a header matches the filter
     */
    static bool matchesFilter(
        const HttpHeaders& header,
        const std::string& filter
    );

    /**
     * Extract domain from URL
     */
    static std::string extractDomain(const std::string& url);
};

} // namespace HWeb
