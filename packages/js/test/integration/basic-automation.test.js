/**
 * Integration tests for basic browser automation
 * These tests use the actual HeadlessWeb functionality
 */

const { Browser } = require('../../index');

describe('Basic Automation Integration', () => {
    let browser;
    
    beforeEach(() => {
        browser = createTestBrowser();
    });
    
    afterEach(() => {
        if (browser) {
            browser.destroy();
        }
    });
    
    describe('Page Navigation', () => {
        test('should navigate to static pages', async () => {
            await browser.navigate('about:blank');
            expect(browser.getCurrentUrl()).toBe('about:blank');
            
            const pageInfo = browser.getPageInfo();
            expect(pageInfo.url).toBe('about:blank');
            expect(pageInfo.readyState).toBe('complete');
        });
        
        test('should handle navigation events', async () => {
            const navigationEvents = [];
            
            browser.on('navigated', (data) => {
                navigationEvents.push(data);
            });
            
            await browser.navigate('about:blank');
            
            expect(navigationEvents).toHaveLength(1);
            expect(navigationEvents[0].url).toBe('about:blank');
        });
        
        test('should navigate between multiple pages', async () => {
            await browser.navigate('about:blank');
            expect(browser.getCurrentUrl()).toBe('about:blank');
            
            // Create a simple HTML page for testing
            const htmlContent = `
                <!DOCTYPE html>
                <html>
                <head><title>Test Page</title></head>
                <body>
                    <h1>Test Page</h1>
                    <p>This is a test page for HeadlessWeb</p>
                </body>
                </html>
            `;
            
            const dataUrl = 'data:text/html;charset=utf-8,' + encodeURIComponent(htmlContent);
            await browser.navigate(dataUrl);
            
            expect(browser.getCurrentUrl()).toContain('data:text/html');
            expect(browser.exists('h1')).toBe(true);
            expect(browser.getText('h1')).toBe('Test Page');
        });
    });
    
    describe('DOM Interaction', () => {
        beforeEach(async () => {
            // Create a test page with interactive elements
            const htmlContent = `
                <!DOCTYPE html>
                <html>
                <head><title>Interactive Test Page</title></head>
                <body>
                    <h1 id="title">Interactive Test</h1>
                    <form id="test-form">
                        <input type="text" id="name-input" placeholder="Enter name">
                        <input type="email" id="email-input" placeholder="Enter email">
                        <select id="country-select">
                            <option value="">Select Country</option>
                            <option value="us">United States</option>
                            <option value="uk">United Kingdom</option>
                            <option value="ca">Canada</option>
                        </select>
                        <button type="button" id="submit-btn">Submit</button>
                    </form>
                    <div id="result" style="display: none;">Form submitted!</div>
                    
                    <script>
                        document.getElementById('submit-btn').addEventListener('click', function() {
                            const name = document.getElementById('name-input').value;
                            const email = document.getElementById('email-input').value;
                            const country = document.getElementById('country-select').value;
                            
                            if (name && email && country) {
                                document.getElementById('result').style.display = 'block';
                                document.getElementById('result').textContent = 
                                    'Form submitted for ' + name + ' (' + email + ') from ' + country;
                            }
                        });
                    </script>
                </body>
                </html>
            `;
            
            const dataUrl = 'data:text/html;charset=utf-8,' + encodeURIComponent(htmlContent);
            await browser.navigate(dataUrl);
        });
        
        test('should interact with form elements', async () => {
            // Check initial state
            expect(browser.exists('#test-form')).toBe(true);
            expect(browser.exists('#result')).toBe(true);
            
            // Fill form fields
            await browser.type('#name-input', 'John Doe');
            await browser.type('#email-input', 'john@example.com');
            
            // Verify values were set
            expect(browser.getAttribute('#name-input', 'value')).toBe('John Doe');
            expect(browser.getAttribute('#email-input', 'value')).toBe('john@example.com');
            
            // Note: selectOption would need to be implemented for full test
            // For now, we'll test clicking
            await browser.click('#submit-btn');
            
            // Take screenshot after interaction
            await browser.screenshot('form-interaction-test.png');
        });
        
        test('should extract text content', async () => {
            const title = browser.getText('#title');
            expect(title).toBe('Interactive Test');
            
            const placeholder = browser.getAttribute('#name-input', 'placeholder');
            expect(placeholder).toBe('Enter name');
        });
        
        test('should count elements', async () => {
            const inputCount = browser.count('input');
            expect(inputCount).toBeGreaterThanOrEqual(2);
            
            const selectCount = browser.count('select');
            expect(selectCount).toBe(1);
        });
        
        test('should execute custom JavaScript', async () => {
            const title = await browser.executeJavaScript('document.title');
            expect(title).toBe('Interactive Test Page');
            
            const bodyText = await browser.executeJavaScript(
                'document.body.textContent.trim().substring(0, 20)'
            );
            expect(bodyText).toContain('Interactive');
        });
    });
    
    describe('Screenshot Capabilities', () => {
        test('should take screenshots', async () => {
            await browser.navigate('about:blank');
            
            const result = await browser.screenshot('blank-page-test.png');
            expect(result).toBe(true);
        });
        
        test('should take screenshots of content pages', async () => {
            const htmlContent = `
                <!DOCTYPE html>
                <html>
                <head><title>Screenshot Test</title></head>
                <body style="background: linear-gradient(45deg, #ff6b6b, #4ecdc4); height: 100vh; margin: 0;">
                    <div style="text-align: center; padding: 50px; color: white; font-size: 24px;">
                        <h1>Screenshot Test Page</h1>
                        <p>This page should be visible in the screenshot</p>
                    </div>
                </body>
                </html>
            `;
            
            const dataUrl = 'data:text/html;charset=utf-8,' + encodeURIComponent(htmlContent);
            await browser.navigate(dataUrl);
            
            await browser.screenshot('content-page-test.png');
            
            // Verify page content exists
            expect(browser.exists('h1')).toBe(true);
            expect(browser.getText('h1')).toBe('Screenshot Test Page');
        });
    });
    
    describe('Session Persistence', () => {
        test('should maintain session across operations', async () => {
            const sessionName = createTestSession();
            const sessionBrowser = createTestBrowser({ session: sessionName });
            
            try {
                await sessionBrowser.navigate('about:blank');
                
                // Execute some JavaScript to set session data
                await sessionBrowser.executeJavaScript(`
                    sessionStorage.setItem('test-key', 'test-value');
                    localStorage.setItem('persist-key', 'persist-value');
                `);
                
                // Verify data was set
                const sessionData = await sessionBrowser.executeJavaScript(
                    `sessionStorage.getItem('test-key')`
                );
                expect(sessionData).toBe('test-value');
                
                const localData = await sessionBrowser.executeJavaScript(
                    `localStorage.getItem('persist-key')`
                );
                expect(localData).toBe('persist-value');
                
            } finally {
                sessionBrowser.destroy();
            }
        });
    });
    
    describe('Error Handling', () => {
        test('should handle invalid URLs gracefully', async () => {
            try {
                await browser.navigate('invalid://url');
            } catch (error) {
                expect(error).toBeInstanceOf(Error);
            }
        });
        
        test('should handle non-existent elements', async () => {
            await browser.navigate('about:blank');
            
            expect(browser.exists('#non-existent')).toBe(false);
            expect(browser.getText('#non-existent')).toBe('');
            expect(browser.count('#non-existent')).toBe(0);
        });
        
        test('should handle JavaScript errors', async () => {
            await browser.navigate('about:blank');
            
            try {
                await browser.executeJavaScript('nonExistentFunction()');
            } catch (error) {
                // Should handle JavaScript execution errors
                expect(error).toBeInstanceOf(Error);
            }
        });
    });
    
    describe('Performance', () => {
        test('should handle rapid operations', async () => {
            await browser.navigate('about:blank');
            
            const start = Date.now();
            
            // Perform many fast operations
            for (let i = 0; i < 10; i++) {
                browser.executeJavaScriptSync(`window.testVar${i} = ${i}`);
            }
            
            for (let i = 0; i < 10; i++) {
                const result = browser.executeJavaScriptSync(`window.testVar${i}`);
                expect(result).toBe(i.toString());
            }
            
            const elapsed = Date.now() - start;
            
            // Should complete reasonably quickly
            expect(elapsed).toBeLessThan(5000);
        });
        
        test('should handle concurrent async operations', async () => {
            await browser.navigate('about:blank');
            
            const operations = [
                browser.executeJavaScript('1 + 1'),
                browser.executeJavaScript('2 + 2'),
                browser.executeJavaScript('3 + 3'),
                browser.executeJavaScript('4 + 4'),
                browser.executeJavaScript('5 + 5')
            ];
            
            const results = await Promise.all(operations);
            
            expect(results).toEqual(['2', '4', '6', '8', '10']);
        });
    });
});