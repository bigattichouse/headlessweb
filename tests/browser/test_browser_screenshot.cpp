#include <gtest/gtest.h>
#include "../../src/Browser/Browser.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <cmath>

extern bool g_debug;

class BrowserScreenshotTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Enable debug output for tests
        g_debug = true;
        
        // Create temporary test directory
        test_dir_ = std::filesystem::temp_directory_path() / "hweb_screenshot_test";
        std::filesystem::create_directories(test_dir_);
        
        // Initialize browser
        HWeb::HWebConfig test_config;
        browser_ = std::make_unique<Browser>(test_config);
        
        // Small delay to ensure proper initialization
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    void TearDown() override {
        // Cleanup
        browser_.reset();
        
        // Clean up temporary directory
        std::filesystem::remove_all(test_dir_);
    }
    
    // Helper to check if PNG file is valid
    bool isValidPNGFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // Check PNG signature (first 8 bytes)
        char signature[8];
        file.read(signature, 8);
        
        const char png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        return std::memcmp(signature, png_signature, 8) == 0;
    }
    
    // Helper to get PNG dimensions (simplified)
    std::pair<int, int> getPNGDimensions(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return {0, 0};
        }
        
        // Skip PNG signature and IHDR length
        file.seekg(16, std::ios::beg);
        
        // Read width and height (big-endian)
        uint32_t width, height;
        file.read(reinterpret_cast<char*>(&width), 4);
        file.read(reinterpret_cast<char*>(&height), 4);
        
        // Convert from big-endian
        width = __builtin_bswap32(width);
        height = __builtin_bswap32(height);
        
        return {static_cast<int>(width), static_cast<int>(height)};
    }
    
    // Helper to get file size
    size_t getFileSize(const std::string& filepath) {
        std::error_code ec;
        auto size = std::filesystem::file_size(filepath, ec);
        return ec ? 0 : size;
    }
    
    // Helper to load simple test HTML
    void loadSimpleTestPage() {
        std::string simple_html = R"(
            <!DOCTYPE html>
            <html>
            <head>
                <title>Screenshot Test Page</title>
                <style>
                    body { font-family: Arial, sans-serif; margin: 20px; }
                    .header { background-color: #4CAF50; color: white; padding: 10px; }
                    .content { margin: 20px 0; }
                    .footer { background-color: #f1f1f1; padding: 10px; }
                </style>
            </head>
            <body>
                <div class="header">
                    <h1>Test Page for Screenshots</h1>
                </div>
                <div class="content">
                    <p>This is a test page used for screenshot functionality testing.</p>
                    <p>It contains various elements to verify screenshot capture.</p>
                </div>
                <div class="footer">
                    <p>Footer content</p>
                </div>
            </body>
            </html>
        )";
        
        browser_->loadHTML(simple_html);
        // Wait for page to load
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    // Helper to load tall test page
    void loadTallTestPage() {
        std::string tall_html = R"(
            <!DOCTYPE html>
            <html>
            <head>
                <title>Tall Screenshot Test Page</title>
                <style>
                    body { font-family: Arial, sans-serif; margin: 20px; }
                    .section { height: 300px; margin: 20px 0; padding: 20px; border: 1px solid #ccc; }
                    .section1 { background-color: #ffebee; }
                    .section2 { background-color: #e8f5e8; }
                    .section3 { background-color: #e3f2fd; }
                    .section4 { background-color: #fff3e0; }
                    .section5 { background-color: #f3e5f5; }
                </style>
            </head>
            <body>
                <div class="section section1">
                    <h2>Section 1</h2>
                    <p>This page is designed to be tall to test full page screenshots.</p>
                </div>
                <div class="section section2">
                    <h2>Section 2</h2>
                    <p>Each section has different background colors for verification.</p>
                </div>
                <div class="section section3">
                    <h2>Section 3</h2>
                    <p>Full page screenshots should capture all sections.</p>
                </div>
                <div class="section section4">
                    <h2>Section 4</h2>
                    <p>Visible area screenshots should only capture what's in viewport.</p>
                </div>
                <div class="section section5">
                    <h2>Section 5</h2>
                    <p>This is the bottom section of the tall page.</p>
                </div>
            </body>
            </html>
        )";
        
        browser_->loadHTML(tall_html);
        // Wait for page to load
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::filesystem::path test_dir_;
    std::unique_ptr<Browser> browser_;
};

// ========== Basic Screenshot Tests ==========

TEST_F(BrowserScreenshotTest, TakeScreenshot_SimpleVisibleArea) {
    loadSimpleTestPage();
    
    std::string screenshot_path = (test_dir_ / "visible_area.png").string();
    
    // Take visible area screenshot
    browser_->takeScreenshot(screenshot_path);
    
    // Verify screenshot file was created
    EXPECT_TRUE(std::filesystem::exists(screenshot_path));
    
    // Verify it's a valid PNG file
    EXPECT_TRUE(isValidPNGFile(screenshot_path));
    
    // Verify file has reasonable size (> 1KB)
    size_t file_size = getFileSize(screenshot_path);
    EXPECT_GT(file_size, 1024);
}

TEST_F(BrowserScreenshotTest, TakeFullPageScreenshot_SimpleContent) {
    loadSimpleTestPage();
    
    std::string screenshot_path = (test_dir_ / "full_page_simple.png").string();
    
    // Take full page screenshot
    browser_->takeFullPageScreenshot(screenshot_path);
    
    // Verify screenshot file was created
    EXPECT_TRUE(std::filesystem::exists(screenshot_path));
    
    // Verify it's a valid PNG file
    EXPECT_TRUE(isValidPNGFile(screenshot_path));
    
    // Verify file has reasonable size
    size_t file_size = getFileSize(screenshot_path);
    EXPECT_GT(file_size, 1024);
}

TEST_F(BrowserScreenshotTest, TakeFullPageScreenshot_TallContent) {
    loadTallTestPage();
    
    std::string screenshot_path = (test_dir_ / "full_page_tall.png").string();
    
    // Take full page screenshot
    browser_->takeFullPageScreenshot(screenshot_path);
    
    // Verify screenshot file was created
    EXPECT_TRUE(std::filesystem::exists(screenshot_path));
    
    // Verify it's a valid PNG file
    EXPECT_TRUE(isValidPNGFile(screenshot_path));
    
    // Full page screenshot of tall content should be larger
    size_t file_size = getFileSize(screenshot_path);
    EXPECT_GT(file_size, 5000); // Should be significantly larger
}

// ========== Screenshot Dimensions Tests ==========

TEST_F(BrowserScreenshotTest, Screenshot_VerifyDimensions) {
    // Set specific viewport size
    browser_->setViewportSize(800, 600);
    loadSimpleTestPage();
    
    std::string screenshot_path = (test_dir_ / "dimensions_test.png").string();
    browser_->takeScreenshot(screenshot_path);
    
    ASSERT_TRUE(std::filesystem::exists(screenshot_path));
    
    auto [width, height] = getPNGDimensions(screenshot_path);
    
    // Visible area screenshot should match viewport dimensions
    EXPECT_EQ(width, 800);
    EXPECT_EQ(height, 600);
}

TEST_F(BrowserScreenshotTest, FullPageScreenshot_DimensionsLargerThanViewport) {
    // Set small viewport
    browser_->setViewportSize(400, 300);
    loadTallTestPage();
    
    std::string visible_path = (test_dir_ / "visible_small.png").string();
    std::string full_path = (test_dir_ / "full_small_viewport.png").string();
    
    browser_->takeScreenshot(visible_path);
    browser_->takeFullPageScreenshot(full_path);
    
    ASSERT_TRUE(std::filesystem::exists(visible_path));
    ASSERT_TRUE(std::filesystem::exists(full_path));
    
    auto [visible_width, visible_height] = getPNGDimensions(visible_path);
    auto [full_width, full_height] = getPNGDimensions(full_path);
    
    // Visible area should match viewport
    EXPECT_EQ(visible_width, 400);
    EXPECT_EQ(visible_height, 300);
    
    // Full page should be larger (especially height for tall page)
    EXPECT_GE(full_width, visible_width);
    EXPECT_GT(full_height, visible_height); // Tall content should be much taller
}

// ========== Screenshot Content Verification Tests ==========

TEST_F(BrowserScreenshotTest, Screenshot_DifferentContentProducesDifferentFiles) {
    // Take screenshot of first page
    loadSimpleTestPage();
    std::string screenshot1_path = (test_dir_ / "content1.png").string();
    browser_->takeScreenshot(screenshot1_path);
    
    // Load different content and take another screenshot
    loadTallTestPage();
    std::string screenshot2_path = (test_dir_ / "content2.png").string();
    browser_->takeScreenshot(screenshot2_path);
    
    ASSERT_TRUE(std::filesystem::exists(screenshot1_path));
    ASSERT_TRUE(std::filesystem::exists(screenshot2_path));
    
    // Files should have different sizes (different content)
    size_t size1 = getFileSize(screenshot1_path);
    size_t size2 = getFileSize(screenshot2_path);
    
    EXPECT_NE(size1, size2);
}

TEST_F(BrowserScreenshotTest, Screenshot_SameContentProducesSimilarFiles) {
    loadSimpleTestPage();
    
    // Take two screenshots of the same content
    std::string screenshot1_path = (test_dir_ / "same1.png").string();
    std::string screenshot2_path = (test_dir_ / "same2.png").string();
    
    browser_->takeScreenshot(screenshot1_path);
    browser_->takeScreenshot(screenshot2_path);
    
    ASSERT_TRUE(std::filesystem::exists(screenshot1_path));
    ASSERT_TRUE(std::filesystem::exists(screenshot2_path));
    
    size_t size1 = getFileSize(screenshot1_path);
    size_t size2 = getFileSize(screenshot2_path);
    
    // Sizes should be very similar (allowing for minor compression differences)
    double size_diff = std::abs(static_cast<double>(size1) - static_cast<double>(size2));
    double size_tolerance = std::max(size1, size2) * 0.05; // 5% tolerance
    
    EXPECT_LT(size_diff, size_tolerance);
}

// ========== Screenshot Path and File Handling Tests ==========

TEST_F(BrowserScreenshotTest, Screenshot_AbsolutePath) {
    loadSimpleTestPage();
    
    std::string screenshot_path = (test_dir_ / "absolute_path.png").string();
    browser_->takeScreenshot(screenshot_path);
    
    EXPECT_TRUE(std::filesystem::exists(screenshot_path));
    EXPECT_TRUE(isValidPNGFile(screenshot_path));
}

TEST_F(BrowserScreenshotTest, Screenshot_FileOverwrite) {
    loadSimpleTestPage();
    
    std::string screenshot_path = (test_dir_ / "overwrite_test.png").string();
    
    // Take first screenshot
    browser_->takeScreenshot(screenshot_path);
    ASSERT_TRUE(std::filesystem::exists(screenshot_path));
    size_t size1 = getFileSize(screenshot_path);
    
    // Load different content
    loadTallTestPage();
    
    // Take second screenshot (should overwrite)
    browser_->takeScreenshot(screenshot_path);
    EXPECT_TRUE(std::filesystem::exists(screenshot_path));
    size_t size2 = getFileSize(screenshot_path);
    
    // Should be different (overwritten)
    EXPECT_NE(size1, size2);
}

TEST_F(BrowserScreenshotTest, Screenshot_NonexistentDirectory) {
    loadSimpleTestPage();
    
    std::string screenshot_path = (test_dir_ / "nonexistent" / "dir" / "test.png").string();
    
    // This should handle directory creation gracefully or fail gracefully
    browser_->takeScreenshot(screenshot_path);
    
    // The behavior depends on implementation - either creates dirs and succeeds,
    // or fails gracefully without crashing
    // We just verify it doesn't crash
    SUCCEED();
}

// ========== Dynamic Content Screenshot Tests ==========

TEST_F(BrowserScreenshotTest, Screenshot_DynamicContent) {
    std::string dynamic_html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Dynamic Content Test</title>
            <style>
                #content { padding: 20px; font-size: 18px; }
                .highlight { background-color: yellow; }
            </style>
        </head>
        <body>
            <div id="content">Initial content</div>
            <script>
                setTimeout(function() {
                    document.getElementById('content').innerHTML = 
                        '<span class="highlight">Updated dynamic content</span>';
                }, 200);
            </script>
        </body>
        </html>
    )";
    
    browser_->loadHTML(dynamic_html);
    
    // Take screenshot immediately
    std::string early_screenshot = (test_dir_ / "early_dynamic.png").string();
    browser_->takeScreenshot(early_screenshot);
    
    // Wait for dynamic content to load
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Take screenshot after dynamic content loads
    std::string late_screenshot = (test_dir_ / "late_dynamic.png").string();
    browser_->takeScreenshot(late_screenshot);
    
    ASSERT_TRUE(std::filesystem::exists(early_screenshot));
    ASSERT_TRUE(std::filesystem::exists(late_screenshot));
    
    // Screenshots should be different
    size_t early_size = getFileSize(early_screenshot);
    size_t late_size = getFileSize(late_screenshot);
    
    EXPECT_NE(early_size, late_size);
}

// ========== Error Handling Tests ==========

TEST_F(BrowserScreenshotTest, Screenshot_EmptyPage) {
    // Load empty page
    browser_->loadHTML("<!DOCTYPE html><html><body></body></html>");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::string screenshot_path = (test_dir_ / "empty_page.png").string();
    browser_->takeScreenshot(screenshot_path);
    
    // Should still create a valid PNG (even if mostly empty)
    EXPECT_TRUE(std::filesystem::exists(screenshot_path));
    EXPECT_TRUE(isValidPNGFile(screenshot_path));
    
    // Should have minimal but non-zero size
    size_t file_size = getFileSize(screenshot_path);
    EXPECT_GT(file_size, 100); // PNG header + minimal content
}

TEST_F(BrowserScreenshotTest, Screenshot_InvalidHTML) {
    // Load invalid HTML
    browser_->loadHTML("This is not valid HTML at all!");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::string screenshot_path = (test_dir_ / "invalid_html.png").string();
    
    // Should handle gracefully
    browser_->takeScreenshot(screenshot_path);
    
    // May or may not create file depending on browser handling,
    // but shouldn't crash
    SUCCEED();
}

// ========== Performance and Resource Tests ==========

TEST_F(BrowserScreenshotTest, Screenshot_MultipleSequential) {
    loadSimpleTestPage();
    
    std::vector<std::string> screenshot_paths;
    
    // Take multiple screenshots
    for (int i = 0; i < 5; ++i) {
        std::string path = (test_dir_ / ("sequential_" + std::to_string(i) + ".png")).string();
        screenshot_paths.push_back(path);
        browser_->takeScreenshot(path);
    }
    
    // Verify all screenshots were created
    for (const auto& path : screenshot_paths) {
        EXPECT_TRUE(std::filesystem::exists(path));
        EXPECT_TRUE(isValidPNGFile(path));
        
        size_t file_size = getFileSize(path);
        EXPECT_GT(file_size, 1024);
    }
}

TEST_F(BrowserScreenshotTest, Screenshot_LargeViewport) {
    // Set large viewport
    browser_->setViewportSize(1920, 1080);
    loadSimpleTestPage();
    
    std::string screenshot_path = (test_dir_ / "large_viewport.png").string();
    browser_->takeScreenshot(screenshot_path);
    
    ASSERT_TRUE(std::filesystem::exists(screenshot_path));
    
    auto [width, height] = getPNGDimensions(screenshot_path);
    EXPECT_EQ(width, 1920);
    EXPECT_EQ(height, 1080);
    
    // Large screenshot should have substantial file size
    size_t file_size = getFileSize(screenshot_path);
    EXPECT_GT(file_size, 10000); // Should be > 10KB for large image
}

// ========== Screenshot Timing Tests ==========

TEST_F(BrowserScreenshotTest, Screenshot_ImmediateAfterLoad) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head><title>Immediate Test</title></head>
        <body>
            <h1>Content Ready Immediately</h1>
            <p>This should be visible in screenshot right after load.</p>
        </body>
        </html>
    )";
    
    browser_->loadHTML(html);
    // Take screenshot immediately after load (no wait)
    
    std::string screenshot_path = (test_dir_ / "immediate.png").string();
    browser_->takeScreenshot(screenshot_path);
    
    EXPECT_TRUE(std::filesystem::exists(screenshot_path));
    EXPECT_TRUE(isValidPNGFile(screenshot_path));
}

// ========== Screenshot Quality Tests ==========

TEST_F(BrowserScreenshotTest, Screenshot_ColoredContent) {
    std::string colored_html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Color Test</title>
            <style>
                .red { background-color: #ff0000; color: white; padding: 20px; }
                .green { background-color: #00ff00; color: black; padding: 20px; }
                .blue { background-color: #0000ff; color: white; padding: 20px; }
            </style>
        </head>
        <body>
            <div class="red">Red Section</div>
            <div class="green">Green Section</div>
            <div class="blue">Blue Section</div>
        </body>
        </html>
    )";
    
    browser_->loadHTML(colored_html);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::string screenshot_path = (test_dir_ / "colored.png").string();
    browser_->takeScreenshot(screenshot_path);
    
    EXPECT_TRUE(std::filesystem::exists(screenshot_path));
    EXPECT_TRUE(isValidPNGFile(screenshot_path));
    
    // Colored content should result in larger file size than plain text
    size_t file_size = getFileSize(screenshot_path);
    EXPECT_GT(file_size, 2000);
}

} // namespace