#!/bin/bash

# Legacy Test Runner
# Provides access to the original comprehensive test scripts for comparison

set -e

# Source helpers
source "$(dirname "$0")/test_helpers.sh"

# Configuration
PROJECT_ROOT="$(dirname "$0")/.."

usage() {
    echo "HeadlessWeb Legacy Test Runner"
    echo ""
    echo "This script provides access to the original comprehensive test scripts"
    echo "for comparison and validation purposes."
    echo ""
    echo "Usage: $0 [test-type]"
    echo ""
    echo "Available legacy tests:"
    echo "  comprehensive    - Original comprehensive test suite"
    echo "  assertions       - Original assertion test suite"  
    echo "  screenshots      - Original screenshot test"
    echo ""
    echo "Note: These tests may be outdated and are provided for reference only."
    echo "Use the modular test suite (comprehensive_test.sh) for current testing."
}

run_comprehensive() {
    echo "=== Running Legacy Comprehensive Test ==="
    echo "Note: This runs the original comprehensive-test.sh script"
    echo ""
    
    if [[ -f "$PROJECT_ROOT/comprehensive-test.sh" && -x "$PROJECT_ROOT/comprehensive-test.sh" ]]; then
        cd "$PROJECT_ROOT"
        ./comprehensive-test.sh
    else
        echo -e "${RED}Error${NC}: comprehensive-test.sh not found or not executable"
        exit 1
    fi
}

run_assertions() {
    echo "=== Running Legacy Assertions Test ==="
    echo "Note: This runs the original test-assertions.sh script"
    echo ""
    
    if [[ -f "$PROJECT_ROOT/test-assertions.sh" && -x "$PROJECT_ROOT/test-assertions.sh" ]]; then
        cd "$PROJECT_ROOT" 
        ./test-assertions.sh
    else
        echo -e "${RED}Error${NC}: test-assertions.sh not found or not executable"
        exit 1
    fi
}

run_screenshots() {
    echo "=== Running Legacy Screenshot Test ==="
    echo "Note: This runs the original test-screenshot.sh script"
    echo ""
    
    if [[ -f "$PROJECT_ROOT/test-screenshot.sh" && -x "$PROJECT_ROOT/test-screenshot.sh" ]]; then
        cd "$PROJECT_ROOT"
        ./test-screenshot.sh
    else
        echo -e "${RED}Error${NC}: test-screenshot.sh not found or not executable"  
        exit 1
    fi
}

# Main execution
case "${1:-help}" in
    comprehensive)
        run_comprehensive
        ;;
    assertions)
        run_assertions
        ;;
    screenshots)
        run_screenshots
        ;;
    help|--help|-h)
        usage
        exit 0
        ;;
    *)
        echo -e "${RED}Error${NC}: Unknown test type: $1"
        echo ""
        usage
        exit 1
        ;;
esac