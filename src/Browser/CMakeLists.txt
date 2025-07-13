# Browser module CMakeLists.txt

set(BROWSER_SOURCES
    Browser.cpp
    BrowserCore.cpp
    BrowserJavaScript.cpp
    BrowserEvents.cpp
    BrowserDOM.cpp
    BrowserStorage.cpp
    BrowserSession.cpp
    BrowserScreenshot.cpp
    BrowserUtilities.cpp
)

# Create list with full paths
set(BROWSER_MODULE_SOURCES "")
foreach(source ${BROWSER_SOURCES})
    list(APPEND BROWSER_MODULE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()

# Export to parent scope
set(BROWSER_MODULE_SOURCES ${BROWSER_MODULE_SOURCES} PARENT_SCOPE)

# Optional: Create a static library
# add_library(browser_module STATIC ${BROWSER_SOURCES})
# target_include_directories(browser_module PUBLIC 
#     ${CMAKE_CURRENT_SOURCE_DIR}/..
#     ${GTK_INCLUDE_DIRS}
#     ${WEBKIT_INCLUDE_DIRS}
# )
