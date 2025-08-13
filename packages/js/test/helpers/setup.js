/**
 * Jest setup file for HeadlessWeb tests
 */

const path = require('path');
const fs = require('fs');

// Test configuration
global.TEST_CONFIG = {
    timeout: 30000,
    headless: true,
    testSessionPrefix: 'test-',
    screenshotDir: path.join(__dirname, '../fixtures/screenshots'),
    sessionDir: path.join(__dirname, '../fixtures/sessions')
};

// Ensure test directories exist
const testDirs = [
    global.TEST_CONFIG.screenshotDir,
    global.TEST_CONFIG.sessionDir
];

testDirs.forEach(dir => {
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir, { recursive: true });
    }
});

// Global test utilities
global.createTestSession = () => {
    return `${global.TEST_CONFIG.testSessionPrefix}${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
};

global.createTestBrowser = (options = {}) => {
    // This will be mocked in unit tests, real in integration tests
    const { Browser } = require('../../index');
    
    return new Browser({
        session: createTestSession(),
        headless: global.TEST_CONFIG.headless,
        timeout: global.TEST_CONFIG.timeout,
        ...options
    });
};

global.cleanupTestSessions = () => {
    const { Session } = require('../../index');
    
    try {
        const sessionDir = global.TEST_CONFIG.sessionDir;
        if (fs.existsSync(sessionDir)) {
            const sessions = fs.readdirSync(sessionDir);
            sessions.forEach(sessionName => {
                if (sessionName.startsWith(global.TEST_CONFIG.testSessionPrefix)) {
                    try {
                        const session = new Session(sessionName, sessionDir);
                        session.delete();
                    } catch (error) {
                        // Ignore cleanup errors
                    }
                }
            });
        }
    } catch (error) {
        // Ignore cleanup errors
    }
};

// Enhanced Jest matchers
expect.extend({
    toBeValidUrl(received) {
        try {
            new URL(received);
            return {
                message: () => `expected ${received} not to be a valid URL`,
                pass: true
            };
        } catch {
            return {
                message: () => `expected ${received} to be a valid URL`,
                pass: false
            };
        }
    },
    
    toHaveProperty(received, property, value) {
        const hasProperty = Object.prototype.hasOwnProperty.call(received, property);
        const hasCorrectValue = value !== undefined ? received[property] === value : true;
        
        return {
            message: () => value !== undefined 
                ? `expected ${received} to have property ${property} with value ${value}`
                : `expected ${received} to have property ${property}`,
            pass: hasProperty && hasCorrectValue
        };
    },
    
    toBeTestReport(received) {
        const isObject = typeof received === 'object' && received !== null;
        const hasRequiredProps = isObject && 
            'name' in received &&
            'summary' in received &&
            'steps' in received &&
            'timing' in received;
        
        const hasValidSummary = hasRequiredProps &&
            typeof received.summary.passed === 'number' &&
            typeof received.summary.failed === 'number' &&
            typeof received.summary.total === 'number' &&
            typeof received.summary.success === 'boolean';
        
        return {
            message: () => `expected ${received} to be a valid test report`,
            pass: hasRequiredProps && hasValidSummary
        };
    }
});

// Global error handling for tests
process.on('unhandledRejection', (reason, promise) => {
    console.error('Unhandled Rejection at:', promise, 'reason:', reason);
});

// Setup environment for headless testing
if (process.env.CI || process.env.HEADLESS) {
    global.TEST_CONFIG.headless = true;
    
    // Set up virtual display for CI environments
    if (!process.env.DISPLAY) {
        process.env.DISPLAY = ':99';
        process.env.GDK_BACKEND = 'broadway';
        process.env.GTK_RECENT_FILES_ENABLED = '0';
        process.env.WEBKIT_DISABLE_COMPOSITING_MODE = '1';
    }
}

console.log('HeadlessWeb test setup complete:', {
    headless: global.TEST_CONFIG.headless,
    timeout: global.TEST_CONFIG.timeout,
    sessionDir: global.TEST_CONFIG.sessionDir
});