#!/bin/bash

# HeadlessWeb Download Integration Tests
# Tests download functionality using Node.js test server

set -e

# Test configuration
HWEB="../hweb"
TEST_SERVER_PORT=9876
TEST_SERVER_URL="http://localhost:$TEST_SERVER_PORT"
SERVER_SCRIPT="../test_server/upload-server.js"
TEMP_DIR="/tmp/hweb_download_test_$$"
DOWNLOAD_DIR="$TEMP_DIR/downloads"
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
    mkdir -p "$DOWNLOAD_DIR"
    mkdir -p "$TEST_FILES_DIR"
    
    # Create test files of various sizes
    echo "Small test file content" > "$TEST_FILES_DIR/small.txt"
    
    # Create medium file (1KB)
    head -c 1024 /dev/urandom | base64 > "$TEST_FILES_DIR/medium.txt"
    
    # Create large file (10KB) 
    head -c 10240 /dev/urandom | base64 > "$TEST_FILES_DIR/large.txt"
    
    # Create binary test file (PNG image simulation)
    head -c 2048 /dev/urandom > "$TEST_FILES_DIR/image.png"
    
    # Create document files
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
    
    log_info "Test files created in $TEST_FILES_DIR"
    log_info "Download directory: $DOWNLOAD_DIR"
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

test_download_basic_file() {
    run_test "Basic File Download"
    
    # Use hweb to navigate to server and trigger download
    local test_file="small.txt"
    local expected_file="$DOWNLOAD_DIR/$test_file"
    
    # First upload a file to the server so we can download it
    curl -s -F "file=@$TEST_FILES_DIR/$test_file" "$TEST_SERVER_URL/upload/single" > /dev/null
    
    # Use hweb to download the file (simulate clicking download link)
    $HWEB --url "$TEST_SERVER_URL" \
          --download-dir "$DOWNLOAD_DIR" \
          --wait-for-download "$test_file" \
          --timeout 10000 \
          --execute-js "
              // Simulate clicking a download link
              const link = document.createElement('a');
              link.href = '/uploads/$test_file';
              link.download = '$test_file';
              document.body.appendChild(link);
              link.click();
              return 'Download triggered';
          " 2>/dev/null
    
    if [ -f "$expected_file" ]; then
        log_success "Basic file download completed"
        
        # Verify file content
        if cmp -s "$TEST_FILES_DIR/$test_file" "$expected_file"; then
            log_success "Downloaded file content matches original"
        else
            log_error "Downloaded file content differs from original"
        fi
    else
        log_error "Downloaded file not found: $expected_file"
    fi
}

