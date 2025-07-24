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

add_executable(debug_resource_management debug_resource_management.cpp)

target_link_libraries(debug_resource_management
    GTest::GTest
    GTest::Main
    ${GTK4_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${GDKPIXBUF_LIBRARIES}
)

target_include_directories(debug_resource_management PRIVATE 
    ../src/Browser
    ../src/Session
    ../src/hweb/Services
    ../tests/utils
)

# Add source files from the project
target_sources(debug_resource_management PRIVATE
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
    ../src/Session/Manager.cpp
    ../src/Session/Session.cpp
    ../src/hweb/Services/ManagerRegistry.cpp
    ../src/hweb/Services/SessionService.cpp
    ../src/hweb/Services/NavigationService.cpp
    ../src/FileOps/PathUtils.cpp
    ../src/Assertion/Manager.cpp
    ../src/Assertion/OutputFormatter.cpp
    ../src/hweb/Output.cpp
    ../tests/utils/test_helpers.cpp
)