#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Session/Session.h"
#include "../../src/hweb/Services/HeaderService.h"
#include <json/json.h>
#include <filesystem>
#include <fstream>
#include <unistd.h>

namespace fs = std::filesystem;

class HeaderServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = fs::temp_directory_path() / "hweb_header_tests";
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        fs::remove_all(test_dir);
    }

    fs::path test_dir;
    
    HttpHeaders createTestHeader(const std::string& url = "https://api.example.com/test") {
        HttpHeaders header;
        header.url = url;
        header.method = "GET";
        header.statusCode = 200;
        header.timestamp = 1709000000;
        header.requestHeaders["Content-Type"] = "application/json";
        header.requestHeaders["Authorization"] = "Bearer test_token";
        header.responseHeaders["Content-Type"] = "application/json";
        header.responseHeaders["X-Custom-Header"] = "test_value";
        return header;
    }
};

// ========== Export Tests ==========

TEST_F(HeaderServiceTest, ExportSingleHeaderToFile) {
    Session session("test");
    session.addHttpHeader(createTestHeader());
    
    std::string output_file = (test_dir / "export_single.json").string();
    bool result = HWeb::HeaderService::exportHeadersToFile(session, output_file);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(output_file));
    
    // Verify file content
    auto loaded = HWeb::HeaderService::loadHeadersFromFile(output_file);
    EXPECT_EQ(loaded.size(), 1);
    EXPECT_EQ(loaded[0].url, "https://api.example.com/test");
}

TEST_F(HeaderServiceTest, ExportMultipleHeadersToFile) {
    Session session("test");
    
    for (int i = 0; i < 5; i++) {
        HttpHeaders header = createTestHeader("https://api.example.com/endpoint" + std::to_string(i));
        session.addHttpHeader(header);
    }
    
    std::string output_file = (test_dir / "export_multiple.json").string();
    bool result = HWeb::HeaderService::exportHeadersToFile(session, output_file);
    
    EXPECT_TRUE(result);
    auto loaded = HWeb::HeaderService::loadHeadersFromFile(output_file);
    EXPECT_EQ(loaded.size(), 5);
}

TEST_F(HeaderServiceTest, ExportWithUrlFilter) {
    Session session("test");
    
    HttpHeaders header1 = createTestHeader("https://api.example.com/users");
    session.addHttpHeader(header1);
    
    HttpHeaders header2 = createTestHeader("https://cdn.example.com/assets");
    session.addHttpHeader(header2);
    
    std::string output_file = (test_dir / "export_filtered.json").string();
    bool result = HWeb::HeaderService::exportHeadersToFile(session, output_file, "api.example.com");
    
    EXPECT_TRUE(result);
    auto loaded = HWeb::HeaderService::loadHeadersFromFile(output_file);
    EXPECT_EQ(loaded.size(), 1);
    EXPECT_EQ(loaded[0].url, "https://api.example.com/users");
}

TEST_F(HeaderServiceTest, ExportWithHeaderNameFilter) {
    Session session("test");
    
    HttpHeaders header1;
    header1.url = "https://api.example.com/test1";
    header1.method = "GET";
    header1.requestHeaders["Authorization"] = "Bearer token1";
    session.addHttpHeader(header1);
    
    HttpHeaders header2;
    header2.url = "https://api.example.com/test2";
    header2.method = "GET";
    header2.requestHeaders["X-Custom"] = "value";
    session.addHttpHeader(header2);
    
    std::string output_file = (test_dir / "export_header_filter.json").string();
    bool result = HWeb::HeaderService::exportHeadersToFile(session, output_file, "Authorization");
    
    EXPECT_TRUE(result);
    auto loaded = HWeb::HeaderService::loadHeadersFromFile(output_file);
    EXPECT_EQ(loaded.size(), 1);
    EXPECT_EQ(loaded[0].requestHeaders.count("Authorization"), 1);
}

TEST_F(HeaderServiceTest, ExportEmptySession) {
    Session session("test");
    std::string output_file = (test_dir / "export_empty.json").string();
    
    bool result = HWeb::HeaderService::exportHeadersToFile(session, output_file);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(output_file));
    
    auto loaded = HWeb::HeaderService::loadHeadersFromFile(output_file);
    EXPECT_TRUE(loaded.empty());
}

