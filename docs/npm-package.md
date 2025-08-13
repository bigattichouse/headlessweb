# @headlessweb/js NPM Package

**Complete guide to the HeadlessWeb npm package for browser automation**

## 📦 Package Information

- **Package Name**: `@headlessweb/js`
- **Current Version**: `1.0.0`
- **License**: MIT
- **Node.js Support**: 16+
- **Platform Support**: Linux, macOS
- **TypeScript Support**: ✅ Full definitions included

## 🚀 Installation

### Quick Installation

```bash
# Install the package
npm install @headlessweb/js

# Install build dependencies
npm install -g node-gyp

# Verify installation
node -e "console.log(require('@headlessweb/js').version)"
```

### System Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update && sudo apt-get install -y \
  build-essential cmake pkg-config \
  libgtk-4-dev libwebkitgtk-6.0-dev libjsoncpp-dev
```

**macOS:**
```bash
brew install cmake pkg-config gtk4
```

### Installation Verification

```javascript
// test-installation.js
const HeadlessWeb = require('@headlessweb/js');

console.log('Package version:', HeadlessWeb.version);
console.log('Core version:', HeadlessWeb.coreVersion);
console.log('System info:', HeadlessWeb.getSystemInfo());

// Test basic functionality
const browser = HeadlessWeb.create({ session: 'test' });
console.log('✅ Installation successful!');
```

## 📋 Package Structure

```
@headlessweb/js/
├── index.js              # Main entry point
├── lib/
│   ├── browser.js         # Browser automation class
│   ├── session.js         # Session management
│   └── test-suite.js      # Testing framework
├── bin/
│   └── hweb-js           # CLI interface
├── types/
│   └── index.d.ts        # TypeScript definitions
└── package.json          # Package configuration
```

## 🎯 Usage Patterns

### CommonJS (Default)

```javascript
const { Browser, Session, TestSuite } = require('@headlessweb/js');

// Or use factory method
const HeadlessWeb = require('@headlessweb/js');
const browser = HeadlessWeb.create({ session: 'my-session' });
```

### ES Modules

```javascript
// If using ES modules (Node.js 14+ with "type": "module")
import { Browser, Session, TestSuite } from '@headlessweb/js';

// Or default import
import HeadlessWeb from '@headlessweb/js';
const browser = HeadlessWeb.create();
```

### TypeScript

```typescript
import { Browser, BrowserOptions, TestReport } from '@headlessweb/js';

const options: BrowserOptions = {
    session: 'typed-session',
    headless: true,
    timeout: 30000
};

const browser = new Browser(options);

// Fully typed API
const result: boolean = await browser.navigate('https://example.com');
const text: string = browser.getText('h1');
```

## 🔧 Configuration

### Browser Configuration

```javascript
const browser = new Browser({
    // Session management
    session: 'my-app',              // Session name
    
    // Display settings
    headless: true,                 // Run in headless mode
    width: 1920,                    // Viewport width
    height: 1080,                   // Viewport height
    
    // Timeouts
    timeout: 30000,                 // Default timeout (ms)
});
```

### Environment Variables

```bash
# Optional environment configuration
export HEADLESSWEB_SESSION_DIR="/custom/sessions"
export HEADLESSWEB_SCREENSHOTS_DIR="/custom/screenshots"
export HEADLESSWEB_DEFAULT_TIMEOUT="45000"
export HEADLESSWEB_LOG_LEVEL="debug"
```

### Package.json Configuration

```json
{
  "scripts": {
    "test:browser": "hweb-js --url https://example.com --screenshot",
    "test:e2e": "jest --testPathPattern=e2e",
    "automation": "node scripts/automation.js"
  },
  "dependencies": {
    "@headlessweb/js": "^1.0.0"
  },
  "devDependencies": {
    "@types/node": "^20.0.0",
    "jest": "^29.0.0"
  }
}
```

## 📱 CLI Usage

### Global Installation

```bash
# Install globally for CLI access
npm install -g @headlessweb/js

# Use CLI anywhere
hweb-js --url https://example.com --screenshot example.png
```

### Local Usage

```bash
# Use locally installed version
npx hweb-js --url https://example.com --click "#submit"

