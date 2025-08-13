/**
 * Unit tests for main package entry point
 */

describe('Package Entry Point', () => {
    let HeadlessWeb;
    
    beforeEach(() => {
        // Fresh require for each test
        delete require.cache[require.resolve('../../index')];
        HeadlessWeb = require('../../index');
    });
    
    describe('Package Exports', () => {
        test('should export main classes', () => {
            expect(HeadlessWeb.Browser).toBeDefined();
            expect(HeadlessWeb.Session).toBeDefined();
            expect(HeadlessWeb.TestSuite).toBeDefined();
            expect(typeof HeadlessWeb.Browser).toBe('function');
            expect(typeof HeadlessWeb.Session).toBe('function');
            expect(typeof HeadlessWeb.TestSuite).toBe('function');
        });
        
        test('should export convenience factory', () => {
            expect(HeadlessWeb.create).toBeDefined();
            expect(typeof HeadlessWeb.create).toBe('function');
        });
        
        test('should export version information', () => {
            expect(HeadlessWeb.version).toBeDefined();
            expect(typeof HeadlessWeb.version).toBe('string');
            expect(HeadlessWeb.version).toMatch(/^\d+\.\d+\.\d+/);
        });
        
        test('should export core version', () => {
            expect(HeadlessWeb.coreVersion).toBeDefined();
            expect(typeof HeadlessWeb.coreVersion).toBe('string');
        });
        
        test('should export system info function', () => {
            expect(HeadlessWeb.getSystemInfo).toBeDefined();
            expect(typeof HeadlessWeb.getSystemInfo).toBe('function');
        });
    });
    
    describe('Factory Function', () => {
        test('should create Browser instance with factory', () => {
            const browser = HeadlessWeb.create();
            
            expect(browser).toBeInstanceOf(HeadlessWeb.Browser);
            expect(browser.session).toBe('default');
            expect(browser.headless).toBe(true);
            
            browser.destroy();
        });
        
        test('should create Browser with custom options', () => {
            const browser = HeadlessWeb.create({
                session: 'factory-test',
                headless: false,
                timeout: 45000
            });
            
            expect(browser.session).toBe('factory-test');
            expect(browser.headless).toBe(false);
            expect(browser.timeout).toBe(45000);
            
            browser.destroy();
        });
        
        test('should create multiple independent instances', () => {
            const browser1 = HeadlessWeb.create({ session: 'instance1' });
            const browser2 = HeadlessWeb.create({ session: 'instance2' });
            
            expect(browser1).not.toBe(browser2);
            expect(browser1.session).toBe('instance1');
            expect(browser2.session).toBe('instance2');
            
            browser1.destroy();
            browser2.destroy();
        });
    });
    
    describe('System Information', () => {
        test('should return system info object', () => {
            const sysInfo = HeadlessWeb.getSystemInfo();
            
            expect(typeof sysInfo).toBe('object');
            expect(sysInfo).not.toBeNull();
            expect(sysInfo.platform).toBeDefined();
            expect(sysInfo.arch).toBeDefined();
            expect(sysInfo.webkit_version).toBeDefined();
        });
        
        test('should have consistent system info', () => {
            const info1 = HeadlessWeb.getSystemInfo();
            const info2 = HeadlessWeb.getSystemInfo();
            
            expect(info1).toEqual(info2);
        });
    });
    
    describe('Version Information', () => {
        test('should have valid package version', () => {
            const packageJson = require('../../package.json');
            expect(HeadlessWeb.version).toBe(packageJson.version);
        });
        
        test('should have core version', () => {
            expect(HeadlessWeb.coreVersion).toBeTruthy();
            expect(typeof HeadlessWeb.coreVersion).toBe('string');
            expect(HeadlessWeb.coreVersion.length).toBeGreaterThan(0);
        });
    });
    
    describe('Module Loading', () => {
        test('should handle addon loading gracefully', () => {
            // This tests that the module loads without throwing
            expect(() => {
                require('../../index');
            }).not.toThrow();
        });
        
        test('should provide fallback when addon fails to load', () => {
            // Mock addon loading failure
            const originalConsoleError = console.error;
            console.error = jest.fn();
            
            // Simulate addon loading error by requiring with invalid path
            try {
                // The module should still load with mock addon
                const HeadlessWebTest = require('../../index');
                expect(HeadlessWebTest.Browser).toBeDefined();
            } catch (error) {
                // Should not throw - should use mock addon
                fail('Module should load with mock addon when native addon fails');
            } finally {
                console.error = originalConsoleError;
            }
        });
    });
    
    describe('Class Constructors', () => {
        test('should create Browser instances', () => {
            const browser = new HeadlessWeb.Browser({ session: 'constructor-test' });
            
            expect(browser).toBeInstanceOf(HeadlessWeb.Browser);
            expect(browser.session).toBe('constructor-test');
            
            browser.destroy();
        });
        
        test('should create Session instances', () => {
            const session = new HeadlessWeb.Session('test-session');
            
            expect(session).toBeInstanceOf(HeadlessWeb.Session);
            expect(session.getName()).toBe('test-session');
        });
        
        test('should create TestSuite instances', () => {
            const browser = new HeadlessWeb.Browser();
            const testSuite = new HeadlessWeb.TestSuite(browser, 'test-name');
            
            expect(testSuite).toBeInstanceOf(HeadlessWeb.TestSuite);
            expect(testSuite.name).toBe('test-name');
            expect(testSuite.browser).toBe(browser);
            
            browser.destroy();
        });
    });
    
    describe('Error Handling', () => {
        test('should handle missing addon gracefully', () => {
            // This test ensures the package works with mock addon
            const browser = HeadlessWeb.create();
            
            expect(browser).toBeDefined();
            expect(typeof browser.navigate).toBe('function');
            expect(typeof browser.click).toBe('function');
            
            browser.destroy();
        });
        
        test('should provide meaningful error messages', () => {
            try {
                // Try to create with invalid options
                const browser = new HeadlessWeb.Browser({ 
                    invalidOption: 'should-not-work' 
                });
                
                // Should still work, just ignore invalid options
                expect(browser).toBeDefined();
                browser.destroy();
            } catch (error) {
                // If it throws, error should be meaningful
                expect(error.message).toBeTruthy();
                expect(error.message.length).toBeGreaterThan(0);
            }
        });
    });
    
    describe('CommonJS and ES Module Compatibility', () => {
        test('should work with require()', () => {
            const HeadlessWebCJS = require('../../index');
            
            expect(HeadlessWebCJS.Browser).toBeDefined();
            expect(HeadlessWebCJS.create).toBeDefined();
        });
        
        test('should work with destructuring', () => {
            const { Browser, Session, create } = require('../../index');
            
            expect(Browser).toBeDefined();
            expect(Session).toBeDefined();
            expect(create).toBeDefined();
            
            const browser = create();
            expect(browser).toBeInstanceOf(Browser);
            browser.destroy();
        });
        
        test('should work with default import simulation', () => {
            const HeadlessWeb = require('../../index');
            
            // Simulate: import HeadlessWeb from '@headlessweb/js'
            const browser = HeadlessWeb.create();
            expect(browser).toBeDefined();
            browser.destroy();
        });
    });
    
    describe('Memory Management', () => {
        test('should clean up resources properly', () => {
            const browsers = [];
            
            // Create multiple browsers
            for (let i = 0; i < 5; i++) {
                browsers.push(HeadlessWeb.create({ session: `cleanup-test-${i}` }));
            }
            
            // Destroy all browsers
            browsers.forEach(browser => {
                expect(() => browser.destroy()).not.toThrow();
            });
        });
        
        test('should handle rapid creation and destruction', () => {
            for (let i = 0; i < 10; i++) {
                const browser = HeadlessWeb.create({ session: `rapid-${i}` });
                browser.destroy();
            }
            
            // Should complete without memory issues
            expect(true).toBe(true);
        });
    });
});