cmake_minimum_required(VERSION 3.10)
project(hweb-poc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBKIT REQUIRED webkitgtk-6.0)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

include_directories(${WEBKIT_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})

add_executable(hweb-poc
    src/main.cpp
    src/Session.cpp
    src/SessionManager.cpp
    src/Browser.cpp
)

target_link_libraries(hweb-poc
    ${WEBKIT_LIBRARIES}
    ${JSONCPP_LIBRARIES}
)
