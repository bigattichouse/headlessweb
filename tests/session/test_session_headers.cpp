#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Session/Session.h"
#include <json/json.h>
#include <thread>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

class SessionHeadersTest : public ::testing::Test {
protected:
    void SetUp() override {
        session = std::make_unique<Session>("test_headers_session");
    }

    void TearDown() override {
        session.reset();
    }

    std::unique_ptr<Session> session;
};

// ========== Basic HTTP Headers Tests ==========

TEST_F(SessionHeadersTest, InitialHeadersEmpty) {
    EXPECT_TRUE(session->getHttpHeaders().empty());
}

TEST_F(SessionHeadersTest, AddSingleHeader) {
    HttpHeaders header;
    header.url = "https://api.example.com/login";
    header.method = "POST";
    header.statusCode = 200;
    header.timestamp = 1709000000;
    header.requestHeaders["Content-Type"] = "application/json";
    header.requestHeaders["Authorization"] = "Bearer token123";
    header.responseHeaders["Content-Type"] = "application/json";
    header.responseHeaders["Set-Cookie"] = "session=abc123; HttpOnly";

    session->addHttpHeader(header);

    const auto& headers = session->getHttpHeaders();
    EXPECT_EQ(headers.size(), 1);
    EXPECT_EQ(headers[0].url, "https://api.example.com/login");
    EXPECT_EQ(headers[0].method, "POST");
    EXPECT_EQ(headers[0].statusCode, 200);
    EXPECT_EQ(headers[0].requestHeaders.at("Content-Type"), "application/json");
    EXPECT_EQ(headers[0].responseHeaders.at("Set-Cookie"), "session=abc123; HttpOnly");
}

TEST_F(SessionHeadersTest, SetMultipleHeaders) {
    std::vector<HttpHeaders> headers;
    
    for (int i = 0; i < 3; i++) {
        HttpHeaders header;
        header.url = "https://api.example.com/endpoint" + std::to_string(i);
        header.method = "GET";
        header.statusCode = 200 + i;
        header.timestamp = 1709000000 + i;
        header.requestHeaders["X-Request-ID"] = "req-" + std::to_string(i);
        headers.push_back(header);
    }

    session->setHttpHeaders(headers);
    EXPECT_EQ(session->getHttpHeaders().size(), 3);
}

TEST_F(SessionHeadersTest, ClearHeaders) {
    HttpHeaders header;
    header.url = "https://example.com";
    session->addHttpHeader(header);
    session->addHttpHeader(header);

    EXPECT_EQ(session->getHttpHeaders().size(), 2);
    session->clearHttpHeaders();
    EXPECT_TRUE(session->getHttpHeaders().empty());
}

// ========== Header Filtering Tests ==========

TEST_F(SessionHeadersTest, GetHeadersByUrlPattern) {
    HttpHeaders header1;
    header1.url = "https://api.example.com/users";
    session->addHttpHeader(header1);

    HttpHeaders header2;
    header2.url = "https://api.example.com/posts";
    session->addHttpHeader(header2);

    HttpHeaders header3;
    header3.url = "https://cdn.example.com/assets";
    session->addHttpHeader(header3);

    auto filtered = session->getHeadersByUrlPattern("api.example.com");
    EXPECT_EQ(filtered.size(), 2);

    filtered = session->getHeadersByUrlPattern("cdn.example.com");
    EXPECT_EQ(filtered.size(), 1);

    filtered = session->getHeadersByUrlPattern("nonexistent.com");
    EXPECT_TRUE(filtered.empty());
}

TEST_F(SessionHeadersTest, GetLatestHeadersForDomain) {
    HttpHeaders header1;
    header1.url = "https://api.example.com/first";
    header1.timestamp = 1709000000;
    header1.method = "GET";
    session->addHttpHeader(header1);

    HttpHeaders header2;
    header2.url = "https://api.example.com/second";
    header2.timestamp = 1709000100;  // Later timestamp
    header2.method = "POST";
    session->addHttpHeader(header2);

    HttpHeaders header3;
    header3.url = "https://other.com/third";
    header3.timestamp = 1709000200;
    session->addHttpHeader(header3);

    auto latest = session->getLatestHeadersForDomain("api.example.com");
    EXPECT_EQ(latest.timestamp, 1709000100);
    EXPECT_EQ(latest.method, "POST");
}

// ========== Serialization Tests ==========

