# HeadlessWeb Node.js Integration Blueprint

**Date**: January 2025  
**Status**: 🚧 **DESIGN PHASE**  
**Goal**: Create npm package for HeadlessWeb C++ library with sync/async JavaScript API

## 🎯 Project Overview

Transform HeadlessWeb from CLI-only to a full-featured Node.js package that can be used:
- **Programmatically** in Node.js applications
- **Command-line** via npm global install
- **Testing frameworks** (Jest, Mocha, Playwright alternatives)
- **CI/CD pipelines** and automation scripts

## 📁 Repository Structure

```
headlessweb/                    # Root (existing C++ project)
├── src/                       # Existing C++ core
├── tests/                     # Existing C++ tests
├── bindings/                  # NEW: Language bindings
│   ├── node/                  # Node.js specific bindings
│   │   ├── binding.gyp        # Node-gyp build configuration
│   │   ├── hweb_addon.cpp     # C++ ↔ Node.js bridge
│   │   ├── session_wrapper.cpp
│   │   ├── browser_wrapper.cpp
│   │   └── utils.cpp
│   ├── python/                # FUTURE: Python bindings
│   └── go/                    # FUTURE: Go bindings
├── packages/                  # NEW: Language packages
│   └── js/                    # npm package directory
│       ├── package.json       # npm package configuration
│       ├── index.js           # Main API entry point
│       ├── lib/
│       │   ├── browser.js     # Browser class wrapper
│       │   ├── session.js     # Session management
│       │   ├── assertions.js  # Testing utilities
│       │   ├── file-ops.js    # Upload/download operations
│       │   └── utils.js       # Helper functions
│       ├── bin/
│       │   └── hweb-js        # CLI interface
│       ├── test/
│       │   ├── unit/          # Unit tests for JS API
│       │   ├── integration/   # Integration tests
│       │   └── examples/      # Example test suites
│       └── types/
│           └── index.d.ts     # TypeScript definitions
├── examples/                  # Enhanced examples
│   ├── cpp/                   # Existing C++ examples
│   └── js/                    # NEW: JavaScript examples
│       ├── basic-usage.js
│       ├── testing-suite.js
│       ├── automation.js
│       └── performance.js
└── docs/
    ├── js-api.md             # JavaScript API documentation
    └── integration-guide.md  # Setup and usage guide
```

## 🔧 Technical Architecture

### 1. Native Binding Layer (C++)

**File: `bindings/node/hweb_addon.cpp`**
```cpp
#include <napi.h>
#include "../../src/Browser/Browser.h"
#include "../../src/Session/Manager.h"

class BrowserWrapper : public Napi::ObjectWrap<BrowserWrapper> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    BrowserWrapper(const Napi::CallbackInfo& info);
    
private:
    static Napi::FunctionReference constructor;
    
    // Async methods
    Napi::Value NavigateAsync(const Napi::CallbackInfo& info);
    Napi::Value ClickAsync(const Napi::CallbackInfo& info);
    Napi::Value TypeAsync(const Napi::CallbackInfo& info);
    
    // Sync methods
    Napi::Value NavigateSync(const Napi::CallbackInfo& info);
    Napi::Value GetText(const Napi::CallbackInfo& info);
    Napi::Value ElementExists(const Napi::CallbackInfo& info);
    
    std::unique_ptr<Browser> browser_;
};
```

**Build Configuration: `bindings/node/binding.gyp`**
```json
{
  "targets": [
    {
      "target_name": "hweb_addon",
      "sources": [
        "hweb_addon.cpp",
        "browser_wrapper.cpp",
        "session_wrapper.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../src"
      ],
      "libraries": [
        "-L../../build",
        "-lhweb_core"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ]
    }
  ]
}
```

### 2. JavaScript API Layer

**Main Entry: `packages/js/index.js`**
```javascript
const addon = require('../../build/Release/hweb_addon');
const Browser = require('./lib/browser');
const Session = require('./lib/session');

module.exports = {
  Browser,
  Session,
  
  // Convenience factory
  create: (options = {}) => new Browser(options),
  
  // Version info
  version: require('./package.json').version,
  coreVersion: addon.getCoreVersion()
};
```

