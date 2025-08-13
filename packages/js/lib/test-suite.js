/**
 * Test suite for browser automation testing
 * 
 * @example
 * const test = browser.createTest('login-flow');
 * await test
 *   .navigate('/login')
 *   .type('#username', 'user')
 *   .click('#submit')
 *   .assertExists('.dashboard');
 * 
 * const report = test.generateReport();
 */
class TestSuite {
    /**
     * Create a new TestSuite
     * @param {Browser} browser - Browser instance to use for testing
     * @param {string} name - Name of the test suite
     */
    constructor(browser, name) {
        this.browser = browser;
        this.name = name;
        this.steps = [];
        this.startTime = Date.now();
    }
    
    // ========== Navigation Actions ==========
    
    /**
     * Navigate to a URL
     * @param {string} url - URL to navigate to
     * @returns {TestSuite} - This instance for chaining
     */
    async navigate(url) {
        const startTime = Date.now();
        try {
            await this.browser.navigate(url);
            this._addStep('navigate', { url }, true, Date.now() - startTime);
        } catch (error) {
            this._addStep('navigate', { url }, false, Date.now() - startTime, error.message);
            throw error;
        }
        return this;
    }
    
    // ========== DOM Interactions ==========
    
    /**
     * Click an element
     * @param {string} selector - CSS selector for the element
     * @returns {TestSuite} - This instance for chaining
     */
    async click(selector) {
        const startTime = Date.now();
        try {
            const result = await this.browser.click(selector);
            this._addStep('click', { selector }, result, Date.now() - startTime);
            if (!result) throw new Error(`Click failed for selector: ${selector}`);
        } catch (error) {
            this._addStep('click', { selector }, false, Date.now() - startTime, error.message);
            throw error;
        }
        return this;
    }
    
    /**
     * Type text into an input
     * @param {string} selector - CSS selector for the input
     * @param {string} text - Text to type
     * @returns {TestSuite} - This instance for chaining
     */
    async type(selector, text) {
        const startTime = Date.now();
        try {
            const result = await this.browser.type(selector, text);
            this._addStep('type', { selector, text }, result, Date.now() - startTime);
            if (!result) throw new Error(`Type failed for selector: ${selector}`);
        } catch (error) {
            this._addStep('type', { selector, text }, false, Date.now() - startTime, error.message);
            throw error;
        }
        return this;
    }
    
    /**
     * Take a screenshot
     * @param {string} [filename] - Screenshot filename
     * @returns {TestSuite} - This instance for chaining
     */
    async screenshot(filename) {
        const startTime = Date.now();
        try {
            const result = await this.browser.screenshot(filename);
            this._addStep('screenshot', { filename }, result, Date.now() - startTime);
        } catch (error) {
            this._addStep('screenshot', { filename }, false, Date.now() - startTime, error.message);
            // Don't throw for screenshots - they're optional
        }
        return this;
    }
    
    // ========== Assertions ==========
    
    /**
     * Assert that an element exists
     * @param {string} selector - CSS selector for the element
     * @returns {TestSuite} - This instance for chaining
     */
    async assertExists(selector) {
        const startTime = Date.now();
        const exists = this.browser.exists(selector);
        const success = exists === true;
        
        this._addStep('assertExists', { selector }, success, Date.now() - startTime,
                     success ? 'Element found' : 'Element not found');
        
        if (!success) {
            throw new Error(`Assertion failed: Element not found - ${selector}`);
        }
        return this;
    }
    
    /**
     * Assert that an element does not exist
     * @param {string} selector - CSS selector for the element
     * @returns {TestSuite} - This instance for chaining
     */
    async assertNotExists(selector) {
        const startTime = Date.now();
        const exists = this.browser.exists(selector);
        const success = exists === false;
        
        this._addStep('assertNotExists', { selector }, success, Date.now() - startTime,
                     success ? 'Element not found (as expected)' : 'Element unexpectedly found');
        
        if (!success) {
            throw new Error(`Assertion failed: Element should not exist - ${selector}`);
        }
        return this;
    }
    
    /**
     * Assert that an element contains specific text
     * @param {string} selector - CSS selector for the element
     * @param {string} expectedText - Expected text content
     * @returns {TestSuite} - This instance for chaining
     */
    async assertText(selector, expectedText) {
        const startTime = Date.now();
        const actualText = this.browser.getText(selector);
        const success = actualText.includes(expectedText);
        
        this._addStep('assertText', { selector, expected: expectedText, actual: actualText }, 
                     success, Date.now() - startTime,
                     success ? 'Text match' : `Expected "${expectedText}", got "${actualText}"`);
        
        if (!success) {
            throw new Error(`Assertion failed: Text mismatch - expected "${expectedText}", got "${actualText}"`);
        }
        return this;
    }
    
