# HeadlessWeb CLI Refactoring Blueprint

## Current State Analysis

The `src/hweb.cpp` file has grown to **970 lines** and contains multiple responsibilities that should be separated into focused components. Currently, it handles:

- Command-line argument parsing (192-543)
- Application initialization and global managers
- Command execution logic (648-924) 
- File operation handling
- Advanced waiting operations
- Browser session management
- Error handling and output formatting

## Proposed File Structure

### Target: Each file should be 100-250 lines maximum

### 1. Core Application Layer (`src/hweb/`)

#### `main.cpp` (~150 lines)
```cpp
// Main entry point and application lifecycle
int main(int argc, char* argv[])
void initialize_application()
void cleanup_application()
int run_application(const HWebConfig& config)
```

#### `Config.h/cpp` (~200 lines)
```cpp
struct HWebConfig {
    std::string sessionName;
    std::string url;
    bool endSession;
    bool listSessions;
    bool json_mode;
    bool silent_mode;
    int browser_width;
    std::vector<Command> commands;
    std::vector<Assertion::Command> assertions;
    FileOperationSettings file_settings;
}

class ConfigParser {
    HWebConfig parseArguments(const std::vector<std::string>& args);
    void print_usage();
    void validate_config(const HWebConfig& config);
}
```

#### `Output.h/cpp` (~120 lines)
```cpp
class Output {
    static void info(const std::string& message);
    static void error(const std::string& message);
    static void set_json_mode(bool enabled);
    static void set_silent_mode(bool enabled);
    static void format_error(const std::string& context, const std::string& error);
}
```

#### `Types.h` (~80 lines)
```cpp
struct Command {
    std::string type;
    std::string selector;
    std::string value;
    int timeout = 10000;
};

struct FileOperationSettings {
    size_t max_file_size = 104857600; // 100MB
    std::vector<std::string> allowed_types = {"*"};
    std::string download_dir = "";
    int upload_timeout = 30000;
    int download_timeout = 30000;
};

enum class NavigationStrategy {
    NEW_URL,
    SESSION_RESTORE,
    CONTINUE_SESSION,
    NO_NAVIGATION
};
```

### 2. Command Processing Layer (`src/hweb/Commands/`)

#### `Executor.h/cpp` (~200 lines)
```cpp
class CommandExecutor {
    int execute_commands(Browser& browser, Session& session, 
                        const std::vector<Command>& commands);
    bool execute_single_command(Browser& browser, Session& session, 
                               const Command& cmd);
    void handle_navigation_update(Browser& browser, Session& session, 
                                bool navigation_expected);
}
```

#### `Parser.h/cpp` (~180 lines)
```cpp
class CommandParser {
    std::vector<Command> parse_commands(const std::vector<std::string>& args, size_t& index);
    Command parse_single_command(const std::string& type, const std::vector<std::string>& args, size_t& index);
    void validate_command(const Command& cmd);
}
```

#### `AssertionParser.h/cpp` (~150 lines)
```cpp
class AssertionParser {
    std::vector<Assertion::Command> parse_assertions(const std::vector<std::string>& args);
    Assertion::Command parse_assertion_command(const std::string& type, const std::vector<std::string>& args, size_t& index);
    void apply_assertion_modifiers(Assertion::Command& assertion, const std::vector<std::string>& args, size_t& index);
}
```

### 3. Specialized Command Handlers (`src/hweb/Handlers/`)

#### `FileOperations.h/cpp` (~220 lines)
```cpp
class FileOperationHandler {
    int handle_upload_command(Browser& browser, const Command& cmd);
    int handle_upload_multiple_command(Browser& browser, const Command& cmd);
    int handle_download_wait_command(const Command& cmd);
    int handle_download_wait_multiple_command(const Command& cmd);
    
    // Configuration
    void configure_upload_manager(const FileOperationSettings& settings);
    void configure_download_manager(const FileOperationSettings& settings);
}
```

#### `AdvancedWait.h/cpp` (~250 lines)
```cpp
class AdvancedWaitHandler {
    int handle_wait_text_advanced(Browser& browser, const Command& cmd);
    int handle_wait_network_idle(Browser& browser, const Command& cmd);
    int handle_wait_network_request(Browser& browser, const Command& cmd);
    int handle_wait_element_visible(Browser& browser, const Command& cmd);
    int handle_wait_element_count(Browser& browser, const Command& cmd);
    int handle_wait_attribute(Browser& browser, const Command& cmd);
    int handle_wait_url_change(Browser& browser, const Command& cmd);
    int handle_wait_title_change(Browser& browser, const Command& cmd);
    int handle_wait_spa_navigation(Browser& browser, const Command& cmd);
    int handle_wait_framework_ready(Browser& browser, const Command& cmd);
    int handle_wait_dom_change(Browser& browser, const Command& cmd);
    int handle_wait_content_change(Browser& browser, const Command& cmd);
}
```

#### `BasicCommands.h/cpp` (~200 lines)
```cpp
class BasicCommandHandler {
    int handle_navigation_command(Browser& browser, Session& session, const Command& cmd);
    int handle_interaction_command(Browser& browser, Session& session, const Command& cmd);
    int handle_data_extraction_command(Browser& browser, const Command& cmd);
    int handle_session_command(Session& session, const Command& cmd);
}
```

### 4. Application Services (`src/hweb/Services/`)

