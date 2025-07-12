cmake_minimum_required(VERSION 3.10)
project(hweb-poc)

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

add_executable(hweb-poc
    src/main.cpp
    src/Session.cpp
    src/SessionManager.cpp
    src/Browser.cpp
)

# In the target_link_libraries section, add:
target_link_libraries(hweb-poc 
    ${GTK4_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${GDKPIXBUF_LIBRARIES}
)

# In the target_include_directories section, add:
target_include_directories(hweb-poc PRIVATE 
    ${GTK4_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${GDKPIXBUF_INCLUDE_DIRS}
)