# Or via package.json scripts
npm run test:browser
```

### CLI Examples

```bash
# Basic navigation and screenshot
hweb-js --url https://example.com --screenshot

# Form interaction
hweb-js --session login \
  --url https://app.com/login \
  --type "#username" "user@example.com" \
  --type "#password" "secretpass" \
  --click "#login"

# JavaScript execution
hweb-js --url https://api.com/data \
  --js "JSON.stringify(document.body.textContent)"

# Chain multiple operations
hweb-js --session workflow \
  --url https://example.com \
  --click ".accept-cookies" \
  --click "#menu" \
  --screenshot "menu-open.png"
```

## 🧪 Testing Integration

### Jest Integration

```javascript
// jest.config.js
module.exports = {
    testEnvironment: 'node',
    setupFilesAfterEnv: ['<rootDir>/test/setup.js'],
    testTimeout: 30000,
    globalTeardown: '<rootDir>/test/teardown.js'
};
```

```javascript
// test/setup.js
const { Browser } = require('@headlessweb/js');

// Global test browser
global.createTestBrowser = (sessionSuffix = '') => {
    return new Browser({ 
        session: `test-${Date.now()}-${sessionSuffix}`,
        headless: true 
    });
};

// Cleanup helper
global.cleanupBrowser = (browser) => {
    if (browser) {
        browser.destroy();
    }
};
```

```javascript
// test/example.test.js
describe('Website Tests', () => {
    let browser;

    beforeEach(() => {
        browser = createTestBrowser('website');
    });

    afterEach(() => {
        cleanupBrowser(browser);
    });

    test('should load homepage', async () => {
        await browser.navigate('https://example.com');
        expect(browser.exists('h1')).toBe(true);
    });
});
```

### Mocha Integration

```javascript
// test/mocha.opts (or .mocharc.json)
{
    "timeout": 30000,
    "recursive": true,
    "require": "test/setup.js"
}
```

```javascript
// test/setup.js
const { Browser } = require('@headlessweb/js');

// Mocha hooks for browser cleanup
let activeBrowsers = [];

beforeEach(function() {
    this.createBrowser = (options = {}) => {
        const browser = new Browser({
            session: `mocha-${Date.now()}`,
            ...options
        });
        activeBrowsers.push(browser);
        return browser;
    };
});

afterEach(function() {
    activeBrowsers.forEach(browser => browser.destroy());
    activeBrowsers = [];
});
```

### Playwright-style Testing

```javascript
// test/playwright-style.js
const { Browser } = require('@headlessweb/js');

async function test(name, testFn) {
    console.log(`Running: ${name}`);
    const browser = new Browser({ session: `test-${Date.now()}` });
    
    try {
        await testFn(browser);
        console.log(`✅ ${name} PASSED`);
    } catch (error) {
        console.log(`❌ ${name} FAILED: ${error.message}`);
        throw error;
    } finally {
        browser.destroy();
    }
}

// Usage
await test('should navigate to homepage', async (browser) => {
    await browser.navigate('https://example.com');
    const title = browser.getText('h1');
    if (!title.includes('Example')) {
        throw new Error(`Expected title to contain 'Example', got: ${title}`);
    }
});
```

## 🔗 Framework Integration

### Express.js Integration

```javascript
// server.js
const express = require('express');
const { Browser } = require('@headlessweb/js');

const app = express();
const browser = new Browser({ session: 'server' });