**Browser Class: `packages/js/lib/browser.js`**
```javascript
const EventEmitter = require('events');
const addon = require('../../../build/Release/hweb_addon');

class Browser extends EventEmitter {
  constructor(options = {}) {
    super();
    this.session = options.session || 'default';
    this.headless = options.headless !== false;
    this.timeout = options.timeout || 30000;
    
    // Create native browser instance
    this._browser = new addon.Browser({
      session: this.session,
      headless: this.headless
    });
  }
  
  // Navigation
  async navigate(url) {
    return new Promise((resolve, reject) => {
      this._browser.navigateAsync(url, (err, result) => {
        if (err) reject(new Error(err));
        else resolve(result);
      });
    });
  }
  
  // DOM Interaction
  async click(selector) {
    return new Promise((resolve, reject) => {
      this._browser.clickAsync(selector, (err, result) => {
        if (err) reject(new Error(err));
        else resolve(result);
      });
    });
  }
  
  async type(selector, text) {
    return new Promise((resolve, reject) => {
      this._browser.typeAsync(selector, text, (err, result) => {
        if (err) reject(new Error(err));
        else resolve(result);
      });
    });
  }
  
  // Data Extraction (Sync - these are fast)
  getText(selector) {
    return this._browser.getText(selector);
  }
  
  exists(selector) {
    return this._browser.elementExists(selector);
  }
  
  getAttribute(selector, attribute) {
    return this._browser.getAttribute(selector, attribute);
  }
  
  // Screenshots
  async screenshot(filename = 'screenshot.png') {
    return new Promise((resolve, reject) => {
      this._browser.screenshotAsync(filename, (err, result) => {
        if (err) reject(new Error(err));
        else resolve(result);
      });
    });
  }
  
  // Fluent API
  async chain() {
    return new BrowserChain(this);
  }
  
  // Testing utilities
  createTest(name) {
    return new TestSuite(this, name);
  }
}

// Fluent API for chaining operations
class BrowserChain {
  constructor(browser) {
    this.browser = browser;
    this.operations = [];
  }
  
  navigate(url) {
    this.operations.push(['navigate', url]);
    return this;
  }
  
  click(selector) {
    this.operations.push(['click', selector]);
    return this;
  }
  
  type(selector, text) {
    this.operations.push(['type', selector, text]);
    return this;
  }
  
  wait(ms) {
    this.operations.push(['wait', ms]);
    return this;
  }
  
  async execute() {
    const results = [];
    for (const [method, ...args] of this.operations) {
      if (method === 'wait') {
        await new Promise(resolve => setTimeout(resolve, args[0]));
      } else {
        const result = await this.browser[method](...args);
        results.push(result);
      }
    }
    return results;
  }
}

module.exports = Browser;
```

### 3. Testing Integration

**Test Suite: `packages/js/lib/assertions.js`**
```javascript
class TestSuite {
  constructor(browser, name) {
    this.browser = browser;
    this.name = name;
    this.steps = [];
    this.results = [];
  }
  
  async navigate(url) {
    await this.browser.navigate(url);
    this.steps.push({ action: 'navigate', url, success: true });
    return this;
  }
  
  async assertExists(selector) {
    const exists = this.browser.exists(selector);
    const success = exists === true;
    this.steps.push({ 
      action: 'assertExists', 
      selector, 
      success,
      message: success ? 'Element found' : 'Element not found'
    });
    if (!success) throw new Error(`Element not found: ${selector}`);
    return this;
  }
  
  async assertText(selector, expected) {
    const actual = this.browser.getText(selector);
    const success = actual.includes(expected);
    this.steps.push({
      action: 'assertText',
      selector,
      expected,
      actual,
      success,
      message: success ? 'Text match' : `Expected "${expected}", got "${actual}"`
    });
    if (!success) throw new Error(`Text mismatch: expected "${expected}", got "${actual}"`);
    return this;
  }
  
  generateReport() {
    const passed = this.steps.filter(s => s.success).length;
    const total = this.steps.length;
    
    return {
      name: this.name,
      passed,
      total,
      success: passed === total,
      steps: this.steps
    };
  }
}

module.exports = TestSuite;
```

## 📦 NPM Package Configuration

**File: `packages/js/package.json`**
```json
{
  "name": "@headlessweb/js",
  "version": "1.0.0",
  "description": "Node.js bindings for HeadlessWeb browser automation",
  "main": "index.js",
  "types": "types/index.d.ts",
  "bin": {
    "hweb-js": "./bin/hweb-js"
  },
  "scripts": {
    "install": "node-gyp rebuild",
    "test": "jest",
    "test:integration": "jest test/integration",
    "docs": "jsdoc -d docs lib/*.js",
    "build": "node-gyp build",
    "clean": "node-gyp clean"
  },
  "dependencies": {
    "node-addon-api": "^7.0.0"
  },
  "devDependencies": {
    "node-gyp": "^10.0.0",
    "jest": "^29.0.0",
    "@types/node": "^20.0.0",
    "jsdoc": "^4.0.0"
  },
  "engines": {
    "node": ">=16.0.0"
  },
  "os": ["linux", "darwin"],
  "keywords": [
    "browser",
    "automation",
    "testing",
    "headless",
    "webkit",
    "scraping"
  ],
  "repository": {
    "type": "git",
    "url": "https://github.com/user/headlessweb.git",
    "directory": "packages/js"
  },
  "gypfile": true,
  "files": [
    "index.js",
    "lib/**/*",
    "bin/**/*",
    "types/**/*",
    "../../bindings/node/**/*",
    "../../build/Release/hweb_addon.node"
  ]
}
```