#### `SessionService.h/cpp` (~180 lines)
```cpp
class SessionService {
    Session initialize_session(const std::string& sessionName, SessionManager& manager);
    bool handle_session_restore(Browser& browser, Session& session, bool is_restore);
    void update_session_state(Browser& browser, Session& session);
    void save_session_safely(SessionManager& manager, const Session& session);
    void handle_history_navigation(Browser& browser, Session& session, const std::string& direction);
}
```

#### `NavigationService.h/cpp` (~150 lines)
```cpp
class NavigationService {
    bool navigate_to_url(Browser& browser, const std::string& url);
    bool wait_for_navigation_complete(Browser& browser, int timeout_ms);
    bool wait_for_page_ready(Browser& browser, int timeout_ms);
    NavigationStrategy determine_navigation_strategy(const HWebConfig& config, const Session& session);
}
```

#### `ManagerRegistry.h/cpp` (~100 lines)
```cpp
class ManagerRegistry {
    // Singleton pattern for global managers
    static Assertion::Manager& get_assertion_manager();
    static FileOps::UploadManager& get_upload_manager();
    static FileOps::DownloadManager& get_download_manager();
    
    static void initialize_managers();
    static void cleanup_managers();
}
```

## Migration Strategy

### Phase 1: Create Directory Structure and Core Infrastructure
1. Create `src/hweb/` directory structure
2. Create `Output` class and move output functions  
3. Create `ManagerRegistry` and centralize global managers
4. Create `Types.h` with shared structures

### Phase 2: Extract Argument Parsing
1. Create `ConfigParser` class in `src/hweb/Config.cpp`
2. Move all argument parsing logic (lines 192-543)
3. Create `HWebConfig` structure to hold parsed configuration

### Phase 3: Extract Command Processing
1. Create `CommandExecutor` in `src/hweb/Commands/Executor.cpp`
2. Create specialized handlers in `src/hweb/Handlers/`:
   - `FileOperations.cpp` (lines 682-760)
   - `AdvancedWait.cpp` (lines 762-879)
   - `BasicCommands.cpp` (remaining commands)

### Phase 4: Extract Services
1. Create `NavigationService` in `src/hweb/Services/`
2. Create `SessionService` in `src/hweb/Services/`
3. Refactor `src/hweb/main.cpp` to coordinate services

### Phase 5: Final Integration and Testing
1. Update `CMakeLists.txt` to include new `src/hweb/` directory
2. Update main `hweb.cpp` to simply call `src/hweb/main.cpp` 
3. Run comprehensive tests to ensure functionality is preserved
4. Update documentation and examples

## Benefits of This Architecture

### Maintainability
- Each file has a single, clear responsibility
- Functions are focused and testable
- Easy to locate and modify specific functionality

### Testability
- Command handlers can be unit tested independently
- Services can be mocked for testing
- Clear interfaces between components

### Extensibility
- New command types can be added by creating new handlers
- New services can be plugged in easily
- Configuration is centralized and type-safe

### Code Quality
- Eliminates the 970-line monolith
- Reduces cognitive load for developers
- Follows SOLID principles

## Implementation Notes

1. **Backward Compatibility**: All existing CLI arguments and behavior must be preserved
2. **Error Handling**: Each component should have consistent error handling patterns
3. **Memory Management**: Maintain existing RAII patterns and smart pointer usage
4. **Performance**: Refactoring should not impact CLI startup time or execution performance
5. **Dependencies**: Minimize cross-dependencies between new components

## File Size Targets

### Core Components (`src/hweb/`):
- `main.cpp`: ~150 lines
- `Config.cpp`: ~200 lines  
- `Output.cpp`: ~120 lines
- `Types.h`: ~80 lines

### Command Processing (`src/hweb/Commands/`):
- `Executor.cpp`: ~200 lines
- `Parser.cpp`: ~180 lines
- `AssertionParser.cpp`: ~150 lines

### Specialized Handlers (`src/hweb/Handlers/`):
- `FileOperations.cpp`: ~220 lines
- `AdvancedWait.cpp`: ~250 lines
- `BasicCommands.cpp`: ~200 lines

### Services (`src/hweb/Services/`):
- `NavigationService.cpp`: ~150 lines
- `SessionService.cpp`: ~180 lines
- `ManagerRegistry.cpp`: ~100 lines

**Total**: ~2,180 lines (distributed across 13 focused files vs 970 lines in one monolithic file)

### Directory Structure:
```
src/hweb/
├── main.cpp                 # Entry point (~150 lines)
├── Config.h/cpp             # Configuration parsing (~200 lines)  
├── Output.h/cpp             # Output management (~120 lines)
├── Types.h                  # Shared types (~80 lines)
├── Commands/
│   ├── Executor.h/cpp       # Command execution (~200 lines)
│   ├── Parser.h/cpp         # Command parsing (~180 lines)
│   └── AssertionParser.h/cpp # Assertion parsing (~150 lines)
├── Handlers/
│   ├── FileOperations.h/cpp  # File operations (~220 lines)
│   ├── AdvancedWait.h/cpp    # Advanced waiting (~250 lines)
│   └── BasicCommands.h/cpp   # Basic commands (~200 lines)
└── Services/
    ├── NavigationService.h/cpp # Navigation logic (~150 lines)
    ├── SessionService.h/cpp    # Session management (~180 lines)
    └── ManagerRegistry.h/cpp   # Manager access (~100 lines)
```

This architecture transforms a monolithic 970-line file into a well-structured, maintainable codebase organized in the `src/hweb/` directory with clear separation of concerns and individual components under 250 lines each. The new structure keeps all CLI-specific code separate from the existing Browser, Session, FileOps, and Assertion components while maintaining full backward compatibility.