TEST_F(HeaderServiceTest, ExportCreatesParentDirectories) {
    Session session("test");
    session.addHttpHeader(createTestHeader());
    
    std::string output_file = (test_dir / "nested" / "dir" / "export.json").string();
    bool result = HWeb::HeaderService::exportHeadersToFile(session, output_file);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(fs::exists(output_file));
}

// ========== Import Tests ==========

TEST_F(HeaderServiceTest, ImportHeadersFromFile) {
    // First create a file to import
    Session source_session("source");
    source_session.addHttpHeader(createTestHeader());
    source_session.addHttpHeader(createTestHeader("https://api.example.com/second"));
    
    std::string temp_file = (test_dir / "import_test.json").string();
    HWeb::HeaderService::exportHeadersToFile(source_session, temp_file);
    
    // Import into new session
    Session target_session("target");
    int imported = HWeb::HeaderService::importHeadersFromFile(target_session, temp_file);
    
    EXPECT_EQ(imported, 2);
    EXPECT_EQ(target_session.getHttpHeaders().size(), 2);
}

TEST_F(HeaderServiceTest, ImportNonExistentFile) {
    Session session("test");
    int result = HWeb::HeaderService::importHeadersFromFile(session, "/nonexistent/path/file.json");
    EXPECT_EQ(result, -1);
}

TEST_F(HeaderServiceTest, ImportInvalidJsonFile) {
    std::string invalid_file = (test_dir / "invalid.json").string();
    std::ofstream f(invalid_file);
    f << "not valid json";
    f.close();
    
    Session session("test");
    int result = HWeb::HeaderService::importHeadersFromFile(session, invalid_file);
    EXPECT_EQ(result, -1);
}

TEST_F(HeaderServiceTest, ImportJsonArray) {
    std::string invalid_file = (test_dir / "not_array.json").string();
    std::ofstream f(invalid_file);
    f << R"({"url": "https://example.com"})";
    f.close();
    
    Session session("test");
    int result = HWeb::HeaderService::importHeadersFromFile(session, invalid_file);
    EXPECT_EQ(result, -1);
}

// ========== Load Tests ==========

TEST_F(HeaderServiceTest, LoadHeadersDirectly) {
    Session session("test");
    session.addHttpHeader(createTestHeader());
    
    std::string file = (test_dir / "load_test.json").string();
    HWeb::HeaderService::exportHeadersToFile(session, file);
    
    auto loaded = HWeb::HeaderService::loadHeadersFromFile(file);
    EXPECT_EQ(loaded.size(), 1);
    EXPECT_EQ(loaded[0].url, "https://api.example.com/test");
}

TEST_F(HeaderServiceTest, LoadEmptyFile) {
    std::string empty_file = (test_dir / "empty.json").string();
    std::ofstream f(empty_file);
    f << "[]";
    f.close();
    
    auto loaded = HWeb::HeaderService::loadHeadersFromFile(empty_file);
    EXPECT_TRUE(loaded.empty());
}

// ========== Validation Tests ==========

TEST_F(HeaderServiceTest, ValidateValidFile) {
    Session session("test");
    session.addHttpHeader(createTestHeader());
    
    std::string file = (test_dir / "valid.json").string();
    HWeb::HeaderService::exportHeadersToFile(session, file);
    
    std::string error = HWeb::HeaderService::validateHeadersFile(file);
    EXPECT_TRUE(error.empty());
}

TEST_F(HeaderServiceTest, ValidateNonExistentFile) {
    std::string error = HWeb::HeaderService::validateHeadersFile("/nonexistent/file.json");
    EXPECT_FALSE(error.empty());
    EXPECT_THAT(error, testing::HasSubstr("does not exist"));
}

TEST_F(HeaderServiceTest, ValidateInvalidJson) {
    std::string file = (test_dir / "invalid_json.json").string();
    std::ofstream f(file);
    f << "not json";
    f.close();
    
    std::string error = HWeb::HeaderService::validateHeadersFile(file);
    EXPECT_FALSE(error.empty());
    EXPECT_THAT(error, testing::HasSubstr("Invalid JSON"));
}

