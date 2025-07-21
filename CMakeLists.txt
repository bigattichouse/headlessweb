cmake_minimum_required(VERSION 3.10)
project(hweb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(WEBKIT REQUIRED webkitgtk-6.0)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(GDKPIXBUF REQUIRED gdk-pixbuf-2.0)

include_directories(${WEBKIT_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${GDKPIXBUF_INCLUDE_DIRS})

add_executable(hweb
    src/hweb.cpp
    src/Session/Session.cpp
    src/Session/Manager.cpp
)

add_subdirectory(src/Browser)
add_subdirectory(src/Assertion)
add_subdirectory(src/FileOps)
add_subdirectory(src/hweb)

# Add tests subdirectory (optional, only if building tests)
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

target_link_libraries(hweb 
    hweb_core
    ${GTK4_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${GDKPIXBUF_LIBRARIES}
)

target_include_directories(hweb PRIVATE 
    ${GTK4_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${GDKPIXBUF_INCLUDE_DIRS}
)
