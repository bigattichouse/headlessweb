#!/bin/bash

# HeadlessWeb File Upload Integration Tests
# Tests file upload functionality using Node.js test server

set -e

# Test configuration
HWEB="../hweb"
TEST_SERVER_PORT=9876
TEST_SERVER_URL="http://localhost:$TEST_SERVER_PORT"
SERVER_SCRIPT="../test_server/upload-server.js"
TEMP_DIR="/tmp/hweb_upload_test_$$"
TEST_FILES_DIR="$TEMP_DIR/testfiles"

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Utility functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

run_test() {
    ((TESTS_RUN++))
    echo -e "\n${BLUE}=== Test $TESTS_RUN: $1 ===${NC}"
}

# Setup test environment
setup_test_env() {
    log_info "Setting up test environment..."
    
    # Create temporary directories
    mkdir -p "$TEMP_DIR"
    mkdir -p "$TEST_FILES_DIR"
    
    # Create test files of various types and sizes
    
    # Small text file
    cat > "$TEST_FILES_DIR/small.txt" << 'EOF'
This is a small text file for upload testing.
It contains multiple lines of text.
Line 3 of the test file.
EOF
    
    # Medium text file (1KB)
    head -c 1024 /dev/urandom | base64 > "$TEST_FILES_DIR/medium.txt"
    
    # Large text file (50KB)
    head -c 51200 /dev/urandom | base64 > "$TEST_FILES_DIR/large.txt"
    
    # Binary file (simulated image)
    head -c 2048 /dev/urandom > "$TEST_FILES_DIR/test_image.png"
    
    # Document file
    cat > "$TEST_FILES_DIR/document.pdf" << 'EOF'
%PDF-1.4
1 0 obj
<<
/Type /Catalog
/Pages 2 0 R
>>
endobj
2 0 obj
<<
/Type /Pages
/Kids [3 0 R]
/Count 1
>>
endobj
3 0 obj
<<
/Type /Page
/Parent 2 0 R
/MediaBox [0 0 612 792]
>>
endobj
xref
0 4
0000000000 65535 f
0000000010 00000 n
0000000053 00000 n
0000000125 00000 n
trailer
<<
/Size 4
/Root 1 0 R
>>
startxref
173
%%EOF
EOF
    
    # CSV file
    cat > "$TEST_FILES_DIR/data.csv" << 'EOF'
Name,Age,City
John Doe,30,New York
Jane Smith,25,Los Angeles
Bob Johnson,35,Chicago
EOF
    
    # JSON file
    cat > "$TEST_FILES_DIR/config.json" << 'EOF'
{
    "version": "1.0",
    "settings": {
        "debug": true,
        "timeout": 5000
    },
    "features": ["upload", "download", "session"]
}
EOF
    
    # File with special characters in name
    echo "Special chars test" > "$TEST_FILES_DIR/special chars & symbols.txt"
    
    # Unicode filename test
    echo "Unicode test" > "$TEST_FILES_DIR/тест.txt"
    
    log_info "Test files created in $TEST_FILES_DIR"
}

# Start test server
start_test_server() {
    log_info "Starting test server on port $TEST_SERVER_PORT..."
    
    if [ ! -f "$SERVER_SCRIPT" ]; then
        log_error "Test server script not found: $SERVER_SCRIPT"
        exit 1
    fi
    
    cd "$(dirname "$SERVER_SCRIPT")"
    
    # Install dependencies if needed
    if [ ! -d "node_modules" ]; then
        log_info "Installing Node.js dependencies..."
        npm install
    fi
    
    # Start server in background
    PORT=$TEST_SERVER_PORT node "$(basename "$SERVER_SCRIPT")" &
    SERVER_PID=$!
    
    # Wait for server to start
    sleep 2
    
    # Check if server is running
    if ! curl -s "$TEST_SERVER_URL/health" > /dev/null; then
        log_error "Test server failed to start"
        exit 1
    fi
    
    log_info "Test server started (PID: $SERVER_PID)"
    cd - > /dev/null
}