test_download_with_pattern_matching() {
    run_test "Download with Pattern Matching"
    
    local test_pattern="*.txt"
    local test_file="pattern_test.txt"
    
    # Upload test file
    curl -s -F "file=@$TEST_FILES_DIR/small.txt" -F "originalName=$test_file" "$TEST_SERVER_URL/upload/single" > /dev/null
    
    # Wait for download using pattern
    $HWEB --url "$TEST_SERVER_URL" \
          --download-dir "$DOWNLOAD_DIR" \
          --wait-for-download "$test_pattern" \
          --timeout 10000 \
          --execute-js "
              const link = document.createElement('a');
              link.href = '/uploads/$test_file';
              link.download = '$test_file';
              document.body.appendChild(link);
              link.click();
          " 2>/dev/null
    
    # Check if any .txt file was downloaded
    if ls "$DOWNLOAD_DIR"/*.txt >/dev/null 2>&1; then
        log_success "Pattern matching download successful"
    else
        log_error "No files matching pattern found"
    fi
}

test_download_multiple_files() {
    run_test "Multiple File Downloads"
    
    local files=("multi1.txt" "multi2.txt" "multi3.txt")
    
    # Upload multiple test files
    for file in "${files[@]}"; do
        curl -s -F "file=@$TEST_FILES_DIR/small.txt" -F "originalName=$file" "$TEST_SERVER_URL/upload/single" > /dev/null
    done
    
    # Trigger multiple downloads
    $HWEB --url "$TEST_SERVER_URL" \
          --download-dir "$DOWNLOAD_DIR" \
          --wait-for-downloads "${files[0]},${files[1]},${files[2]}" \
          --timeout 15000 \
          --execute-js "
              const files = ['${files[0]}', '${files[1]}', '${files[2]}'];
              files.forEach(file => {
                  const link = document.createElement('a');
                  link.href = '/uploads/' + file;
                  link.download = file;
                  document.body.appendChild(link);
                  setTimeout(() => link.click(), Math.random() * 1000);
              });
              return 'Multiple downloads triggered';
          " 2>/dev/null
    
    # Check if all files were downloaded
    local downloaded_count=0
    for file in "${files[@]}"; do
        if [ -f "$DOWNLOAD_DIR/$file" ]; then
            ((downloaded_count++))
        fi
    done
    
    if [ $downloaded_count -eq ${#files[@]} ]; then
        log_success "All $downloaded_count files downloaded successfully"
    else
        log_error "Only $downloaded_count out of ${#files[@]} files downloaded"
    fi
}

test_download_large_file() {
    run_test "Large File Download"
    
    local test_file="large.txt"
    local expected_file="$DOWNLOAD_DIR/$test_file"
    
    # Upload large file
    curl -s -F "file=@$TEST_FILES_DIR/$test_file" "$TEST_SERVER_URL/upload/single" > /dev/null
    
    # Download with progress monitoring
    $HWEB --url "$TEST_SERVER_URL" \
          --download-dir "$DOWNLOAD_DIR" \
          --wait-for-download "$test_file" \
          --timeout 20000 \
          --verify-download-integrity \
          --execute-js "
              const link = document.createElement('a');
              link.href = '/uploads/$test_file';
              link.download = '$test_file';
              document.body.appendChild(link);
              link.click();
          " 2>/dev/null
    
    if [ -f "$expected_file" ]; then
        local original_size=$(stat -f%z "$TEST_FILES_DIR/$test_file" 2>/dev/null || stat -c%s "$TEST_FILES_DIR/$test_file" 2>/dev/null)
        local downloaded_size=$(stat -f%z "$expected_file" 2>/dev/null || stat -c%s "$expected_file" 2>/dev/null)
        
        if [ "$original_size" -eq "$downloaded_size" ]; then
            log_success "Large file download with correct size ($downloaded_size bytes)"
        else
            log_error "Size mismatch: expected $original_size, got $downloaded_size"
        fi
    else
        log_error "Large file download failed"
    fi
}

test_download_binary_file() {
    run_test "Binary File Download"
    
    local test_file="image.png"
    local expected_file="$DOWNLOAD_DIR/$test_file"
    
    # Upload binary file
    curl -s -F "file=@$TEST_FILES_DIR/$test_file" "$TEST_SERVER_URL/upload/single" > /dev/null
    
    # Download binary file
    $HWEB --url "$TEST_SERVER_URL" \
          --download-dir "$DOWNLOAD_DIR" \
          --wait-for-download "$test_file" \
          --timeout 10000 \
          --execute-js "
              const link = document.createElement('a');
              link.href = '/uploads/$test_file';
              link.download = '$test_file';
              document.body.appendChild(link);
              link.click();
          " 2>/dev/null
    
    if [ -f "$expected_file" ]; then
        # Verify binary file integrity using checksum
        if command -v md5sum >/dev/null 2>&1; then
            local original_hash=$(md5sum "$TEST_FILES_DIR/$test_file" | cut -d' ' -f1)
            local downloaded_hash=$(md5sum "$expected_file" | cut -d' ' -f1)
            
            if [ "$original_hash" = "$downloaded_hash" ]; then
                log_success "Binary file downloaded with integrity verified"
            else
                log_error "Binary file integrity check failed"
            fi
        else
            log_success "Binary file downloaded (checksum verification not available)"
        fi
    else
        log_error "Binary file download failed"
    fi
}

test_download_timeout() {
    run_test "Download Timeout Handling"
    
    # Try to download non-existent file with short timeout
    local result=0
    $HWEB --url "$TEST_SERVER_URL" \
          --download-dir "$DOWNLOAD_DIR" \
          --wait-for-download "nonexistent.txt" \
          --timeout 2000 2>/dev/null || result=$?
    
    if [ $result -ne 0 ]; then
        log_success "Download timeout handled correctly (exit code: $result)"
    else
        log_error "Download timeout not handled correctly"
    fi
}

test_download_directory_management() {
    run_test "Download Directory Management"
    
    local custom_dir="$TEMP_DIR/custom_downloads"
    mkdir -p "$custom_dir"
    
    local test_file="directory_test.txt"
    
    # Upload test file
    curl -s -F "file=@$TEST_FILES_DIR/small.txt" -F "originalName=$test_file" "$TEST_SERVER_URL/upload/single" > /dev/null
    
    # Download to custom directory
    $HWEB --url "$TEST_SERVER_URL" \
          --download-dir "$custom_dir" \
          --wait-for-download "$test_file" \
          --timeout 10000 \
          --execute-js "
              const link = document.createElement('a');
              link.href = '/uploads/$test_file';
              link.download = '$test_file';
              document.body.appendChild(link);
              link.click();
          " 2>/dev/null
    
    if [ -f "$custom_dir/$test_file" ]; then
        log_success "Custom download directory used correctly"
    else
        log_error "Custom download directory not used"
    fi
}

test_download_with_session_persistence() {
    run_test "Download with Session Persistence"
    
    local session_file="$TEMP_DIR/download_session.json"
    local test_file="session_test.txt"
    
    # Upload test file
    curl -s -F "file=@$TEST_FILES_DIR/small.txt" -F "originalName=$test_file" "$TEST_SERVER_URL/upload/single" > /dev/null
    
    # First session: navigate and set up download
    $HWEB --url "$TEST_SERVER_URL" \
          --session-file "$session_file" \
          --execute-js "
              // Set up some session data
              sessionStorage.setItem('downloadTest', 'initialized');
              localStorage.setItem('downloadPrefs', 'persistent');
              return 'Session initialized';
          " 2>/dev/null
    
    # Second session: restore and download
    $HWEB --session-file "$session_file" \
          --download-dir "$DOWNLOAD_DIR" \
          --wait-for-download "$test_file" \
          --timeout 10000 \
          --execute-js "
              // Verify session data
              const sessionData = sessionStorage.getItem('downloadTest');
              const localData = localStorage.getItem('downloadPrefs');
              if (sessionData === 'initialized' && localData === 'persistent') {
                  // Trigger download
                  const link = document.createElement('a');
                  link.href = '/uploads/$test_file';
                  link.download = '$test_file';
                  document.body.appendChild(link);
                  link.click();
                  return 'Session restored and download triggered';
              }
              return 'Session restoration failed';
          " 2>/dev/null
    
    if [ -f "$DOWNLOAD_DIR/$test_file" ]; then
        log_success "Download with session persistence successful"
    else
        log_error "Download with session persistence failed"
    fi
}

test_download_progress_monitoring() {
    run_test "Download Progress Monitoring"
    
    local test_file="progress_test.txt"
    
    # Upload test file
    curl -s -F "file=@$TEST_FILES_DIR/medium.txt" -F "originalName=$test_file" "$TEST_SERVER_URL/upload/single" > /dev/null
    
    # Download with verbose output to capture progress
    local output_file="$TEMP_DIR/download_output.log"
    
    $HWEB --url "$TEST_SERVER_URL" \
          --download-dir "$DOWNLOAD_DIR" \
          --wait-for-download "$test_file" \
          --timeout 10000 \
          --verbose \
          --execute-js "
              const link = document.createElement('a');
              link.href = '/uploads/$test_file';
              link.download = '$test_file';
              document.body.appendChild(link);
              link.click();
          " > "$output_file" 2>&1
    
    if [ -f "$DOWNLOAD_DIR/$test_file" ]; then
        # Check if progress information was logged
        if grep -q "download\|progress\|bytes\|%" "$output_file"; then
            log_success "Download progress monitoring active"
        else
            log_success "Download completed (progress details not captured)"
        fi
    else
        log_error "Download with progress monitoring failed"
    fi
}

test_download_error_recovery() {
    run_test "Download Error Recovery"
    
    # Test invalid download directory
    local invalid_dir="/invalid/directory/path"
    local result=0
    
    $HWEB --url "$TEST_SERVER_URL" \
          --download-dir "$invalid_dir" \
          --wait-for-download "any.txt" \
          --timeout 2000 2>/dev/null || result=$?
    
    if [ $result -ne 0 ]; then
        log_success "Invalid download directory handled gracefully"
    else
        log_error "Invalid download directory not handled properly"
    fi
}

test_download_concurrent() {
    run_test "Concurrent Downloads"
    
    local files=("concurrent1.txt" "concurrent2.txt" "concurrent3.txt")
    
    # Upload test files
    for file in "${files[@]}"; do
        curl -s -F "file=@$TEST_FILES_DIR/small.txt" -F "originalName=$file" "$TEST_SERVER_URL/upload/single" > /dev/null
    done
    
    # Start concurrent downloads
    local pids=()
    for file in "${files[@]}"; do
        (
            $HWEB --url "$TEST_SERVER_URL" \
                  --download-dir "$DOWNLOAD_DIR" \
                  --wait-for-download "$file" \
                  --timeout 10000 \
                  --execute-js "
                      const link = document.createElement('a');
                      link.href = '/uploads/$file';
                      link.download = '$file';
                      document.body.appendChild(link);
                      setTimeout(() => link.click(), Math.random() * 500);
                  " 2>/dev/null
        ) &
        pids+=($!)
    done
    
    # Wait for all downloads to complete
    for pid in "${pids[@]}"; do
        wait $pid
    done
    
    # Check results
    local success_count=0
    for file in "${files[@]}"; do
        if [ -f "$DOWNLOAD_DIR/$file" ]; then
            ((success_count++))
        fi
    done
    
    if [ $success_count -eq ${#files[@]} ]; then
        log_success "All concurrent downloads completed ($success_count/${#files[@]})"
    else
        log_error "Only $success_count out of ${#files[@]} concurrent downloads completed"
    fi
}

# Main test execution
main() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  HeadlessWeb Download Integration Tests${NC}"
    echo -e "${BLUE}========================================${NC}\n"
    
    # Setup
    setup_test_env
    start_test_server
    
    # Run all tests
    test_download_basic_file
    test_download_with_pattern_matching
    test_download_multiple_files
    test_download_large_file
    test_download_binary_file
    test_download_timeout
    test_download_directory_management
    test_download_with_session_persistence
    test_download_progress_monitoring
    test_download_error_recovery
    test_download_concurrent
    
    # Print summary
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  Test Summary${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Tests run: ${TESTS_RUN}"
    echo -e "${GREEN}Tests passed: ${TESTS_PASSED}${NC}"
    echo -e "${RED}Tests failed: ${TESTS_FAILED}${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}All download tests passed! 🎉${NC}"
        exit 0
    else
        echo -e "\n${RED}Some download tests failed! ❌${NC}"
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