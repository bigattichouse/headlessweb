# HeadlessWeb Node.js Integration

**Complete guide to using HeadlessWeb with JavaScript and Node.js**

## 📋 Table of Contents

- [Overview](#overview)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Testing Framework](#testing-framework)
- [Comprehensive Testing Suite](#comprehensive-testing-suite)
- [Development](#development)
- [Troubleshooting](#troubleshooting)

## 🎯 Overview

HeadlessWeb provides both command-line and programmatic Node.js APIs for browser automation. Built on the rock-solid C++ WebKitGTK framework with **629/629 tests passing**, it offers:

- **Native performance** via C++ bindings
- **Dual API design** (async/sync operations)
- **Session persistence** across runs
- **Built-in testing framework** with assertions
- **Event-driven architecture** for monitoring
- **TypeScript support** with complete definitions

### Architecture

```
┌─────────────────────────────────────────┐
│           JavaScript Application        │
├─────────────────────────────────────────┤
│         @headlessweb/js Package         │
│  ┌─────────────┐ ┌─────────────────────┐ │
│  │   Browser   │ │    TestSuite        │ │
│  │   Session   │ │    Fluent API       │ │
│  └─────────────┘ └─────────────────────┘ │
├─────────────────────────────────────────┤
│         Node.js Native Bindings         │
│         (C++ ↔ JavaScript Bridge)       │
├─────────────────────────────────────────┤
│          HeadlessWeb C++ Core           │
│     (WebKitGTK 6.0 + 629 tests)        │
└─────────────────────────────────────────┘
```

## 🚀 Installation

### Prerequisites

**System Requirements:**
- Node.js 16+ 
- Linux or macOS
- Build tools (make, cmake, gcc/clang)

**Linux (Ubuntu/Debian):**
```bash
# Install system dependencies
sudo apt-get update && sudo apt-get install -y \
  build-essential cmake pkg-config \
  libgtk-4-dev libwebkitgtk-6.0-dev libjsoncpp-dev \
  libcairo2-dev libgdk-pixbuf2.0-dev \
  nodejs npm

# Install Node.js build tools
npm install -g node-gyp
```

**macOS:**
```bash
# Install via Homebrew
brew install node cmake pkg-config gtk4

# Install Node.js build tools
npm install -g node-gyp
```

### Install from Source

```bash
# Clone the repository
git clone https://github.com/user/headlessweb.git
cd headlessweb

# Build C++ core
make clean && make

# Build JavaScript bindings
make js

# Optional: Link for global development
cd packages/js && npm link
```

### Install from npm (Future)

```bash
# When published to npm
npm install @headlessweb/js

# Or globally for CLI usage
npm install -g @headlessweb/js
```

## ⚡ Quick Start

### Basic Browser Automation

```javascript
const { Browser } = require('@headlessweb/js');

async function basicExample() {
    // Create browser instance
    const browser = new Browser({ 
        session: 'my-session',
        headless: true 
    });
    
    try {
        // Navigate to a website
        await browser.navigate('https://example.com');
        
        // Extract information
        const title = browser.getText('h1');
        const exists = browser.exists('p');
        
        // Take screenshot
        await browser.screenshot('example.png');
        
        console.log(`Title: ${title}`);
        console.log(`Has paragraph: ${exists}`);
        
    } catch (error) {
        console.error('Error:', error.message);
    }
}

basicExample();
```

### Form Interaction

```javascript
async function formExample() {
    const browser = new Browser({ session: 'form-test' });
    
    await browser.navigate('https://httpbin.org/forms/post');
    
    // Fill form fields
    await browser.type('input[name="custname"]', 'John Doe');
    await browser.type('input[name="custtel"]', '555-1234');
    
    // Submit form
    await browser.click('input[type="submit"]');
    
    // Wait and verify
    await browser.wait(2000);
    const success = browser.exists('body');
    console.log('Form submitted:', success);
}
```

### Testing Framework

```javascript
async function testingExample() {
    const browser = new Browser({ session: 'test-suite' });
    
    // Create test suite
    const test = browser.createTest('Login Flow Test');
    
    try {
        await test
            .navigate('https://example.com')
            .assertExists('h1')
            .assertText('h1', 'Example Domain')
            .screenshot('test-result.png')
            .assertUrl('example.com');
        
        // Generate report
        const report = test.generateReport();
        console.log(`Test ${report.summary.success ? 'PASSED' : 'FAILED'}`);
        console.log(`Results: ${report.summary.passed}/${report.summary.total}`);
        
    } catch (error) {
        console.error('Test failed:', error.message);
    }
}
```

## 📖 API Reference

### Browser Class

#### Constructor
```javascript
new Browser(options)
```

**Options:**
- `session: string` - Session name for persistence (default: 'default')
- `headless: boolean` - Run headless (default: true)
- `timeout: number` - Default timeout in ms (default: 30000)
- `width: number` - Browser width (default: 1024)
- `height: number` - Browser height (default: 768)

#### Navigation Methods

```javascript
// Async navigation (recommended)
await browser.navigate(url: string): Promise<boolean>

// Sync navigation (faster)
browser.navigateSync(url: string): boolean

// Get current URL
browser.getCurrentUrl(): string
```

#### DOM Interaction (Async)

```javascript
// Click element
await browser.click(selector: string): Promise<boolean>

// Type text into input
await browser.type(selector: string, text: string): Promise<boolean>

// Take screenshot
await browser.screenshot(filename?: string): Promise<boolean>

// Execute JavaScript
await browser.executeJavaScript(code: string): Promise<string>
```

#### DOM Interaction (Sync)

```javascript
// Sync versions (faster, use for simple operations)
browser.clickSync(selector: string): boolean
browser.typeSync(selector: string, text: string): boolean
browser.screenshotSync(filename?: string): boolean
browser.executeJavaScriptSync(code: string): string
```

#### Element Queries (All Sync)

```javascript
// Check element existence
browser.exists(selector: string): boolean

// Get text content
browser.getText(selector: string): string

// Get HTML content
browser.getHtml(selector: string): string

// Get attribute value
browser.getAttribute(selector: string, attribute: string): string

// Count elements
browser.count(selector: string): number
```

#### Utility Methods

```javascript
// Wait for time
await browser.wait(milliseconds: number): Promise<void>

// Get page information
browser.getPageInfo(): { url: string, title: string, readyState: string }

// Create fluent chain
browser.chain(): BrowserChain

// Create test suite
browser.createTest(name: string): TestSuite

// Clean up
browser.destroy(): void
```

### Fluent API

```javascript
const chain = browser.chain()
    .navigate('https://example.com')
    .click('#button')
    .wait(1000)
    .screenshot('result.png');

// Execute all operations
const results = await chain.execute();
```

### TestSuite Class

#### Navigation & Interaction
```javascript
await test.navigate(url: string): Promise<TestSuite>
await test.click(selector: string): Promise<TestSuite>
await test.type(selector: string, text: string): Promise<TestSuite>
await test.screenshot(filename?: string): Promise<TestSuite>
```

#### Assertions
```javascript
await test.assertExists(selector: string): Promise<TestSuite>
await test.assertNotExists(selector: string): Promise<TestSuite>
await test.assertText(selector: string, expected: string): Promise<TestSuite>
await test.assertAttribute(selector: string, attr: string, value: string): Promise<TestSuite>
await test.assertUrl(pattern: string | RegExp): Promise<TestSuite>
```

#### Utilities
```javascript
await test.wait(milliseconds: number): Promise<TestSuite>
test.message(text: string): TestSuite
```

#### Reporting
```javascript
const report = test.generateReport(): TestReport
const textReport = test.generateTextReport(): string
```

### Session Class

```javascript
const session = new Session(sessionName?: string, sessionsDir?: string)

session.getName(): string
await session.save(): Promise<boolean>
await session.load(): Promise<boolean>
await session.delete(): Promise<boolean>
session.list(): string[]
session.exists(): boolean
```

### Events

```javascript
browser.on('navigated', ({ url }) => console.log(`Navigated to: ${url}`));
browser.on('clicked', ({ selector }) => console.log(`Clicked: ${selector}`));
browser.on('screenshot', ({ filename }) => console.log(`Screenshot: ${filename}`));
browser.on('error', (error) => console.error(`Error: ${error.message}`));
```

## 🧪 Examples

### E-commerce Automation

```javascript
async function ecommerceFlow() {
    const browser = new Browser({ session: 'shopping' });
    
    try {
        // Navigate to store
        await browser.navigate('https://store.example.com');
        
        // Search for product
        await browser.type('#search', 'laptop');
        await browser.click('#search-button');
        
        // Select first product
        await browser.click('.product:first-child');
        
        // Add to cart
        await browser.click('#add-to-cart');
        
        // Verify cart
        const cartCount = browser.getText('.cart-count');
        console.log(`Items in cart: ${cartCount}`);
        
        // Take confirmation screenshot
        await browser.screenshot('cart-added.png');
        
    } catch (error) {
        console.error('Shopping flow failed:', error.message);
    }
}
```

### Social Media Posting

```javascript
async function socialMediaPost() {
    const browser = new Browser({ session: 'social' });
    
    await browser.navigate('https://social.example.com/login');
    
    // Login (assuming already logged in via session)
    if (browser.exists('#login-form')) {
        await browser.type('#username', process.env.SOCIAL_USERNAME);
        await browser.type('#password', process.env.SOCIAL_PASSWORD);
        await browser.click('#login-button');
        await browser.wait(3000);
    }
    
    // Create post
    await browser.click('#new-post');
    await browser.type('#post-content', 'Hello from HeadlessWeb! 🚀');
    await browser.click('#publish');
    
    // Verify posted
    const success = browser.exists('.post-success');
    console.log('Post published:', success);
}
```

### API Testing

```javascript
async function apiTesting() {
    const browser = new Browser({ session: 'api-test' });
    
    const test = browser.createTest('API Response Test');
    
    try {
        await test
            .navigate('https://httpbin.org/json')
            .assertExists('body')
            .screenshot('api-response.png');
        
        // Extract JSON response
        const jsonText = browser.getText('body');
        const apiData = JSON.parse(jsonText);
        
        console.log('API Response:', apiData);
        
        const report = test.generateReport();
        console.log('API test completed:', report.summary.success);
        
    } catch (error) {
        console.error('API test failed:', error.message);
    }
}
```

### Parallel Testing

```javascript
async function parallelTests() {
    const urls = [
        'https://example.com',
        'https://httpbin.org/html',
        'https://httpbin.org/json'
    ];
    
    const promises = urls.map(async (url, index) => {
        const browser = new Browser({ session: `parallel-${index}` });
        
        try {
            await browser.navigate(url);
            const title = browser.getText('title') || 'No title';
            await browser.screenshot(`parallel-${index}.png`);
            
            return { url, title, success: true };
        } catch (error) {
            return { url, error: error.message, success: false };
        }
    });
    
    const results = await Promise.all(promises);
    console.log('Parallel test results:', results);
}
```

## 🧪 Testing Framework

### Jest Integration

```javascript
// test/browser.test.js
const { Browser } = require('@headlessweb/js');

describe('Website Tests', () => {
    let browser;
    
    beforeEach(async () => {
        browser = new Browser({ session: `test-${Date.now()}` });
    });
    
    afterEach(() => {
        browser.destroy();
    });
    
    test('homepage loads correctly', async () => {
        await browser.navigate('https://example.com');
        
        expect(browser.exists('h1')).toBe(true);
        expect(browser.getText('h1')).toContain('Example Domain');
    });
    
    test('can interact with forms', async () => {
        await browser.navigate('https://httpbin.org/forms/post');
        
        await browser.type('input[name="custname"]', 'Test User');
        const value = browser.getAttribute('input[name="custname"]', 'value');
        
        expect(value).toBe('Test User');
    });
    
    test('complete user journey', async () => {
        const test = browser.createTest('user-journey');
        
        await test
            .navigate('https://example.com')
            .assertExists('h1')
            .assertText('h1', 'Example Domain')
            .screenshot('journey-end.png');
        
        const report = test.generateReport();
        expect(report.summary.success).toBe(true);
        expect(report.summary.passed).toBeGreaterThan(0);
    });
});
```

### Mocha Integration

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
    
    after(() => {
        browser.destroy();
    });
    
    it('should navigate and extract data', async () => {
        await browser.navigate('https://example.com');
        
        const title = browser.getText('h1');
        assert(title.includes('Example'));
        
        const exists = browser.exists('p');
        assert.strictEqual(exists, true);
    });
    
    it('should handle form submissions', async () => {
        await browser.navigate('https://httpbin.org/forms/post');
        
        await browser.type('input[name="custname"]', 'Mocha Test');
        await browser.click('input[type="submit"]');
        
        // Verify form was submitted (check URL change)
        const currentUrl = browser.getCurrentUrl();
        assert(currentUrl.includes('httpbin.org'));
    });
});
```

### Custom Test Runner

```javascript
// custom-test-runner.js
const { Browser } = require('@headlessweb/js');

class TestRunner {
    constructor() {
        this.tests = [];
        this.results = [];
    }
    
    addTest(name, testFn) {
        this.tests.push({ name, testFn });
    }
    
    async runAll() {
        console.log(`Running ${this.tests.length} tests...\n`);
        
        for (const { name, testFn } of this.tests) {
            const browser = new Browser({ session: `test-${Date.now()}` });
            
            try {
                console.log(`Running: ${name}`);
                await testFn(browser);
                this.results.push({ name, status: 'PASSED' });
                console.log(`✅ ${name} PASSED\n`);
            } catch (error) {
                this.results.push({ name, status: 'FAILED', error: error.message });
                console.log(`❌ ${name} FAILED: ${error.message}\n`);
            } finally {
                browser.destroy();
            }
        }
        
        this.generateReport();
    }
    
    generateReport() {
        const passed = this.results.filter(r => r.status === 'PASSED').length;
        const failed = this.results.filter(r => r.status === 'FAILED').length;
        
        console.log('='.repeat(50));
        console.log('TEST SUMMARY');
        console.log('='.repeat(50));
        console.log(`Total tests: ${this.results.length}`);
        console.log(`Passed: ${passed}`);
        console.log(`Failed: ${failed}`);
        console.log(`Success rate: ${Math.round((passed / this.results.length) * 100)}%`);
        
        if (failed > 0) {
            console.log('\nFailed tests:');
            this.results
                .filter(r => r.status === 'FAILED')
                .forEach(r => console.log(`  - ${r.name}: ${r.error}`));
        }
    }
}

// Usage
const runner = new TestRunner();

runner.addTest('Homepage Test', async (browser) => {
    await browser.navigate('https://example.com');
    const title = browser.getText('h1');
    if (!title.includes('Example')) {
        throw new Error(`Expected title to contain 'Example', got: ${title}`);
    }
});

runner.addTest('Form Test', async (browser) => {
    await browser.navigate('https://httpbin.org/forms/post');
    await browser.type('input[name="custname"]', 'Test User');
    const value = browser.getAttribute('input[name="custname"]', 'value');
    if (value !== 'Test User') {
        throw new Error(`Expected 'Test User', got: ${value}`);
    }
});

runner.runAll();
```

## 🛠️ Development

### Building from Source

```bash
# Clone repository
git clone https://github.com/user/headlessweb.git
cd headlessweb

# Development setup (installs all dependencies)
make dev-setup

# Build everything
make build-all

# Run tests
make test-all

# Development iteration (faster)
make dev-build && make dev-test
```

### Development Workflow

```bash
# Link package for local development
make dev-link

# In your project
npm link @headlessweb/js

# Make changes to HeadlessWeb
cd /path/to/headlessweb
make dev-build

# Changes are immediately available in your project
```

### Custom Makefile Targets

```bash
# JavaScript-specific targets
make js-install        # Install Node.js dependencies
make js-build          # Build Node.js bindings
make js                # Full JavaScript build
make js-test           # Run JavaScript tests
make js-examples       # Run example scripts
make js-clean          # Clean JavaScript artifacts

# Combined targets
make build-all         # Build C++ + JavaScript
make test-all          # Test everything
make clean-all         # Clean everything

# Development targets
make dev-setup         # Set up development environment
make dev-build         # Fast development build
make dev-test          # Run core tests only
make dev-link          # Link for local development

# Utility targets
make check-deps        # Check system dependencies
make version           # Show version information
make info              # Show project information
make demo              # Quick demo
```

### Debugging

```javascript
// Enable debug output
const browser = new Browser({ session: 'debug' });

browser.on('navigated', ({ url }) => {
    console.log(`[DEBUG] Navigated to: ${url}`);
});

browser.on('clicked', ({ selector }) => {
    console.log(`[DEBUG] Clicked: ${selector}`);
});

browser.on('error', (error) => {
    console.error(`[DEBUG] Error: ${error.message}`);
});

// Detailed logging in tests
const test = browser.createTest('debug-test');
test.message('Starting debug test');

await test
    .navigate('https://example.com')
    .message('Navigation completed')
    .assertExists('h1')
    .message('H1 element found')
    .screenshot('debug.png');

console.log(test.generateTextReport());
```

### Memory Management

```javascript
// Always clean up browsers in long-running applications
const browser = new Browser({ session: 'app' });

try {
    await browser.navigate('https://example.com');
    // ... do work
} finally {
    browser.destroy(); // Important for memory cleanup
}

// Use session cleanup for temporary sessions
const tempSession = `temp-${Date.now()}`;
const browser = new Browser({ session: tempSession });

try {
    // ... do work
} finally {
    const session = new Session(tempSession);
    await session.delete(); // Clean up session data
    browser.destroy();
}
```

## 🧪 Comprehensive Testing Suite

HeadlessWeb includes a robust testing infrastructure with **61 comprehensive test cases** across multiple categories to ensure reliability and provide development examples.

### Test Architecture

```
test/
├── helpers/                    # Test utilities and setup
│   ├── setup.js               # Global test configuration
│   └── teardown.js            # Cleanup and resource management
├── mocks/                     # Mock implementations
│   └── addon-mock.js          # Complete mock of native C++ addon
├── unit/                      # Unit tests for individual classes
│   ├── browser.test.js        # Browser class functionality
│   ├── session.test.js        # Session management
│   ├── test-suite.test.js     # TestSuite framework
│   ├── index.test.js          # Package entry point
│   └── cli.test.js            # Command-line interface
└── integration/               # Real-world automation scenarios
    ├── basic-automation.test.js      # Navigation and DOM interaction
    └── test-suite-integration.test.js # Complete workflows
```

### Running Tests

```bash
# Run all tests with coverage
npm test

# Run specific test category
npm run test:unit
npm run test:integration

# Run with verbose output
npm run test:verbose

# Generate coverage report
npm run test:coverage

# Watch mode for development
npm run test:watch
```

### Mock Development Environment

The test suite includes a complete mock addon that simulates all HeadlessWeb functionality, enabling:

- **Development without C++ compilation** - Write and test JavaScript code immediately
- **Consistent test behavior** - Predictable responses for reliable CI/CD
- **Complete API coverage** - All methods and events are mocked
- **Performance testing** - Mock timing matches real implementation patterns

```javascript
// Mock automatically loaded in Jest environment
const { Browser } = require('@headlessweb/js');

// Works immediately without native compilation
const browser = new Browser({ session: 'test' });
await browser.navigate('https://example.com');
expect(browser.exists('h1')).toBe(true);
```

### Unit Test Examples

#### Browser Class Testing
```javascript
// test/unit/browser.test.js
describe('Browser Class', () => {
    test('should create browser with custom options', () => {
        const browser = createTestBrowser({
            session: 'custom-test',
            headless: false,
            timeout: 45000
        });
        
        expect(browser.session).toBe('custom-test');
        expect(browser.headless).toBe(false);
        expect(browser.timeout).toBe(45000);
    });
    
    test('should handle navigation with events', async () => {
        const events = [];
        browser.on('navigated', (data) => events.push(data));
        
        await browser.navigate('https://example.com');
        
        expect(events).toHaveLength(1);
        expect(events[0].url).toBe('https://example.com');
    });
});
```

#### TestSuite Framework Testing
```javascript
// test/unit/test-suite.test.js
describe('TestSuite Class', () => {
    test('should support method chaining', async () => {
        const test = browser.createTest('Chaining Test');
        
        const result = await test
            .navigate('https://example.com')
            .assertExists('h1')
            .assertText('h1', 'Example Domain')
            .screenshot('chaining-test.png')
            .message('Chaining completed');
        
        expect(result).toBe(test);
        expect(test.steps).toHaveLength(5);
    });
    
    test('should generate comprehensive reports', () => {
        test.message('Test step 1');
        test.message('Test step 2');
        
        const report = test.generateReport();
        
        expect(report).toBeTestReport();
        expect(report.summary.total).toBe(2);
        expect(report.summary.success).toBe(true);
        expect(report.timing.totalTime).toBeGreaterThan(0);
    });
});
```

### Integration Test Examples

#### E-commerce Workflow Testing
```javascript
// test/integration/test-suite-integration.test.js
test('should test an e-commerce shopping flow', async () => {
    const test = browser.createTest('E-commerce Shopping Flow');
    
    await test
        .navigate(dataUrl) // Mock e-commerce page
        .assertExists('h1')
        .assertText('h1', 'Welcome to Test Shop')
        
        // Add items to cart
        .click('.product[data-id="1"] .add-to-cart')
        .wait(100)
        .assertText('#cart-count', '1')
        .message('Added laptop to cart')
        
        .click('.product[data-id="2"] .add-to-cart')
        .wait(100)
        .assertText('#cart-count', '2')
        .message('Added mouse to cart')
        
        // Checkout process
        .screenshot('cart-with-items.png')
        .click('#checkout-btn')
        .wait(100)
        .assertExists('#checkout-success')
        .assertText('#checkout-success h2', 'Order Completed!')
        .screenshot('checkout-success.png')
        .message('Checkout completed successfully');
    
    const report = test.generateReport();
    expect(report.summary.success).toBe(true);
    expect(report.summary.total).toBeGreaterThan(15);
});
```

#### Form Validation Testing
```javascript
test('should test form validation scenarios', async () => {
    const test = browser.createTest('Form Validation Test');
    
    await test
        .navigate(formPageUrl)
        .assertExists('#contact-form')
        
        // Test empty form submission
        .click('#submit-btn')
        .wait(100)
        .assertExists('#name-error')
        .assertExists('#email-error')
        .message('Validated empty form shows errors')
        
        // Test invalid email
        .type('#name', 'John Doe')
        .type('#email', 'invalid-email')
        .click('#submit-btn')
        .wait(100)
        .assertText('#email-error', 'Invalid email format')
        .message('Validated invalid email format')
        
        // Test valid form
        .type('#email', 'john@example.com')
        .click('#submit-btn')
        .wait(100)
        .assertExists('#success-msg')
        .message('Form submitted successfully with valid data');
    
    const report = test.generateReport();
    expect(report.summary.success).toBe(true);
});
```

### CLI Testing

```javascript
// test/unit/cli.test.js
describe('CLI Interface', () => {
    test('should execute multiple commands in sequence', async () => {
        const result = await runCli([
            '--session', 'multi-command',
            '--url', 'data:text/html,<h1>Test</h1>',
            '--get-text', 'h1',
            '--screenshot', 'multi-test.png'
        ]);
        
        expect(result.stdout).toContain('Session: multi-command');
        expect(result.stdout).toContain('Navigating to:');
        expect(result.stdout).toContain('Text:');
        expect(result.stdout).toContain('Taking screenshot:');
    });
});
```

### Custom Jest Matchers

The test suite includes custom matchers for HeadlessWeb-specific testing:

```javascript
// Available custom matchers
expect(report).toBeTestReport();
expect(browser).toBeBrowserInstance();
expect(sessionName).toBeValidSessionName();

// Usage examples
const report = test.generateReport();
expect(report).toBeTestReport();
expect(report.summary.success).toBe(true);

const browser = createTestBrowser();
expect(browser).toBeBrowserInstance();
```

### Performance Testing

```javascript
test('should handle rapid operations', async () => {
    const start = Date.now();
    
    // Perform many fast operations
    for (let i = 0; i < 10; i++) {
        browser.executeJavaScriptSync(`window.testVar${i} = ${i}`);
    }
    
    const elapsed = Date.now() - start;
    expect(elapsed).toBeLessThan(5000); // Should complete quickly
});

test('should track performance metrics', async () => {
    const test = browser.createTest('Performance Test');
    
    await test
        .navigate('about:blank')
        .wait(100)
        .screenshot('performance-test.png');
    
    const report = test.generateReport();
    
    // Timing validation
    expect(report.timing.totalTime).toBeGreaterThan(90);
    report.steps.forEach(step => {
        expect(step.duration).toBeGreaterThanOrEqual(0);
        expect(typeof step.timestamp).toBe('number');
    });
});
```

### Memory Management Testing

```javascript
test('should clean up resources properly', () => {
    const browsers = [];
    
    // Create multiple browsers
    for (let i = 0; i < 5; i++) {
        browsers.push(createTestBrowser({ session: `cleanup-test-${i}` }));
    }
    
    // Destroy all browsers
    browsers.forEach(browser => {
        expect(() => browser.destroy()).not.toThrow();
    });
});
```

### Test Configuration

```javascript
// jest.config.js
module.exports = {
    testEnvironment: 'node',
    setupFilesAfterEnv: ['<rootDir>/test/helpers/setup.js'],
    globalTeardown: '<rootDir>/test/helpers/teardown.js',
    
    // Mock native modules for testing
    moduleNameMapper: {
        '^.*\\.node$': '<rootDir>/test/mocks/addon-mock.js'
    },
    
    // Coverage configuration
    collectCoverageFrom: [
        'lib/**/*.js',
        'index.js',
        '!lib/**/*.test.js',
        '!test/**/*'
    ],
    
    testTimeout: 30000
};
```

### Continuous Integration

```yaml
# .github/workflows/test.yml
name: Test Suite
on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-node@v3
        with:
          node-version: '18'
      
      - name: Install dependencies
        run: npm install
        working-directory: packages/js
      
      - name: Run tests with mock
        run: npm test
        working-directory: packages/js
      
      - name: Upload coverage
        uses: codecov/codecov-action@v3
```

This comprehensive test suite ensures the reliability and maintainability of the HeadlessWeb Node.js integration while providing extensive examples for developers.

## 🔧 Troubleshooting

### Common Issues

**Build fails with "node-gyp not found":**
```bash
npm install -g node-gyp
# or
npm install node-gyp --save-dev
```

**WebKit not found error:**
```bash
# Ubuntu/Debian
sudo apt-get install libwebkitgtk-6.0-dev

# Check installation
pkg-config --exists webkit2gtk-6.0 && echo "WebKit found" || echo "WebKit missing"
```

**Browser crashes or segfaults:**
```bash
# Run with debugging
node --inspect-brk your-script.js

# Check for memory issues
valgrind node your-script.js

# Use smaller test cases
const browser = new Browser({ session: 'minimal-test' });
await browser.navigate('about:blank');
console.log('Basic navigation works');
```

**Tests fail with "Display not found":**
```bash
# Set up virtual display for headless environments
export DISPLAY=:99
Xvfb :99 -screen 0 1024x768x24 &

# Or use truly headless mode
const browser = new Browser({ headless: true });
```

**Session persistence issues:**
```bash
# Check session directory permissions
ls -la ./sessions/

# Clear corrupted sessions
rm -rf ./sessions/problematic-session

# Test session operations
const session = new Session('test-session');
console.log('Exists:', session.exists());
await session.save();
console.log('Saved successfully');
```

### Performance Optimization

```javascript
// Use sync methods for simple queries
const title = browser.getText('h1');           // Fast
const exists = browser.exists('.error');       // Fast

// Use async methods for interactions
await browser.click('#submit');                // Reliable
await browser.type('#input', 'text');         // Reliable

// Batch operations with fluent API
const results = await browser.chain()
    .navigate('https://example.com')
    .click('#button1')
    .click('#button2')
    .execute();                                 // Efficient

// Reuse sessions for related operations
const browser = new Browser({ session: 'workflow' });
// ... multiple operations using same session
```

### CI/CD Integration

**GitHub Actions:**
```yaml
name: HeadlessWeb Tests
on: [push, pull_request]

jobs:
  test:
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
          sudo apt-get install -y libwebkitgtk-6.0-dev xvfb
      
      - name: Build HeadlessWeb
        run: |
          make clean && make
          make js
      
      - name: Run tests
        run: |
          export DISPLAY=:99
          Xvfb :99 -screen 0 1024x768x24 &
          make test-all
```

**Docker:**
```dockerfile
FROM node:18-bullseye

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential cmake pkg-config \
    libgtk-4-dev libwebkitgtk-6.0-dev libjsoncpp-dev \
    xvfb

# Set up virtual display
ENV DISPLAY=:99
RUN echo "Xvfb :99 -screen 0 1024x768x24 &" > /usr/local/bin/start-xvfb.sh \
    && chmod +x /usr/local/bin/start-xvfb.sh

WORKDIR /app
COPY . .

# Build HeadlessWeb
RUN make clean && make && make js

# Start Xvfb and run application
CMD ["/bin/bash", "-c", "start-xvfb.sh && node your-app.js"]
```

---

## 📚 Additional Resources

- **Main Repository**: [HeadlessWeb](https://github.com/user/headlessweb)
- **C++ Documentation**: [C++ API Reference](../README.md)
- **Examples**: [JavaScript Examples](../examples/js/)
- **Blueprint**: [Node.js Integration Blueprint](../spec/NODEJS-INTEGRATION-BLUEPRINT.md)
- **Issues**: [GitHub Issues](https://github.com/user/headlessweb/issues)

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/new-feature`
3. Make changes to both C++ core and JS bindings as needed
4. Run tests: `make test-all`
5. Submit a pull request

For Node.js specific contributions:
- Test changes with `make js-test`
- Update TypeScript definitions if needed
- Add examples for new features
- Update this documentation

---

**Built with ❤️ by the HeadlessWeb team**