TEST_F(SessionHeadersTest, SerializeWithHeaders) {
    HttpHeaders header;
    header.url = "https://api.example.com/test";
    header.method = "POST";
    header.statusCode = 201;
    header.timestamp = 1709000000;
    header.requestHeaders["Content-Type"] = "application/json";
    header.requestHeaders["Authorization"] = "Bearer secret";
    header.responseHeaders["X-RateLimit-Remaining"] = "99";

    session->addHttpHeader(header);

    std::string serialized = session->serialize();
    EXPECT_FALSE(serialized.empty());
    EXPECT_THAT(serialized, testing::HasSubstr("httpHeaders"));
    EXPECT_THAT(serialized, testing::HasSubstr("api.example.com"));
    EXPECT_THAT(serialized, testing::HasSubstr("Authorization"));
}

TEST_F(SessionHeadersTest, DeserializeWithHeaders) {
    // Create and serialize a session with headers
    HttpHeaders header;
    header.url = "https://api.example.com/test";
    header.method = "GET";
    header.statusCode = 200;
    header.timestamp = 1709000000;
    header.requestHeaders["Accept"] = "application/json";
    header.responseHeaders["Content-Type"] = "application/json";

    session->addHttpHeader(header);
    std::string serialized = session->serialize();

    // Deserialize into new session
    Session restored = Session::deserialize(serialized);
    const auto& restoredHeaders = restored.getHttpHeaders();

    EXPECT_EQ(restoredHeaders.size(), 1);
    EXPECT_EQ(restoredHeaders[0].url, "https://api.example.com/test");
    EXPECT_EQ(restoredHeaders[0].method, "GET");
    EXPECT_EQ(restoredHeaders[0].statusCode, 200);
    EXPECT_EQ(restoredHeaders[0].requestHeaders.at("Accept"), "application/json");
    EXPECT_EQ(restoredHeaders[0].responseHeaders.at("Content-Type"), "application/json");
}

TEST_F(SessionHeadersTest, SerializeMultipleHeaders) {
    for (int i = 0; i < 5; i++) {
        HttpHeaders header;
        header.url = "https://api.example.com/endpoint" + std::to_string(i);
        header.method = "GET";
        header.statusCode = 200;
        header.timestamp = 1709000000 + i;
        header.requestHeaders["X-Index"] = std::to_string(i);
        session->addHttpHeader(header);
    }

    std::string serialized = session->serialize();
    Session restored = Session::deserialize(serialized);

    EXPECT_EQ(restored.getHttpHeaders().size(), 5);
}

TEST_F(SessionHeadersTest, RoundTripComplexHeaders) {
    HttpHeaders header;
    header.url = "https://api.example.com/complex";
    header.method = "PUT";
    header.statusCode = 200;
    header.timestamp = 1709000000;
    
    // Multiple request headers
    header.requestHeaders["Content-Type"] = "application/json";
    header.requestHeaders["Authorization"] = "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    header.requestHeaders["X-Custom-Header"] = "custom-value";
    header.requestHeaders["Accept-Language"] = "en-US,en;q=0.9";
    
    // Multiple response headers
    header.responseHeaders["Content-Type"] = "application/json";
    header.responseHeaders["Set-Cookie"] = "session=abc123; HttpOnly; Secure";
    header.responseHeaders["X-RateLimit-Limit"] = "100";
    header.responseHeaders["X-RateLimit-Remaining"] = "99";
    header.responseHeaders["Cache-Control"] = "no-cache, no-store";

    session->addHttpHeader(header);
    std::string serialized = session->serialize();
    Session restored = Session::deserialize(serialized);

    const auto& restoredHeaders = restored.getHttpHeaders();
    EXPECT_EQ(restoredHeaders.size(), 1);
    EXPECT_EQ(restoredHeaders[0].requestHeaders.size(), 4);
    EXPECT_EQ(restoredHeaders[0].responseHeaders.size(), 5);
    EXPECT_EQ(restoredHeaders[0].requestHeaders.at("Authorization"), "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9");
    EXPECT_EQ(restoredHeaders[0].responseHeaders.at("Set-Cookie"), "session=abc123; HttpOnly; Secure");
}

// ========== Version Compatibility Tests ==========

TEST_F(SessionHeadersTest, OldVersionWithoutHeaders) {
    // Create a session JSON with version 3 (no headers support)
    std::string oldVersionJson = R"({
        "version": 3,
        "name": "old_session",
        "currentUrl": "https://example.com",
        "cookies": [],
        "localStorage": {},
        "sessionStorage": {}
    })";

    Session restored = Session::deserialize(oldVersionJson);
    EXPECT_TRUE(restored.getHttpHeaders().empty());
    EXPECT_EQ(restored.getName(), "old_session");
}