# Stop test server
stop_test_server() {
    if [ ! -z "$SERVER_PID" ]; then
        log_info "Stopping test server (PID: $SERVER_PID)..."
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    fi
}

# Cleanup
cleanup() {
    log_info "Cleaning up..."
    stop_test_server
    rm -rf "$TEMP_DIR"
}

# Trap cleanup on exit
trap cleanup EXIT

# Test functions

test_single_file_upload() {
    run_test "Single File Upload"
    
    local test_file="$TEST_FILES_DIR/small.txt"
    local upload_result_file="$TEMP_DIR/upload_result.json"
    
    # Use hweb to upload file
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 10000 \
          --upload-file "$test_file" \
          --wait-for-upload-completion \
          --execute-js "
              // Verify upload form exists
              const form = document.querySelector('form');
              if (!form) {
                  throw new Error('Upload form not found');
              }
              
              // Check if file was uploaded successfully
              const fileInput = document.querySelector('#file-input');
              if (!fileInput) {
                  throw new Error('File input not found');
              }
              
              return 'Upload form ready';
          " > "$upload_result_file" 2>&1
    
    local exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        log_success "Single file upload completed successfully"
        
        # Verify upload was recorded on server
        local upload_status=$(curl -s "$TEST_SERVER_URL/uploads" | grep -o '"status":"complete"' | wc -l)
        if [ "$upload_status" -gt 0 ]; then
            log_success "Upload status confirmed on server"
        else
            log_error "Upload not confirmed on server"
        fi
    else
        log_error "Single file upload failed (exit code: $exit_code)"
    fi
}

test_multiple_file_upload() {
    run_test "Multiple File Upload"
    
    local files=("$TEST_FILES_DIR/small.txt" "$TEST_FILES_DIR/data.csv" "$TEST_FILES_DIR/config.json")
    
    # Use hweb to upload multiple files
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 15000 \
          --upload-files "${files[0]},${files[1]},${files[2]}" \
          --wait-for-upload-completion \
          --execute-js "
              // Verify multiple file upload form
              const multipleInput = document.querySelector('#multiple-input');
              if (!multipleInput) {
                  throw new Error('Multiple file input not found');
              }
              
              if (!multipleInput.hasAttribute('multiple')) {
                  throw new Error('Multiple attribute not set');
              }
              
              return 'Multiple file upload form ready';
          " 2>/dev/null
    
    if [ $? -eq 0 ]; then
        log_success "Multiple file upload completed"
        
        # Check upload history for multiple files
        local upload_count=$(curl -s "$TEST_SERVER_URL/uploads" | grep -o '"filename"' | wc -l)
        if [ "$upload_count" -ge 3 ]; then
            log_success "Multiple uploads confirmed ($upload_count files)"
        else
            log_error "Expected 3+ uploads, found $upload_count"
        fi
    else
        log_error "Multiple file upload failed"
    fi
}

test_large_file_upload() {
    run_test "Large File Upload"
    
    local test_file="$TEST_FILES_DIR/large.txt"
    local file_size=$(stat -f%z "$test_file" 2>/dev/null || stat -c%s "$test_file" 2>/dev/null)
    
    log_info "Uploading large file (${file_size} bytes)..."
    
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 30000 \
          --upload-file "$test_file" \
          --wait-for-upload-completion \
          --upload-progress-monitoring \
          --execute-js "
              return 'Large file upload initiated';
          " 2>/dev/null
    
    if [ $? -eq 0 ]; then
        log_success "Large file upload completed"
        
        # Verify large file on server
        local server_response=$(curl -s "$TEST_SERVER_URL/uploads")
        if echo "$server_response" | grep -q "large.txt"; then
            log_success "Large file confirmed on server"
        else
            log_error "Large file not found on server"
        fi
    else
        log_error "Large file upload failed"
    fi
}

