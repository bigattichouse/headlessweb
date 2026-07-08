#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <glib.h>
#include <random>
#include <sstream>
#include <iomanip>

#include "Types.h"
#include "Config.h"
#include "Output.h"
#include "Commands/Executor.h"
#include "Services/ManagerRegistry.h"
#include "Services/NavigationService.h"
#include "Services/SessionService.h"
#include "Services/HeaderService.h"
#include "Handlers/FileOperations.h"
#include "../Browser/Browser.h"
#include "../Session/Manager.h"
#include "../Debug.h"

#if defined(__linux__)
#include <sched.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#endif

namespace HWeb {

// ── Sandbox preflight ─────────────────────────────────────────────────────────
// WebKitGTK 6.0's bubblewrap sandbox is MANDATORY (no API/env to disable it). It creates an
// unprivileged user namespace and writes /proc/self/uid_map. On AppArmor-hardened hosts
// (kernel.apparmor_restrict_unprivileged_userns=1) that write is denied and WebKit aborts the
// whole process with a cryptic "bwrap: setting up uid map: Permission denied" / dbus-proxy
// error and a SIGTRAP coredump — indistinguishable, to a caller, from a page/assertion failure.
// We probe the SAME operation in a throwaway child first, so we can fail cleanly with a distinct
// exit code and a fix hint. See docs/sandbox-and-userns.md.  Bypass: HWEB_SKIP_SANDBOX_PREFLIGHT=1.
static constexpr int EXIT_SANDBOX_UNAVAILABLE = 3;

#if defined(__linux__)
static bool userns_uidmap_works() {
    pid_t pid = fork();
    if (pid < 0) return true;                       // can't probe → don't block; let WebKit try
    if (pid == 0) {                                 // child: mimic exactly what bwrap does
        unsigned euid = (unsigned)geteuid();        // capture BEFORE unshare — after it, geteuid() returns the
                                                    // overflow uid (nobody), and an unprivileged process may only
                                                    // map its OWN parent uid, so mapping that would be rejected.
        if (unshare(CLONE_NEWUSER) != 0) _exit(1);
        int fd = open("/proc/self/setgroups", O_WRONLY);   // deny setgroups so the map write is allowed
        if (fd >= 0) { if (write(fd, "deny", 4) < 0) { /* best effort */ } close(fd); }
        char buf[64];
        int n = snprintf(buf, sizeof buf, "0 %u 1", euid);
        fd = open("/proc/self/uid_map", O_WRONLY);
        if (fd < 0) _exit(2);
        ssize_t w = write(fd, buf, (size_t)n);      // THIS is what AppArmor / the userns restriction denies
        close(fd);
        _exit(w == n ? 0 : 3);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return true;  // probe inconclusive → don't block
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
#endif

// Returns an exit code (EXIT_SANDBOX_UNAVAILABLE) if WebKit's sandbox cannot start here, else 0.
static int sandbox_preflight() {
#if defined(__linux__)
    if (std::getenv("HWEB_SKIP_SANDBOX_PREFLIGHT")) return 0;
    if (userns_uidmap_works()) return 0;
    std::fprintf(stderr,
        "HWEB: WebKit's mandatory sandbox cannot start — creating an unprivileged user namespace\n"
        "      is blocked on this host (kernel.apparmor_restrict_unprivileged_userns=1). Without the\n"
        "      fix, hweb crashes inside WebKit with a bwrap/dbus-proxy error and a coredump.\n"
        "      Fix (scoped, recommended):  sudo ./scripts/allow-userns-bwrap.sh\n"
        "      Details:                    docs/sandbox-and-userns.md\n"
        "      Bypass this check:          HWEB_SKIP_SANDBOX_PREFLIGHT=1\n");
    return EXIT_SANDBOX_UNAVAILABLE;
#else
    return 0;
#endif
}

// Forward declaration for the main entry point
int main(int argc, char* argv[]);

void initialize_application() {
    ManagerRegistry::initialize();
}

void cleanup_application() {
    ManagerRegistry::cleanup();
}

std::string generateSessionUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "session-";
    
    // Generate 8 random hex characters
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

int run_application(const HWebConfig& config) {
    // Initialize services
    const char* homeEnv = std::getenv("HOME");
    if (!homeEnv) {
        Output::error("HOME environment variable is not set");
        return 1;
    }
    std::string home = homeEnv;
    SessionManager sessionManager(home + "/.hweb/sessions");
    SessionService sessionService(sessionManager);
    NavigationService navigationService;
    CommandExecutor commandExecutor;
    
    // Configure output modes
    Output::set_json_mode(config.json_mode);
    Output::set_silent_mode(config.silent_mode);
    Output::set_verbose_mode(config.verbose_mode);
    
    // Configure managers
    auto& assertionManager = ManagerRegistry::get_assertion_manager();
    assertionManager.setSilentMode(config.silent_mode);
    assertionManager.setJsonOutput(config.json_mode);
    
    // Handle list sessions
    if (config.listSessions) {
        return sessionService.handle_session_list() ? 0 : 1;
    }
    
    // Handle help
    if (config.showHelp) {
        ConfigParser parser;
        parser.print_usage();
        return 0;
    }
    
    // Determine session name
    std::string sessionName;
    bool auto_generated_session = false;
    
    if (config.sessionName.empty()) {
        sessionName = generateSessionUUID();
        auto_generated_session = true;
    } else {
        sessionName = config.sessionName;
    }
    
    // Inform user about auto-generated session
    if (auto_generated_session && !config.silent_mode) {
        Output::info("Auto-generated session: " + sessionName);
    }
    
    // Handle end session
    if (config.endSession) {
        return sessionService.handle_session_end(sessionName) ? 0 : 1;
    }
    
    // Initialize session
    Session session = config.start_fresh ?
        sessionService.initialize_fresh_session(sessionName) :
        sessionService.initialize_session(sessionName);

    // Handle header import (before navigation)
    if (!config.importHeadersFile.empty()) {
        std::string validation = HeaderService::validateHeadersFile(config.importHeadersFile);
        if (!validation.empty()) {
            Output::error("Invalid headers file: " + validation);
            return 1;
        }
        
        int imported = HeaderService::importHeadersFromFile(session, config.importHeadersFile);
        if (imported < 0) {
            Output::error("Failed to import headers from: " + config.importHeadersFile);
            return 1;
        }
        
        if (!config.silent_mode) {
            Output::info("Imported " + std::to_string(imported) + " headers from " + config.importHeadersFile);
        }
    }

    // Check if we need browser
    if (config.url.empty() && config.commands.empty() && config.assertions.empty() && session.getCurrentUrl().empty()) {
        Output::error("No URL in session. Use --url to navigate.");
        return 1;
    }
    
    // Preflight WebKit's mandatory sandbox before we spin it up — a clean, distinct exit
    // beats an in-WebKit abort/coredump the caller can't interpret. (Only reached once we
    // actually need a browser, so --help / session-list paths are unaffected.)
    if (int rc = sandbox_preflight()) return rc;

    // Create and configure browser
    Browser browser(config);
    browser.setViewport(config.browser_width, 800);
    
    // Configure file operation handlers
    FileOperationHandler fileHandler;
    fileHandler.configure_managers(config.file_settings);
    
    // Plan and execute navigation
    auto navigationPlan = navigationService.create_navigation_plan(config, session);
    if (!navigationService.execute_navigation_plan(browser, session, navigationPlan)) {
        return 1;
    }
    
    int exit_code = 0;
    bool state_modified = false;
    
    // Execute commands
    if (!config.commands.empty()) {
        int cmd_result = commandExecutor.execute_commands(browser, session, config.commands);
        if (cmd_result != 0) {
            exit_code = cmd_result;
        }
        state_modified = true;
    }
    
    // Execute assertions
    if (!config.assertions.empty()) {
        int assertion_result = commandExecutor.execute_assertions(browser, session, config.assertions);
        if (assertion_result != 0) {
            exit_code = assertion_result;
        }
    }
    
    // Update session state if needed
    if (state_modified || navigationPlan.should_navigate) {
        sessionService.update_session_state(browser, session);
    }

    // Handle header export (after all operations)
    if (!config.exportHeadersFile.empty()) {
        bool exported = HeaderService::exportHeadersToFile(
            session,
            config.exportHeadersFile,
            config.exportHeadersFilter
        );
        
        if (!exported) {
            Output::error("Failed to export headers to: " + config.exportHeadersFile);
            exit_code = 1;
        } else if (!config.silent_mode) {
            auto stats = HeaderService::getHeadersFileStats(config.exportHeadersFile);
            Output::info("Exported " + std::to_string(stats.totalHeaders) + 
                        " headers to " + config.exportHeadersFile);
        }
    }

    // Save session
    if (!config.commands.empty() || !config.assertions.empty() || state_modified || navigationPlan.should_navigate) {
        if (!sessionService.save_session_safely(session, sessionName)) {
            exit_code = 1;
        }
    }
    
    // Ensure all GTK events are processed before exit
    for (int i = 0; i < 10; i++) {
        while (g_main_context_pending(g_main_context_default())) {
            g_main_context_iteration(g_main_context_default(), FALSE);
        }
        browser.wait(10);
    }
    
    return exit_code;
}

} // namespace HWeb

int HWeb::main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    
    HWeb::initialize_application();
    
    try {
        HWeb::ConfigParser parser;
        HWeb::HWebConfig config = parser.parseArguments(args);
        
        // Handle debug flag  
        for (const auto& arg : args) {
            if (arg == "--debug") {
                g_debug = true;
                break;
            }
        }
        
        int result = HWeb::run_application(config);
        
        HWeb::cleanup_application();
        return result;
        
    } catch (const std::exception& e) {
        HWeb::Output::error("Application error: " + std::string(e.what()));
        HWeb::cleanup_application();
        return 1;
    } catch (...) {
        HWeb::Output::error("Unknown application error occurred");
        HWeb::cleanup_application();
        return 1;
    }
}