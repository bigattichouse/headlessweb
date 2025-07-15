set(BROWSER_SOURCES
    Browser.cpp
    Core.cpp
    JavaScript.cpp
    Events.cpp
    DOM.cpp
    Storage.cpp
    Session.cpp
    Screenshot.cpp
    Utilities.cpp
    Wait.cpp
)

set(BROWSER_HEADERS
    Browser.h
    WebKitCompat.h
)

set(BROWSER_MODULE_SOURCES "")
foreach(source ${BROWSER_SOURCES})
    list(APPEND BROWSER_MODULE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()

set(BROWSER_MODULE_HEADERS "")
foreach(header ${BROWSER_HEADERS})
    list(APPEND BROWSER_MODULE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
endforeach()

target_sources(hweb PRIVATE ${BROWSER_MODULE_SOURCES})