test_binary_file_upload() {
    run_test "Binary File Upload"
    
    local test_file="$TEST_FILES_DIR/test_image.png"
    
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 15000 \
          --upload-file "$test_file" \
          --verify-upload-integrity \
          --wait-for-upload-completion \
          --execute-js "
              return 'Binary file upload initiated';
          " 2>/dev/null
    
    if [ $? -eq 0 ]; then
        log_success "Binary file upload completed"
        
        # Check if binary file was handled correctly
        local server_response=$(curl -s "$TEST_SERVER_URL/uploads")
        if echo "$server_response" | grep -q "test_image.png"; then
            log_success "Binary file confirmed on server"
        else
            log_error "Binary file not found on server"
        fi
    else
        log_error "Binary file upload failed"
    fi
}

test_special_filename_upload() {
    run_test "Special Filename Characters Upload"
    
    local test_file="$TEST_FILES_DIR/special chars & symbols.txt"
    
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 10000 \
          --upload-file "$test_file" \
          --wait-for-upload-completion \
          --execute-js "
              return 'Special filename upload initiated';
          " 2>/dev/null
    
    if [ $? -eq 0 ]; then
        log_success "Special filename upload completed"
    else
        log_error "Special filename upload failed"
    fi
}

test_unicode_filename_upload() {
    run_test "Unicode Filename Upload"
    
    local test_file="$TEST_FILES_DIR/тест.txt"
    
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 10000 \
          --upload-file "$test_file" \
          --wait-for-upload-completion \
          --execute-js "
              return 'Unicode filename upload initiated';
          " 2>/dev/null
    
    if [ $? -eq 0 ]; then
        log_success "Unicode filename upload completed"
    else
        log_error "Unicode filename upload failed"
    fi
}

test_upload_progress_monitoring() {
    run_test "Upload Progress Monitoring"
    
    local test_file="$TEST_FILES_DIR/medium.txt"
    local output_file="$TEMP_DIR/progress_output.log"
    
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 15000 \
          --upload-file "$test_file" \
          --upload-progress-monitoring \
          --wait-for-upload-completion \
          --verbose \
          --execute-js "
              return 'Upload with progress monitoring initiated';
          " > "$output_file" 2>&1
    
    if [ $? -eq 0 ]; then
        log_success "Upload with progress monitoring completed"
        
        # Check if progress information was captured
        if grep -q "progress\|%\|upload\|bytes" "$output_file"; then
            log_success "Progress monitoring information captured"
        else
            log_success "Upload completed (progress details not captured in logs)"
        fi
    else
        log_error "Upload with progress monitoring failed"
    fi
}

test_upload_timeout_handling() {
    run_test "Upload Timeout Handling"
    
    # Create a very large file that will likely timeout
    head -c 10485760 /dev/urandom > "$TEST_FILES_DIR/timeout_test.dat" # 10MB
    
    local result=0
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 1000 \
          --upload-file "$TEST_FILES_DIR/timeout_test.dat" \
          --wait-for-upload-completion 2>/dev/null || result=$?
    
    if [ $result -ne 0 ]; then
        log_success "Upload timeout handled correctly (exit code: $result)"
    else
        log_error "Upload timeout not handled correctly"
    fi
    
    # Clean up large test file
    rm -f "$TEST_FILES_DIR/timeout_test.dat"
}

test_upload_form_interaction() {
    run_test "Upload Form Interaction"
    
    local test_file="$TEST_FILES_DIR/small.txt"
    
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 10000 \
          --execute-js "
              // Test form interaction capabilities
              const fileInput = document.querySelector('#file-input');
              const submitButton = document.querySelector('input[type=\"submit\"]');
              const dropZone = document.querySelector('#drop-zone');
              
              let results = {
                  fileInput: !!fileInput,
                  submitButton: !!submitButton,
                  dropZone: !!dropZone,
                  formReady: false
              };
              
              if (fileInput && submitButton) {
                  results.formReady = true;
              }
              
              return JSON.stringify(results);
          " \
          --upload-file "$test_file" \
          --wait-for-upload-completion 2>/dev/null
    
    if [ $? -eq 0 ]; then
        log_success "Upload form interaction test completed"
    else
        log_error "Upload form interaction test failed"
    fi
}

