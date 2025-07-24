# HeadlessWeb Unit Tests
cmake_minimum_required(VERSION 3.10)

# Find Google Test (includes GMock)
find_package(GTest REQUIRED)
include(GoogleTest)

# Enable testing
enable_testing()

# Include directories
include_directories(../src)

# Find required packages (same as main project)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(WEBKIT REQUIRED webkitgtk-6.0)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# Test source files - focused on non-Browser dependent tests for now
set(TEST_SOURCES
    test_main.cpp
    session/test_session.cpp
    session/test_session_manager.cpp
    assertion/test_assertion_types.cpp
    assertion/test_output_formatter.cpp
    fileops/test_types.cpp
    fileops/test_path_utils.cpp
    fileops/test_upload_manager.cpp
    fileops/test_download_manager.cpp
    utils/test_helpers.cpp
    main/test_command_parsing.cpp
    hweb/test_output.cpp
    hweb/test_config_parser.cpp
    hweb/test_manager_registry.cpp
    hweb/test_navigation_service.cpp
    hweb/test_service_architecture_coordination.cpp
)

# Browser component tests (excluded due to WebKit/GTK dependencies)
# These can be enabled when WebKit integration is available
set(BROWSER_TEST_SOURCES
    browser/test_browser_core.cpp
    browser/test_browser_dom.cpp
    browser/test_browser_javascript.cpp
    browser/test_browser_events.cpp
    browser/test_browser_storage.cpp
    browser/test_browser_utilities.cpp
    browser/test_browser_wait.cpp
    browser/test_browser_main.cpp
    browser/test_browser_session.cpp
    browser/test_browser_advanced_form_operations.cpp
)

# Integration test sources
set(INTEGRATION_TEST_SOURCES
    integration/test_browser_fileops_integration.cpp
    integration/test_complex_workflow_chains.cpp
)

# Enable browser tests with WebKit integration
list(APPEND TEST_SOURCES ${BROWSER_TEST_SOURCES})

# Enable integration tests
list(APPEND TEST_SOURCES ${INTEGRATION_TEST_SOURCES})

# Create test executable
add_executable(hweb_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(hweb_tests
    hweb_core
    GTest::gtest
    GTest::gmock
    GTest::gtest_main
    ${GTK4_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    pthread
)

# Link against project source files (including Browser with WebKit)
target_sources(hweb_tests PRIVATE
    ../src/Session/Session.cpp
    ../src/Session/Manager.cpp
    ../src/Assertion/Manager.cpp
    ../src/Assertion/OutputFormatter.cpp
    ../src/FileOps/Types.cpp
    ../src/FileOps/UploadManager.cpp
    ../src/FileOps/DownloadManager.cpp
    ../src/FileOps/PathUtils.cpp
    ../src/Browser/Browser.cpp
    ../src/Browser/Core.cpp
    ../src/Browser/DOM.cpp
    ../src/Browser/Events.cpp
    ../src/Browser/JavaScript.cpp
    ../src/Browser/Screenshot.cpp
    ../src/Browser/Session.cpp
    ../src/Browser/Storage.cpp
    ../src/Browser/Utilities.cpp
    ../src/Browser/Wait.cpp
)

# Include directories for tests
target_include_directories(hweb_tests PRIVATE
    ${GTK4_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ../src
    .
)

# Compiler flags
target_compile_definitions(hweb_tests PRIVATE
    -DTESTING_MODE=1
    -DDISABLE_GTK_INIT=1
)

# Add test discovery
gtest_discover_tests(hweb_tests)

# Custom test target
add_custom_target(run_tests
    COMMAND ./hweb_tests
    DEPENDS hweb_tests
    COMMENT "Running unit tests"
)

# Coverage target (if gcov is available)
find_program(GCOV_PATH gcov)
if(GCOV_PATH)
    add_custom_target(coverage
        COMMAND ${CMAKE_BINARY_DIR}/tests/hweb_tests
        COMMAND gcov -r ${CMAKE_SOURCE_DIR}/src/*.cpp
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND genhtml coverage.info --output-directory coverage_html
        DEPENDS hweb_tests
        COMMENT "Generating test coverage report"
    )
endif()