const path = require('path');
const Browser = require('./lib/browser');
const Session = require('./lib/session');
const TestSuite = require('./lib/test-suite');

// Load the native addon
let addon;
try {
    addon = require(path.join(__dirname, '../../build/Release/hweb_addon.node'));
} catch (error) {
    throw new Error(`Failed to load HeadlessWeb native addon: ${error.message}\n` +
                    `Make sure to run 'npm run build' first or check that all dependencies are installed.`);
}

/**
 * HeadlessWeb JavaScript API
 * 
 * @example
 * const HeadlessWeb = require('@headlessweb/js');
 * 
 * // Create a browser instance
 * const browser = HeadlessWeb.create({ session: 'my-session' });
 * 
 * // Use async/await API
 * await browser.navigate('https://example.com');
 * await browser.click('#submit');
 * const title = browser.getText('h1');
 */
module.exports = {
    // Main classes
    Browser,
    Session,
    TestSuite,
    
    // Convenience factory method
    create: (options = {}) => new Browser(options),
    
    // Version information
    version: require('./package.json').version,
    coreVersion: addon.getCoreVersion(),
    
    // System information
    getSystemInfo: () => addon.getSystemInfo(),
    
    // Native addon (for advanced usage)
    _addon: addon
};