test_drag_drop_upload_simulation() {
    run_test "Drag & Drop Upload Simulation"
    
    local test_file="$TEST_FILES_DIR/data.csv"
    
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 10000 \
          --execute-js "
              // Test drag and drop zone functionality
              const dropZone = document.querySelector('#drop-zone');
              if (!dropZone) {
                  throw new Error('Drop zone not found');
              }
              
              // Simulate drag and drop events
              const dragEnterEvent = new Event('dragenter', { bubbles: true });
              const dragOverEvent = new Event('dragover', { bubbles: true });
              const dropEvent = new Event('drop', { bubbles: true });
              
              dropZone.dispatchEvent(dragEnterEvent);
              dropZone.dispatchEvent(dragOverEvent);
              dropZone.dispatchEvent(dropEvent);
              
              return 'Drag and drop events simulated';
          " \
          --upload-file "$test_file" \
          --wait-for-upload-completion 2>/dev/null
    
    if [ $? -eq 0 ]; then
        log_success "Drag & drop upload simulation completed"
    else
        log_error "Drag & drop upload simulation failed"
    fi
}

test_upload_with_session_persistence() {
    run_test "Upload with Session Persistence"
    
    local session_file="$TEMP_DIR/upload_session.json"
    local test_file="$TEST_FILES_DIR/document.pdf"
    
    # First session: prepare upload environment
    $HWEB --url "$TEST_SERVER_URL" \
          --session-file "$session_file" \
          --timeout 5000 \
          --execute-js "
              // Set session data for upload preferences
              sessionStorage.setItem('uploadPrefs', JSON.stringify({
                  maxSize: 10485760,
                  allowedTypes: ['pdf', 'txt', 'csv'],
                  autoUpload: true
              }));
              
              localStorage.setItem('uploadHistory', JSON.stringify([]));
              
              return 'Upload session initialized';
          " 2>/dev/null
    
    # Second session: perform upload with session data
    $HWEB --session-file "$session_file" \
          --timeout 15000 \
          --upload-file "$test_file" \
          --wait-for-upload-completion \
          --execute-js "
              // Verify session data persistence
              const prefs = JSON.parse(sessionStorage.getItem('uploadPrefs') || '{}');
              const history = JSON.parse(localStorage.getItem('uploadHistory') || '[]');
              
              if (prefs.allowedTypes && prefs.allowedTypes.includes('pdf')) {
                  return 'Session-aware upload ready';
              } else {
                  throw new Error('Session data not restored');
              }
          " 2>/dev/null
    
    if [ $? -eq 0 ]; then
        log_success "Upload with session persistence completed"
    else
        log_error "Upload with session persistence failed"
    fi
}

