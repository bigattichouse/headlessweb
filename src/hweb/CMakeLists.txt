cmake_minimum_required(VERSION 3.10)

# Create hweb_core library from the refactored components
add_library(hweb_core
    # Core components
    Types.h
    Output.cpp
    Config.cpp
    main.cpp
    
    # Command processing
    Commands/Executor.cpp
    
    # Specialized handlers
    Handlers/FileOperations.cpp
    Handlers/AdvancedWait.cpp
    Handlers/BasicCommands.cpp
    
    # Services
    Services/ManagerRegistry.cpp
    Services/NavigationService.cpp
    Services/SessionService.cpp
)

# Link required libraries to hweb_core
target_link_libraries(hweb_core
    ${GTK4_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${GDKPIXBUF_LIBRARIES}
)

# Link against project component libraries
target_sources(hweb_core PRIVATE
    ../Assertion/Manager.cpp
    ../FileOps/UploadManager.cpp
    ../FileOps/DownloadManager.cpp
    ../FileOps/Types.cpp
    ../FileOps/PathUtils.cpp
    ../Session/Session.cpp
    ../Session/Manager.cpp
)

target_include_directories(hweb_core PRIVATE 
    ${GTK4_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${GDKPIXBUF_INCLUDE_DIRS}
)