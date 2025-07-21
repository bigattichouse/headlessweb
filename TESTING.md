# HeadlessWeb Testing Guide

This document provides a comprehensive overview of the testing infrastructure for HeadlessWeb, including both unit tests and integration tests.

## Testing Architecture

HeadlessWeb uses a multi-layered testing approach:

### 1. Unit Tests (`tests/`)
- **Framework**: Google Test (GTest/GMock)
- **Coverage**: 450+ tests across all components
- **Focus**: Component isolation, method validation, edge cases
- **Execution**: `make test` or `./hweb_tests`

### 2. Shell Integration Tests (`sh_test/`)
- **Framework**: Bash scripts with custom test helpers
- **Coverage**: 250+ end-to-end test scenarios  
- **Focus**: Real-world usage, CLI interface, feature integration
- **Execution**: `./sh_test/comprehensive_test.sh`

### 3. Analysis Tools (`sh_test/analysis_tools.sh`)
- **Coverage Analysis**: Identify untested components
- **Gap Analysis**: Prioritize testing improvements
- **Recommendations**: Actionable testing improvements

## Quick Start

### Run All Tests
```bash
# Unit tests
make test

# Integration tests  
./sh_test/comprehensive_test.sh

# Both (recommended for full validation)
make test && ./sh_test/comprehensive_test.sh
```

### Run Specific Test Categories
```bash
# Individual shell test modules
./sh_test/test_navigation.sh     # Navigation and sessions
./sh_test/test_screenshot.sh     # Screenshot functionality
./sh_test/test_assertions.sh     # Assertion system
./sh_test/test_forms.sh          # Form interactions
./sh_test/test_javascript.sh     # JavaScript execution
./sh_test/test_sessions.sh       # Session management

# Individual unit test suites
./hweb_tests --gtest_filter="ConfigParserTest*"
./hweb_tests --gtest_filter="BrowserCoreTest*"
./hweb_tests --gtest_filter="SessionTest*"
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

### Shell Test Structure (`sh_test/`)
```
sh_test/
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

### Current Status
- **Unit Tests**: 450+ tests covering core components and modular architecture
- **Integration Tests**: 250+ scenarios covering end-to-end workflows
- **Component Coverage**: 95%+ of source files have corresponding tests
- **Feature Coverage**: All major CLI features tested

### Key Achievements
✅ **Modular Architecture**: New hweb components (22+ tests)  
✅ **Browser Components**: Core, DOM, JavaScript, Events (100+ tests)  
✅ **Session Management**: Persistence, isolation, restoration (30+ tests)  
✅ **File Operations**: Upload, download, validation (25+ tests)  
✅ **Assertion Framework**: All assertion types and modes (50+ tests)  
✅ **Integration Scenarios**: Cross-component workflows (15+ tests)  

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
make test && ./sh_test/comprehensive_test.sh
```

### Adding New Features
1. **Write unit tests** for the component (`tests/`)
2. **Add integration tests** for the feature (`sh_test/`)
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
./sh_test/comprehensive_test.sh

echo "5. Analysis..."
./sh_test/analysis_tools.sh coverage

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
bash -x ./sh_test/test_navigation.sh

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
time ./sh_test/test_forms.sh
time ./hweb_tests --gtest_filter="BrowserCoreTest*"

# Profile test execution
./sh_test/comprehensive_test.sh 2>&1 | tee test_log.txt
```

## Legacy Test Migration

### Original Scripts Status
- `comprehensive-test.sh` → Replaced by `sh_test/comprehensive_test.sh`
- `test-assertions.sh` → Replaced by `sh_test/test_assertions.sh`  
- `test-screenshot.sh` → Replaced by `sh_test/test_screenshot.sh`
- `analyze_test_coverage.sh` → Available via `sh_test/analysis_tools.sh coverage`
- `identify_test_gaps.sh` → Available via `sh_test/analysis_tools.sh gaps`

### Access Legacy Tests
```bash
# Run original scripts for comparison
./sh_test/run_legacy_tests.sh comprehensive
./sh_test/run_legacy_tests.sh assertions
./sh_test/run_legacy_tests.sh screenshots
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