TEST_F(HeaderServiceTest, ValidateNonArrayJson) {
    std::string file = (test_dir / "not_array.json").string();
    std::ofstream f(file);
    f << R"({"key": "value"})";
    f.close();
    
    std::string error = HWeb::HeaderService::validateHeadersFile(file);
    EXPECT_FALSE(error.empty());
    EXPECT_THAT(error, testing::HasSubstr("must contain a JSON array"));
}

TEST_F(HeaderServiceTest, ValidateMissingUrlField) {
    std::string file = (test_dir / "missing_url.json").string();
    std::ofstream f(file);
    f << R"([{"method": "GET"}])";
    f.close();
    
    std::string error = HWeb::HeaderService::validateHeadersFile(file);
    EXPECT_FALSE(error.empty());
    EXPECT_THAT(error, testing::HasSubstr("missing 'url' field"));
}

// ========== Statistics Tests ==========

TEST_F(HeaderServiceTest, GetStatsBasic) {
    Session session("test");
    
    HttpHeaders header1 = createTestHeader("https://api.example.com/users");
    header1.method = "GET";
    session.addHttpHeader(header1);
    
    HttpHeaders header2 = createTestHeader("https://api.example.com/posts");
    header2.method = "POST";
    session.addHttpHeader(header2);
    
    std::string file = (test_dir / "stats_test.json").string();
    HWeb::HeaderService::exportHeadersToFile(session, file);
    
    auto stats = HWeb::HeaderService::getHeadersFileStats(file);
    
    EXPECT_EQ(stats.totalHeaders, 2);
    EXPECT_EQ(stats.uniqueUrls, 2);
    EXPECT_EQ(stats.uniqueDomains, 1);  // Both are api.example.com
    EXPECT_EQ(stats.methods.size(), 2);  // GET and POST
}

TEST_F(HeaderServiceTest, GetStatsMultipleDomains) {
    Session session("test");
    
    HttpHeaders header1 = createTestHeader("https://api.example.com/test");
    session.addHttpHeader(header1);
    
    HttpHeaders header2 = createTestHeader("https://cdn.example.com/test");
    session.addHttpHeader(header2);
    
    HttpHeaders header3 = createTestHeader("https://other.com/test");
    session.addHttpHeader(header3);
    
    std::string file = (test_dir / "multi_domain.json").string();
    HWeb::HeaderService::exportHeadersToFile(session, file);
    
    auto stats = HWeb::HeaderService::getHeadersFileStats(file);
    
    EXPECT_EQ(stats.uniqueDomains, 3);
}

TEST_F(HeaderServiceTest, GetStatsTimestamps) {
    Session session("test");
    
    HttpHeaders header1;
    header1.url = "https://example.com/1";
    header1.timestamp = 1709000000;
    session.addHttpHeader(header1);
    
    HttpHeaders header2;
    header2.url = "https://example.com/2";
    header2.timestamp = 1709000100;
    session.addHttpHeader(header2);
    
    HttpHeaders header3;
    header3.url = "https://example.com/3";
    header3.timestamp = 1709000050;
    session.addHttpHeader(header3);
    
    std::string file = (test_dir / "timestamps.json").string();
    HWeb::HeaderService::exportHeadersToFile(session, file);
    
    auto stats = HWeb::HeaderService::getHeadersFileStats(file);
    
    EXPECT_EQ(stats.oldestTimestamp, 1709000000);
    EXPECT_EQ(stats.newestTimestamp, 1709000100);
}

TEST_F(HeaderServiceTest, GetStatsEmptyFile) {
    std::string file = (test_dir / "empty_stats.json").string();
    std::ofstream f(file);
    f << "[]";
    f.close();
    
    auto stats = HWeb::HeaderService::getHeadersFileStats(file);
    
    EXPECT_EQ(stats.totalHeaders, 0);
    EXPECT_EQ(stats.uniqueUrls, 0);
    EXPECT_EQ(stats.uniqueDomains, 0);
}

// ========== Export Vectors Tests ==========

TEST_F(HeaderServiceTest, ExportVectorOfHeaders) {
    std::vector<HttpHeaders> headers;
    
    for (int i = 0; i < 3; i++) {
        HttpHeaders header = createTestHeader("https://api.example.com/" + std::to_string(i));
        headers.push_back(header);
    }
    
    std::string file = (test_dir / "export_vector.json").string();
    bool result = HWeb::HeaderService::exportHeadersToFile(headers, file);
    
    EXPECT_TRUE(result);
    auto loaded = HWeb::HeaderService::loadHeadersFromFile(file);
    EXPECT_EQ(loaded.size(), 3);
}

