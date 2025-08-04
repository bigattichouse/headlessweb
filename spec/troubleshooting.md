# HeadlessWeb Troubleshooting Guide

This guide helps you resolve common issues when using HeadlessWeb.

## 🚨 Common Issues & Solutions

### Session-Related Issues

#### Session Won't Start or Restore
**Symptoms**: Error messages about session creation or restoration
```bash
Error: Failed to create session 'mysession'
Error: Session 'mysession' not found
```

**Solutions**:
```bash
# 1. List existing sessions
./hweb --list

# 2. Clean up corrupted session
./hweb --session problematic_session --end

# 3. Clear all sessions (nuclear option)
rm -rf ~/.headlessweb/sessions/*

# 4. Check permissions
ls -la ~/.headlessweb/
chmod 755 ~/.headlessweb/
chmod 644 ~/.headlessweb/sessions/*
```

#### Session Data Bleeding Between Sessions
**Symptoms**: Data from one session appears in another
```bash
# Expected: clean session
./hweb --session clean --js "localStorage.getItem('data')"
# Actual: Returns data from another session
```

**Solutions**:
```bash
# 1. Use unique session names
./hweb --session "project_$(date +%s)" --url example.com

# 2. Explicitly end sessions
./hweb --session mysession --end

# 3. Clear browser cache
./hweb --session mysession --js "localStorage.clear(); sessionStorage.clear();"
```

### Page Loading Issues

#### Page Loads But Elements Not Found
**Symptoms**: Elements exist in browser but HeadlessWeb can't find them
```bash
./hweb --session test --url example.com --click "#button"
# Error: Element '#button' not found
```

**Solutions**:
```bash
# 1. Wait for elements to load
./hweb --session test --url example.com \
  --wait-selector "#button" 5000 \
  --click "#button"

# 2. Wait for page ready state
./hweb --session test --url example.com \
  --wait-ready 3000 \
  --click "#button"

# 3. Check if element exists first
./hweb --session test --exists "#button"
# Returns: true/false

# 4. Use enhanced waiting for SPAs
./hweb --session test --url example.com \
  --wait-spa-navigation \
  --wait-framework-ready react \
  --click "#button"
```

#### JavaScript Not Working
**Symptoms**: JavaScript commands return empty results or errors
```bash
./hweb --session test --js "document.title"
# Returns: empty or error
```

**Solutions**:
```bash
# 1. Ensure page is fully loaded
./hweb --session test --url example.com \
  --wait-ready 3000 \
  --js "document.title"

# 2. Use wrapped JavaScript execution
./hweb --session test --js "(function() { return document.title; })()"

# 3. Check for JavaScript errors
./hweb --session test --js "console.log('test'); return 'working';"

# 4. Debug with screenshot
./hweb --session test --screenshot "debug.png" --js "document.readyState"
```

### Form Interaction Issues

#### Forms Not Submitting
**Symptoms**: Form fields filled but submission doesn't work
```bash
./hweb --session form --type "#email" "test@example.com" --click "#submit"
# Form doesn't submit or shows validation errors
```

**Solutions**:
```bash
# 1. Use proper event simulation
./hweb --session form \
  --focus "#email" \
  --type "#email" "test@example.com" \
  --wait 500 \
  --click "#submit"

# 2. Use enhanced form filling for modern frameworks
./hweb --session form \
  --fill-enhanced "#email" "test@example.com" \
  --click "#submit"

# 3. Trigger change events manually
./hweb --session form \
  --type "#email" "test@example.com" \
  --js "document.getElementById('email').dispatchEvent(new Event('change'))" \
  --click "#submit"

# 4. Check form validation
./hweb --session form --js "document.querySelector('form').checkValidity()"
```

#### Dropdown/Select Elements Not Working
**Symptoms**: Select options not being chosen properly
```bash
./hweb --session test --select "#country" "USA"
# Option not selected or form shows error
```

**Solutions**:
```bash
# 1. Check available options first
./hweb --session test --js "Array.from(document.querySelector('#country').options).map(o => o.value)"

# 2. Use exact option value
./hweb --session test --select "#country" "US"  # Use value, not text

# 3. Use text content if needed
./hweb --session test --js "document.querySelector('#country').value = 'US'; document.querySelector('#country').dispatchEvent(new Event('change'));"

# 4. Wait after selection
./hweb --session test --select "#country" "USA" --wait 500
```

### File Operation Issues

#### File Uploads Not Working
**Symptoms**: File upload fields not accepting files
```bash
./hweb --session upload --upload "#file-input" "/path/to/file.txt"
# Error: Upload failed
```

**Solutions**:
```bash
# 1. Check file exists and permissions
ls -la /path/to/file.txt
chmod 644 /path/to/file.txt

# 2. Use absolute paths
./hweb --session upload --upload "#file-input" "$(realpath file.txt)"

# 3. Check file size limits
du -h /path/to/file.txt

# 4. Verify file input selector
./hweb --session upload --exists "input[type='file']"
```

#### Downloads Not Completing
**Symptoms**: Download commands timeout or fail
```bash
./hweb --session download --download-wait "https://example.com/file.pdf"
# Timeout or file not found
```

