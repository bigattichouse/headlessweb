#!/bin/bash

# Shared helper functions for HeadlessWeb shell tests

# Color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SESSION_DIR="$HOME/.hweb/sessions"

# Test execution helpers
check_command() {
    local cmd="$1"
    local description="$2"
    
    echo "Running: $cmd"
    if eval "$cmd"; then
        echo -e "${GREEN}✓ SUCCESS${NC}: $description"
    else
        local exit_code=$?
        echo -e "${RED}✗ FAILED${NC}: $description (exit code: $exit_code)"
        return $exit_code
    fi
    echo ""
}

verify_value() {
    local actual="$1"
    local expected="$2"
    local description="$3"
    
    if [[ "$actual" == "$expected" ]]; then
        echo -e "${GREEN}✓ PASS${NC}: $description"
        echo "  Value: '$actual'"
    else
        echo -e "${RED}✗ FAIL${NC}: $description"
        echo "  Expected: '$expected'"
        echo "  Actual: '$actual'"
    fi
    echo ""
}

verify_contains() {
    local actual="$1"
    local expected="$2"
    local description="$3"
    
    if [[ "$actual" == *"$expected"* ]]; then
        echo -e "${GREEN}✓ PASS${NC}: $description"
        echo "  Value contains: '$expected'"
    else
        echo -e "${RED}✗ FAIL${NC}: $description"
        echo "  Expected to contain: '$expected'"
        echo "  Actual: '$actual'"
    fi
    echo ""
}

info_message() {
    echo -e "${YELLOW}ℹ INFO${NC}: $1"
    echo ""
}

# Test counting and reporting
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

run_test() {
    local test_name="$1"
    local cmd="$2"
    local expected_exit_code="${3:-0}"
    local description="$4"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "${BLUE}Test $TESTS_RUN${NC}: $test_name"
    echo "Command: $cmd"
    
    set +e
    eval "$cmd" >/dev/null 2>&1
    local actual_exit_code=$?
    set -e
    
    if [[ $actual_exit_code -eq $expected_exit_code ]]; then
        echo -e "${GREEN}✓ PASS${NC}: $description (exit code: $actual_exit_code)"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: $description"
        echo "  Expected exit code: $expected_exit_code"
        echo "  Actual exit code: $actual_exit_code"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    echo ""
}

run_test_with_output() {
    local test_name="$1"
    local cmd="$2"
    local expected_output="$3"
    local description="$4"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "${BLUE}Test $TESTS_RUN${NC}: $test_name"
    echo "Command: $cmd"
    
    set +e
    local actual_output=$(eval "$cmd" 2>/dev/null)
    local exit_code=$?
    set -e
    
    if [[ "$actual_output" == *"$expected_output"* ]] && [[ $exit_code -eq 0 ]]; then
        echo -e "${GREEN}✓ PASS${NC}: $description"
        echo "  Output contains: '$expected_output'"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: $description"
        echo "  Expected to contain: '$expected_output'"
        echo "  Actual output: '$actual_output'"
        echo "  Exit code: $exit_code"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    echo ""
}

# Cleanup functions
cleanup() {
    echo "=== Cleanup Phase ==="
    rm -rf "$SESSION_DIR"
    rm -f /tmp/hweb_*.html
    rm -f *.png
    rm -f test-results.xml
    mkdir -p "$SESSION_DIR"
    echo "Cleaned up session directory and test files"
    echo ""
}

# Test summary reporting
print_test_summary() {
    local suite_name="$1"
    echo "=== $suite_name Test Summary ==="
    echo -e "${BLUE}Total Tests Run: $TESTS_RUN${NC}"
    echo -e "${GREEN}Tests Passed: $TESTS_PASSED${NC}"
    echo -e "${RED}Tests Failed: $TESTS_FAILED${NC}"
    
    if [[ $TESTS_FAILED -eq 0 ]]; then
        echo -e "${GREEN}🎉 ALL TESTS PASSED!${NC}"
        return 0
    else
        echo -e "${RED}❌ Some tests failed${NC}"
        echo "Success rate: $(( TESTS_PASSED * 100 / TESTS_RUN ))%"
        return 1
    fi
}