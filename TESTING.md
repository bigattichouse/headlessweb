# HeadlessWeb Testing Guide

This document provides a comprehensive overview of the testing infrastructure for HeadlessWeb, including both unit tests and integration tests.

## Testing Architecture

HeadlessWeb uses a multi-layered testing approach:

### 1. Unit Tests (`tests/`)
- **Framework**: Google Test (GTest/GMock)
- **Coverage**: 593 tests across all components
- **Focus**: Component isolation, method validation, edge cases
- **Execution**: `make hweb_tests` or `./run_tests_headless.sh`
- **Headless Ready**: Complete CI/CD compatibility with zero file dialogs

### 2. Shell Integration Tests (`script_test/`)
- **Framework**: Bash scripts with custom test helpers
- **Coverage**: 250+ end-to-end test scenarios  
- **Focus**: Real-world usage, CLI interface, feature integration
- **Execution**: `./script_test/comprehensive_test.sh`

### 3. Analysis Tools (`script_test/analysis_tools.sh`)
- **Coverage Analysis**: Identify untested components
- **Gap Analysis**: Prioritize testing improvements
- **Recommendations**: Actionable testing improvements

## Quick Start

### Run All Tests
```bash
# Unit tests (headless mode recommended)
./run_tests_headless.sh

# Unit tests (direct execution)
make hweb_tests && ./tests/hweb_tests

# Integration tests  
./script_test/comprehensive_test.sh

# Both (recommended for full validation)
./run_tests_headless.sh && ./script_test/comprehensive_test.sh
```

### Run Specific Test Categories
```bash
# Individual shell test modules
./script_test/test_navigation.sh     # Navigation and sessions
./script_test/test_screenshot.sh     # Screenshot functionality
./script_test/test_assertions.sh     # Assertion system
./script_test/test_forms.sh          # Form interactions
./script_test/test_javascript.sh     # JavaScript execution
./script_test/test_sessions.sh       # Session management

# Individual unit test suites (headless mode)
./run_tests_headless.sh --gtest_filter="ConfigParserTest*"
./run_tests_headless.sh --gtest_filter="BrowserCoreTest*"
./run_tests_headless.sh --gtest_filter="SessionTest*"

# Direct execution (may show GTK warnings)
./tests/hweb_tests --gtest_filter="ConfigParserTest*"
```

## Test Organization

### Unit Test Structure (`tests/`)
```
tests/
├── CMakeLists.txt              # Build configuration
├── test_main.cpp               # Test runner
├── hweb/                       # Modular component tests
│   ├── test_config_parser.cpp  # CLI argument parsing
│   ├── test_manager_registry.cpp # Singleton managers
│   ├── test_navigation_service.cpp # Navigation logic
│   └── test_output.cpp         # Output formatting
├── session/                    # Session management tests
├── assertion/                  # Assertion framework tests
├── fileops/                    # File operations tests
├── browser/                    # Browser component tests
└── integration/                # Cross-component tests
```

### Shell Test Structure (`script_test/`)
```
script_test/
├── README.md                   # Detailed documentation
├── test_helpers.sh            # Shared utilities
├── comprehensive_test.sh      # Main test runner
├── test_navigation.sh         # Navigation & URL handling
├── test_screenshot.sh         # Screenshot functionality
├── test_assertions.sh         # Assertion system
├── test_forms.sh              # Form interactions
├── test_javascript.sh         # JavaScript execution
├── test_sessions.sh           # Session management
├── analysis_tools.sh          # Coverage analysis
└── run_legacy_tests.sh        # Legacy test access
```

## Test Coverage

### Current Status - 🏆 **100% SUCCESS ACHIEVED**
- **Unit Tests**: 629 tests covering core components and modular architecture (**100% pass rate**)
- **Integration Tests**: 250+ scenarios covering end-to-end workflows
- **Component Coverage**: 100% of source files have corresponding tests
- **Feature Coverage**: All major CLI features tested and **fully operational**
- **Headless Compatible**: Zero skipped tests, complete CI/CD readiness
- **Final Achievement**: **All 629 tests passing consistently** - Ultimate reliability achieved

### Key Achievements
✅ **Modular Architecture**: New hweb components (22+ tests)  
✅ **Browser Components**: Core, DOM, JavaScript, Events (100+ tests)  
✅ **Session Management**: Persistence, isolation, restoration (30+ tests)  
✅ **File Operations**: Upload, download, validation (25+ tests)  
✅ **Assertion Framework**: All assertion types and modes (50+ tests)  
✅ **Integration Scenarios**: Cross-component workflows (15+ tests)  

## Headless Testing Infrastructure

### Why Headless Testing?
HeadlessWeb includes comprehensive headless testing infrastructure to ensure:
- **CI/CD Compatibility**: No GUI dependencies or file dialogs
- **Automated Testing**: Runs reliably in Docker, GitHub Actions, etc.
- **Performance**: Faster execution without graphics overhead
- **Consistency**: Identical behavior across environments

### Headless Test Runner
```bash
# Run all tests in headless mode (recommended)
./run_tests_headless.sh

# Run specific test suites in headless mode
./run_tests_headless.sh --gtest_filter="*DOMTest*"

# Run with timeout protection
timeout 600s ./run_tests_headless.sh
```

