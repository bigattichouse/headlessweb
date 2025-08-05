set(FILEOPS_SOURCES
    AsyncFileOperations.cpp
    DownloadManager.cpp
    PathUtils.cpp
    Types.cpp
    UploadManager.cpp
)

set(FILEOPS_HEADERS
    AsyncFileOperations.h
    DownloadManager.h
    PathUtils.h
    Types.h
    UploadManager.h
)

set(FILEOPS_MODULE_SOURCES "")
foreach(source ${FILEOPS_SOURCES})
    list(APPEND FILEOPS_MODULE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()

set(FILEOPS_MODULE_HEADERS "")
foreach(header ${FILEOPS_HEADERS})
    list(APPEND FILEOPS_MODULE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
endforeach()

target_sources(hweb PRIVATE ${FILEOPS_MODULE_SOURCES})
