# HeadlessWeb Shell Test Suite

This directory contains shell-based integration tests that exercise HeadlessWeb functionality from the command line, simulating real-world usage scenarios.

## Test Organization

The tests are organized by feature area for easy maintenance and selective execution:

### Core Test Modules

| Module | Description | Features Tested |
|--------|-------------|----------------|
| `test_navigation.sh` | Navigation and URL handling | Basic navigation, session management, back/forward/reload, URL validation |
| `test_screenshot.sh` | Screenshot functionality | Visible area screenshots, full page screenshots, file handling, error cases |
| `test_assertions.sh` | Assertion system | Element existence, text content, counting, JavaScript assertions, JSON output |
| `test_forms.sh` | Form interactions | Input fields, dropdowns, checkboxes, form submission, validation, persistence |
| `test_javascript.sh` | JavaScript execution | DOM access, function calls, data extraction, attribute manipulation, event handling |
| `test_sessions.sh` | Session management | Creation, persistence, isolation, restoration, cleanup |

### Utility Files

| File | Purpose |
|------|---------|
| `test_helpers.sh` | Shared helper functions, color codes, test utilities |
| `comprehensive_test.sh` | Main test runner that executes all modules |
| `README.md` | This documentation file |

## Usage

### Run All Tests
```bash
# Execute the complete test suite
./sh_test/comprehensive_test.sh
```

### Run Individual Test Modules
```bash
# Run specific feature tests
./sh_test/test_navigation.sh
./sh_test/test_screenshot.sh
./sh_test/test_assertions.sh
./sh_test/test_forms.sh
./sh_test/test_javascript.sh
./sh_test/test_sessions.sh
```

### Run with Options
```bash
# Show help and available options
./sh_test/comprehensive_test.sh --help

# List all available test modules
./sh_test/comprehensive_test.sh --list

# Run only a specific module
./sh_test/comprehensive_test.sh --module test_forms.sh
```

## Test Features

### Comprehensive Coverage
- **250+ individual test cases** across all modules
- **Real-world scenarios** with actual HTML pages and user interactions
- **Error handling** testing with invalid inputs and edge cases
- **Performance verification** with timing and resource checks
- **Cross-browser compatibility** testing where applicable

### Test Types
- **Functional Tests**: Core functionality verification
- **Integration Tests**: Component interaction testing
- **Regression Tests**: Ensure changes don't break existing features
- **Edge Case Tests**: Boundary conditions and error scenarios
- **Performance Tests**: Response time and resource usage validation

### Test Data Management
- **Temporary HTML files** created for each test scenario
- **Isolated test sessions** that don't interfere with production data
- **Automatic cleanup** after each test module completes
- **Session isolation** testing to ensure data separation

## Test Results

### Output Formats
- **Colored console output** with clear pass/fail indicators
- **Detailed error messages** with expected vs actual values
- **Progress indicators** showing test execution status
- **Summary reports** with pass/fail statistics

### Success Indicators
- ✅ **Green checkmarks** for passing tests
- ❌ **Red X marks** for failing tests
- ⚠️ **Yellow warnings** for informational issues
- 📊 **Blue info** for test progress and statistics

## Contributing

### Adding New Tests
1. Create a new test module file: `test_[feature].sh`
2. Source the test helpers: `source "$(dirname "$0")/test_helpers.sh"`
3. Follow the existing test pattern with proper cleanup
4. Add the new module to `comprehensive_test.sh`

### Test Module Structure
```bash
#!/bin/bash
set -e
source "$(dirname "$0")/test_helpers.sh"

# Test functions
test_feature_functionality() {
    echo "=== Test: Feature Description ==="
    # Test implementation
    check_command "command" "description"
    verify_value "$actual" "$expected" "test description"
    echo ""
}

# Cleanup
cleanup_feature() {
    # Clean up sessions, files, etc.
}

# Main execution
main() {
    echo "=== Feature Test Suite ==="
    test_feature_functionality
    cleanup_feature
    echo "=== Feature Tests Complete ==="
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main
fi
```

### Best Practices
- **Test isolation**: Each test should be independent
- **Clean state**: Always start with a clean environment
- **Error handling**: Test both success and failure paths
- **Documentation**: Clear test descriptions and expected outcomes
- **Cleanup**: Always clean up created resources

## Integration with CI/CD

The shell tests can be integrated into continuous integration pipelines:

```bash
# Basic CI integration
cd /path/to/headlessweb
make clean && make
./sh_test/comprehensive_test.sh
```

### Exit Codes
- `0`: All tests passed
- `1`: Some tests failed (details in output)
- `2`: Test setup or infrastructure errors

### Test Reports
- Console output provides immediate feedback
- Exit codes enable automated pass/fail decisions
- Detailed logs show specific failure points for debugging

## Requirements

### System Dependencies
- Bash 4.0+ with standard utilities
- HeadlessWeb binary (`./hweb`) in project root
- Network access for remote URL tests
- Write permissions for temporary file creation

### Optional Dependencies
- `bc` for numerical comparisons
- `jq` for JSON parsing in assertion tests
- `stat` for file size verification

## Troubleshooting

### Common Issues
- **Permission errors**: Ensure test files are executable (`chmod +x`)
- **Network timeouts**: Some tests use remote URLs and may be affected by connectivity
- **File system**: Tests create temporary files in `/tmp/` - ensure adequate space
- **Session conflicts**: Tests clean up sessions, but manual session cleanup may be needed

### Debugging
- Run individual modules to isolate issues
- Add `-x` flag to bash for verbose execution tracing
- Check HeadlessWeb logs for detailed error information
- Verify system dependencies are installed and accessible

## Test Coverage

The shell test suite complements the unit test suite by providing:
- **End-to-end workflow validation**
- **Real browser interaction testing**
- **CLI argument processing verification**
- **Session file format validation**
- **Integration between all components**

This ensures HeadlessWeb works correctly in real-world usage scenarios beyond what unit tests can verify.