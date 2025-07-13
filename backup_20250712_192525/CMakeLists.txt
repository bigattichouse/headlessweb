# Example CMakeLists.txt update for the refactored Browser class

# Original Browser sources (remove Browser.cpp from here if it was listed)
set(BROWSER_SOURCES
    # Core browser files
    Browser.cpp
    BrowserCore.cpp
    BrowserJavaScript.cpp
    BrowserEvents.cpp
    BrowserDOM.cpp
    BrowserStorage.cpp
    BrowserSession.cpp
    BrowserScreenshot.cpp
    BrowserUtilities.cpp
    
    # Other existing files
    Session.cpp
    Debug.cpp
    # ... other source files ...
)

# If you prefer to organize in a subdirectory
# set(BROWSER_SOURCES
#     browser/Browser.cpp
#     browser/BrowserCore.cpp
#     browser/BrowserJavaScript.cpp
#     browser/BrowserEvents.cpp
#     browser/BrowserDOM.cpp
#     browser/BrowserStorage.cpp
#     browser/BrowserSession.cpp
#     browser/BrowserScreenshot.cpp
#     browser/BrowserUtilities.cpp
# )

# Add to your executable or library
add_executable(your_app 
    main.cpp
    ${BROWSER_SOURCES}
    # ... other sources ...
)

# Or if building as a library
# add_library(browser_lib STATIC ${BROWSER_SOURCES})
# target_include_directories(browser_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Dependencies remain the same
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk4)
pkg_check_modules(WEBKIT REQUIRED webkitgtk-6.0)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

target_include_directories(your_app PRIVATE 
    ${GTK_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
)

target_link_libraries(your_app 
    ${GTK_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    ${JSONCPP_LIBRARIES}
)

# Compiler flags
target_compile_options(your_app PRIVATE 
    ${GTK_CFLAGS_OTHER}
    ${WEBKIT_CFLAGS_OTHER}
)
