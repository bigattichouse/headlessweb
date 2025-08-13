/**
 * Unit tests for TestSuite class
 */

const { Browser } = require('../../index');

describe('TestSuite Class', () => {
    let browser;
    let testSuite;
    
    beforeEach(() => {
        browser = createTestBrowser();
        testSuite = browser.createTest('Unit Test Suite');
    });
    
    afterEach(() => {
        if (browser) {
            browser.destroy();
        }
    });
    
    describe('Constructor', () => {
        test('should create test suite with browser and name', () => {
            expect(testSuite.browser).toBe(browser);
            expect(testSuite.name).toBe('Unit Test Suite');
            expect(testSuite.steps).toEqual([]);
            expect(typeof testSuite.startTime).toBe('number');
        });
        
        test('should have unique start time', () => {
            const suite1 = browser.createTest('Suite 1');
            const suite2 = browser.createTest('Suite 2');
            
            expect(suite1.startTime).not.toBe(suite2.startTime);
        });
    });
    
    describe('Navigation Actions', () => {
        test('should navigate and record step', async () => {
            await testSuite.navigate('https://example.com');
            
            expect(testSuite.steps).toHaveLength(1);
            expect(testSuite.steps[0]).toMatchObject({
                action: 'navigate',
                details: { url: 'https://example.com' },
                success: true
            });
        });
        
        test('should handle navigation errors', async () => {
            // Mock a navigation error by using invalid URL
            try {
                await testSuite.navigate('invalid-url');
            } catch (error) {
                expect(testSuite.steps).toHaveLength(1);
                expect(testSuite.steps[0].success).toBe(false);
                expect(testSuite.steps[0].error).toBeTruthy();
            }
        });
        
        test('should return TestSuite for chaining', async () => {
            const result = await testSuite.navigate('https://example.com');
            expect(result).toBe(testSuite);
        });
    });
    
    describe('DOM Interaction Actions', () => {
        beforeEach(async () => {
            await testSuite.navigate('https://example.com');
        });
        
        test('should click element and record step', async () => {
            await testSuite.click('#submit');
            
            const clickStep = testSuite.steps.find(step => step.action === 'click');
            expect(clickStep).toBeDefined();
            expect(clickStep.details.selector).toBe('#submit');
            expect(clickStep.success).toBe(true);
        });
        
        test('should type text and record step', async () => {
            await testSuite.type('#username', 'testuser');
            
            const typeStep = testSuite.steps.find(step => step.action === 'type');
            expect(typeStep).toBeDefined();
            expect(typeStep.details).toMatchObject({
                selector: '#username',
                text: 'testuser'
            });
            expect(typeStep.success).toBe(true);
        });
        
        test('should take screenshot and record step', async () => {
            await testSuite.screenshot('test-screenshot.png');
            
            const screenshotStep = testSuite.steps.find(step => step.action === 'screenshot');
            expect(screenshotStep).toBeDefined();
            expect(screenshotStep.details.filename).toBe('test-screenshot.png');
            expect(screenshotStep.success).toBe(true);
        });
        
        test('should handle failed interactions gracefully', async () => {
            try {
                await testSuite.click('.non-existent');
            } catch (error) {
                const failedStep = testSuite.steps.find(step => 
                    step.action === 'click' && !step.success
                );
                expect(failedStep).toBeDefined();
                expect(failedStep.error).toBeTruthy();
            }
        });
    });
    
    describe('Assertion Methods', () => {
        beforeEach(async () => {
            await testSuite.navigate('https://example.com');
        });
        
        test('should assert element exists', async () => {
            await testSuite.assertExists('h1');
            
            const assertStep = testSuite.steps.find(step => step.action === 'assertExists');
            expect(assertStep).toBeDefined();
            expect(assertStep.details.selector).toBe('h1');
            expect(assertStep.success).toBe(true);
            expect(assertStep.message).toBe('Element found');
        });
        
        test('should fail when element does not exist', async () => {
            try {
                await testSuite.assertExists('.non-existent');
            } catch (error) {
                const assertStep = testSuite.steps.find(step => 
                    step.action === 'assertExists' && !step.success
                );
                expect(assertStep).toBeDefined();
                expect(assertStep.message).toBe('Element not found');
                expect(error.message).toContain('Element not found');
            }
        });
        
        test('should assert element does not exist', async () => {
            await testSuite.assertNotExists('.non-existent');
            
            const assertStep = testSuite.steps.find(step => step.action === 'assertNotExists');
            expect(assertStep).toBeDefined();
            expect(assertStep.success).toBe(true);
            expect(assertStep.message).toBe('Element not found (as expected)');
        });
        
        test('should assert text content', async () => {
            await testSuite.assertText('h1', 'Mock Page Title');
            
            const assertStep = testSuite.steps.find(step => step.action === 'assertText');
            expect(assertStep).toBeDefined();
            expect(assertStep.details).toMatchObject({
                selector: 'h1',
                expected: 'Mock Page Title',
                actual: 'Mock Page Title'
            });
            expect(assertStep.success).toBe(true);
        });
        
        test('should fail text assertion with mismatch', async () => {
            try {
                await testSuite.assertText('h1', 'Wrong Title');
            } catch (error) {
                const assertStep = testSuite.steps.find(step => 
                    step.action === 'assertText' && !step.success
                );
                expect(assertStep).toBeDefined();
                expect(assertStep.message).toContain('Expected "Wrong Title"');
            }
        });
        
        test('should assert attribute value', async () => {
            // First set an attribute
            await browser.typeSync('#username', 'testvalue');
            
            await testSuite.assertAttribute('#username', 'value', 'testvalue');
            
            const assertStep = testSuite.steps.find(step => step.action === 'assertAttribute');
            expect(assertStep).toBeDefined();
            expect(assertStep.success).toBe(true);
        });
        
        test('should assert URL patterns', async () => {
            await testSuite.assertUrl('example.com');
            
            const assertStep = testSuite.steps.find(step => step.action === 'assertUrl');
            expect(assertStep).toBeDefined();
            expect(assertStep.success).toBe(true);
        });
        
        test('should assert URL with regex', async () => {
            await testSuite.assertUrl(/example\.com/);
            
            const assertStep = testSuite.steps.find(step => step.action === 'assertUrl');
            expect(assertStep).toBeDefined();
            expect(assertStep.success).toBe(true);
        });
    });
    
    describe('Utility Methods', () => {
        test('should wait and record step', async () => {
            const start = Date.now();
            await testSuite.wait(100);
            const elapsed = Date.now() - start;
            
            expect(elapsed).toBeGreaterThanOrEqual(90);
            
            const waitStep = testSuite.steps.find(step => step.action === 'wait');
            expect(waitStep).toBeDefined();
            expect(waitStep.details.duration).toBe(100);
            expect(waitStep.success).toBe(true);
        });
        
        test('should add custom message', () => {
            testSuite.message('Custom test message');
            
            const messageStep = testSuite.steps.find(step => step.action === 'message');
            expect(messageStep).toBeDefined();
            expect(messageStep.details.message).toBe('Custom test message');
            expect(messageStep.success).toBe(true);
            expect(messageStep.duration).toBe(0);
        });
        
        test('should support method chaining', async () => {
            const result = await testSuite
                .navigate('https://example.com')
                .message('Navigation complete')
                .assertExists('h1')
                .wait(50);
            
            expect(result).toBe(testSuite);
            expect(testSuite.steps).toHaveLength(4);
        });
    });
    
    describe('Test Reporting', () => {
        test('should generate basic report', () => {
            testSuite.message('Test step 1');
            testSuite.message('Test step 2');
            
            const report = testSuite.generateReport();
            
            expect(report).toBeTestReport();
            expect(report.name).toBe('Unit Test Suite');
            expect(report.summary.total).toBe(2);
            expect(report.summary.passed).toBe(2);
            expect(report.summary.failed).toBe(0);
            expect(report.summary.success).toBe(true);
        });
        
        test('should generate report with failures', async () => {
            testSuite.message('Successful step');
            
            try {
                await testSuite.assertExists('.non-existent');
            } catch (error) {
                // Expected failure
            }
            
            const report = testSuite.generateReport();
            
            expect(report.summary.total).toBe(2);
            expect(report.summary.passed).toBe(1);
            expect(report.summary.failed).toBe(1);
            expect(report.summary.success).toBe(false);
        });
        
        test('should include timing information', () => {
            testSuite.message('Test message');
            
            const report = testSuite.generateReport();
            
            expect(report.timing).toBeDefined();
            expect(typeof report.timing.startTime).toBe('number');
            expect(typeof report.timing.endTime).toBe('number');
            expect(typeof report.timing.totalTime).toBe('number');
            expect(report.timing.totalTime).toBeGreaterThanOrEqual(0);
        });
        
        test('should generate text report', () => {
            testSuite.message('Test step 1');
            testSuite.message('Test step 2');
            
            const textReport = testSuite.generateTextReport();
            
            expect(typeof textReport).toBe('string');
            expect(textReport).toContain('Unit Test Suite');
            expect(textReport).toContain('PASSED');
            expect(textReport).toContain('2/2 passed');
            expect(textReport).toContain('Test step 1');
            expect(textReport).toContain('Test step 2');
        });
        
        test('should generate text report with failures', async () => {
            testSuite.message('Successful step');
            
            try {
                await testSuite.assertExists('.non-existent');
            } catch (error) {
                // Expected failure
            }
            
            const textReport = testSuite.generateTextReport();
            
            expect(textReport).toContain('FAILED');
            expect(textReport).toContain('1/2 passed');
            expect(textReport).toContain('✓');  // Success symbol
            expect(textReport).toContain('✗');  // Failure symbol
        });
    });
    
    describe('Step Recording', () => {
        test('should record step timing', async () => {
            await testSuite.wait(100);
            
            const waitStep = testSuite.steps[0];
            expect(waitStep.duration).toBeGreaterThanOrEqual(90);
            expect(typeof waitStep.timestamp).toBe('number');
        });
        
        test('should record step details', async () => {
            await testSuite.navigate('https://example.com');
            await testSuite.click('#button');
            await testSuite.type('#input', 'text');
            
            const steps = testSuite.steps;
            
            expect(steps[0].action).toBe('navigate');
            expect(steps[0].details.url).toBe('https://example.com');
            
            expect(steps[1].action).toBe('click');
            expect(steps[1].details.selector).toBe('#button');
            
            expect(steps[2].action).toBe('type');
            expect(steps[2].details.selector).toBe('#input');
            expect(steps[2].details.text).toBe('text');
        });
        
        test('should record success/failure status', async () => {
            await testSuite.navigate('https://example.com');
            
            try {
                await testSuite.assertExists('.non-existent');
            } catch (error) {
                // Expected
            }
            
            expect(testSuite.steps[0].success).toBe(true);
            expect(testSuite.steps[1].success).toBe(false);
        });
    });
    
    describe('Error Handling', () => {
        test('should continue test after assertion failure', async () => {
            await testSuite.navigate('https://example.com');
            
            try {
                await testSuite.assertExists('.non-existent');
            } catch (error) {
                // Expected failure
            }
            
            // Should be able to continue
            await testSuite.message('Continuing after failure');
            
            expect(testSuite.steps).toHaveLength(3);
            expect(testSuite.steps[2].action).toBe('message');
            expect(testSuite.steps[2].success).toBe(true);
        });
        
        test('should record error messages', async () => {
            try {
                await testSuite.assertText('h1', 'Wrong Text');
            } catch (error) {
                const failedStep = testSuite.steps[0];
                expect(failedStep.error).toContain('Text mismatch');
            }
        });
        
        test('should handle browser errors gracefully', async () => {
            // Simulate browser error by destroying browser mid-test
            browser.destroy();
            
            try {
                await testSuite.navigate('https://example.com');
            } catch (error) {
                expect(error).toBeInstanceOf(Error);
            }
        });
    });
});