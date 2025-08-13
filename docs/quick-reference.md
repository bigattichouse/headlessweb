# HeadlessWeb Quick Reference

**Fast reference for HeadlessWeb JavaScript API**

## 🚀 Installation

```bash
npm install @headlessweb/js
make js  # If building from source
```

## ⚡ Quick Start

```javascript
const { Browser } = require('@headlessweb/js');

const browser = new Browser({ session: 'quick' });
await browser.navigate('https://example.com');
const title = browser.getText('h1');
await browser.screenshot('page.png');
```

## 🎯 Browser API Cheatsheet

### Navigation
```javascript
await browser.navigate('https://example.com')    // Navigate to URL
browser.getCurrentUrl()                           // Get current URL
await browser.wait(1000)                         // Wait milliseconds
```

### DOM Interaction (Async)
```javascript
await browser.click('#submit')                   // Click element
await browser.type('#input', 'text')            // Type text
await browser.screenshot('file.png')            // Take screenshot
await browser.executeJavaScript('code')         // Run JavaScript
```

### DOM Interaction (Sync)
```javascript
browser.clickSync('#submit')                     // Sync click
browser.typeSync('#input', 'text')              // Sync type
browser.screenshotSync('file.png')              // Sync screenshot
browser.executeJavaScriptSync('code')           // Sync JavaScript
```

### Element Queries (All Sync)
```javascript
browser.exists('#element')                       // Check if exists
browser.getText('#element')                      // Get text content
browser.getHtml('#element')                      // Get HTML content
browser.getAttribute('#element', 'class')        // Get attribute
browser.count('.items')                          // Count elements
```

### Utility
```javascript
browser.getPageInfo()                            // Get page info
browser.chain()                                  // Start fluent chain
browser.createTest('test-name')                  // Create test suite
browser.destroy()                                // Clean up
```

## 🔗 Fluent API

```javascript
const results = await browser.chain()
    .navigate('https://example.com')
    .click('#button')
    .wait(1000)
    .screenshot('result.png')
    .execute();
```

## 🧪 Testing Framework

```javascript
const test = browser.createTest('Login Test');

await test
    .navigate('/login')
    .type('#username', 'user')
    .type('#password', 'pass')
    .click('#submit')
    .assertExists('.dashboard')
    .assertText('.welcome', 'Welcome')
    .assertUrl('/dashboard');

const report = test.generateReport();
console.log(report.summary); // { passed, failed, success, duration }
```

### Test Assertions
```javascript
await test.assertExists('#element')              // Element exists
await test.assertNotExists('.error')             // Element doesn't exist
await test.assertText('h1', 'Title')            // Text contains
await test.assertAttribute('img', 'src', 'url') // Attribute equals
await test.assertUrl('dashboard')                // URL contains
await test.assertUrl(/\/user\/\d+/)             // URL regex match
```

## 📋 Common Patterns

### Basic Automation
```javascript
const browser = new Browser({ session: 'automation' });

try {
    await browser.navigate('https://app.com');
    await browser.type('#search', 'query');
    await browser.click('#submit');
    
    const results = browser.getText('.results');
    console.log('Results:', results);
} finally {
    browser.destroy();
}
```

### Form Handling
```javascript
await browser.navigate('https://form.com');
await browser.type('#name', 'John Doe');
await browser.type('#email', 'john@example.com');
await browser.click('input[type="submit"]');

const success = browser.exists('.success-message');
```

### Session Management
```javascript
// Persistent session across runs
const browser = new Browser({ session: 'user-session' });
await browser.navigate('https://app.com/login');
// ... perform login ...

// Later, session is automatically restored
const sameBrowser = new Browser({ session: 'user-session' });
// Still logged in!
```

### Error Handling
```javascript
try {
    await browser.click('#might-not-exist');
} catch (error) {
    console.log('Element not found, continuing...');
    await browser.screenshot('error-state.png');
}
```

## 🔧 Configuration

```javascript
const browser = new Browser({
    session: 'my-session',      // Session name
    headless: true,             // Headless mode
    timeout: 30000,             // Default timeout
    width: 1920,                // Viewport width
    height: 1080                // Viewport height
});
```

## 📱 CLI Usage

```bash
# Basic navigation
hweb-js --url https://example.com --screenshot

# Form interaction
hweb-js --session login \
  --url https://app.com/login \
  --type "#username" "user" \
  --type "#password" "pass" \
  --click "#submit"

# JavaScript execution
hweb-js --url https://api.com \
  --js "document.title"

# Chain operations
hweb-js --session workflow \
  --url https://example.com \
  --click "#accept" \
  --screenshot "step1.png"
```

## 🧪 Jest Integration

```javascript
// Setup
global.createTestBrowser = () => new Browser({ 
    session: `test-${Date.now()}`,
    headless: true 
});

// Test
describe('App Tests', () => {
    let browser;
    
    beforeEach(() => {
        browser = createTestBrowser();
    });
    
    afterEach(() => {
        browser.destroy();
    });
    
    test('should load page', async () => {
        await browser.navigate('https://example.com');
        expect(browser.exists('h1')).toBe(true);
    });
});
```

## 🎯 Selectors Reference

```javascript
// ID selector
browser.click('#submit-button')

// Class selector
browser.getText('.welcome-message')

// Attribute selector
browser.type('input[name="username"]', 'user')

// CSS selector
browser.click('button:contains("Submit")')

// Complex selector
browser.exists('.modal.active .close-button')
```

## ⚡ Performance Tips

```javascript
// ✅ Use sync methods for queries (fast)
const title = browser.getText('h1');
const exists = browser.exists('.error');

// ✅ Use async methods for interactions (reliable)
await browser.click('#submit');
await browser.type('#input', 'text');

// ✅ Batch operations with fluent API
await browser.chain()
    .click('#btn1')
    .click('#btn2')
    .click('#btn3')
    .execute();

// ✅ Reuse browser instances
const browser = new Browser({ session: 'reusable' });
// ... multiple operations
```

## 🚨 Common Errors

### Build Issues
```bash
# Missing node-gyp
npm install -g node-gyp

# Missing WebKit
sudo apt-get install libwebkitgtk-6.0-dev
```

### Runtime Issues
```javascript
// Element not found
if (browser.exists('#element')) {
    await browser.click('#element');
}

// Display issues in CI
export DISPLAY=:99
Xvfb :99 -screen 0 1024x768x24 &
```

## 📦 Package Structure

```
@headlessweb/js/
├── index.js           # Main entry
├── lib/
│   ├── browser.js     # Browser class
│   ├── session.js     # Session management
│   └── test-suite.js  # Testing framework
├── bin/hweb-js        # CLI tool
└── types/index.d.ts   # TypeScript definitions
```

## 🔗 Links

- **Full Documentation**: [nodejs.md](nodejs.md)
- **NPM Guide**: [npm-package.md](npm-package.md)
- **Examples**: [../examples/js/](../examples/js/)
- **TypeScript Types**: [../packages/js/types/index.d.ts](../packages/js/types/index.d.ts)

---

**Need more details? Check the [complete documentation](nodejs.md)!**