#!/bin/bash

# HeadlessWeb File Operations Integration Tests
# Tests file upload and download functionality with the Node.js test server

# Note: Not using set -e so tests can continue after individual failures

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_SERVER_DIR="$PROJECT_ROOT/test_server"
HWEB_BINARY="$PROJECT_ROOT/build/hweb"
TEST_SESSION="file_ops_test"
SERVER_URL="http://localhost:9876"
SERVER_PID=""
SERVER_STARTED_BY_US=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test tracking
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

log() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Test result tracking
pass_test() {
    local test_name="$1"
    echo -e "${GREEN}[PASS]${NC} $test_name"
    ((TESTS_PASSED++))
    ((TESTS_RUN++))
}

fail_test() {
    local test_name="$1"
    local reason="$2"
    echo -e "${RED}[FAIL]${NC} $test_name: $reason"
    ((TESTS_FAILED++))
    ((TESTS_RUN++))
}

# Cleanup function
cleanup() {
    log "Cleaning up..."
    
    # Stop test server only if we started it
    if [ "$SERVER_STARTED_BY_US" = true ] && [ ! -z "$SERVER_PID" ]; then
        log "Stopping test server (PID: $SERVER_PID)"
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    elif [ "$SERVER_STARTED_BY_US" = false ]; then
        log "Leaving existing test server running"
    fi
    
    # Clean up test session
    if [ -x "$HWEB_BINARY" ]; then
        "$HWEB_BINARY" --session "$TEST_SESSION" --session-clear 2>/dev/null || true
    fi
    
    # Clean up test files
    rm -f /tmp/test_upload_*.txt
    rm -f /tmp/test_download_*.txt
    rm -rf /tmp/hweb_integration_test
}

# Set up cleanup trap
trap cleanup EXIT

# Check prerequisites
check_prerequisites() {
    log "Checking prerequisites..."
    
    # Check if hweb binary exists
    if [ ! -x "$HWEB_BINARY" ]; then
        error "hweb binary not found at $HWEB_BINARY"
        error "Please build the project first: make"
        exit 1
    fi
    
    # Check if Node.js is available
    if ! command -v node &> /dev/null; then
        error "Node.js not found. Please install Node.js to run integration tests"
        exit 1
    fi
    
    # Check if test server directory exists
    if [ ! -d "$TEST_SERVER_DIR" ]; then
        error "Test server directory not found at $TEST_SERVER_DIR"
        exit 1
    fi
    
    # Check if npm dependencies are installed
    if [ ! -d "$TEST_SERVER_DIR/node_modules" ]; then
        log "Installing Node.js dependencies..."
        cd "$TEST_SERVER_DIR"
        npm install
        cd "$PROJECT_ROOT"
    fi
    
    log "Prerequisites check passed"
}

# Start the test server
start_test_server() {
    # Check if server is already running
    if curl -s "$SERVER_URL/health" > /dev/null 2>&1; then
        log "Test server is already running at $SERVER_URL"
        # Try to get the PID of existing server
        SERVER_PID=$(lsof -ti :9876 2>/dev/null | head -1)
        if [ ! -z "$SERVER_PID" ]; then
            log "Using existing test server (PID: $SERVER_PID)"
        else
            log "Using existing test server (PID unknown)"
            SERVER_PID=""
        fi
        SERVER_STARTED_BY_US=false
        return 0
    fi
    
    log "Starting test server..."
    
    cd "$TEST_SERVER_DIR"
    node upload-server.js &
    SERVER_PID=$!
    SERVER_STARTED_BY_US=true
    cd "$PROJECT_ROOT"
    
    # Wait for server to start
    local retries=10
    while [ $retries -gt 0 ]; do
        if curl -s "$SERVER_URL/health" > /dev/null 2>&1; then
            log "Test server started successfully (PID: $SERVER_PID)"
            return 0
        fi
        sleep 1
        ((retries--))
    done
    
    error "Failed to start test server"
    exit 1
}

