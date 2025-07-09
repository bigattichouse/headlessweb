# Proof of Concept: `hweb-poc`

## Goal

The primary goal of this proof-of-concept (`hweb-poc`) was to demonstrate the core functionalities required for a headless web automation tool, specifically focusing on:

*   Loading a web page using WebKit.
*   Saving and persisting a web session (cookies, current URL).
*   Re-opening a saved session.
*   Executing basic JavaScript commands to verify state changes within the session.

## Initial Plan

The initial plan involved developing a C++ command-line application using WebKitGTK+ (initially targeting version 4.0) and `jsoncpp` for session serialization. The application would support commands for navigating to URLs, executing JavaScript, and managing sessions (save, load, end).

## Development Journey & Challenges

Developing `hweb-poc` presented several significant challenges, primarily due to the evolving APIs of WebKitGTK+ and GTK, and environmental considerations for headless execution.

### 1. Initial Compilation Issues (Missing Dependencies)

**Problem:** The initial build attempts failed due to missing development libraries for WebKitGTK+ and `jsoncpp`.

**Solution:** Installed the necessary development packages:

*   `libwebkit2gtk-4.0-dev` (later upgraded to `libwebkitgtk-6.0-dev`)
*   `libjsoncpp-dev`

**Lesson Learned:** Always ensure all build-time dependencies are explicitly installed, especially for C/C++ projects with external libraries.

### 2. WebKitGTK+ 4.1 API Mismatches

**Problem:** After installing `libwebkit2gtk-4.1-dev`, compilation errors arose due to API changes between WebKitGTK+ 4.0 and 4.1, particularly concerning cookie management (`webkit_cookie_manager_get_cookies_sync`, `soup_cookie_to_string`) and JavaScript execution (`webkit_web_view_evaluate_javascript`). The `g_main_loop_quit` assertion failures also indicated issues with event loop management.

**Solution:** Iteratively adapted the code to the WebKitGTK+ 4.1 API. This involved:

*   Manually constructing cookie strings as `soup_cookie_to_string` was unavailable.
*   Correcting argument orders for `webkit_web_view_evaluate_javascript`.
*   Temporarily using `sleep` calls for page load and JS execution to bypass complex signal handling for the POC.

**Lesson Learned:** WebKitGTK+ APIs can change significantly between minor versions, requiring careful adaptation. Asynchronous operations require robust event loop management.

### 3. Runtime Environment Issues (`bwrap` and D-Bus)

**Problem:** After successful compilation, the application would hang or crash with errors like `bwrap: setting up uid map: Permission denied` and `Failed to fully launch dbus-proxy`.

**Solution:** These errors pointed to system-level sandboxing and inter-process communication issues. The solutions involved:

*   **Installing `bubblewrap` and `xdg-utils`:** These are runtime dependencies for WebKitGTK+'s sandboxing.
    ```bash
    sudo apt-get update && sudo apt-get install -y bubblewrap xdg-utils
    ```
*   **Enabling Unprivileged User Namespaces:** This is crucial for `bwrap` to function. If `sysctl kernel.unprivileged_userns_clone` returns `0`, it needs to be enabled:
    ```bash
    # Add to /etc/sysctl.d/10-userns.conf (or create it)
    kernel.unprivileged_userns_clone=1
    sudo sysctl -p /etc/sysctl.d/10-userns.conf
    # Reboot your system
    ```
    *(Caution: Enabling unprivileged user namespaces has security implications. Research thoroughly before enabling on production systems.)*
*   **AppArmor Configuration:** If the above didn't suffice, AppArmor might be restricting `bwrap`:
    ```bash
    sudo apt install apparmor-profiles # (if not sufficient sudo apt install apparmor-profiles-extra)
    sudo ln -s /usr/share/apparmor/extra-profiles/bwrap-userns-restrict /etc/apparmor.d/bwrap
    sudo apparmor_parser /etc/apparmor.d/bwrap
    ```

**Lesson Learned:** Headless browser automation often involves complex interactions with the operating system's security and sandboxing features. Runtime environment setup is as critical as code compilation.

### 4. Upgrading to WebKitGTK+ 6.0 (GTK 4) and Event Loop Refinement

**Problem:** The deprecation warning for `webkit_web_view_run_javascript` prompted an attempt to upgrade to `libwebkitgtk-6.0-dev`. This introduced a new set of breaking changes, not just in WebKitGTK+ but also in GTK (from GTK 3 to GTK 4). Initial attempts to use `webkit_web_view_evaluate_javascript` and manage `GMainLoop`s led to persistent `g_main_loop_quit` errors and hangs.

**Solution:** A significant refactoring of the `Browser` class and event loop management was undertaken:

*   **GTK 4 API Adaptation:** Updated `gtk_init()`, `gtk_window_new()`, and `gtk_window_set_child()` to their GTK 4 equivalents.
*   **Simplified `Browser` Structure:** Removed the `Browser::Private` PIMPL idiom for simplicity in the POC, making `window`, `webView`, and `operation_completed` public members.
*   **Event-Driven Completion:** Replaced fixed `sleep` times and complex `GMainLoop` management with a robust event-driven approach:
    *   `operation_completed` flag in `Browser` to signal when an asynchronous operation is done.
    *   `g_object_set_data` to pass the `Browser` instance to callbacks.
    *   `g_signal_connect` to `load-changed` (for page loading) and `webkit_web_view_evaluate_javascript`'s callback (for JS execution) to set `operation_completed`.
    *   A `wait_for_completion` helper function in `main.cpp` that uses `g_main_context_iteration` to process events until `operation_completed` is true or a timeout is reached.

**Lesson Learned:** Upgrading major library versions (especially UI toolkits) can involve extensive API changes. Event-driven programming with proper signal handling is essential for responsive and non-blocking asynchronous operations. Debugging `GMainLoop` issues requires careful attention to event processing and callback execution.

## Final Working State & Limitations

The `hweb-poc` application successfully demonstrates:

*   Loading web pages using WebKitGTK+ 6.0 (GTK 4).
*   Saving and loading session state (specifically the URL).
*   Executing JavaScript commands and retrieving their results.
*   Efficient, event-driven operation without arbitrary `sleep` times.

**Current Limitations:**

*   **Cookie Persistence:** The cookie management code was removed during the WebKitGTK+ 6.0 upgrade due to API complexities. Full cookie persistence is not implemented in this POC.
*   **Error Handling:** Robust error handling for WebKit operations (e.g., navigation failures, JavaScript errors) is minimal.
*   **Full Session State:** Only the URL is persisted. A complete session would include local storage, session storage, HTTP authentication credentials, etc., as outlined in the `headlessweb.blueprint`.
*   **Headless Mode:** While the window is not shown, full headless rendering (e.g., to an off-screen buffer for screenshots) is not explicitly implemented beyond the basic WebKit setup.

## Conclusion

This POC successfully validates the feasibility of building a headless web automation tool with WebKitGTK+. It highlights the importance of understanding library API evolution and robust event loop management in asynchronous programming. The journey through various compilation and runtime challenges provided valuable insights into the complexities of such a project.
