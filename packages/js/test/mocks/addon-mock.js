/**
 * Mock implementation of the native addon for testing
 * This allows testing JavaScript logic without requiring C++ compilation
 */

class MockBrowser {
    constructor(options = {}) {
        this.options = options;
        this.currentUrl = 'about:blank';
        this.elements = new Map();
        this.screenshots = [];
        this.jsResults = new Map();
        
        // Mock DOM state
        this.mockDOM = {
            'h1': 'Mock Page Title',
            '#submit': 'Submit',
            '.error': null, // Not exists
            'body': 'Mock page content',
            '#username': '',
            '#password': ''
        };
    }
    
    // Navigation methods
    loadUri(url) {
        this.currentUrl = url;
        return true;
    }
    
    loadUriAsync(url, callback) {
        setTimeout(() => {
            this.currentUrl = url;
            callback(null, true);
        }, 10);
    }
    
    getCurrentUrl() {
        return this.currentUrl;
    }
    
    // DOM interaction methods
    clickElement(selector) {
        if (selector === '.non-existent') return false;
        return true;
    }
    
    clickElementAsync(selector, callback) {
        setTimeout(() => {
            const result = this.clickElement(selector);
            callback(null, result);
        }, 10);
    }
    
    fillInput(selector, value) {
        if (selector === '.non-existent') return false;
        this.mockDOM[selector] = value;
        return true;
    }
    
    fillInputAsync(selector, value, callback) {
        setTimeout(() => {
            const result = this.fillInput(selector, value);
            callback(null, result);
        }, 10);
    }
    
    // Element query methods
    elementExists(selector) {
        if (selector === '.non-existent' || selector === '.error') return false;
        return this.mockDOM[selector] !== undefined;
    }
    
    getInnerText(selector) {
        const content = this.mockDOM[selector];
        return content || '';
    }
    
    getElementHtml(selector) {
        const content = this.mockDOM[selector];
        return content ? `<div>${content}</div>` : '';
    }
    
    getAttribute(selector, attribute) {
        if (attribute === 'value') {
            return this.mockDOM[selector] || '';
        }
        return '';
    }
    
    countElements(selector) {
        return this.elementExists(selector) ? 1 : 0;
    }
    
    // JavaScript execution
    executeJavaScript(code) {
        // Mock common JavaScript operations
        if (code === 'document.title') return 'Mock Page Title';
        if (code === 'document.readyState') return 'complete';
        if (code === 'window.location.href') return this.currentUrl;
        if (code.includes('document.querySelector')) {
            // Extract selector from querySelector
            const match = code.match(/document\.querySelector\(['"`]([^'"`]+)['"`]\)/);
            if (match) {
                const selector = match[1];
                return this.elementExists(selector) ? 'mock-element' : null;
            }
        }
        return 'mock-result';
    }
    
    executeJavaScriptAsync(code, callback) {
        setTimeout(() => {
            const result = this.executeJavaScript(code);
            callback(null, result);
        }, 10);
    }
    
    // Screenshot methods
    takeScreenshot(filename) {
        this.screenshots.push(filename);
        return true;
    }
    
    takeScreenshotAsync(filename, callback) {
        setTimeout(() => {
            const result = this.takeScreenshot(filename);
            callback(null, result);
        }, 10);
    }
}

class MockSession {
    constructor(sessionName, sessionsDir) {
        this.sessionName = sessionName;
        this.sessionsDir = sessionsDir;
    }
    
    getSessionName() {
        return this.sessionName;
    }
    
    saveSession() {
        return true;
    }
    
    loadSession() {
        return true;
    }
    
    deleteSession() {
        return true;
    }
    
    listSessions() {
        return ['session1', 'session2', 'test-session'];
    }
}

// Mock addon object
const mockAddon = {
    Browser: MockBrowser,
    Session: MockSession,
    
    getCoreVersion() {
        return '1.0.0-mock';
    },
    
    getSystemInfo() {
        return {
            platform: 'mock',
            arch: 'mock64',
            webkit_version: '6.0-mock'
        };
    }
};

module.exports = mockAddon;