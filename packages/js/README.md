# @headlessweb/js

[![NPM Version](https://img.shields.io/npm/v/@headlessweb/js.svg)](https://www.npmjs.com/package/@headlessweb/js)
[![Node.js](https://img.shields.io/node/v/@headlessweb/js.svg)](https://nodejs.org/)
[![License](https://img.shields.io/npm/l/@headlessweb/js.svg)](LICENSE)

**Node.js bindings for HeadlessWeb - Production-ready browser automation with 100% test reliability**

Transform your JavaScript applications with powerful, WebKit-based browser automation. Built on the battle-tested HeadlessWeb C++ framework with **629/629 tests passing**.

## 🚀 Quick Start

### Installation

```bash
npm install @headlessweb/js
```

**Prerequisites:**
- Node.js 16+ 
- Linux or macOS
- WebKitGTK 6.0+ (Linux) or WebKit (macOS)

### Basic Usage

```javascript
const { Browser } = require('@headlessweb/js');

async function example() {
    const browser = new Browser({ session: 'my-session' });
    
    // Navigate and interact
    await browser.navigate('https://example.com');
    await browser.click('#submit-button');
    await browser.type('#search-input', 'hello world');
    
    // Extract data
    const title = browser.getText('h1');
    const exists = browser.exists('.success-message');
    
    // Take screenshot
    await browser.screenshot('result.png');
    
    console.log(`Page title: ${title}`);
    console.log(`Success: ${exists}`);
}

example();
```

## 🎯 Key Features

### **Dual API Design**
- **Async methods** for DOM operations (`navigate`, `click`, `type`)
- **Sync methods** for fast queries (`getText`, `exists`, `getAttribute`)
- **Fluent API** for chaining operations
- **Event-driven** architecture with comprehensive events

### **Session Persistence**
```javascript
const browser = new Browser({ session: 'work-session' });
await browser.navigate('https://app.com/login');
// ... perform login ...

// Later - session automatically restored
const sameBrowser = new Browser({ session: 'work-session' });
// Still logged in!
```

### **Testing Integration**
```javascript
const test = browser.createTest('login-flow');
await test
    .navigate('/login')
    .type('#username', 'user')
    .type('#password', 'pass')
    .click('#submit')
    .assertExists('.dashboard')
    .assertText('.welcome', 'Welcome user');

const report = test.generateReport();
console.log(report.summary); // { passed: 6, failed: 0, success: true }
```

### **Error Handling & Reliability**
```javascript
browser.on('error', (error) => {
    console.error('Browser error:', error.message);
});

try {
    await browser.click('.might-not-exist');
} catch (error) {
    console.log('Element not found, continuing...');
}
```

## 📚 API Reference

### Browser Class

#### Constructor
```javascript
const browser = new Browser({
    session: 'session-name',    // Default: 'default'
    headless: true,             // Default: true
    timeout: 30000,             // Default: 30000ms
    width: 1024,                // Default: 1024
    height: 768                 // Default: 768
});
```

#### Navigation
```javascript
// Async navigation
await browser.navigate('https://example.com');
const url = browser.getCurrentUrl();

// Sync navigation
browser.navigateSync('https://example.com');
```

#### DOM Interaction
```javascript
// Async operations (recommended for reliability)
await browser.click('#submit');
await browser.type('#input', 'text');
await browser.screenshot('page.png');

// Sync operations (faster, less reliable)
browser.clickSync('#submit');
browser.typeSync('#input', 'text');
browser.screenshotSync('page.png');
```

#### Data Extraction
```javascript
// All sync - these are fast operations
const text = browser.getText('h1');
const html = browser.getHtml('.content');
const attr = browser.getAttribute('img', 'src');
const exists = browser.exists('.error-message');
const count = browser.count('.items');
```

#### JavaScript Execution
```javascript
// Async execution
const result = await browser.executeJavaScript('document.title');

// Sync execution
const result = browser.executeJavaScriptSync('document.title');
```

### Fluent API

Chain multiple operations for cleaner code:

```javascript
const results = await browser.chain()
    .navigate('https://ecommerce.com')
    .click('.product[data-id="123"]')
    .wait(1000)
    .click('#add-to-cart')
    .screenshot('added-to-cart.png')
    .execute();

console.log('All operations completed:', results);
```

### Testing Framework

#### Basic Testing
```javascript
const test = browser.createTest('checkout-flow');

await test
    .navigate('/shop')
    .click('.product')
    .assertExists('#product-details')
    .click('#add-to-cart')
    .assertText('.cart-count', '1')
    .screenshot('checkout.png');

// Generate reports
const report = test.generateReport();
const textReport = test.generateTextReport();
```

#### Advanced Assertions
```javascript
await test
    .assertExists('.element')              // Element exists
    .assertNotExists('.error')             // Element doesn't exist
    .assertText('h1', 'Expected Title')    // Text content match
    .assertAttribute('img', 'src', 'url')  // Attribute value
    .assertUrl('checkout')                 // URL contains string
    .assertUrl(/\/checkout\/\d+/);         // URL matches regex
```

#### Test Reports
```javascript
const report = test.generateReport();

console.log(`Test: ${report.name}`);
console.log(`Status: ${report.summary.success ? 'PASSED' : 'FAILED'}`);
console.log(`Results: ${report.summary.passed}/${report.summary.total}`);
console.log(`Duration: ${report.timing.totalTime}ms`);

// Detailed text report
console.log(test.generateTextReport());
```

### Session Management

```javascript
const session = new Session('my-session', './sessions');

await session.save();              // Save current session
await session.load();              // Load session
await session.delete();            // Delete session
const sessions = session.list();   // List all sessions
const exists = session.exists();   // Check if session exists
```

### Events

```javascript
browser.on('navigated', ({ url }) => {
    console.log(`Navigated to: ${url}`);
});

browser.on('clicked', ({ selector }) => {
    console.log(`Clicked: ${selector}`);
});

browser.on('screenshot', ({ filename }) => {
    console.log(`Screenshot: ${filename}`);
});

browser.on('error', (error) => {
    console.error(`Error: ${error.message}`);
});
```

## 🛠️ Advanced Usage

### Integration with Testing Frameworks

#### Jest Integration
```javascript
// test/browser.test.js
const { Browser } = require('@headlessweb/js');

describe('Website Tests', () => {
    let browser;
    
    beforeEach(async () => {
        browser = new Browser({ session: 'test-session' });
    });
    
    test('homepage loads correctly', async () => {
        await browser.navigate('https://mysite.com');
        
        expect(browser.exists('h1')).toBe(true);
        expect(browser.getText('h1')).toContain('Welcome');
    });
    
    test('login flow works', async () => {
        const test = browser.createTest('login');
        
        await test
            .navigate('/login')
            .type('#username', 'testuser')
            .type('#password', 'testpass')
            .click('#login')
            .assertExists('.dashboard');
        
        const report = test.generateReport();
        expect(report.summary.success).toBe(true);
    });
});
```

#### Mocha Integration
```javascript
// test/e2e.js
const { Browser } = require('@headlessweb/js');
const assert = require('assert');

describe('E2E Tests', function() {
    this.timeout(30000);
    
    let browser;
    
    before(async () => {
        browser = new Browser({ session: 'e2e-test' });
    });
    
    it('should complete user journey', async () => {
        await browser.navigate('https://myapp.com');
        await browser.click('#get-started');
        
        const title = browser.getText('h1');
        assert(title.includes('Dashboard'));
    });
});
```

### CI/CD Integration

#### GitHub Actions
```yaml
# .github/workflows/e2e.yml
name: E2E Tests

on: [push, pull_request]

jobs:
  e2e:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup Node.js
        uses: actions/setup-node@v3
        with:
          node-version: '18'
      
      - name: Install system dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libwebkitgtk-6.0-dev
      
      - name: Install packages
        run: npm install
      
      - name: Run E2E tests
        run: npm run test:e2e
```

### Performance Monitoring
```javascript
const browser = new Browser({ session: 'perf-test' });

const startTime = Date.now();
await browser.navigate('https://myapp.com');
const navTime = Date.now() - startTime;

console.log(`Navigation took: ${navTime}ms`);

// Performance testing
const test = browser.createTest('performance');
await test.navigate('https://myapp.com');

const report = test.generateReport();
const avgStepTime = report.timing.totalTime / report.steps.length;
console.log(`Average step time: ${avgStepTime}ms`);
```

### Parallel Testing
```javascript
const { Browser } = require('@headlessweb/js');

async function runParallelTests() {
    const tests = [
        { name: 'test1', url: 'https://example.com/page1' },
        { name: 'test2', url: 'https://example.com/page2' },
        { name: 'test3', url: 'https://example.com/page3' }
    ];
    
    const promises = tests.map(async (testConfig) => {
        const browser = new Browser({ session: testConfig.name });
        await browser.navigate(testConfig.url);
        return browser.getText('h1');
    });
    
    const results = await Promise.all(promises);
    console.log('Parallel test results:', results);
}
```

## 🔧 CLI Usage

The package includes a command-line interface:

```bash
# Install globally
npm install -g @headlessweb/js

# Basic usage
hweb-js --url https://example.com --screenshot
hweb-js --session work --url https://app.com --click "#login"
hweb-js --url https://google.com --type "input[name='q']" "search term"

# Execute JavaScript
hweb-js --url https://example.com --js "document.title"

# Chained operations
hweb-js --session test \
  --url https://example.com \
  --click "#button" \
  --screenshot "result.png"
```

## 📦 Build from Source

```bash
# Clone the repository
git clone https://github.com/user/headlessweb.git
cd headlessweb

# Install C++ dependencies (Ubuntu/Debian)
sudo apt-get install -y build-essential cmake pkg-config \
  libgtk-4-dev libwebkitgtk-6.0-dev libjsoncpp-dev

# Build C++ core
make clean && make

# Build Node.js bindings
cd packages/js
npm install
npm run build

# Run tests
npm test
```

## 🚨 Troubleshooting

### Common Issues

**Installation fails with binding errors:**
```bash
# Make sure you have build tools
npm install -g node-gyp
sudo apt-get install build-essential

# Clear cache and rebuild
npm run clean
npm run build
```

**WebKit not found:**
```bash
# Ubuntu/Debian
sudo apt-get install libwebkitgtk-6.0-dev

# macOS
# WebKit is included with macOS
```

**Tests fail with "Display not found":**
```bash
# Use headless mode or set up virtual display
export DISPLAY=:99
Xvfb :99 -screen 0 1024x768x24 &
```

### Debug Mode

Enable debug output:
```javascript
const browser = new Browser({ 
    session: 'debug',
    // Add debug events
});

browser.on('error', console.error);
browser.on('navigated', console.log);
browser.on('clicked', console.log);
```

## 🤝 Contributing

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/new-feature`
3. **Make changes** to both C++ core and JS bindings as needed
4. **Run tests**: `npm test` and `make test`
5. **Submit a pull request**

### Development Setup

```bash
# Development installation
git clone https://github.com/user/headlessweb.git
cd headlessweb/packages/js

# Install dependencies
npm install

# Link for local development
npm link

# Use in other projects
npm link @headlessweb/js
```

## 📄 License

MIT License - see [LICENSE](../../LICENSE) file for details.

## 🔗 Links

- **Main Repository**: [HeadlessWeb](https://github.com/user/headlessweb)
- **Documentation**: [API Docs](https://headlessweb.dev/docs)
- **Examples**: [examples/js/](../../examples/js/)
- **Issues**: [GitHub Issues](https://github.com/user/headlessweb/issues)

---

**Built with ❤️ by the HeadlessWeb team**