    /**
     * Assert that an element has a specific attribute value
     * @param {string} selector - CSS selector for the element
     * @param {string} attribute - Attribute name
     * @param {string} expectedValue - Expected attribute value
     * @returns {TestSuite} - This instance for chaining
     */
    async assertAttribute(selector, attribute, expectedValue) {
        const startTime = Date.now();
        const actualValue = this.browser.getAttribute(selector, attribute);
        const success = actualValue === expectedValue;
        
        this._addStep('assertAttribute', 
                     { selector, attribute, expected: expectedValue, actual: actualValue }, 
                     success, Date.now() - startTime,
                     success ? 'Attribute match' : `Expected "${expectedValue}", got "${actualValue}"`);
        
        if (!success) {
            throw new Error(`Assertion failed: Attribute mismatch - expected "${expectedValue}", got "${actualValue}"`);
        }
        return this;
    }
    
    /**
     * Assert that the current URL matches a pattern
     * @param {string|RegExp} pattern - URL pattern to match
     * @returns {TestSuite} - This instance for chaining
     */
    async assertUrl(pattern) {
        const startTime = Date.now();
        const currentUrl = this.browser.getCurrentUrl();
        const success = typeof pattern === 'string' ? 
                       currentUrl.includes(pattern) : 
                       pattern.test(currentUrl);
        
        this._addStep('assertUrl', { pattern: pattern.toString(), actual: currentUrl }, 
                     success, Date.now() - startTime,
                     success ? 'URL match' : `URL "${currentUrl}" doesn't match pattern "${pattern}"`);
        
        if (!success) {
            throw new Error(`Assertion failed: URL mismatch - "${currentUrl}" doesn't match "${pattern}"`);
        }
        return this;
    }
    
    // ========== Utility Methods ==========
    
    /**
     * Wait for a specified amount of time
     * @param {number} ms - Milliseconds to wait
     * @returns {TestSuite} - This instance for chaining
     */
    async wait(ms) {
        const startTime = Date.now();
        await this.browser.wait(ms);
        this._addStep('wait', { duration: ms }, true, Date.now() - startTime);
        return this;
    }
    
    /**
     * Add a custom message to the test log
     * @param {string} message - Message to add
     * @returns {TestSuite} - This instance for chaining
     */
    message(message) {
        this._addStep('message', { message }, true, 0, message);
        return this;
    }
    
    // ========== Reporting ==========
    
    /**
     * Generate a test report
     * @returns {Object} - Test report object
     */
    generateReport() {
        const endTime = Date.now();
        const totalTime = endTime - this.startTime;
        const passed = this.steps.filter(s => s.success).length;
        const failed = this.steps.filter(s => !s.success).length;
        const total = this.steps.length;
        
        return {
            name: this.name,
            summary: {
                passed,
                failed,
                total,
                success: failed === 0,
                duration: totalTime
            },
            steps: this.steps,
            timing: {
                startTime: this.startTime,
                endTime,
                totalTime
            }
        };
    }
    
    /**
     * Generate a human-readable test report
     * @returns {string} - Formatted test report
     */
    generateTextReport() {
        const report = this.generateReport();
        const lines = [];
        
        lines.push(`\n=== Test Suite: ${report.name} ===`);
        lines.push(`Status: ${report.summary.success ? 'PASSED' : 'FAILED'}`);
        lines.push(`Results: ${report.summary.passed}/${report.summary.total} passed`);
        lines.push(`Duration: ${report.timing.totalTime}ms`);
        lines.push('');
        
        report.steps.forEach((step, index) => {
            const status = step.success ? '✓' : '✗';
            const timing = step.duration ? ` (${step.duration}ms)` : '';
            lines.push(`${index + 1}. ${status} ${step.action}${timing}`);
            
            if (step.details) {
                Object.entries(step.details).forEach(([key, value]) => {
                    lines.push(`   ${key}: ${value}`);
                });
            }
            
            if (step.message && step.action !== 'message') {
                lines.push(`   ${step.message}`);
            }
            
            if (!step.success && step.error) {
                lines.push(`   Error: ${step.error}`);
            }
        });
        
        return lines.join('\n');
    }
    
    // ========== Private Methods ==========
    
    /**
     * Add a step to the test log
     * @private
     */
    _addStep(action, details, success, duration, message = '', error = '') {
        this.steps.push({
            action,
            details,
            success,
            duration,
            message,
            error,
            timestamp: Date.now()
        });
    }
}

module.exports = TestSuite;