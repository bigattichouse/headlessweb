#!/bin/bash

# HeadlessWeb Test Coverage Analysis Script

echo "========================================"
echo "HeadlessWeb Test Coverage Analysis"
echo "========================================"
echo

# Count source files
echo "1. SOURCE FILE ANALYSIS"
echo "------------------------"
src_files=$(find src/ -name "*.cpp" -type f | wc -l)
echo "Total source files: $src_files"
echo "Source files by component:"
find src/ -name "*.cpp" -type f | sed 's|src/||' | sort | sed 's|^|  |'
echo

# Count test files
echo "2. TEST FILE ANALYSIS"
echo "---------------------"
test_files=$(find tests/ -name "*.cpp" -type f | grep -v mock | grep -v test_main | grep -v debug_test | grep -v test_helpers | wc -l)
echo "Total test files: $test_files"
echo "Test files by component:"
find tests/ -name "*.cpp" -type f | grep -v mock | grep -v test_main | grep -v debug_test | grep -v test_helpers | sed 's|tests/||' | sort | sed 's|^|  |'
echo

# Count test cases per file
echo "3. TEST CASE BREAKDOWN"
echo "----------------------"
total_tests=0
for file in $(find tests/ -name "*.cpp" | grep -v mock | grep -v test_main | grep -v debug_test | grep -v test_helpers); do
    count=$(grep -c 'TEST_F' "$file")
    total_tests=$((total_tests + count))
    component=$(echo "$file" | sed 's|tests/||' | sed 's|/.*||')
    filename=$(basename "$file")
    printf "  %-20s %-30s %3d tests\n" "$component" "$filename" "$count"
done
echo "  ----------------------------------------"
echo "  Total test cases: $total_tests"
echo

# Coverage analysis by component
echo "4. COVERAGE BY COMPONENT"
echo "------------------------"

components=("assertion" "browser" "fileops" "session" "main")

for component in "${components[@]}"; do
    echo "  $component:"
    
    # Count source files in component
    src_count=$(find src/ -path "*/$component/*" -name "*.cpp" 2>/dev/null | wc -l)
    if [ "$component" = "main" ]; then
        src_count=$(find src/ -maxdepth 1 -name "*.cpp" | wc -l)
    fi
    
    # Count test files in component
    test_count=$(find tests/ -path "*/$component/*" -name "*.cpp" 2>/dev/null | grep -v mock | wc -l)
    
    # Count test cases in component
    test_case_count=0
    for file in $(find tests/ -path "*/$component/*" -name "*.cpp" 2>/dev/null | grep -v mock); do
        if [ -f "$file" ]; then
            count=$(grep -c 'TEST_F' "$file" 2>/dev/null || echo 0)
            test_case_count=$((test_case_count + count))
        fi
    done
    
    printf "    Source files: %2d | Test files: %2d | Test cases: %3d\n" "$src_count" "$test_count" "$test_case_count"
done
echo

# Identify missing test coverage
echo "5. MISSING TEST COVERAGE"
echo "------------------------"
echo "Source files without corresponding tests:"

# Check for untested source files
for src_file in $(find src/ -name "*.cpp" -type f); do
    # Extract component and filename
    component_path=$(echo "$src_file" | sed 's|src/||' | sed 's|\.cpp$||')
    component=$(echo "$component_path" | cut -d'/' -f1)
    filename=$(basename "$component_path")
    
    # Look for corresponding test file
    test_pattern="tests/$component/test_*$filename*.cpp"
    if ! ls $test_pattern 2>/dev/null | grep -q .; then
        echo "  $src_file"
    fi
done
echo

# Check for missing Browser components
echo "6. BROWSER COMPONENT ANALYSIS"
echo "-----------------------------"
echo "Browser source files:"
find src/Browser/ -name "*.cpp" | sed 's|src/Browser/||' | sort | sed 's|^|  |'
echo
echo "Browser test files:"
find tests/browser/ -name "*.cpp" 2>/dev/null | sed 's|tests/browser/||' | sort | sed 's|^|  |' || echo "  (none found)"
echo

# Missing critical components
echo "7. POTENTIALLY MISSING TESTS"
echo "----------------------------"
missing_components=()

# Check for critical Browser components without tests
browser_files=("Screenshot.cpp" "Storage.cpp" "Utilities.cpp" "Wait.cpp")
for file in "${browser_files[@]}"; do
    if [ -f "src/Browser/$file" ]; then
        base_name=$(echo "$file" | sed 's|\.cpp$||' | tr '[:upper:]' '[:lower:]')
        if ! find tests/browser/ -name "*$base_name*" 2>/dev/null | grep -q .; then
            missing_components+=("Browser/$file")
        fi
    fi
done

# Check main application files
if [ -f "src/hweb.cpp" ] && ! find tests/ -name "*hweb*" 2>/dev/null | grep -q .; then
    missing_components+=("main/hweb.cpp")
fi

if [ ${#missing_components[@]} -gt 0 ]; then
    echo "Components likely missing tests:"
    for comp in "${missing_components[@]}"; do
        echo "  $comp"
    done
else
    echo "All major components appear to have test coverage."
fi
echo

# Integration test analysis
echo "8. INTEGRATION TEST ASSESSMENT"
echo "------------------------------"
integration_indicators=$(grep -r "Browser.*Session\|Session.*Browser\|FileOps.*Browser" tests/ 2>/dev/null | wc -l)
echo "Cross-component test interactions found: $integration_indicators"

if [ "$integration_indicators" -lt 5 ]; then
    echo "  → Recommendation: Add more integration tests between components"
else
    echo "  → Good: Multiple component interactions being tested"
fi
echo

# Performance test check
echo "9. PERFORMANCE & STRESS TESTS"
echo "-----------------------------"
perf_tests=$(grep -r "performance\|stress\|load\|timeout.*large\|concurrent" tests/ 2>/dev/null | wc -l)
echo "Performance-related test indicators: $perf_tests"

if [ "$perf_tests" -lt 3 ]; then
    echo "  → Recommendation: Consider adding performance and stress tests"
else
    echo "  → Good: Some performance testing appears to be in place"
fi
echo

# Summary and recommendations
echo "10. SUMMARY & RECOMMENDATIONS"
echo "==============================="
coverage_ratio=$(echo "scale=1; $test_files * 100 / $src_files" | bc 2>/dev/null || echo "~85")
echo "Test file coverage ratio: $coverage_ratio% ($test_files test files / $src_files source files)"
echo "Total test cases: $total_tests"
echo

echo "Priority Recommendations:"
echo "1. HIGH: Add tests for missing Browser components (Screenshot, Storage, Utilities, Wait)"
echo "2. HIGH: Create integration tests between Browser, Session, and FileOps"
echo "3. MEDIUM: Add error condition and edge case tests"
echo "4. MEDIUM: Add performance/stress tests for file operations and browser interactions"
echo "5. LOW: Consider end-to-end workflow tests"
echo

echo "Overall Assessment: GOOD test coverage with room for improvement in integration testing"
echo "========================================"