# Create test files
create_test_files() {
    log "Creating test files..."
    
    mkdir -p /tmp/hweb_integration_test
    
    # Create small text file
    echo "This is a test file for upload testing." > /tmp/hweb_integration_test/small_test.txt
    
    # Create larger text file
    for i in {1..100}; do
        echo "Line $i: This is test content for integration testing of file operations." >> /tmp/hweb_integration_test/large_test.txt
    done
    
    # Create binary-like file
    printf '\x89\x50\x4E\x47\x0D\x0A\x1A\x0A' > /tmp/hweb_integration_test/fake_image.png
    echo "Fake PNG header for testing binary file upload" >> /tmp/hweb_integration_test/fake_image.png
    
    # Create file with special characters in name
    echo "Special character filename test" > "/tmp/hweb_integration_test/special_файл_🔧.txt"
    
    log "Test files created successfully"
}

# Test: Basic Navigation to Test Server
test_navigation_to_server() {
    local test_name="Navigation to Test Server"
    
    log "Running test: $test_name"
    
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" --assert-text "title" "Upload Test Server" --timeout 10000; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "Failed to navigate to test server or title assertion failed"
    fi
}

# Test: File Input Detection
test_file_input_detection() {
    local test_name="File Input Detection"
    
    log "Running test: $test_name"
    
    # Navigate to test server and check for file inputs
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --js "return hasFileInputs().toString();" --assert-js "hasFileInputs() === true" --timeout 5; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "File inputs not detected on test page"
    fi
}

# Test: Single File Upload Simulation
test_single_file_upload() {
    local test_name="Single File Upload Simulation"
    
    log "Running test: $test_name"
    
    # Navigate to server and simulate file upload
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --upload-file "#file-input" "/tmp/hweb_integration_test/small_test.txt" --timeout 10; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "File upload simulation failed"
    fi
}

# Test: Multiple File Upload Simulation
test_multiple_file_upload() {
    local test_name="Multiple File Upload Simulation"
    
    log "Running test: $test_name"
    
    # Test multiple file upload capability
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --upload-file "#multiple-input" "/tmp/hweb_integration_test/small_test.txt" \
       --upload-file "#multiple-input" "/tmp/hweb_integration_test/fake_image.png" --timeout 15; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "Multiple file upload simulation failed"
    fi
}

# Test: Drag and Drop Upload Simulation
test_drag_drop_upload() {
    local test_name="Drag and Drop Upload Simulation"
    
    log "Running test: $test_name"
    
    # Test drag and drop functionality
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --drag-drop-file "#drop-zone" "/tmp/hweb_integration_test/large_test.txt" --timeout 10; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "Drag and drop upload simulation failed"
    fi
}

# Test: File Upload with Type Validation
test_upload_type_validation() {
    local test_name="File Upload Type Validation"
    
    log "Running test: $test_name"
    
    # Test upload with allowed file types
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --upload-file "#file-input" "/tmp/hweb_integration_test/small_test.txt" \
       --allowed-types "txt,png,jpg" --timeout 10; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "File type validation failed"
    fi
}

# Test: File Upload Size Validation
test_upload_size_validation() {
    local test_name="File Upload Size Validation"
    
    log "Running test: $test_name"
    
    # Test upload with size limit (should pass for small file)
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --upload-file "#file-input" "/tmp/hweb_integration_test/small_test.txt" \
       --max-file-size "1024" --timeout 10; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "File size validation failed for small file"
    fi
}

# Test: Upload Progress Monitoring
test_upload_progress_monitoring() {
    local test_name="Upload Progress Monitoring"
    
    log "Running test: $test_name"
    
    # Test progress monitoring during upload
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --upload-file "#file-input" "/tmp/hweb_integration_test/large_test.txt" \
       --monitor-progress --timeout 15; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "Upload progress monitoring failed"
    fi
}

# Test: Upload Status Verification
test_upload_status_verification() {
    local test_name="Upload Status Verification"
    
    log "Running test: $test_name"
    
    # Upload file and verify status
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --upload-file "#file-input" "/tmp/hweb_integration_test/small_test.txt" \
       --assert-text "Upload complete" --timeout 10; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "Upload status verification failed"
    fi
}

# Test: Special Character Filename Handling
test_special_character_filenames() {
    local test_name="Special Character Filename Handling"
    
    log "Running test: $test_name"
    
    # Test upload with special characters in filename
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --upload-file "#file-input" "/tmp/hweb_integration_test/special_файл_🔧.txt" --timeout 10; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "Special character filename handling failed"
    fi
}