TEST_F(SessionHeadersTest, NewVersionWithHeaders) {
    std::string newVersionJson = R"({
        "version": 4,
        "name": "new_session",
        "currentUrl": "https://example.com",
        "cookies": [],
        "localStorage": {},
        "sessionStorage": {},
        "httpHeaders": [
            {
                "url": "https://api.example.com/test",
                "method": "GET",
                "statusCode": 200,
                "timestamp": 1709000000,
                "requestHeaders": {
                    "Accept": "application/json"
                },
                "responseHeaders": {
                    "Content-Type": "application/json"
                }
            }
        ]
    })";

    Session restored = Session::deserialize(newVersionJson);
    EXPECT_EQ(restored.getHttpHeaders().size(), 1);
    EXPECT_EQ(restored.getHttpHeaders()[0].url, "https://api.example.com/test");
}

// ========== Integration Tests ==========

TEST_F(SessionHeadersTest, HeadersWithOtherSessionData) {
    // Set various session data
    session->setCurrentUrl("https://example.com");
    session->setUserAgent("TestAgent/1.0");
    
    Cookie cookie;
    cookie.name = "test";
    cookie.value = "value";
    cookie.domain = "example.com";
    cookie.path = "/";
    cookie.secure = true;
    cookie.httpOnly = true;
    cookie.expires = -1;
    session->addCookie(cookie);

    session->setLocalStorageItem("key1", "value1");
    
    HttpHeaders header;
    header.url = "https://api.example.com/test";
    header.method = "POST";
    header.statusCode = 200;
    header.requestHeaders["Content-Type"] = "application/json";
    header.responseHeaders["X-Custom"] = "test";
    session->addHttpHeader(header);

    // Serialize and deserialize
    std::string serialized = session->serialize();
    Session restored = Session::deserialize(serialized);

    // Verify all data preserved
    EXPECT_EQ(restored.getCurrentUrl(), "https://example.com");
    EXPECT_EQ(restored.getUserAgent(), "TestAgent/1.0");
    EXPECT_EQ(restored.getCookies().size(), 1);
    EXPECT_EQ(restored.getHttpHeaders().size(), 1);
    EXPECT_EQ(restored.getHttpHeaders()[0].url, "https://api.example.com/test");
}

TEST_F(SessionHeadersTest, SizeCalculationIncludesHeaders) {
    size_t initialSize = session->getApproximateSize();
    
    HttpHeaders header;
    header.url = "https://api.example.com/test";
    header.method = "POST";
    header.requestHeaders["Authorization"] = "Bearer very_long_token_value";
    header.responseHeaders["X-Custom-Header"] = "custom_value";
    session->addHttpHeader(header);

    size_t newSize = session->getApproximateSize();
    EXPECT_GT(newSize, initialSize);
}

// ========== Edge Cases ==========

TEST_F(SessionHeadersTest, EmptyHeaderMaps) {
    HttpHeaders header;
    header.url = "https://example.com";
    header.method = "GET";
    header.statusCode = 200;
    header.timestamp = 1709000000;
    // requestHeaders and responseHeaders are empty
    
    session->addHttpHeader(header);
    std::string serialized = session->serialize();
    Session restored = Session::deserialize(serialized);

    EXPECT_EQ(restored.getHttpHeaders().size(), 1);
    EXPECT_TRUE(restored.getHttpHeaders()[0].requestHeaders.empty());
    EXPECT_TRUE(restored.getHttpHeaders()[0].responseHeaders.empty());
}

TEST_F(SessionHeadersTest, SpecialCharactersInHeaders) {
    HttpHeaders header;
    header.url = "https://example.com/path?param=value&other=123";
    header.method = "GET";
    header.requestHeaders["Authorization"] = "Bearer token.with.dots_and-dashes";
    header.responseHeaders["Set-Cookie"] = "session=abc%20def; path=/; secure";
    
    session->addHttpHeader(header);
    std::string serialized = session->serialize();
    Session restored = Session::deserialize(serialized);

    EXPECT_EQ(restored.getHttpHeaders()[0].requestHeaders.at("Authorization"),
              "Bearer token.with.dots_and-dashes");
    EXPECT_EQ(restored.getHttpHeaders()[0].responseHeaders.at("Set-Cookie"),
              "session=abc%20def; path=/; secure");
}

TEST_F(SessionHeadersTest, ManyHeaders) {
    // Add 100 headers to test performance and memory
    for (int i = 0; i < 100; i++) {
        HttpHeaders header;
        header.url = "https://api.example.com/endpoint" + std::to_string(i);
        header.method = (i % 2 == 0) ? "GET" : "POST";
        header.statusCode = 200;
        header.timestamp = 1709000000 + i;
        header.requestHeaders["X-Request-ID"] = "req-" + std::to_string(i);
        session->addHttpHeader(header);
    }

    std::string serialized = session->serialize();
    Session restored = Session::deserialize(serialized);

    EXPECT_EQ(restored.getHttpHeaders().size(), 100);
}
