cmake_minimum_required(VERSION 3.10)
project(DebugResourceManagement)

set(CMAKE_CXX_STANDARD 17)

# Find GTest
find_package(GTest REQUIRED)

# Find PkgConfig
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(WEBKIT REQUIRED webkitgtk-6.0)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(GDKPIXBUF REQUIRED gdk-pixbuf-2.0)

include_directories(${GTK4_INCLUDE_DIRS})
include_directories(${WEBKIT_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${GDKPIXBUF_INCLUDE_DIRS})
include_directories(../src)

# Debug executables for quick testing
add_executable(debug_resource_management debug_resource_management.cpp)
add_executable(debug_storage debug_storage_test.cpp)
add_executable(debug_events debug_events_test.cpp)
add_executable(debug_fileops debug_fileops_test.cpp)
add_executable(debug_minimal debug_minimal_browser.cpp)

# Common link libraries
set(COMMON_LIBS
    ${GTK4_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${GDKPIXBUF_LIBRARIES}
)

target_link_libraries(debug_resource_management GTest::GTest GTest::Main ${COMMON_LIBS})
target_link_libraries(debug_storage ${COMMON_LIBS})
target_link_libraries(debug_events ${COMMON_LIBS})
target_link_libraries(debug_fileops ${COMMON_LIBS})
target_link_libraries(debug_minimal ${COMMON_LIBS})

# Common include directories
set(COMMON_INCLUDES
    ../src/Browser
    ../src/Session
    ../src/FileOps
    ../src/hweb/Services
    ../tests/utils
)

target_include_directories(debug_resource_management PRIVATE ${COMMON_INCLUDES})
target_include_directories(debug_storage PRIVATE ${COMMON_INCLUDES})
target_include_directories(debug_events PRIVATE ${COMMON_INCLUDES})
target_include_directories(debug_fileops PRIVATE ${COMMON_INCLUDES})
target_include_directories(debug_minimal PRIVATE ${COMMON_INCLUDES})

# Common source files needed by most debug tests
set(BROWSER_SOURCES
    ../src/Browser/Browser.cpp
    ../src/Browser/Core.cpp
    ../src/Browser/DOM.cpp
    ../src/Browser/Events.cpp
    ../src/Browser/EventLoopManager.cpp
    ../src/Browser/JavaScript.cpp
    ../src/Browser/Screenshot.cpp
    ../src/Browser/Session.cpp
    ../src/Browser/Storage.cpp
    ../src/Browser/Utilities.cpp
    ../src/Browser/Wait.cpp
    ../src/Session/Manager.cpp
    ../src/Session/Session.cpp
    ../tests/utils/test_helpers.cpp
)

set(FILEOPS_SOURCES
    ../src/FileOps/PathUtils.cpp
    ../src/FileOps/UploadManager.cpp
    ../src/FileOps/DownloadManager.cpp
    ../src/FileOps/Types.cpp
)

set(SERVICE_SOURCES
    ../src/hweb/Services/ManagerRegistry.cpp
    ../src/hweb/Services/SessionService.cpp
    ../src/hweb/Services/NavigationService.cpp
    ../src/Assertion/Manager.cpp
    ../src/Assertion/OutputFormatter.cpp
    ../src/hweb/Output.cpp
)

# Add sources to each executable
target_sources(debug_resource_management PRIVATE ${BROWSER_SOURCES} ${SERVICE_SOURCES} ${FILEOPS_SOURCES})
target_sources(debug_storage PRIVATE ${BROWSER_SOURCES} ${FILEOPS_SOURCES})
target_sources(debug_events PRIVATE ${BROWSER_SOURCES} ${FILEOPS_SOURCES})  
target_sources(debug_fileops PRIVATE ${FILEOPS_SOURCES})
target_sources(debug_minimal PRIVATE ${BROWSER_SOURCES} ${FILEOPS_SOURCES})