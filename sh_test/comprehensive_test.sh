#!/bin/bash

# Comprehensive Test Suite
# Runs all individual test modules and provides overall results

set -e

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source test helpers
source "$SCRIPT_DIR/test_helpers.sh"

# Test module configuration
MODULES=(
    "test_navigation.sh"
    "test_screenshot.sh" 
    "test_assertions.sh"
    "test_forms.sh"
    "test_javascript.sh"
    "test_sessions.sh"
)

# Results tracking
TOTAL_MODULES=0
PASSED_MODULES=0
FAILED_MODULES=0

# Main test execution
main() {
    echo "========================================"
    echo "HeadlessWeb Comprehensive Test Suite"
    echo "========================================"
    echo "Testing all functionality of hweb"
    echo ""
    
    # Initial cleanup
    cleanup
    
    echo "Running individual test modules..."
    echo ""
    
    for module in "${MODULES[@]}"; do
        TOTAL_MODULES=$((TOTAL_MODULES + 1))
        
        echo "========================================" 
        echo "Running: $module"
        echo "========================================"
        
        if [[ -f "$SCRIPT_DIR/$module" && -x "$SCRIPT_DIR/$module" ]]; then
            set +e  # Don't exit on test failures
            "$SCRIPT_DIR/$module"
            MODULE_EXIT_CODE=$?
            set -e
            
            if [[ $MODULE_EXIT_CODE -eq 0 ]]; then
                echo -e "${GREEN}✅ MODULE PASSED${NC}: $module"
                PASSED_MODULES=$((PASSED_MODULES + 1))
            else
                echo -e "${RED}❌ MODULE FAILED${NC}: $module (exit code: $MODULE_EXIT_CODE)"
                FAILED_MODULES=$((FAILED_MODULES + 1))
            fi
        else
            echo -e "${RED}❌ MODULE NOT FOUND${NC}: $module"
            FAILED_MODULES=$((FAILED_MODULES + 1))
        fi
        
        echo ""
        echo "----------------------------------------"
        echo ""
    done
    
    # Final cleanup
    cleanup
    
    # Overall results
    echo "========================================"
    echo "COMPREHENSIVE TEST RESULTS"
    echo "========================================"
    echo -e "${BLUE}Total Test Modules: $TOTAL_MODULES${NC}"
    echo -e "${GREEN}Modules Passed: $PASSED_MODULES${NC}"
    echo -e "${RED}Modules Failed: $FAILED_MODULES${NC}"
    
    if [[ $FAILED_MODULES -eq 0 ]]; then
        echo ""
        echo -e "${GREEN}🎉 ALL TESTS PASSED!${NC}"
        echo ""
        echo "HeadlessWeb is functioning correctly across all tested features:"
        echo "✅ Navigation and session management"
        echo "✅ Screenshot functionality"
        echo "✅ Assertion system"
        echo "✅ Form interactions"
        echo "✅ JavaScript execution"
        echo "✅ Session persistence and isolation"
        echo ""
        echo "The application is ready for production use!"
        exit 0
    else
        echo ""
        echo -e "${RED}❌ SOME TESTS FAILED${NC}"
        echo "Success rate: $(( PASSED_MODULES * 100 / TOTAL_MODULES ))%"
        echo ""
        echo "Please review the failed modules above and fix any issues."
        exit 1
    fi
}

# Print usage information
usage() {
    echo "HeadlessWeb Comprehensive Test Suite"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  -l, --list     List available test modules"
    echo "  -m, --module   Run specific module only"
    echo ""
    echo "Available modules:"
    for module in "${MODULES[@]}"; do
        echo "  - $module"
    done
    echo ""
    echo "Examples:"
    echo "  $0                    # Run all tests"
    echo "  $0 --list             # List test modules"
    echo "  $0 -m test_forms.sh   # Run only form tests"
}

# List available modules
list_modules() {
    echo "Available HeadlessWeb test modules:"
    echo ""
    for module in "${MODULES[@]}"; do
        if [[ -f "$SCRIPT_DIR/$module" ]]; then
            echo -e "  ${GREEN}✓${NC} $module"
        else
            echo -e "  ${RED}✗${NC} $module (not found)"
        fi
    done
}

# Run specific module
run_module() {
    local module="$1"
    
    if [[ -f "$SCRIPT_DIR/$module" && -x "$SCRIPT_DIR/$module" ]]; then
        echo "Running single module: $module"
        echo ""
        "$SCRIPT_DIR/$module"
    else
        echo -e "${RED}Error${NC}: Module '$module' not found or not executable"
        echo "Available modules:"
        list_modules
        exit 1
    fi
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -l|--list)
            list_modules
            exit 0
            ;;
        -m|--module)
            if [[ -n "$2" ]]; then
                run_module "$2"
                exit $?
            else
                echo -e "${RED}Error${NC}: Module name required"
                usage
                exit 1
            fi
            ;;
        *)
            echo -e "${RED}Error${NC}: Unknown option: $1"
            usage
            exit 1
            ;;
    esac
    shift
done

# Run main test suite if no specific options provided
main