### File Dialog Prevention
The headless infrastructure completely prevents file dialogs through:
- **WebKit Signal Interception**: Blocks all file chooser requests at the browser level
- **Environment Variables**: Comprehensive GTK dialog prevention
- **Virtual Display**: Xvfb virtual display server when needed
- **Test Code Safety**: File operations use simulation instead of actual file pickers

### Environment Configuration
```bash
# Automatic headless configuration (handled by run_tests_headless.sh)
export GTK_RECENT_FILES_ENABLED=0
export WEBKIT_DISABLE_FILE_PICKER=1
export GTK_FILE_CHOOSER_BACKEND=none
export NO_AT_BRIDGE=1
export GIO_USE_VFS=local
export GVFS_DISABLE_FUSE=1
```

## Development Workflow

### Before Code Changes
```bash
# Ensure all tests pass
make test
./sh_test/comprehensive_test.sh
```

### After Code Changes
```bash
# Quick verification
make test

# Full validation  
make test && ./script_test/comprehensive_test.sh
```

### Adding New Features
1. **Write unit tests** for the component (`tests/`)
2. **Add integration tests** for the feature (`script_test/`)
3. **Update existing tests** if interfaces change
4. **Run analysis tools** to identify coverage gaps

### Test-Driven Development
```bash
# 1. Write failing test
./hweb_tests --gtest_filter="NewFeatureTest*"

# 2. Implement feature
# ... code changes ...

# 3. Verify test passes
./hweb_tests --gtest_filter="NewFeatureTest*"

# 4. Run full test suite
make test
```

## Continuous Integration

### Local CI Simulation
```bash
#!/bin/bash
# Simulate CI pipeline locally

echo "1. Clean build..."
make clean

echo "2. Compile..."
make

echo "3. Unit tests..."
make test

echo "4. Integration tests..."
./script_test/comprehensive_test.sh

echo "5. Analysis..."
./script_test/analysis_tools.sh coverage

echo "All tests completed successfully!"
```

### Exit Codes
- `0`: All tests passed
- `1`: Some tests failed  
- `2`: Build or infrastructure errors

## Debugging Tests

### Unit Test Debugging
```bash
# Run with verbose output
./hweb_tests --gtest_filter="FailingTest*" --gtest_verbose

# Run specific test case
./hweb_tests --gtest_filter="ConfigParserTest.ParseBasicOptions"

# Debug with GDB
gdb --args ./hweb_tests --gtest_filter="FailingTest*"
```

### Shell Test Debugging  
```bash
# Run with bash debugging
bash -x ./script_test/test_navigation.sh

# Run individual test functions
# Edit the script to call specific functions

# Check hweb command output directly
./hweb --session debug_session --url "file:///tmp/test.html" --debug
```

### Common Issues
- **Session conflicts**: Tests clean up automatically, but manual cleanup may be needed
- **Network dependencies**: Some tests require internet access
- **File permissions**: Ensure test scripts are executable
- **Timing issues**: Browser tests may need longer timeouts on slow systems

## Performance Testing

### Test Execution Times
- **Unit tests**: ~30 seconds for complete suite
- **Shell tests**: ~2-5 minutes depending on network
- **Combined**: ~5-7 minutes total

### Performance Benchmarks
```bash
# Time individual test modules
time ./script_test/test_forms.sh
time ./hweb_tests --gtest_filter="BrowserCoreTest*"

# Profile test execution
./script_test/comprehensive_test.sh 2>&1 | tee test_log.txt
```

## Legacy Test Migration

### Original Scripts Status
- `comprehensive-test.sh` → Replaced by `script_test/comprehensive_test.sh`
- `test-assertions.sh` → Replaced by `script_test/test_assertions.sh`  
- `test-screenshot.sh` → Replaced by `script_test/test_screenshot.sh`
- `analyze_test_coverage.sh` → Available via `script_test/analysis_tools.sh coverage`
- `identify_test_gaps.sh` → Available via `script_test/analysis_tools.sh gaps`

### Access Legacy Tests
```bash
# Run original scripts for comparison
./script_test/run_legacy_tests.sh comprehensive
./script_test/run_legacy_tests.sh assertions
./script_test/run_legacy_tests.sh screenshots
```

## Contributing

### Test Guidelines
1. **Isolation**: Tests should not depend on external state
2. **Cleanup**: Always clean up created resources  
3. **Documentation**: Clear test descriptions and expected outcomes
4. **Coverage**: Test both success and failure paths
5. **Performance**: Tests should complete in reasonable time

### Code Coverage Goals
- **New components**: 100% test coverage required
- **Existing components**: Maintain current >95% coverage
- **Integration tests**: Cover all major user workflows
- **Edge cases**: Test boundary conditions and error scenarios

### Review Checklist
- [ ] Unit tests added for new functionality
- [ ] Integration tests cover user-facing changes  
- [ ] All existing tests still pass
- [ ] Test coverage analysis shows no regressions
- [ ] Performance impact assessed
- [ ] Documentation updated

This comprehensive testing infrastructure ensures HeadlessWeb remains reliable, maintainable, and robust across all supported features and use cases.