app.get('/screenshot', async (req, res) => {
    try {
        const { url } = req.query;
        await browser.navigate(url);
        await browser.screenshot('temp-screenshot.png');
        res.sendFile(path.join(__dirname, 'temp-screenshot.png'));
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

app.get('/extract', async (req, res) => {
    try {
        const { url, selector } = req.query;
        await browser.navigate(url);
        const text = browser.getText(selector);
        res.json({ text });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

app.listen(3000, () => {
    console.log('HeadlessWeb server running on port 3000');
});
```

### Next.js API Routes

```javascript
// pages/api/browser/[...action].js
import { Browser } from '@headlessweb/js';

const browser = new Browser({ session: 'nextjs-api' });

export default async function handler(req, res) {
    const { action } = req.query;
    
    try {
        switch (action[0]) {
            case 'navigate':
                const result = await browser.navigate(req.body.url);
                res.json({ success: result });
                break;
                
            case 'extract':
                const text = browser.getText(req.body.selector);
                res.json({ text });
                break;
                
            case 'screenshot':
                await browser.screenshot('api-screenshot.png');
                res.json({ screenshot: 'api-screenshot.png' });
                break;
                
            default:
                res.status(400).json({ error: 'Unknown action' });
        }
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
}
```

### Electron Integration

```javascript
// main.js (Electron main process)
const { app, BrowserWindow, ipcMain } = require('electron');
const { Browser } = require('@headlessweb/js');

let headlessBrowser;

app.whenReady().then(() => {
    headlessBrowser = new Browser({ session: 'electron-app' });
    
    // IPC handlers for renderer process
    ipcMain.handle('browser-navigate', async (event, url) => {
        return await headlessBrowser.navigate(url);
    });
    
    ipcMain.handle('browser-extract', async (event, selector) => {
        return headlessBrowser.getText(selector);
    });
    
    createWindow();
});

app.on('window-all-closed', () => {
    if (headlessBrowser) {
        headlessBrowser.destroy();
    }
    app.quit();
});
```

```javascript
// renderer.js (Electron renderer process)
const { ipcRenderer } = require('electron');

async function automateWebsite() {
    await ipcRenderer.invoke('browser-navigate', 'https://example.com');
    const title = await ipcRenderer.invoke('browser-extract', 'h1');
    console.log('Page title:', title);
}
```

## 📊 Performance Considerations

### Memory Management

```javascript
// Good: Reuse browser instances
const browser = new Browser({ session: 'app' });

async function processUrls(urls) {
    for (const url of urls) {
        await browser.navigate(url);
        const data = browser.getText('body');
        // Process data...
    }
}

// Better: Clean up when done
try {
    await processUrls(['url1', 'url2', 'url3']);
} finally {
    browser.destroy();
}
```

```javascript
// Bad: Creating new browsers repeatedly
async function processUrls(urls) {
    for (const url of urls) {
        const browser = new Browser(); // Memory leak!
        await browser.navigate(url);
        // No cleanup
    }
}
```

### Performance Optimization

```javascript
// Use sync methods for fast queries
const title = browser.getText('h1');           // ~1ms
const exists = browser.exists('.error');       // ~1ms

// Use async methods for reliable interactions
await browser.click('#submit');                // ~100ms but reliable
await browser.type('#input', 'text');         // ~200ms but reliable

// Batch operations efficiently
const results = await browser.chain()
    .navigate('https://example.com')           // 2000ms
    .click('#button1')                         // 100ms
    .click('#button2')                         // 100ms
    .execute();                                // Total: ~2200ms

// vs individual operations: ~2400ms
```

### Session Strategy

```javascript
// Strategy 1: One session per user workflow
const loginBrowser = new Browser({ session: 'user-login' });
const shoppingBrowser = new Browser({ session: 'user-shopping' });

// Strategy 2: Temporary sessions for testing
const testBrowser = new Browser({ session: `test-${Date.now()}` });

// Strategy 3: Shared session for related operations
const workflowBrowser = new Browser({ session: 'daily-workflow' });
```

## 🔒 Security Considerations

### Secure Credential Handling

```javascript
// Good: Use environment variables
const browser = new Browser({ session: 'secure-login' });

await browser.navigate('https://app.com/login');
await browser.type('#username', process.env.APP_USERNAME);
await browser.type('#password', process.env.APP_PASSWORD);
await browser.click('#login');
```

```javascript
// Better: Use secure credential stores
const keytar = require('keytar');

const username = await keytar.getPassword('myapp', 'username');
const password = await keytar.getPassword('myapp', 'password');

await browser.type('#username', username);
await browser.type('#password', password);
```

### Session Isolation

```javascript
// Isolate sensitive operations
const sensitiveSession = `secure-${Date.now()}-${Math.random()}`;
const browser = new Browser({ session: sensitiveSession });

try {
    // Perform sensitive operations
    await browser.navigate('https://banking.com');
    // ...
} finally {
    // Clean up immediately
    browser.destroy();
    const session = new Session(sensitiveSession);
    await session.delete();
}
```

### Input Sanitization

```javascript
// Sanitize user inputs
function sanitizeInput(input) {
    return input.replace(/[<>'"]/g, '');
}

const userQuery = sanitizeInput(req.body.query);
await browser.type('#search', userQuery);
```

## 🚀 Deployment

### Docker Deployment

```dockerfile
FROM node:18-slim

# Install system dependencies
RUN apt-get update && apt-get install -y \
    libwebkitgtk-6.0-dev \
    xvfb \
    && rm -rf /var/lib/apt/lists/*

# Set up virtual display
ENV DISPLAY=:99
RUN echo '#!/bin/bash\nXvfb :99 -screen 0 1024x768x24 &\nexec "$@"' > /entrypoint.sh \
    && chmod +x /entrypoint.sh

WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production

COPY . .

ENTRYPOINT ["/entrypoint.sh"]
CMD ["node", "app.js"]
```

### Heroku Deployment

```json
{
  "name": "headlessweb-app",
  "description": "Browser automation with HeadlessWeb",
  "image": "heroku/nodejs",
  "addons": [],
  "buildpacks": [
    {
      "url": "https://github.com/heroku/heroku-buildpack-apt"
    },
    {
      "url": "heroku/nodejs"
    }
  ],
  "env": {
    "NODE_ENV": "production",
    "DISPLAY": ":99"
  }
}
```

```
# Aptfile (for heroku-buildpack-apt)
libwebkitgtk-6.0-dev
xvfb
```

### AWS Lambda

```javascript
// lambda-handler.js
const { Browser } = require('@headlessweb/js');

exports.handler = async (event) => {
    // Note: Lambda requires custom runtime for WebKit
    const browser = new Browser({ 
        session: `lambda-${event.requestContext.requestId}`,
        headless: true 
    });
    
    try {
        await browser.navigate(event.url);
        const title = browser.getText('title');
        
        return {
            statusCode: 200,
            body: JSON.stringify({ title })
        };
    } catch (error) {
        return {
            statusCode: 500,
            body: JSON.stringify({ error: error.message })
        };
    } finally {
        browser.destroy();
    }
};
```

## 📈 Monitoring & Observability

### Performance Monitoring

```javascript
const { Browser } = require('@headlessweb/js');

class MonitoredBrowser extends Browser {
    constructor(options) {
        super(options);
        this.metrics = {
            navigations: 0,
            clicks: 0,
            totalTime: 0
        };
        
        this.on('navigated', () => this.metrics.navigations++);
        this.on('clicked', () => this.metrics.clicks++);
    }
    
    async navigate(url) {
        const start = Date.now();
        const result = await super.navigate(url);
        this.metrics.totalTime += Date.now() - start;
        return result;
    }
    
    getMetrics() {
        return { ...this.metrics };
    }
}

// Usage
const browser = new MonitoredBrowser({ session: 'monitored' });
await browser.navigate('https://example.com');
console.log('Metrics:', browser.getMetrics());
```

### Error Tracking

```javascript
const Sentry = require('@sentry/node');

Sentry.init({ dsn: 'your-sentry-dsn' });

const browser = new Browser({ session: 'production' });

browser.on('error', (error) => {
    Sentry.captureException(error, {
        tags: {
            component: 'headlessweb',
            session: browser.session
        }
    });
});

// Wrap operations for error tracking
async function safeNavigate(url) {
    try {
        return await browser.navigate(url);
    } catch (error) {
        Sentry.captureException(error, {
            extra: { url, operation: 'navigate' }
        });
        throw error;
    }
}
```

### Health Checks

```javascript
// health-check.js
const { Browser } = require('@headlessweb/js');

async function healthCheck() {
    const browser = new Browser({ session: `health-${Date.now()}` });
    
    try {
        // Test basic functionality
        await browser.navigate('about:blank');
        const result = browser.executeJavaScriptSync('true');
        
        if (result === 'true') {
            return { status: 'healthy', timestamp: Date.now() };
        } else {
            throw new Error('JavaScript execution failed');
        }
    } catch (error) {
        return { 
            status: 'unhealthy', 
            error: error.message, 
            timestamp: Date.now() 
        };
    } finally {
        browser.destroy();
    }
}

// Express health endpoint
app.get('/health', async (req, res) => {
    const health = await healthCheck();
    res.status(health.status === 'healthy' ? 200 : 503).json(health);
});
```

## 🎓 Best Practices

### 1. Resource Management

```javascript
// ✅ Good: Always clean up
async function automationTask() {
    const browser = new Browser({ session: 'task' });
    try {
        await browser.navigate('https://example.com');
        // ... perform operations
    } finally {
        browser.destroy(); // Always clean up
    }
}

// ✅ Better: Use try-with-resources pattern
async function withBrowser(sessionName, callback) {
    const browser = new Browser({ session: sessionName });
    try {
        return await callback(browser);
    } finally {
        browser.destroy();
    }
}

await withBrowser('my-task', async (browser) => {
    await browser.navigate('https://example.com');
    return browser.getText('h1');
});
```

### 2. Error Handling

```javascript
// ✅ Graceful error handling
async function robustAutomation() {
    const browser = new Browser({ session: 'robust' });
    
    try {
        await browser.navigate('https://example.com');
        
        // Check if element exists before interacting
        if (browser.exists('#submit')) {
            await browser.click('#submit');
        } else {
            console.log('Submit button not found, skipping...');
        }
        
        // Verify expected state
        if (!browser.exists('.success')) {
            throw new Error('Operation did not complete successfully');
        }
        
    } catch (error) {
        // Take screenshot for debugging
        await browser.screenshot('error-state.png');
        throw error;
    } finally {
        browser.destroy();
    }
}
```

### 3. Testing Patterns

```javascript
// ✅ Page Object Model
class LoginPage {
    constructor(browser) {
        this.browser = browser;
    }
    
    async navigate() {
        await this.browser.navigate('https://app.com/login');
    }
    
    async login(username, password) {
        await this.browser.type('#username', username);
        await this.browser.type('#password', password);
        await this.browser.click('#login');
    }
    
    isLoggedIn() {
        return this.browser.exists('.user-menu');
    }
}

// Usage in tests
test('should login successfully', async () => {
    const browser = createTestBrowser();
    const loginPage = new LoginPage(browser);
    
    await loginPage.navigate();
    await loginPage.login('user@test.com', 'password');
    
    expect(loginPage.isLoggedIn()).toBe(true);
});
```

### 4. Configuration Management

```javascript
// ✅ Centralized configuration
const config = {
    browser: {
        headless: process.env.NODE_ENV === 'production',
        timeout: parseInt(process.env.BROWSER_TIMEOUT) || 30000,
        width: parseInt(process.env.BROWSER_WIDTH) || 1920,
        height: parseInt(process.env.BROWSER_HEIGHT) || 1080
    },
    
    sessions: {
        directory: process.env.SESSION_DIR || './sessions',
        cleanup: process.env.CLEANUP_SESSIONS === 'true'
    },
    
    urls: {
        base: process.env.BASE_URL || 'https://example.com',
        login: process.env.LOGIN_URL || 'https://example.com/login'
    }
};

function createBrowser(sessionName) {
    return new Browser({
        session: sessionName,
        ...config.browser
    });
}
```

---

## 🔗 Related Documentation

- **Main Documentation**: [Node.js Integration Guide](nodejs.md)
- **API Reference**: [TypeScript Definitions](../packages/js/types/index.d.ts)
- **Examples**: [JavaScript Examples](../examples/js/)
- **C++ Core**: [Main README](../README.md)

---

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/user/headlessweb/issues)
- **Discussions**: [GitHub Discussions](https://github.com/user/headlessweb/discussions)
- **Documentation**: [Full Documentation](nodejs.md)

---

**Happy Automating! 🚀**