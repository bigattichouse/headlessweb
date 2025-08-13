const EventEmitter = require('events');
const path = require('path');

// Load the native addon
const addon = require(path.join(__dirname, '../../../build/Release/hweb_addon.node'));

/**
 * Browser automation class providing both sync and async methods
 * 
 * @example
 * const browser = new Browser({ session: 'my-session' });
 * await browser.navigate('https://example.com');
 * await browser.click('#submit');
 * const title = browser.getText('h1');
 */
class Browser extends EventEmitter {
    /**
     * Create a new Browser instance
     * @param {Object} options - Configuration options
     * @param {string} [options.session='default'] - Session name for persistence
     * @param {boolean} [options.headless=true] - Run in headless mode
     * @param {number} [options.timeout=30000] - Default timeout in milliseconds
     * @param {number} [options.width=1024] - Browser width
     * @param {number} [options.height=768] - Browser height
     */
    constructor(options = {}) {
        super();
        
        this.session = options.session || 'default';
        this.headless = options.headless !== false;
        this.timeout = options.timeout || 30000;
        this.width = options.width || 1024;
        this.height = options.height || 768;
        
        // Create native browser instance
        this._browser = new addon.Browser({
            session: this.session,
            headless: this.headless
        });
        
        this.emit('created', { session: this.session });
    }
    
    // ========== Navigation Methods ==========
    
    /**
     * Navigate to a URL (async)
     * @param {string} url - The URL to navigate to
     * @returns {Promise<boolean>} - True if navigation succeeded
     */
    async navigate(url) {
        return new Promise((resolve, reject) => {
            this._browser.loadUriAsync(url, (err, result) => {
                if (err) {
                    this.emit('error', new Error(err));
                    reject(new Error(err));
                } else {
                    this.emit('navigated', { url });
                    resolve(result);
                }
            });
        });
    }
    
    /**
     * Navigate to a URL (sync)
     * @param {string} url - The URL to navigate to
     * @returns {boolean} - True if navigation succeeded
     */
    navigateSync(url) {
        const result = this._browser.loadUri(url);
        if (result) {
            this.emit('navigated', { url });
        }
        return result;
    }
    
    /**
     * Get the current URL
     * @returns {string} - Current URL
     */
    getCurrentUrl() {
        return this._browser.getCurrentUrl();
    }
    
    // ========== DOM Interaction Methods (Async) ==========
    
    /**
     * Click an element (async)
     * @param {string} selector - CSS selector for the element
     * @returns {Promise<boolean>} - True if click succeeded
     */
    async click(selector) {
        return new Promise((resolve, reject) => {
            this._browser.clickElementAsync(selector, (err, result) => {
                if (err) {
                    this.emit('error', new Error(err));
                    reject(new Error(err));
                } else {
                    this.emit('clicked', { selector });
                    resolve(result);
                }
            });
        });
    }
    
    /**
     * Type text into an input element (async)
     * @param {string} selector - CSS selector for the input element
     * @param {string} text - Text to type
     * @returns {Promise<boolean>} - True if typing succeeded
     */
    async type(selector, text) {
        return new Promise((resolve, reject) => {
            this._browser.fillInputAsync(selector, text, (err, result) => {
                if (err) {
                    this.emit('error', new Error(err));
                    reject(new Error(err));
                } else {
                    this.emit('typed', { selector, text });
                    resolve(result);
                }
            });
        });
    }
    
    /**
     * Execute JavaScript code (async)
     * @param {string} code - JavaScript code to execute
     * @returns {Promise<string>} - Result of the JavaScript execution
     */
    async executeJavaScript(code) {
        return new Promise((resolve, reject) => {
            this._browser.executeJavaScriptAsync(code, (err, result) => {
                if (err) {
                    this.emit('error', new Error(err));
                    reject(new Error(err));
                } else {
                    resolve(result);
                }
            });
        });
    }
    
    /**
     * Take a screenshot (async)
     * @param {string} [filename='screenshot.png'] - Output filename
     * @returns {Promise<boolean>} - True if screenshot succeeded
     */
    async screenshot(filename = 'screenshot.png') {
        return new Promise((resolve, reject) => {
            this._browser.takeScreenshotAsync(filename, (err, result) => {
                if (err) {
                    this.emit('error', new Error(err));
                    reject(new Error(err));
                } else {
                    this.emit('screenshot', { filename });
                    resolve(result);
                }
            });
        });
    }
    