**Solutions**:
```bash
# 1. Increase timeout
./hweb --session download --download-timeout 30000 --download-wait "https://example.com/file.pdf"

# 2. Check download directory permissions
mkdir -p ~/.headlessweb/downloads
chmod 755 ~/.headlessweb/downloads

# 3. Set custom download directory
./hweb --session download --download-dir "/tmp/downloads" --download-wait "url"

# 4. Check if login required
./hweb --session download --url "https://site.com/login" --type "#user" "..." --click "#login" --download-wait "protected-file.pdf"
```

### Environment and Installation Issues

#### GTK/WebKit Errors
**Symptoms**: Errors about GTK or WebKit libraries
```bash
./hweb --version
# Error: cannot open shared object file: libgtk-4.so.1
```

**Solutions**:
```bash
# 1. Install missing dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y libgtk-4-dev libwebkitgtk-6.0-dev

# 2. Check library paths
ldconfig -p | grep gtk
ldconfig -p | grep webkit

# 3. Update library cache
sudo ldconfig

# 4. Check WebKit version compatibility
pkg-config --modversion webkit2gtk-6.0
```

#### File Dialog Popups During Testing
**Symptoms**: File chooser dialogs appear during tests
```bash
make test
# GUI file dialogs open
```

**Solutions**:
```bash
# 1. Use headless test runner
./run_tests_headless.sh

# 2. Set environment variables manually
export GTK_RECENT_FILES_ENABLED=0
export WEBKIT_DISABLE_FILE_PICKER=1
make test

# 3. Run in virtual display
Xvfb :99 -screen 0 1024x768x24 &
export DISPLAY=:99
make test
```

#### Build Errors
**Symptoms**: Compilation fails with missing headers or libraries
```bash
make
# Error: webkit2/webkit2.h: No such file or directory
```

**Solutions**:
```bash
# 1. Install development packages
sudo apt-get install -y build-essential cmake pkg-config \
  libgtk-4-dev libwebkitgtk-6.0-dev libjsoncpp-dev

# 2. Clean and rebuild
make clean
cmake .
make

# 3. Check pkg-config
pkg-config --cflags gtk4
pkg-config --cflags webkit2gtk-6.0

# 4. Verify CMake finds dependencies
cmake . -DCMAKE_VERBOSE_MAKEFILE=ON
```

## 🔍 Advanced Debugging

### Debug Mode
```bash
# Enable detailed debug output
./hweb --debug --session debug --url example.com --click "#button"
```

### Screenshot Debugging
```bash
# Take screenshots at each step
./hweb --session debug \
  --url example.com \
  --screenshot "step1-loaded.png" \
  --type "#search" "query" \
  --screenshot "step2-typed.png" \
  --click "#submit" \
  --screenshot "step3-clicked.png"
```

### JavaScript Console Debugging
```bash
# Check for JavaScript errors
./hweb --session debug --js "console.error.toString()"

# Inspect page state
./hweb --session debug --js "document.readyState"
./hweb --session debug --js "window.location.href"
./hweb --session debug --js "document.querySelector('#element') ? 'found' : 'not found'"
```

### Session State Inspection
```bash
# Check session files
ls -la ~/.headlessweb/sessions/your_session/

# Inspect session data
./hweb --session your_session --js "Object.keys(localStorage)"
./hweb --session your_session --js "document.cookie"
```

## 🚨 Emergency Fixes

### Complete Reset
```bash
# Nuclear option: reset everything
./hweb --list | xargs -I {} ./hweb --session {} --end
rm -rf ~/.headlessweb/
mkdir -p ~/.headlessweb/sessions
```

### Test Environment Reset
```bash
# Clean test environment
make clean
rm -rf Testing/
cmake .
make test
```

### Performance Issues
```bash
# Reduce browser resource usage
./hweb --width 800 --session light --url example.com

# Disable images and CSS for faster loading
./hweb --session fast --js "
  document.querySelectorAll('img').forEach(img => img.style.display='none');
  document.querySelectorAll('link[rel=stylesheet]').forEach(link => link.disabled=true);
"
```

## 📞 Getting Help

### Before Asking for Help
1. **Try the solutions above** for your specific issue
2. **Check existing issues** on GitHub
3. **Enable debug mode** and include output
4. **Create minimal reproduction case**

### Information to Include in Bug Reports
```bash
# System information
uname -a
./hweb --version
pkg-config --modversion webkit2gtk-6.0

# Minimal reproduction case
./hweb --debug --session minimal --url "simple-page.html" --click "#button"

# Include any error output and screenshots
```

### Common Solutions Summary
- **Session issues**: Clean up with `--end`, use unique names
- **Element not found**: Add `--wait-selector` or `--wait-ready`  
- **Form problems**: Use `--focus` before `--type`, wait between actions
- **File operations**: Check permissions, use absolute paths
- **GTK errors**: Install development packages, use headless mode for tests
- **JavaScript issues**: Ensure page loaded, use wrapped execution

Most issues are resolved by adding appropriate waiting commands and ensuring pages are fully loaded before interaction.