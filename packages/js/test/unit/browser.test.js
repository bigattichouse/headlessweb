/**
 * Unit tests for Browser class
 */

const { Browser } = require('../../index');

describe('Browser Class', () => {
    let browser;
    
    beforeEach(() => {
        browser = createTestBrowser();
    });
    
    afterEach(() => {
        if (browser) {
            browser.destroy();
        }
    });
    
    describe('Constructor', () => {
        test('should create browser with default options', () => {
            const defaultBrowser = new Browser();
            
            expect(defaultBrowser.session).toBe('default');
            expect(defaultBrowser.headless).toBe(true);
            expect(defaultBrowser.timeout).toBe(30000);
            expect(defaultBrowser.width).toBe(1024);
            expect(defaultBrowser.height).toBe(768);
            
            defaultBrowser.destroy();
        });
        
        test('should create browser with custom options', () => {
            const customBrowser = new Browser({
                session: 'custom-session',
                headless: false,
                timeout: 45000,
                width: 1920,
                height: 1080
            });
            
            expect(customBrowser.session).toBe('custom-session');
            expect(customBrowser.headless).toBe(false);
            expect(customBrowser.timeout).toBe(45000);
            expect(customBrowser.width).toBe(1920);
            expect(customBrowser.height).toBe(1080);
            
            customBrowser.destroy();
        });
        
        test('should emit created event', (done) => {
            const testBrowser = new Browser({ session: 'event-test' });
            
            testBrowser.on('created', (data) => {
                expect(data.session).toBe('event-test');
                testBrowser.destroy();
                done();
            });
        });
    });
    
    describe('Navigation Methods', () => {
        test('should navigate to URL async', async () => {
            const result = await browser.navigate('https://example.com');
            expect(result).toBe(true);
            expect(browser.getCurrentUrl()).toBe('https://example.com');
        });
        
        test('should navigate to URL sync', () => {
            const result = browser.navigateSync('https://example.com');
            expect(result).toBe(true);
            expect(browser.getCurrentUrl()).toBe('https://example.com');
        });
        
        test('should emit navigated event', (done) => {
            browser.on('navigated', (data) => {
                expect(data.url).toBe('https://example.com');
                done();
            });
            
            browser.navigate('https://example.com');
        });
        
        test('should get current URL', () => {
            browser.navigateSync('https://test.com');
            expect(browser.getCurrentUrl()).toBe('https://test.com');
        });
    });
    
    describe('DOM Interaction (Async)', () => {
        beforeEach(async () => {
            await browser.navigate('https://example.com');
        });
        
        test('should click element async', async () => {
            const result = await browser.click('#submit');
            expect(result).toBe(true);
        });
        
        test('should type text async', async () => {
            const result = await browser.type('#username', 'testuser');
            expect(result).toBe(true);
        });
        
        test('should execute JavaScript async', async () => {
            const result = await browser.executeJavaScript('document.title');
            expect(result).toBe('Mock Page Title');
        });
        
        test('should take screenshot async', async () => {
            const result = await browser.screenshot('test.png');
            expect(result).toBe(true);
        });
        
        test('should emit events for interactions', (done) => {
            let eventCount = 0;
            const expectedEvents = ['clicked', 'typed'];
            
            const handleEvent = (eventType) => {
                return () => {
                    eventCount++;
                    if (eventCount === expectedEvents.length) {
                        done();
                    }
                };
            };
            
            browser.on('clicked', handleEvent('clicked'));
            browser.on('typed', handleEvent('typed'));
            
            browser.click('#submit').then(() => {
                browser.type('#input', 'text');
            });
        });
        
        test('should handle errors gracefully', async () => {
            browser.on('error', (error) => {
                expect(error).toBeInstanceOf(Error);
            });
            
            // This should trigger an error event but not throw
            try {
                await browser.click('.non-existent');
            } catch (error) {
                expect(error.message).toContain('non-existent');
            }
        });
    });
    
    describe('DOM Interaction (Sync)', () => {
        beforeEach(() => {
            browser.navigateSync('https://example.com');
        });
        
        test('should click element sync', () => {
            const result = browser.clickSync('#submit');
            expect(result).toBe(true);
        });
        
        test('should type text sync', () => {
            const result = browser.typeSync('#username', 'testuser');
            expect(result).toBe(true);
        });
        
        test('should execute JavaScript sync', () => {
            const result = browser.executeJavaScriptSync('document.title');
            expect(result).toBe('Mock Page Title');
        });
        
        test('should take screenshot sync', () => {
            const result = browser.screenshotSync('test-sync.png');
            expect(result).toBe(true);
        });
    });
    
    describe('Element Query Methods', () => {
        beforeEach(() => {
            browser.navigateSync('https://example.com');
        });
        
        test('should check if element exists', () => {
            expect(browser.exists('h1')).toBe(true);
            expect(browser.exists('.non-existent')).toBe(false);
        });
        
        test('should get element text', () => {
            const text = browser.getText('h1');
            expect(text).toBe('Mock Page Title');
        });
        
        test('should get element HTML', () => {
            const html = browser.getHtml('h1');
            expect(html).toContain('Mock Page Title');
        });
        
        test('should get element attribute', () => {
            browser.typeSync('#username', 'testvalue');
            const value = browser.getAttribute('#username', 'value');
            expect(value).toBe('testvalue');
        });
        
        test('should count elements', () => {
            const count = browser.count('h1');
            expect(count).toBe(1);
            
            const nonExistentCount = browser.count('.non-existent');
            expect(nonExistentCount).toBe(0);
        });
    });
    
    describe('Utility Methods', () => {
        test('should wait for specified time', async () => {
            const start = Date.now();
            await browser.wait(100);
            const elapsed = Date.now() - start;
            
            expect(elapsed).toBeGreaterThanOrEqual(90); // Allow some variance
        });
        
        test('should get page info', () => {
            browser.navigateSync('https://example.com');
            const pageInfo = browser.getPageInfo();
            
            expect(pageInfo).toHaveProperty('url', 'https://example.com');
            expect(pageInfo).toHaveProperty('title');
            expect(pageInfo).toHaveProperty('readyState');
        });
        
        test('should create fluent chain', () => {
            const chain = browser.chain();
            expect(chain).toBeDefined();
            expect(typeof chain.navigate).toBe('function');
            expect(typeof chain.click).toBe('function');
            expect(typeof chain.execute).toBe('function');
        });
        
        test('should create test suite', () => {
            const test = browser.createTest('test-name');
            expect(test).toBeDefined();
            expect(test.name).toBe('test-name');
            expect(test.browser).toBe(browser);
        });
    });
    
    describe('Fluent API', () => {
        test('should chain operations', async () => {
            const results = await browser.chain()
                .navigate('https://example.com')
                .click('#submit')
                .wait(100)
                .execute();
            
            expect(results).toHaveLength(3);
            expect(results[0]).toBe(true); // navigate
            expect(results[1]).toBe(true); // click
            expect(results[2]).toBe(true); // wait
        });
        
        test('should handle chain errors', async () => {
            try {
                await browser.chain()
                    .navigate('https://example.com')
                    .click('.non-existent')
                    .execute();
            } catch (error) {
                expect(error).toBeInstanceOf(Error);
            }
        });
    });
    
    describe('Event Handling', () => {
        test('should emit and handle events', (done) => {
            const events = [];
            
            browser.on('navigated', (data) => {
                events.push({ type: 'navigated', data });
            });
            
            browser.on('clicked', (data) => {
                events.push({ type: 'clicked', data });
                
                expect(events).toHaveLength(2);
                expect(events[0].type).toBe('navigated');
                expect(events[1].type).toBe('clicked');
                done();
            });
            
            browser.navigate('https://example.com').then(() => {
                browser.click('#submit');
            });
        });
        
        test('should handle error events', (done) => {
            browser.on('error', (error) => {
                expect(error).toBeInstanceOf(Error);
                done();
            });
            
            // Trigger an error
            browser.click('.non-existent').catch(() => {
                // Error is handled by event listener
            });
        });
    });
    
    describe('Memory Management', () => {
        test('should emit destroyed event on destroy', (done) => {
            const testBrowser = createTestBrowser();
            
            testBrowser.on('destroyed', () => {
                done();
            });
            
            testBrowser.destroy();
        });
        
        test('should clean up resources', () => {
            const testBrowser = createTestBrowser();
            
            // Should not throw
            expect(() => {
                testBrowser.destroy();
            }).not.toThrow();
        });
    });
});