## 🚀 Usage Examples

### Basic Automation
```javascript
const { Browser } = require('@headlessweb/js');

async function example() {
  const browser = new Browser({ session: 'my-session' });
  
  await browser.navigate('https://example.com');
  await browser.type('#search', 'hello world');
  await browser.click('#submit');
  
  const title = browser.getText('h1');
  console.log('Page title:', title);
  
  await browser.screenshot('result.png');
}
```

### Testing Framework Integration
```javascript
const { Browser } = require('@headlessweb/js');

describe('Website Tests', () => {
  let browser;
  
  beforeEach(async () => {
    browser = new Browser({ session: 'test-session' });
  });
  
  test('login flow', async () => {
    const test = browser.createTest('login-flow');
    
    await test
      .navigate('https://app.com/login')
      .type('#username', 'testuser')
      .type('#password', 'testpass')
      .click('#login')
      .assertExists('.dashboard')
      .assertText('.welcome', 'Welcome testuser');
      
    const report = test.generateReport();
    expect(report.success).toBe(true);
  });
});
```

### Fluent API Chaining
```javascript
const browser = new Browser();

const results = await browser.chain()
  .navigate('https://ecommerce.com')
  .click('.product[data-id="123"]')
  .click('#add-to-cart')
  .wait(1000)
  .click('.cart-icon')
  .execute();
```

## 🔧 Build System Integration

### Enhanced Makefile
```makefile
# Existing C++ targets
all: hweb
test: run_tests

# NEW: Node.js targets
js: js-build
js-build:
	cd packages/js && npm install

js-test:
	cd packages/js && npm test

js-publish:
	cd packages/js && npm publish

js-clean:
	cd packages/js && npm run clean

# Combined targets
build-all: all js-build
test-all: test js-test
clean-all: clean js-clean

.PHONY: js js-build js-test js-publish js-clean build-all test-all clean-all
```

### GitHub Actions CI
```yaml
# .github/workflows/nodejs.yml
name: Node.js Integration

on: [push, pull_request]

jobs:
  nodejs:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        node-version: [16, 18, 20]
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup Node.js
      uses: actions/setup-node@v3
      with:
        node-version: ${{ matrix.node-version }}
    
    - name: Install C++ dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential cmake pkg-config \
          libgtk-4-dev libwebkitgtk-6.0-dev libjsoncpp-dev
    
    - name: Build C++ core
      run: make clean && make
    
    - name: Build Node.js bindings
      run: cd packages/js && npm install
    
    - name: Run JavaScript tests
      run: cd packages/js && npm test
```

## 📚 Documentation Structure

### API Documentation
```markdown
# HeadlessWeb JavaScript API

## Installation
```bash
npm install @headlessweb/js
```

## Quick Start
```javascript
const { Browser } = require('@headlessweb/js');
const browser = new Browser();
await browser.navigate('https://example.com');
```

## API Reference
- [Browser Class](api/browser.md)
- [Session Management](api/session.md)
- [Testing Utilities](api/testing.md)
- [File Operations](api/file-ops.md)
```

## 🎯 Implementation Phases

### Phase 1: Foundation (Week 1)
- [ ] Set up directory structure
- [ ] Create basic binding.gyp configuration
- [ ] Implement minimal C++ ↔ Node.js bridge
- [ ] Basic Browser class with navigate/click/type
- [ ] Simple examples and tests

### Phase 2: Core Features (Week 2)
- [ ] Complete DOM interaction methods
- [ ] Session management integration
- [ ] Error handling and validation
- [ ] Screenshot and file operations
- [ ] Comprehensive test suite

### Phase 3: Advanced Features (Week 3)
- [ ] Fluent API and chaining
- [ ] Testing framework integration
- [ ] TypeScript definitions
- [ ] Performance optimizations
- [ ] Documentation and examples

### Phase 4: Release (Week 4)
- [ ] CI/CD pipeline setup
- [ ] npm package preparation
- [ ] Final documentation
- [ ] Community examples
- [ ] Public release

## 🔍 Success Criteria

1. **Functionality**: All C++ features accessible via JavaScript
2. **Performance**: Minimal overhead compared to direct C++ usage
3. **Usability**: Intuitive API following Node.js conventions
4. **Reliability**: 100% test coverage with both unit and integration tests
5. **Documentation**: Comprehensive guides and examples
6. **Ecosystem**: Easy integration with popular testing frameworks

## 🚨 Risk Mitigation

### Memory Management
- Use RAII patterns in C++ bindings
- Proper cleanup of Browser instances
- Handle Node.js garbage collection correctly

### Platform Compatibility
- Support Linux and macOS initially
- Windows support as future enhancement
- Pre-compiled binaries for popular platforms

### API Stability
- Version C++ core and Node.js package together
- Use semantic versioning
- Deprecation warnings for breaking changes

---

**Next Steps**: Begin Phase 1 implementation with basic directory structure and minimal binding layer.