#!/bin/bash

# Analysis Tools Runner
# Provides access to code analysis and coverage tools

set -e

# Configuration
PROJECT_ROOT="$(dirname "$0")/.."
SCRIPT_DIR="$(dirname "$0")"

# Source helpers
source "$SCRIPT_DIR/test_helpers.sh"

usage() {
    echo "HeadlessWeb Analysis Tools"
    echo ""
    echo "Usage: $0 [tool]"
    echo ""
    echo "Available analysis tools:"
    echo "  coverage     - Analyze test coverage across components"
    echo "  gaps         - Identify specific testing gaps and priorities"
    echo "  all          - Run all analysis tools"
    echo ""
    echo "These tools help identify areas that need additional testing"
    echo "and provide recommendations for test improvements."
}

run_coverage_analysis() {
    echo "=== Test Coverage Analysis ==="
    echo ""
    
    if [[ -f "$PROJECT_ROOT/analyze_test_coverage.sh" && -x "$PROJECT_ROOT/analyze_test_coverage.sh" ]]; then
        cd "$PROJECT_ROOT"
        ./analyze_test_coverage.sh
    else
        echo -e "${RED}Error${NC}: analyze_test_coverage.sh not found"
        return 1
    fi
}

run_gap_analysis() {
    echo "=== Test Gap Analysis ==="
    echo ""
    
    if [[ -f "$PROJECT_ROOT/identify_test_gaps.sh" && -x "$PROJECT_ROOT/identify_test_gaps.sh" ]]; then
        cd "$PROJECT_ROOT"
        ./identify_test_gaps.sh
    else
        echo -e "${RED}Error${NC}: identify_test_gaps.sh not found"
        return 1
    fi
}

run_all_analysis() {
    echo "=== Running All Analysis Tools ==="
    echo ""
    
    echo "1/2: Test Coverage Analysis"
    echo "==============================="
    run_coverage_analysis
    echo ""
    
    echo "2/2: Test Gap Analysis" 
    echo "======================="
    run_gap_analysis
    echo ""
    
    echo "=== Analysis Complete ==="
    echo ""
    echo "Review the output above for:"
    echo "- Components lacking test coverage"
    echo "- Integration test opportunities"
    echo "- Performance test recommendations"
    echo "- Priority areas for test development"
}

# Main execution
case "${1:-help}" in
    coverage)
        run_coverage_analysis
        ;;
    gaps)
        run_gap_analysis
        ;;
    all)
        run_all_analysis
        ;;
    help|--help|-h)
        usage
        exit 0
        ;;
    *)
        echo -e "${RED}Error${NC}: Unknown analysis tool: $1"
        echo ""
        usage
        exit 1
        ;;
esac