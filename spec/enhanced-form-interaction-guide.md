# Enhanced Form Interaction Guide

## Overview

This guide documents the enhanced form interaction patterns developed to handle modern dynamic forms like Google Search, React forms, and other JavaScript-heavy web applications.

## The Problem

Traditional form filling using simple `element.value = 'text'` doesn't work reliably with modern web applications because:

1. **JavaScript frameworks** (React, Vue, Angular) manage form state and require proper event dispatching
2. **Dynamic forms** like Google Search require activation sequences before input is accepted
3. **Event listeners** expect comprehensive event chains (focus, input, change, blur, etc.)
4. **Form validation** may be triggered by specific event combinations

## Enhanced Solutions

### 1. Improved `--fill` Command

The standard `--fill` (alias for `--type`) command has been enhanced with:

```javascript
// Enhanced focus handling
element.focus();
element.click();  // Some forms require click to activate

// Clear existing value first
element.value = '';

// Set new value
element.value = 'user_input';

// Comprehensive event dispatching
element.dispatchEvent(new Event('focus', { bubbles: true }));
element.dispatchEvent(new Event('input', { bubbles: true }));
element.dispatchEvent(new Event('keydown', { bubbles: true }));
element.dispatchEvent(new Event('keyup', { bubbles: true }));
element.dispatchEvent(new Event('change', { bubbles: true }));

// React/Vue compatibility
if (element._valueTracker) {
  element._valueTracker.setValue('');
}
```

### 2. New `--fill-enhanced` Command

For extremely complex forms, use the new `--fill-enhanced` command:

```bash
./hweb --fill-enhanced "textarea[aria-label='Search']" "search query"
```

**Features:**
- Extended event chain including `keypress`, `focusin`, `focusout`
- React/Vue.js specific handling with `_valueTracker`
- Angular.js support with scope application
- Better error handling and fallback mechanisms
- Longer timeouts for dynamic content loading

### 3. Dynamic Form Helper Function

For complex multi-step interactions:

```cpp
browser.interactWithDynamicForm(
    "textarea[aria-label='Search']",  // input selector
    "LLM wiki",                       // text to fill
    "input[name='btnK']",            // submit button selector
    1000                             // wait timeout ms
);
```

## Working Patterns

### Google Search Pattern

**Problem:** Google's search requires specific activation sequence.

**Solution:**
```bash
./hweb --start https://www.google.com \
  --js "var searchBox = document.querySelector('textarea[aria-label=\"Search\"]'); searchBox.focus(); searchBox.value = 'LLM wiki'; var event = new Event('input', { bubbles: true }); searchBox.dispatchEvent(event);" \
  --click "input[name='btnK']" \
  --wait-selector "h3" 5000 \
  --text "h3 a"
```

**Key Points:**
- Use `textarea[aria-label="Search"]` not `input[name='q']`
- Must dispatch `input` event after setting value
- Use `--start` flag to avoid session restoration bugs

### React Form Pattern

**Problem:** React forms don't respond to simple value setting.

**Solution:**
```bash
./hweb --fill-enhanced "#react-input" "user input"
```

**Or manually:**
```bash
./hweb --js "
var input = document.querySelector('#react-input');
input.focus();
input.value = '';
input.value = 'user input';
var event = new Event('input', { bubbles: true });
input.dispatchEvent(event);
if (input._valueTracker) {
  input._valueTracker.setValue('user input');
}
"
```

### Vue.js Form Pattern

Similar to React, but may require additional event dispatching:

```javascript
var input = document.querySelector('#vue-input');
input.focus();
input.value = 'user input';
input.dispatchEvent(new Event('input', { bubbles: true }));
input.dispatchEvent(new Event('change', { bubbles: true }));
```

### Angular Form Pattern

For Angular forms with two-way binding:

```javascript
var input = document.querySelector('#angular-input');
input.focus();
input.value = 'user input';
// Trigger Angular's digest cycle
var scope = angular.element(input).scope();
if (scope) {
  scope.$apply();
}
```

## Best Practices

### 1. Selector Strategy

**Use robust selectors that are less likely to change:**

✅ **Good:**
- `textarea[aria-label="Search"]`
- `input[placeholder="Enter email"]`
- `[data-testid="login-input"]`