TEST_F(HeaderServiceTest, ExportVectorWithFilter) {
    std::vector<HttpHeaders> headers;
    
    HttpHeaders header1 = createTestHeader("https://api.example.com/include");
    headers.push_back(header1);
    
    HttpHeaders header2 = createTestHeader("https://other.com/exclude");
    headers.push_back(header2);
    
    std::string file = (test_dir / "export_filtered_vector.json").string();
    bool result = HWeb::HeaderService::exportHeadersToFile(
        headers, 
        file,
        [](const HttpHeaders& h) {
            return h.url.find("api.example.com") != std::string::npos;
        }
    );
    
    EXPECT_TRUE(result);
    auto loaded = HWeb::HeaderService::loadHeadersFromFile(file);
    EXPECT_EQ(loaded.size(), 1);
    EXPECT_EQ(loaded[0].url, "https://api.example.com/include");
}

// ========== Edge Cases ==========

TEST_F(HeaderServiceTest, ExportToReadOnlyPath) {
    if (getuid() == 0) {
        GTEST_SKIP() << "Skipping read-only path test when running as root";
    }

    Session session("test");
    session.addHttpHeader(createTestHeader());

    // Try to write to a path that doesn't exist and can't be created
    std::string file = "/root/cannot_write_here.json";
    bool result = HWeb::HeaderService::exportHeadersToFile(session, file);
    EXPECT_FALSE(result);
}

TEST_F(HeaderServiceTest, SpecialCharactersInHeaderValues) {
    Session session("test");
    
    HttpHeaders header;
    header.url = "https://example.com/path?param=value&other=123";
    header.method = "GET";
    header.requestHeaders["Authorization"] = "Bearer token.with特殊 chars!@#$%";
    header.responseHeaders["Set-Cookie"] = "session=abc%20def; path=/";
    session.addHttpHeader(header);
    
    std::string file = (test_dir / "special_chars.json").string();
    bool result = HWeb::HeaderService::exportHeadersToFile(session, file);
    EXPECT_TRUE(result);
    
    auto loaded = HWeb::HeaderService::loadHeadersFromFile(file);
    EXPECT_EQ(loaded.size(), 1);
    // JSON should preserve the special characters
    EXPECT_THAT(loaded[0].requestHeaders["Authorization"], testing::HasSubstr("Bearer"));
}

TEST_F(HeaderServiceTest, LargeNumberOfHeaders) {
    Session session("test");
    
    for (int i = 0; i < 1000; i++) {
        HttpHeaders header;
        header.url = "https://api.example.com/" + std::to_string(i);
        header.method = "GET";
        header.timestamp = 1709000000 + i;
        session.addHttpHeader(header);
    }
    
    std::string file = (test_dir / "large_export.json").string();
    bool result = HWeb::HeaderService::exportHeadersToFile(session, file);
    EXPECT_TRUE(result);
    
    auto stats = HWeb::HeaderService::getHeadersFileStats(file);
    EXPECT_EQ(stats.totalHeaders, 1000);
}

TEST_F(HeaderServiceTest, CommaSeparatedHeaderFilter) {
    Session session("test");
    
    HttpHeaders header1;
    header1.url = "https://api.example.com/test1";
    header1.requestHeaders["Authorization"] = "Bearer token1";
    header1.requestHeaders["X-Custom"] = "value1";
    session.addHttpHeader(header1);
    
    HttpHeaders header2;
    header2.url = "https://api.example.com/test2";
    header2.requestHeaders["Content-Type"] = "application/json";
    session.addHttpHeader(header2);
    
    std::string file = (test_dir / "comma_filter.json").string();
    bool result = HWeb::HeaderService::exportHeadersToFile(
        session, file, "Authorization,X-Custom"
    );
    
    EXPECT_TRUE(result);
    auto loaded = HWeb::HeaderService::loadHeadersFromFile(file);
    EXPECT_EQ(loaded.size(), 1);
    EXPECT_EQ(loaded[0].url, "https://api.example.com/test1");
}
