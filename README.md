# HeadlessWeb

HeadlessWeb is a command-line tool for automating and interacting with web pages without a graphical user interface. It allows you to write scripts that can navigate websites, interact with forms, execute JavaScript, and persist sessions (including cookies, local storage, and form state) for later use.

This tool is ideal for:

*   **Web Scraping:** Extracting data from websites.
*   **Automated Testing:** Running tests against your web application in a CI/CD environment.
*   **Task Automation:** Automating repetitive web-based tasks.

## Features

*   **Session Management:** Start, stop, and list sessions. Session data is saved to disk and can be restored later.
*   **Navigation:** Navigate to URLs, go back/forward, and reload pages.
*   **Interaction:**
    *   Type into input fields.
    *   Click buttons and other elements.
    *   Select options from dropdowns.
    *   Check/uncheck checkboxes.
*   **JavaScript Execution:** Execute arbitrary JavaScript on the page and get the result.
*   **State Persistence:** Sessions save and restore:
    *   Cookies
    *   `localStorage`
    *   Form field values
    *   Scroll position
*   **Screenshots:** Take screenshots of the visible part of the page or the full page.
*   **Command Chaining:** Chain multiple commands together in a single execution for efficient scripting.

## Installation

This project is built with C++ and uses CMake. You will need to install the following dependencies to build the `hweb-poc` executable.

### Dependencies

You will need a C++ compiler and the following development libraries:

*   **GTK4:** A multi-platform toolkit for creating graphical user interfaces.
*   **WebKitGTK (6.0):** The port of the WebKit rendering engine to the GTK platform.
*   **jsoncpp:** A C++ library for interacting with JSON.
*   **Cairo:** A 2D graphics library.
*   **gdk-pixbuf:** A library for image loading and manipulation.

On a Debian-based system (like Ubuntu), you can install these with:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libgtk-4-dev libwebkitgtk-6.0-dev libjsoncpp-dev libcairo2-dev libgdk-pixbuf2.0-dev
```

### Building

Once the dependencies are installed, you can build the project using CMake:

```bash
mkdir build
cd build
cmake ..
make
```

This will create the `hweb-poc` executable in the `build` directory. For ease of use, you can copy it to a location in your `PATH`, or run it from the `build` directory. The rest of this README will assume the executable is in the current directory and aliased as `./hweb-poc`.

## Usage

The `hweb-poc` tool is controlled via command-line arguments. The core concept is the **session**, which represents a single browsing instance.

### Basic Commands

*   `--session <name>`: Start a new session or resume an existing one.
*   `--url <url>`: Navigate to a URL.
*   `--end`: End the current session and save its state.
*   `--list`: List all available sessions.
*   `--debug`: Enable verbose debug output.

### Example: Starting a session and navigating

```bash
# Start a new session named 'my_session' and go to example.com
./hweb-poc --session my_session --url https://example.com

# The session is now active. You can run more commands against it.
# For example, get the page title:
./hweb-poc --session my_session --js "document.title"

# End the session when you're done
./hweb-poc --session my_session --end
```

### Interacting with Pages

You can interact with elements on the page using CSS selectors.

*   `--type <selector> <text>`: Type text into an element.
*   `--click <selector>`: Click an element.
*   `--select <selector> <value>`: Select an option in a `<select>` element.
*   `--check <selector>`: Check a checkbox.
*   `--uncheck <selector>`: Uncheck a checkbox.
*   `--wait <selector>`: Wait for an element to exist before proceeding.

### Querying the Page

*   `--text <selector>`: Get the text content of an element.
*   `--html <selector>`: Get the HTML content of an element.
*   `--attr <selector> <attribute>`: Get the value of an attribute on an element.
*   `--exists <selector>`: Check if an element exists.
*   `--count <selector>`: Count how many elements match a selector.
*   `--js <javascript>`: Execute JavaScript and get the return value.

### Command Chaining

For more complex scripts, you can chain multiple commands together in a single line. This is more efficient as it only loads the session once.

```bash
# In a single command: navigate, fill out a form, and click submit
./hweb-poc --session search_session \
    --url https://duckduckgo.com \
    --wait "#search_form_input_homepage" \
    --type "#search_form_input_homepage" "headless web automation" \
    --click "#search_button_homepage"
```

### Screenshots

*   `--screenshot [filename]`: Take a screenshot of the visible area. Defaults to `screenshot.png`.
*   `--screenshot-full [filename]`: Take a screenshot of the entire page.

### Comprehensive Test

To see a wide variety of commands in action, you can run the comprehensive test script. This script will create a local HTML file and run a suite of tests against it, demonstrating most of the tool's features.

```bash
./comprehensive-test.sh
```