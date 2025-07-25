#include <iostream>
#include "src/Browser/Browser.h"
#include "src/FileOps/UploadManager.h"
#include "tests/utils/test_helpers.h"
#include <thread>
#include <chrono>

using namespace FileOps;

int main() {
    HWeb::HWebConfig config;
    Browser browser(config);
    
    // Create temp directory
    auto temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("debug_upload");
    
    // Create test HTML
    std::string test_html = R"(
<!DOCTYPE html>
<html>
<head><title>Upload Debug Test</title></head>
<body>
    <h1>Upload Test</h1>
    <input type='file' id='file-input'/>
    <script>
        function elementExists(sel) { 
            return document.querySelector(sel) !== null; 
        }
        console.log('Script loaded, element exists:', elementExists('#file-input'));
    </script>
</body>
</html>
)";
    
    // Create HTML file and load
    auto html_file = temp_dir->createFile("test.html", test_html);
    std::string file_url = "file://" + html_file.string();
    
    std::cout << "Loading: " << file_url << std::endl;
    browser.loadUri(file_url);
    
    // Wait for page load
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // Test basic JavaScript execution
    std::cout << "Testing basic JS..." << std::endl;
    std::string basic_test = browser.executeJavascriptSync("'hello world'");
    std::cout << "Basic JS result: '" << basic_test << "'" << std::endl;
    
    // Test document ready state
    std::cout << "Testing document ready..." << std::endl;  
    std::string ready_state = browser.executeJavascriptSync("document.readyState");
    std::cout << "Document ready state: '" << ready_state << "'" << std::endl;
    
    // Test element exists via direct querySelector
    std::cout << "Testing direct querySelector..." << std::endl;
    std::string direct_query = browser.executeJavascriptSync("document.querySelector('#file-input') !== null");
    std::cout << "Direct query result: '" << direct_query << "'" << std::endl;
    
    // Test element exists via function
    std::cout << "Testing elementExists function..." << std::endl;
    std::string func_query = browser.executeJavascriptSync("elementExists('#file-input').toString()");
    std::cout << "Function query result: '" << func_query << "'" << std::endl;
    
    // Test file input type
    std::cout << "Testing file input type..." << std::endl;
    std::string type_query = browser.executeJavascriptSync("document.querySelector('#file-input')?.type === 'file'");
    std::cout << "Type query result: '" << type_query << "'" << std::endl;
    
    // Test with UploadManager
    std::cout << "Testing UploadManager validation..." << std::endl;
    UploadManager manager;
    bool validation_result = manager.validateUploadTarget(browser, "#file-input");
    std::cout << "UploadManager validation: " << (validation_result ? "SUCCESS" : "FAILED") << std::endl;
    
    return 0;
}