❌ **Avoid:**
- `#APjFqb` (Google's dynamic IDs)
- `.css-specific-classes`
- Complex nested selectors

### 2. Session Management

**Always use `--start` flag for fresh sessions:**

```bash
./hweb --session my-session --start https://example.com
```

**Why:** Avoids the GLib-CRITICAL session restoration bug that causes:
- `Source ID 65 was not found when attempting to remove it`
- `Page load timeout during session restore`
- `Error in session restoration: Failed to load session URL`

### 3. Timing and Waits

**Allow time for JavaScript frameworks:**

```bash
./hweb --fill "input" "text" --wait 200 --click "button"
```

**Use appropriate wait selectors:**

```bash
./hweb --click "button" --wait-selector "h1" 5000
```

### 4. Event Sequence

**For maximum compatibility, use this event sequence:**

1. `focus` - Activate the input field
2. `click` - Some forms require click activation  
3. `input` - Notify of value change
4. `keydown`/`keyup` - Simulate typing
5. `change` - Confirm value change
6. `blur` - Deactivate (if submitting)

## Debugging Tips

### 1. Test Individual Steps

Break complex interactions into steps:

```bash
# Step 1: Fill
./hweb --session test --fill "input" "text" --screenshot "step1.png"

# Step 2: Click
./hweb --session test --click "button" --screenshot "step2.png"

# Step 3: Verify
./hweb --session test --text "h1" --screenshot "step3.png"
```

### 2. Use Debug Mode

Enable debug output to see what's happening:

```bash
./hweb --debug --fill "input" "text"
```

### 3. Check Event Dispatching

Verify events are being fired:

```bash
./hweb --js "
document.querySelector('input').addEventListener('input', function() {
  console.log('Input event fired!');
});
"
```

### 4. Inspect Form State

Check if the form is properly activated:

```bash
./hweb --js "document.querySelector('input').value"
./hweb --js "document.activeElement.tagName"
```

## Session Restoration Bug Workarounds

The session restoration system has a critical bug. **Always use these patterns:**

### ✅ Working Pattern
```bash
./hweb --session clean-session --start https://example.com \
  --fill "input" "text" \
  --click "button"
```

### ❌ Problematic Pattern
```bash
./hweb --session existing-session https://example.com \
  --fill "input" "text"
```

**Why:** The second pattern triggers session restoration which has GLib timeout source cleanup bugs.

## Command Reference

### Standard Commands
- `--fill selector text` (alias for `--type`)
- `--type selector text` (enhanced with better event handling)

### Enhanced Commands  
- `--fill-enhanced selector text` (maximum compatibility)

### Helper Patterns
- Always use `--start` flag for new sessions
- Use `textarea[aria-label="..."]` for search boxes
- Add delays with `--wait milliseconds` if needed
- Take screenshots for debugging: `--screenshot filename.png`

## Testing

Enhanced form interaction includes comprehensive test coverage:

- **Standard form compatibility** - Traditional HTML forms
- **React/Vue.js compatibility** - Modern framework forms  
- **Google search pattern** - Complex dynamic forms
- **Session restoration bug tests** - Documents known issues
- **Performance benchmarks** - Ensures responsiveness

Run tests with:
```bash
cd tests && ./test-forms.sh
```

## Future Improvements

Potential enhancements being considered:

1. **Auto-detection** of form framework (React/Vue/Angular)
2. **Smart waiting** - automatically wait for dynamic content
3. **Form validation** - check for validation errors after input
4. **Multi-step form wizards** - handle complex form flows
5. **File upload enhancement** - better handling of file inputs

## Implementation Files

The enhanced form interaction is implemented across:

- `src/Browser/DOM.cpp` - Core `fillInput()` improvements
- `src/Browser/EnhancedFormInteraction.cpp` - Advanced form handling
- `src/hweb/Config.cpp` - Command parsing for `--fill` and `--fill-enhanced`
- `src/hweb/Handlers/BasicCommands.cpp` - Command execution
- `tests/browser/test_session_restoration_bug.cpp` - Bug documentation and tests

This enhanced system transforms HeadlessWeb from basic form filling to comprehensive modern web application automation.