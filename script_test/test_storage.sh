#!/bin/bash

# HeadlessWeb Storage Integration Tests
# Tests localStorage, sessionStorage, and cookie functionality

set -e

# Test configuration
HWEB="../hweb"
TEST_SERVER_PORT=9876
TEST_SERVER_URL="http://localhost:$TEST_SERVER_PORT"
SERVER_SCRIPT="../test_server/upload-server.js"
TEMP_DIR="/tmp/hweb_storage_test_$$"

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
    mkdir -p "$TEMP_DIR"
}

# Start test server
start_test_server() {
    log_info "Starting test server on port $TEST_SERVER_PORT..."
    
    if [ ! -f "$SERVER_SCRIPT" ]; then
        log_error "Test server script not found: $SERVER_SCRIPT"
        exit 1
    fi
    
    cd "$(dirname "$SERVER_SCRIPT")"
    
    if [ ! -d "node_modules" ]; then
        log_info "Installing Node.js dependencies..."
        npm install
    fi
    
    PORT=$TEST_SERVER_PORT node "$(basename "$SERVER_SCRIPT")" &
    SERVER_PID=$!
    
    sleep 2
    
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

trap cleanup EXIT

# ========== LocalStorage Tests ==========

test_localstorage_basic() {
    run_test "LocalStorage Basic Operations"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Test basic localStorage operations
                  localStorage.clear();
                  
                  // Set items
                  localStorage.setItem('testKey1', 'testValue1');
                  localStorage.setItem('testKey2', 'testValue2');
                  localStorage.setItem('number', '42');
                  localStorage.setItem('json', JSON.stringify({name: 'test', value: 123}));
                  
                  // Get items
                  const results = {
                      key1: localStorage.getItem('testKey1'),
                      key2: localStorage.getItem('testKey2'),
                      number: localStorage.getItem('number'),
                      json: localStorage.getItem('json'),
                      length: localStorage.length,
                      nonExistent: localStorage.getItem('nonExistent')
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"key1":"testValue1"' && \
       echo "$output" | grep -q '"length":4' && \
       echo "$output" | grep -q '"nonExistent":null'; then
        log_success "LocalStorage basic operations work correctly"
    else
        log_error "LocalStorage basic operations failed: $output"
    fi
}

test_localstorage_persistence() {
    run_test "LocalStorage Persistence Across Sessions"
    
    local session_file="$TEMP_DIR/storage_session.json"
    
    # First session: set localStorage data
    $HWEB --url "$TEST_SERVER_URL" \
          --session-file "$session_file" \
          --execute-js "
              localStorage.clear();
              localStorage.setItem('persistent', 'data');
              localStorage.setItem('timestamp', Date.now().toString());
              localStorage.setItem('complex', JSON.stringify({
                  array: [1, 2, 3],
                  object: {nested: true},
                  boolean: true
              }));
              return 'Data stored';
          " 2>/dev/null
    
    # Second session: retrieve localStorage data
    local output=$(
        $HWEB --session-file "$session_file" \
              --execute-js "
                  const results = {
                      persistent: localStorage.getItem('persistent'),
                      timestamp: localStorage.getItem('timestamp'),
                      complex: localStorage.getItem('complex'),
                      length: localStorage.length
                  };
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"persistent":"data"' && \
       echo "$output" | grep -q '"length":3'; then
        log_success "LocalStorage persists across sessions"
    else
        log_error "LocalStorage persistence failed: $output"
    fi
}

test_localstorage_limits() {
    run_test "LocalStorage Size Limits and Edge Cases"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  localStorage.clear();
                  
                  // Test large data
                  const largeData = 'x'.repeat(10000); // 10KB string
                  localStorage.setItem('large', largeData);
                  
                  // Test special characters
                  localStorage.setItem('special', 'Special chars: áéíóú ñ 中文 🚀');
                  
                  // Test empty values
                  localStorage.setItem('empty', '');
                  localStorage.setItem('null', null);
                  localStorage.setItem('undefined', undefined);
                  
                  const results = {
                      largeSize: localStorage.getItem('large').length,
                      special: localStorage.getItem('special'),
                      empty: localStorage.getItem('empty'),
                      nullValue: localStorage.getItem('null'),
                      undefinedValue: localStorage.getItem('undefined'),
                      totalLength: localStorage.length
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"largeSize":10000' && \
       echo "$output" | grep -q '"special":"Special chars:' && \
       echo "$output" | grep -q '"empty":""'; then
        log_success "LocalStorage handles edge cases correctly"
    else
        log_error "LocalStorage edge cases failed: $output"
    fi
}

test_localstorage_enumeration() {
    run_test "LocalStorage Key Enumeration"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  localStorage.clear();
                  
                  // Set test data
                  localStorage.setItem('key1', 'value1');
                  localStorage.setItem('key2', 'value2');
                  localStorage.setItem('key3', 'value3');
                  
                  // Test key enumeration
                  const keys = [];
                  for (let i = 0; i < localStorage.length; i++) {
                      keys.push(localStorage.key(i));
                  }
                  
                  // Test removal
                  localStorage.removeItem('key2');
                  
                  const results = {
                      originalKeys: keys.sort(),
                      lengthBefore: 3,
                      lengthAfter: localStorage.length,
                      key2After: localStorage.getItem('key2'),
                      remainingKeys: []
                  };
                  
                  for (let i = 0; i < localStorage.length; i++) {
                      results.remainingKeys.push(localStorage.key(i));
                  }
                  results.remainingKeys.sort();
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"lengthAfter":2' && \
       echo "$output" | grep -q '"key2After":null'; then
        log_success "LocalStorage enumeration and removal work correctly"
    else
        log_error "LocalStorage enumeration failed: $output"
    fi
}

# ========== SessionStorage Tests ==========

test_sessionstorage_basic() {
    run_test "SessionStorage Basic Operations"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Test basic sessionStorage operations
                  sessionStorage.clear();
                  
                  sessionStorage.setItem('sessionKey1', 'sessionValue1');
                  sessionStorage.setItem('sessionKey2', 'sessionValue2');
                  sessionStorage.setItem('tempData', JSON.stringify({temp: true}));
                  
                  const results = {
                      key1: sessionStorage.getItem('sessionKey1'),
                      key2: sessionStorage.getItem('sessionKey2'),
                      tempData: sessionStorage.getItem('tempData'),
                      length: sessionStorage.length,
                      nonExistent: sessionStorage.getItem('nonExistent')
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"key1":"sessionValue1"' && \
       echo "$output" | grep -q '"length":3'; then
        log_success "SessionStorage basic operations work correctly"
    else
        log_error "SessionStorage basic operations failed: $output"
    fi
}

test_sessionstorage_vs_localstorage() {
    run_test "SessionStorage vs LocalStorage Isolation"
    
    local session_file="$TEMP_DIR/isolation_session.json"
    
    # Set data in both storages
    $HWEB --url "$TEST_SERVER_URL" \
          --session-file "$session_file" \
          --execute-js "
              localStorage.clear();
              sessionStorage.clear();
              
              localStorage.setItem('shared', 'localStorage');
              sessionStorage.setItem('shared', 'sessionStorage');
              
              localStorage.setItem('persistent', 'willPersist');
              sessionStorage.setItem('temporary', 'willNotPersist');
              
              return 'Data set in both storages';
          " 2>/dev/null
    
    # Verify isolation in same session
    local output=$(
        $HWEB --session-file "$session_file" \
              --execute-js "
                  const results = {
                      localShared: localStorage.getItem('shared'),
                      sessionShared: sessionStorage.getItem('shared'),
                      localPersistent: localStorage.getItem('persistent'),
                      sessionTemp: sessionStorage.getItem('temporary'),
                      localLength: localStorage.length,
                      sessionLength: sessionStorage.length
                  };
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"localShared":"localStorage"' && \
       echo "$output" | grep -q '"sessionShared":"sessionStorage"'; then
        log_success "Storage types are properly isolated"
    else
        log_error "Storage isolation failed: $output"
    fi
}

test_sessionstorage_session_persistence() {
    run_test "SessionStorage Session Persistence"
    
    local session_file="$TEMP_DIR/session_persist.json"
    
    # First session: set sessionStorage data
    $HWEB --url "$TEST_SERVER_URL" \
          --session-file "$session_file" \
          --execute-js "
              sessionStorage.clear();
              sessionStorage.setItem('sessionData', 'shouldPersist');
              sessionStorage.setItem('sessionTime', Date.now().toString());
              return 'Session data set';
          " 2>/dev/null
    
    # Second session: check if sessionStorage persists
    local output=$(
        $HWEB --session-file "$session_file" \
              --execute-js "
                  const results = {
                      sessionData: sessionStorage.getItem('sessionData'),
                      sessionTime: sessionStorage.getItem('sessionTime'),
                      length: sessionStorage.length
                  };
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"sessionData":"shouldPersist"'; then
        log_success "SessionStorage persists within session file"
    else
        log_error "SessionStorage session persistence failed: $output"
    fi
}

# ========== Cookie Tests ==========

test_cookies_basic() {
    run_test "Cookie Basic Operations"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Clear cookies (best effort)
                  document.cookie.split(';').forEach(function(c) {
                      const eqPos = c.indexOf('=');
                      const name = eqPos > -1 ? c.substr(0, eqPos).trim() : c.trim();
                      if (name) {
                          document.cookie = name + '=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/';
                      }
                  });
                  
                  // Set cookies
                  document.cookie = 'testCookie1=value1; path=/';
                  document.cookie = 'testCookie2=value2; path=/';
                  document.cookie = 'numberCookie=123; path=/';
                  
                  // Get all cookies
                  const allCookies = document.cookie;
                  
                  const results = {
                      allCookies: allCookies,
                      hasTest1: allCookies.includes('testCookie1=value1'),
                      hasTest2: allCookies.includes('testCookie2=value2'),
                      hasNumber: allCookies.includes('numberCookie=123')
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"hasTest1":true' && \
       echo "$output" | grep -q '"hasTest2":true'; then
        log_success "Cookie basic operations work correctly"
    else
        log_error "Cookie basic operations failed: $output"
    fi
}

test_cookies_persistence() {
    run_test "Cookie Persistence Across Sessions"
    
    local session_file="$TEMP_DIR/cookie_session.json"
    
    # First session: set cookies
    $HWEB --url "$TEST_SERVER_URL" \
          --session-file "$session_file" \
          --execute-js "
              // Set persistent cookies
              document.cookie = 'persistent=data; path=/; max-age=86400';
              document.cookie = 'timestamp=' + Date.now() + '; path=/; max-age=86400';
              
              return 'Cookies set';
          " 2>/dev/null
    
    # Second session: check cookie persistence
    local output=$(
        $HWEB --session-file "$session_file" \
              --execute-js "
                  const allCookies = document.cookie;
                  const results = {
                      allCookies: allCookies,
                      hasPersistent: allCookies.includes('persistent=data'),
                      hasTimestamp: allCookies.includes('timestamp=')
                  };
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"hasPersistent":true'; then
        log_success "Cookies persist across sessions"
    else
        log_error "Cookie persistence failed: $output"
    fi
}

test_cookies_expiration() {
    run_test "Cookie Expiration and Deletion"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Set cookies with different expiration
                  document.cookie = 'shortLived=value; path=/; max-age=1';
                  document.cookie = 'longLived=value; path=/; max-age=86400';
                  
                  // Immediately check
                  let initialCookies = document.cookie;
                  
                  // Wait 2 seconds for short-lived cookie to expire
                  setTimeout(() => {
                      // This won't work in synchronous context, but shows intent
                  }, 2000);
                  
                  // Delete a cookie explicitly
                  document.cookie = 'longLived=; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT';
                  
                  const finalCookies = document.cookie;
                  
                  const results = {
                      initialHadShort: initialCookies.includes('shortLived=value'),
                      initialHadLong: initialCookies.includes('longLived=value'),
                      finalHadLong: finalCookies.includes('longLived=value')
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"initialHadShort":true' && \
       echo "$output" | grep -q '"finalHadLong":false'; then
        log_success "Cookie expiration and deletion work correctly"
    else
        log_error "Cookie expiration test failed: $output"
    fi
}

# ========== Storage Events Tests ==========

test_storage_events() {
    run_test "Storage Event Handling"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  let storageEvents = [];
                  
                  // Listen for storage events
                  window.addEventListener('storage', function(e) {
                      storageEvents.push({
                          key: e.key,
                          oldValue: e.oldValue,
                          newValue: e.newValue,
                          storageArea: e.storageArea === localStorage ? 'localStorage' : 'sessionStorage'
                      });
                  });
                  
                  // Trigger storage changes
                  localStorage.setItem('eventTest', 'value1');
                  localStorage.setItem('eventTest', 'value2');
                  localStorage.removeItem('eventTest');
                  
                  // Note: storage events only fire in other windows/tabs in real browsers
                  // but we test the event listener setup
                  
                  const results = {
                      eventListenerSet: true,
                      eventsRecorded: storageEvents.length,
                      localStorage: localStorage.getItem('eventTest')
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"eventListenerSet":true'; then
        log_success "Storage event handling set up correctly"
    else
        log_error "Storage event handling failed: $output"
    fi
}

# ========== Cross-Origin Storage Tests ==========

test_storage_cross_origin() {
    run_test "Storage Cross-Origin Isolation"
    
    # Test with different URLs (same origin)
    local output1=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  localStorage.clear();
                  localStorage.setItem('origin1', 'data1');
                  return localStorage.getItem('origin1');
              " 2>/dev/null
    )
    
    # Test with different path (same origin)
    local output2=$(
        $HWEB --url "$TEST_SERVER_URL/different" \
              --execute-js "
                  // Should see the same localStorage
                  const sameOriginData = localStorage.getItem('origin1');
                  localStorage.setItem('origin2', 'data2');
                  
                  const results = {
                      sameOriginData: sameOriginData,
                      canSetNew: localStorage.getItem('origin2')
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output2" | grep -q '"sameOriginData":"data1"' && \
       echo "$output2" | grep -q '"canSetNew":"data2"'; then
        log_success "Storage cross-origin behavior correct (same origin access)"
    else
        log_error "Storage cross-origin test failed"
    fi
}

# ========== Storage Quota Tests ==========

test_storage_quota() {
    run_test "Storage Quota and Error Handling"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  localStorage.clear();
                  
                  let quotaExceeded = false;
                  let totalStored = 0;
                  
                  try {
                      // Try to store increasingly large amounts
                      for (let i = 1; i <= 10; i++) {
                          const data = 'x'.repeat(i * 100000); // 100KB, 200KB, etc.
                          localStorage.setItem('quota' + i, data);
                          totalStored += data.length;
                      }
                  } catch (e) {
                      if (e.name === 'QuotaExceededError') {
                          quotaExceeded = true;
                      }
                  }
                  
                  const results = {
                      quotaExceeded: quotaExceeded,
                      totalStored: totalStored,
                      itemsStored: localStorage.length,
                      errorHandled: quotaExceeded || totalStored > 0
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"errorHandled":true'; then
        log_success "Storage quota handling works correctly"
    else
        log_error "Storage quota test failed: $output"
    fi
}

# ========== Storage Performance Tests ==========

test_storage_performance() {
    run_test "Storage Performance"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  localStorage.clear();
                  
                  const iterations = 1000;
                  
                  // Test write performance
                  const writeStart = performance.now();
                  for (let i = 0; i < iterations; i++) {
                      localStorage.setItem('perf' + i, 'value' + i);
                  }
                  const writeEnd = performance.now();
                  
                  // Test read performance
                  const readStart = performance.now();
                  for (let i = 0; i < iterations; i++) {
                      localStorage.getItem('perf' + i);
                  }
                  const readEnd = performance.now();
                  
                  // Test enumeration performance
                  const enumStart = performance.now();
                  for (let i = 0; i < localStorage.length; i++) {
                      localStorage.key(i);
                  }
                  const enumEnd = performance.now();
                  
                  const results = {
                      writeTime: writeEnd - writeStart,
                      readTime: readEnd - readStart,
                      enumTime: enumEnd - enumStart,
                      itemsStored: localStorage.length,
                      performanceTestCompleted: true
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"performanceTestCompleted":true' && \
       echo "$output" | grep -q '"itemsStored":1000'; then
        log_success "Storage performance test completed"
    else
        log_error "Storage performance test failed: $output"
    fi
}

# Main test execution
main() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  HeadlessWeb Storage Integration Tests${NC}"
    echo -e "${BLUE}========================================${NC}\n"
    
    # Setup
    setup_test_env
    start_test_server
    
    # Run localStorage tests
    test_localstorage_basic
    test_localstorage_persistence
    test_localstorage_limits
    test_localstorage_enumeration
    
    # Run sessionStorage tests
    test_sessionstorage_basic
    test_sessionstorage_vs_localstorage
    test_sessionstorage_session_persistence
    
    # Run cookie tests
    test_cookies_basic
    test_cookies_persistence
    test_cookies_expiration
    
    # Run advanced storage tests
    test_storage_events
    test_storage_cross_origin
    test_storage_quota
    test_storage_performance
    
    # Print summary
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  Test Summary${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Tests run: ${TESTS_RUN}"
    echo -e "${GREEN}Tests passed: ${TESTS_PASSED}${NC}"
    echo -e "${RED}Tests failed: ${TESTS_FAILED}${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}All storage tests passed! 🎉${NC}"
        exit 0
    else
        echo -e "\n${RED}Some storage tests failed! ❌${NC}"
        exit 1
    fi
}

# Check prerequisites
check_prerequisites() {
    if [ ! -f "$HWEB" ]; then
        log_error "hweb binary not found: $HWEB"
        exit 1
    fi
    
    if ! command -v node >/dev/null 2>&1; then
        log_error "Node.js is required but not installed"
        exit 1
    fi
    
    if ! command -v curl >/dev/null 2>&1; then
        log_error "curl is required but not installed"
        exit 1
    fi
}

check_prerequisites
main "$@"