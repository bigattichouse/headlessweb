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
    src/main.cpp
    src/Session/Session.cpp
    src/Session/Manager.cpp
    src/Browser/Browser.cpp 
    src/Browser/Core.cpp
    src/Browser/JavaScript.cpp
    src/Browser/Events.cpp
    src/Browser/DOM.cpp
    src/Browser/Storage.cpp
    src/Browser/Session.cpp
    src/Browser/Screenshot.cpp
    src/Browser/Utilities.cpp
    src/Assertion/Manager.cpp  
    src/Assertion/Manager.h  
    src/Assertion/OutputFormatter.cpp  
    src/Assertion/OutputFormatter.h  
    src/Assertion/Types.h

)

target_link_libraries(hweb 
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
