set(ASSERTION_SOURCES
    Manager.cpp
    OutputFormatter.cpp
)

set(ASSERTION_HEADERS
    Manager.h
    OutputFormatter.h
    Types.h
)

set(ASSERTION_MODULE_SOURCES "")
foreach(source ${ASSERTION_SOURCES})
    list(APPEND ASSERTION_MODULE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()

set(ASSERTION_MODULE_HEADERS "")
foreach(header ${ASSERTION_HEADERS})
    list(APPEND ASSERTION_MODULE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
endforeach()

target_sources(hweb PRIVATE ${ASSERTION_MODULE_SOURCES})
