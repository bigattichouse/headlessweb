# HeadlessWeb 🌐

**Automate any website from the command line.** HeadlessWeb lets you control web pages like a human would - click buttons, fill forms, take screenshots - but from scripts and terminal commands.

## Why HeadlessWeb?

Ever needed to:
- **Check if your website is working** after a deployment?
- **Fill out the same web form** dozens of times?
- **Download files** from a site that requires logging in?
- **Monitor a website** for changes or new content?
- **Test your web app** automatically in your CI/CD pipeline?
- **Take screenshots** of how your site looks on different pages?

HeadlessWeb makes all of this simple with human-readable commands that work exactly like using a real browser.

## Quick Start

### 1. Check if a website is up
```bash
# Navigate to a site and check the title
./hweb --session test --url https://example.com --js "document.title"
# Output: "Example Domain"
```

### 2. Fill out a search form
```bash
# Search for something on DuckDuckGo
./hweb --session search \
  --url https://duckduckgo.com \
  --type "#search_form_input_homepage" "your search term" \
  --click "#search_button_homepage"
```

### 3. Take a screenshot
```bash
# Capture what a webpage looks like
./hweb --session screenshot \
  --url https://your-website.com \
  --screenshot "homepage.png"
```

### 4. Login and save session
```bash
# Login once, then reuse the session (with cookies) later
./hweb --session mysite \
  --url https://example.com/login \
  --type "#username" "your-username" \
  --type "#password" "your-password" \
  --click "#login-button"

# Later, use the same session (already logged in!)
./hweb --session mysite --url https://example.com/dashboard
```

## Common Tasks

### 🔐 **Automated Login & Data Extraction**
```bash
# Login to a site and extract some data
./hweb --session worksite \
  --url https://internal-tool.company.com \
  --type "#email" "you@company.com" \
  --type "#password" "your-password" \
  --click "#login" \
  --wait ".dashboard" \
  --text ".stats-number"
```

### 📊 **Website Monitoring**
```bash
# Check if a specific element exists (useful for monitoring)
./hweb --session monitor \
  --url https://your-app.com/status \
  --exists ".error-message"
# Returns: true/false
```

### 🧪 **Automated Testing**
```bash
# Test a user workflow end-to-end
./hweb --session test \
  --url https://your-app.com \
  --click "#sign-up-button" \
  --type "#email" "test@example.com" \
  --type "#password" "testpass123" \
  --click "#submit" \
  --wait ".welcome-message" \
  --text ".welcome-message"
```

### 📸 **Visual Documentation**
```bash
# Take screenshots of different pages for documentation
./hweb --session docs \
  --url https://your-app.com \
  --screenshot "landing-page.png" \
  --url https://your-app.com/features \
  --screenshot "features-page.png" \
  --url https://your-app.com/pricing \
  --screenshot-full "pricing-full.png"
```

### 🔄 **Batch Processing**
```bash
# Process a list of URLs (great for testing or monitoring)
for url in $(cat urls.txt); do
  ./hweb --session batch \
    --url "$url" \
    --screenshot "$(basename $url).png" \
    --js "document.title" >> results.txt
done
```

## Key Features

### 💾 **Smart Session Management**
Sessions automatically save everything:
- **Cookies** (stay logged in)
- **Form data** (remember what you typed)
- **Local storage** (app preferences)
- **Scroll position** (pick up where you left off)

```bash
# Start working on something
./hweb --session work --url https://app.com --type "#search" "important stuff"

# Come back later - everything is exactly as you left it
./hweb --session work --text ".results"
```

### ⚡ **Command Chaining**
Chain multiple actions in one command for speed:

```bash
# Do everything in one go
./hweb --session order \
  --url https://store.com \
  --click ".product" \
  --select "#size" "Large" \
  --select "#color" "Blue" \
  --type "#quantity" "2" \
  --click "#add-to-cart" \
  --wait ".cart-success"
```

### 🎯 **Precise Element Selection**
Use any CSS selector to target exactly what you want:

```bash
# By ID, class, attribute, or complex selectors
--click "#submit-button"
--type ".search-input"
--select "[name='country']" "USA"
--click "button:contains('Next')"
--wait ".loading:not(.hidden)"
```

### 🔍 **Rich Data Extraction**
Get text, HTML, attributes, or run custom JavaScript:

```bash
# Get different types of data
./hweb --session data \
  --url https://news-site.com \
  --text "h1" \                          # Get headline text
  --attr "img" "src" \                   # Get image URL
  --count ".article" \                   # Count articles
  --html ".summary" \                    # Get full HTML
  --js "window.location.href"            # Run custom JavaScript
```

## Advanced Usage

### Custom Attributes
Set and retrieve custom data attributes:
```bash
# Set custom attributes for tracking
./hweb --session app --attr "#element" "data-processed" "true"

# Check custom attributes later
./hweb --session app --attr "#element" "data-processed"
```

### Form Automation
Handle complex forms easily:
```bash
./hweb --session form \
  --url https://complex-form.com \
  --type "#first-name" "John" \
  --type "#last-name" "Doe" \
  --select "#country" "United States" \
  --check "#newsletter" \
  --uncheck "#promotional-emails" \
  --click "#submit"
```

### Error Handling
Check if operations succeeded:
```bash
# Check if login was successful
./hweb --session login \
  --url https://app.com/login \
  --type "#username" "user" \
  --type "#password" "pass" \
  --click "#login"

# Verify we're logged in
if ./hweb --session login --exists ".user-menu"; then
  echo "Login successful!"
else
  echo "Login failed!"
fi
```

## Installation

### Quick Install (Ubuntu/Debian)
```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config \
  libgtk-4-dev libwebkitgtk-6.0-dev libjsoncpp-dev \
  libcairo2-dev libgdk-pixbuf2.0-dev

# Build HeadlessWeb
git clone <repository-url>
cd headlessweb
make clean && make

# Ready to use!
./hweb --help
```

### Other Systems
- **Dependencies:** GTK4, WebKitGTK 6.0, jsoncpp, Cairo, gdk-pixbuf
- **Build:** Standard CMake build process
- **Requirements:** Modern C++ compiler with C++17 support

## Session Management

### Working with Sessions
```bash
# List all your sessions
./hweb --list

# Resume any session
./hweb --session old-session

# Clean up when done
./hweb --session old-session --end
```

### Session Persistence
Sessions are automatically saved and include:
- Current page URL
- All cookies
- LocalStorage data
- Form field values
- Custom attributes
- Scroll positions

This means you can close your terminal, restart your computer, and pick up exactly where you left off.

## Tips & Tricks

### 🚀 **Performance Tips**
- Use `--wait` to ensure elements load before interacting
- Chain commands together instead of running separate processes
- Reuse sessions instead of creating new ones

### 🎯 **Selector Tips**
- Use browser dev tools to find the right selectors
- Prefer IDs (`#id`) when available for reliability
- Use `:contains()` for text-based selection
- Use attribute selectors for form elements: `[name="field"]`

### 🔧 **Debugging**
```bash
# See what's happening under the hood
./hweb --debug --session test --url https://example.com

# Take screenshots to see the current state
./hweb --session test --screenshot "debug.png"

# Check if elements exist before using them
./hweb --session test --exists "#my-element"
```

## Examples & Use Cases

Check out `comprehensive-test.sh` for a complete example that demonstrates all features, or visit our examples directory for specific use cases like:

- **E-commerce automation** (adding items to cart, checkout process)
- **Social media posting** (automated content publishing)
- **Data scraping** (extracting structured data from websites)
- **Website monitoring** (checking for changes or errors)
- **Visual regression testing** (screenshot comparisons)

---

**Ready to automate the web?** Start with a simple command and build up to complex workflows. HeadlessWeb makes web automation accessible to everyone, from system administrators to developers to QA engineers.