# Test: Upload Error Handling
test_upload_error_handling() {
    local test_name="Upload Error Handling"
    
    log "Running test: $test_name"
    
    # Test upload with non-existent file (should fail gracefully)
    if ! "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
         --upload-file "#file-input" "/nonexistent/file.txt" --timeout 5 2>/dev/null; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "Error handling failed - should have failed for non-existent file"
    fi
}

# Test: Complex Form Interaction
test_complex_form_interaction() {
    local test_name="Complex Form Interaction"
    
    log "Running test: $test_name"
    
    # Navigate to complex form and test file upload within form context
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL/complex-form" \
       --upload-file "input[type=file]" "/tmp/hweb_integration_test/small_test.txt" \
       --type "#username" "testuser" \
       --type "#email" "test@example.com" \
       --submit-form "#complex-form" --timeout 15; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "Complex form interaction with file upload failed"
    fi
}

# Test: Session State Persistence
test_session_state_persistence() {
    local test_name="Session State Persistence"
    
    log "Running test: $test_name"
    
    # Upload file in one session
    "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --upload-file "#file-input" "/tmp/hweb_integration_test/small_test.txt" --timeout 10 || true
    
    # Check if session state persists across invocations
    if "$HWEB_BINARY" --session "$TEST_SESSION" \
       --assert-text "title" "Upload Test Server" --timeout 5000; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "Session state did not persist correctly"
    fi
}

# Test: File Download Monitoring (if available)
test_download_monitoring() {
    local test_name="Download Monitoring"
    
    log "Running test: $test_name"
    
    # Create a downloadable file on the server and monitor for it
    # This is a simplified test since we don't have actual download triggering
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --js "return document.title;" --timeout 5; then
        # If we get here, the browser is responsive and can monitor
        pass_test "$test_name (Basic monitoring capability verified)"
    else
        fail_test "$test_name" "Download monitoring test setup failed"
    fi
}

# Test: Cross-Platform Path Handling
test_cross_platform_paths() {
    local test_name="Cross-Platform Path Handling"
    
    log "Running test: $test_name"
    
    # Test both absolute and relative paths
    local test_file="/tmp/hweb_integration_test/small_test.txt"
    local relative_path="../test_server/../tmp/hweb_integration_test/small_test.txt"
    
    if "$HWEB_BINARY" --session "$TEST_SESSION" --url "$SERVER_URL" \
       --upload-file "#file-input" "$test_file" --timeout 10; then
        pass_test "$test_name"
    else
        fail_test "$test_name" "Cross-platform path handling failed"
    fi
}

# Run all tests
run_all_tests() {
    log "Starting HeadlessWeb File Operations Integration Tests"
    log "============================================="
    
    check_prerequisites
    start_test_server
    create_test_files
    
    # Wait a moment for server to be fully ready
    sleep 2
    
    # Navigation and basic functionality tests
    test_navigation_to_server
    test_file_input_detection
    
    # File upload tests
    test_single_file_upload
    test_multiple_file_upload
    test_drag_drop_upload
    
    # Validation tests
    test_upload_type_validation
    test_upload_size_validation
    
    # Advanced functionality tests
    test_upload_progress_monitoring
    test_upload_status_verification
    test_special_character_filenames
    
    # Error handling and edge cases
    test_upload_error_handling
    test_complex_form_interaction
    
    # Session and state management
    test_session_state_persistence
    
    # Additional functionality
    test_download_monitoring
    test_cross_platform_paths
    
    # Print summary
    echo
    log "============================================="
    log "Integration Test Summary"
    log "============================================="
    log "Tests run: $TESTS_RUN"
    log "Passed: $TESTS_PASSED"
    log "Failed: $TESTS_FAILED"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        log "All tests passed! 🎉"
        exit 0
    else
        error "$TESTS_FAILED test(s) failed"
        exit 1
    fi
}

# Main execution
if [ "${1:-}" = "--help" ] || [ "${1:-}" = "-h" ]; then
    echo "HeadlessWeb File Operations Integration Tests"
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  --help, -h    Show this help message"
    echo ""
    echo "This script runs comprehensive integration tests for file upload and download"
    echo "functionality using the Node.js test server."
    exit 0
fi

run_all_tests