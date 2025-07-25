#include <iostream>
#include <filesystem>
#include <fstream>
#include "../src/FileOps/UploadManager.h"
#include "../src/FileOps/DownloadManager.h"

int main() {
    std::cout << "=== FileOps Debug Test ===" << std::endl;
    
    try {
        // Create temporary directory for testing
        std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "hweb_debug_fileops";
        std::filesystem::create_directories(temp_dir);
        std::cout << "✓ Temp directory created: " << temp_dir << std::endl;
        
        // Test 1: UploadManager creation and basic operations
        std::cout << "\n=== Testing UploadManager ===" << std::endl;
        
        FileOps::UploadManager upload_manager;
        std::cout << "✓ UploadManager created" << std::endl;
        
        // Test file size limits
        std::cout << "Testing setMaxFileSize..." << std::endl;
        upload_manager.setMaxFileSize(1000); // 1KB limit
        std::cout << "✓ Max file size set to 1000 bytes" << std::endl;
        
        // Create test files
        std::filesystem::path small_file = temp_dir / "small.txt";
        std::filesystem::path large_file = temp_dir / "large.txt";
        
        // Create small file (should pass)
        {
            std::ofstream ofs(small_file);
            ofs << "Small test content";
        }
        std::cout << "✓ Small file created: " << std::filesystem::file_size(small_file) << " bytes" << std::endl;
        
        // Create large file (should fail)
        {
            std::ofstream ofs(large_file);
            std::string large_content(2000, 'A'); // 2KB content
            ofs << large_content;
        }
        std::cout << "✓ Large file created: " << std::filesystem::file_size(large_file) << " bytes" << std::endl;
        
        // Test file validation
        std::cout << "\nTesting file validation..." << std::endl;
        
        FileOps::UploadFile small_upload;
        small_upload.local_path = small_file.string();
        small_upload.filename = "small.txt";
        small_upload.content_type = "text/plain";
        
        bool small_valid = upload_manager.validateFile(small_upload);
        std::cout << "Small file validation: " << (small_valid ? "PASS" : "FAIL") << std::endl;
        
        FileOps::UploadFile large_upload;
        large_upload.local_path = large_file.string();
        large_upload.filename = "large.txt";
        large_upload.content_type = "text/plain";
        
        bool large_valid = upload_manager.validateFile(large_upload);
        std::cout << "Large file validation: " << (large_valid ? "PASS" : "FAIL") << std::endl;
        std::cout << "Expected: large file should FAIL validation due to size limit" << std::endl;
        
        // Test 2: MIME type detection
        std::cout << "\n=== Testing MIME Type Detection ===" << std::endl;
        
        // Create HTML file
        std::filesystem::path html_file = temp_dir / "test.html";
        {
            std::ofstream ofs(html_file);
            ofs << "<!DOCTYPE html><html><head><title>Test</title></head><body>Test</body></html>";
        }
        
        std::string detected_mime = upload_manager.detectMimeType(html_file.string());
        std::cout << "HTML file MIME type: '" << detected_mime << "'" << std::endl;
        
        // Test 3: Base64 encoding
        std::cout << "\n=== Testing Base64 Encoding ===" << std::endl;
        std::string test_content = "Hello, World!";
        std::string encoded = upload_manager.encodeBase64(test_content);
        std::cout << "Original: '" << test_content << "'" << std::endl;
        std::cout << "Encoded:  '" << encoded << "'" << std::endl;
        
        // Test 4: DownloadManager
        std::cout << "\n=== Testing DownloadManager ===" << std::endl;
        
        FileOps::DownloadManager download_manager;
        std::cout << "✓ DownloadManager created" << std::endl;
        
        // Test download path setup
        std::filesystem::path download_dir = temp_dir / "downloads";
        download_manager.setDownloadDirectory(download_dir.string());
        std::cout << "✓ Download directory set to: " << download_dir << std::endl;
        
        // Test completion hook
        bool hook_called = false;
        download_manager.setCompletionHook([&hook_called](const std::string& path, bool success) {
            hook_called = true;
            std::cout << "Completion hook called: path=" << path << ", success=" << success << std::endl;
        });
        std::cout << "✓ Completion hook set" << std::endl;
        
        // Cleanup
        std::filesystem::remove_all(temp_dir);
        std::cout << "✓ Cleanup completed" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== FileOps debug test completed ===" << std::endl;
    return 0;
}