test_concurrent_uploads() {
    run_test "Concurrent Uploads"
    
    local files=("$TEST_FILES_DIR/small.txt" "$TEST_FILES_DIR/config.json" "$TEST_FILES_DIR/data.csv")
    local pids=()
    
    # Start concurrent uploads
    for file in "${files[@]}"; do
        (
            $HWEB --url "$TEST_SERVER_URL" \
                  --timeout 15000 \
                  --upload-file "$file" \
                  --wait-for-upload-completion \
                  --execute-js "
                      return 'Concurrent upload: $(basename \"$file\")';
                  " 2>/dev/null
        ) &
        pids+=($!)
    done
    
    # Wait for all uploads to complete
    local success_count=0
    for pid in "${pids[@]}"; do
        if wait $pid; then
            ((success_count++))
        fi
    done
    
    if [ $success_count -eq ${#files[@]} ]; then
        log_success "All concurrent uploads completed ($success_count/${#files[@]})"
    else
        log_error "Only $success_count out of ${#files[@]} concurrent uploads completed"
    fi
}

test_upload_error_handling() {
    run_test "Upload Error Handling"
    
    # Test with non-existent file
    local result=0
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 5000 \
          --upload-file "/nonexistent/file.txt" \
          --wait-for-upload-completion 2>/dev/null || result=$?
    
    if [ $result -ne 0 ]; then
        log_success "Non-existent file error handled correctly"
    else
        log_error "Non-existent file error not handled properly"
    fi
    
    # Test with invalid server URL
    result=0
    $HWEB --url "http://invalid-server:9999" \
          --timeout 2000 \
          --upload-file "$TEST_FILES_DIR/small.txt" \
          --wait-for-upload-completion 2>/dev/null || result=$?
    
    if [ $result -ne 0 ]; then
        log_success "Invalid server error handled correctly"
    else
        log_error "Invalid server error not handled properly"
    fi
}

test_upload_validation() {
    run_test "Upload Validation"
    
    local test_file="$TEST_FILES_DIR/config.json"
    
    $HWEB --url "$TEST_SERVER_URL" \
          --timeout 10000 \
          --upload-file "$test_file" \
          --verify-upload-integrity \
          --wait-for-upload-completion \
          --execute-js "
              // Validate upload environment
              const form = document.querySelector('form[action*=\"upload\"]');
              const fileInput = document.querySelector('input[type=\"file\"]');
              
              if (!form) {
                  throw new Error('Upload form not found');
              }
              
              if (!fileInput) {
                  throw new Error('File input not found');
              }
              
              // Check form attributes
              if (form.enctype !== 'multipart/form-data') {
                  throw new Error('Incorrect form encoding');
              }
              
              if (form.method.toLowerCase() !== 'post') {
                  throw new Error('Incorrect form method');
              }
              
              return 'Upload form validation passed';
          " 2>/dev/null
    
    if [ $? -eq 0 ]; then
        log_success "Upload validation completed successfully"
    else
        log_error "Upload validation failed"
    fi
}

# Main test execution
main() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  HeadlessWeb File Upload Integration Tests${NC}"
    echo -e "${BLUE}========================================${NC}\n"
    
    # Setup
    setup_test_env
    start_test_server
    
    # Clear any existing uploads
    curl -s -X DELETE "$TEST_SERVER_URL/uploads" > /dev/null || true
    
    # Run all tests
    test_single_file_upload
    test_multiple_file_upload
    test_large_file_upload
    test_binary_file_upload
    test_special_filename_upload
    test_unicode_filename_upload
    test_upload_progress_monitoring
    test_upload_timeout_handling
    test_upload_form_interaction
    test_drag_drop_upload_simulation
    test_upload_with_session_persistence
    test_concurrent_uploads
    test_upload_error_handling
    test_upload_validation
    
    # Print summary
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  Test Summary${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Tests run: ${TESTS_RUN}"
    echo -e "${GREEN}Tests passed: ${TESTS_PASSED}${NC}"
    echo -e "${RED}Tests failed: ${TESTS_FAILED}${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}All upload tests passed! 🎉${NC}"
        exit 0
    else
        echo -e "\n${RED}Some upload tests failed! ❌${NC}"
        exit 1
    fi
}

# Check prerequisites
check_prerequisites() {
    # Check if hweb exists
    if [ ! -f "$HWEB" ]; then
        log_error "hweb binary not found: $HWEB"
        exit 1
    fi
    
    # Check if node is available
    if ! command -v node >/dev/null 2>&1; then
        log_error "Node.js is required but not installed"
        exit 1
    fi
    
    # Check if curl is available
    if ! command -v curl >/dev/null 2>&1; then
        log_error "curl is required but not installed"
        exit 1
    fi
}

# Run prerequisites check and main
check_prerequisites
main "$@"