    // ========== DOM Interaction Methods (Sync) ==========
    
    /**
     * Click an element (sync)
     * @param {string} selector - CSS selector for the element
     * @returns {boolean} - True if click succeeded
     */
    clickSync(selector) {
        const result = this._browser.clickElement(selector);
        if (result) {
            this.emit('clicked', { selector });
        }
        return result;
    }
    
    /**
     * Type text into an input element (sync)
     * @param {string} selector - CSS selector for the input element
     * @param {string} text - Text to type
     * @returns {boolean} - True if typing succeeded
     */
    typeSync(selector, text) {
        const result = this._browser.fillInput(selector, text);
        if (result) {
            this.emit('typed', { selector, text });
        }
        return result;
    }
    
    /**
     * Execute JavaScript code (sync)
     * @param {string} code - JavaScript code to execute
     * @returns {string} - Result of the JavaScript execution
     */
    executeJavaScriptSync(code) {
        return this._browser.executeJavaScript(code);
    }
    
    /**
     * Take a screenshot (sync)
     * @param {string} [filename='screenshot.png'] - Output filename
     * @returns {boolean} - True if screenshot succeeded
     */
    screenshotSync(filename = 'screenshot.png') {
        const result = this._browser.takeScreenshot(filename);
        if (result) {
            this.emit('screenshot', { filename });
        }
        return result;
    }
    
    // ========== Element Query Methods (All Sync) ==========
    
    /**
     * Check if an element exists
     * @param {string} selector - CSS selector for the element
     * @returns {boolean} - True if element exists
     */
    exists(selector) {
        return this._browser.elementExists(selector);
    }
    
    /**
     * Get text content of an element
     * @param {string} selector - CSS selector for the element
     * @returns {string} - Text content of the element
     */
    getText(selector) {
        return this._browser.getInnerText(selector);
    }
    
    /**
     * Get HTML content of an element
     * @param {string} selector - CSS selector for the element
     * @returns {string} - HTML content of the element
     */
    getHtml(selector) {
        return this._browser.getElementHtml(selector);
    }
    
    /**
     * Get an attribute value from an element
     * @param {string} selector - CSS selector for the element
     * @param {string} attribute - Attribute name
     * @returns {string} - Attribute value
     */
    getAttribute(selector, attribute) {
        return this._browser.getAttribute(selector, attribute);
    }
    
    /**
     * Count elements matching a selector
     * @param {string} selector - CSS selector
     * @returns {number} - Number of matching elements
     */
    count(selector) {
        return this._browser.countElements(selector);
    }
    
    // ========== Fluent API Support ==========
    
    /**
     * Create a fluent chain for batch operations
     * @returns {BrowserChain} - Chain object for fluent API
     */
    chain() {
        return new BrowserChain(this);
    }
    
    // ========== Testing Utilities ==========
    
    /**
     * Create a test suite for this browser
     * @param {string} name - Test suite name
     * @returns {TestSuite} - Test suite instance
     */
    createTest(name) {
        const TestSuite = require('./test-suite');
        return new TestSuite(this, name);
    }
    
    // ========== Convenience Methods ==========
    
    /**
     * Wait for a specified amount of time
     * @param {number} ms - Milliseconds to wait
     * @returns {Promise<void>}
     */
    async wait(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
    
    /**
     * Get basic page information
     * @returns {Object} - Page info object
     */
    getPageInfo() {
        return {
            url: this.getCurrentUrl(),
            title: this.executeJavaScriptSync('document.title'),
            readyState: this.executeJavaScriptSync('document.readyState')
        };
    }
    
    /**
     * Clean up and destroy the browser instance
     */
    destroy() {
        this.emit('destroyed');
        // Native cleanup happens automatically in destructor
    }
}

/**
 * Fluent API chain for batch operations
 */
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
    
    screenshot(filename) {
        this.operations.push(['screenshot', filename]);
        return this;
    }
    
    /**
     * Execute all chained operations
     * @returns {Promise<Array>} - Array of results from each operation
     */
    async execute() {
        const results = [];
        for (const [method, ...args] of this.operations) {
            if (method === 'wait') {
                await this.browser.wait(args[0]);
                results.push(true);
            } else {
                const result = await this.browser[method](...args);
                results.push(result);
            }
        }
        return results;
    }
}

module.exports = Browser;