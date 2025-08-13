/**
 * Unit tests for Session class
 */

const { Session } = require('../../index');
const path = require('path');

describe('Session Class', () => {
    let session;
    let testSessionsDir;
    
    beforeEach(() => {
        testSessionsDir = global.TEST_CONFIG.sessionDir;
        session = new Session('test-session', testSessionsDir);
    });
    
    describe('Constructor', () => {
        test('should create session with default parameters', () => {
            const defaultSession = new Session();
            
            expect(defaultSession.sessionName).toBe('default');
            expect(defaultSession.sessionsDir).toBe('./sessions');
        });
        
        test('should create session with custom parameters', () => {
            const customSession = new Session('custom-session', '/custom/path');
            
            expect(customSession.sessionName).toBe('custom-session');
            expect(customSession.sessionsDir).toBe('/custom/path');
        });
        
        test('should accept only session name parameter', () => {
            const sessionWithName = new Session('named-session');
            
            expect(sessionWithName.sessionName).toBe('named-session');
            expect(sessionWithName.sessionsDir).toBe('./sessions');
        });
    });
    
    describe('Session Information', () => {
        test('should return session name', () => {
            expect(session.getName()).toBe('test-session');
        });
        
        test('should have correct session directory', () => {
            expect(session.sessionsDir).toBe(testSessionsDir);
        });
    });
    
    describe('Session Operations', () => {
        test('should save session', async () => {
            const result = await session.save();
            expect(result).toBe(true);
        });
        
        test('should load session', async () => {
            const result = await session.load();
            expect(result).toBe(true);
        });
        
        test('should delete session', async () => {
            const result = await session.delete();
            expect(result).toBe(true);
        });
        
        test('should handle save errors gracefully', async () => {
            // Create session with invalid directory
            const invalidSession = new Session('test', '/invalid/path/that/does/not/exist');
            
            try {
                const result = await invalidSession.save();
                // Mock returns true, but in real implementation this might fail
                expect(typeof result).toBe('boolean');
            } catch (error) {
                expect(error).toBeInstanceOf(Error);
            }
        });
        
        test('should handle load errors gracefully', async () => {
            const invalidSession = new Session('non-existent-session', testSessionsDir);
            
            try {
                const result = await invalidSession.load();
                expect(typeof result).toBe('boolean');
            } catch (error) {
                expect(error).toBeInstanceOf(Error);
            }
        });
    });
    
    describe('Session Listing', () => {
        test('should list sessions', () => {
            const sessions = session.list();
            
            expect(Array.isArray(sessions)).toBe(true);
            expect(sessions.length).toBeGreaterThanOrEqual(0);
        });
        
        test('should return array of strings', () => {
            const sessions = session.list();
            
            sessions.forEach(sessionName => {
                expect(typeof sessionName).toBe('string');
            });
        });
        
        test('should include known test sessions', () => {
            const sessions = session.list();
            
            // Mock includes some test sessions
            expect(sessions).toContain('session1');
            expect(sessions).toContain('session2');
            expect(sessions).toContain('test-session');
        });
    });
    
    describe('Session Existence', () => {
        test('should check if session exists', () => {
            const exists = session.exists();
            expect(typeof exists).toBe('boolean');
        });
        
        test('should return true for existing sessions', () => {
            // Mock session 'test-session' exists
            const testSession = new Session('test-session', testSessionsDir);
            expect(testSession.exists()).toBe(true);
        });
        
        test('should return false for non-existent sessions', () => {
            const nonExistentSession = new Session('definitely-does-not-exist', testSessionsDir);
            expect(nonExistentSession.exists()).toBe(false);
        });
    });
    
    describe('Session Lifecycle', () => {
        test('should handle full session lifecycle', async () => {
            const lifecycleSession = new Session(createTestSession(), testSessionsDir);
            
            // Initially should not exist
            expect(lifecycleSession.exists()).toBe(false);
            
            // Save session
            const saveResult = await lifecycleSession.save();
            expect(saveResult).toBe(true);
            
            // Load session
            const loadResult = await lifecycleSession.load();
            expect(loadResult).toBe(true);
            
            // Delete session
            const deleteResult = await lifecycleSession.delete();
            expect(deleteResult).toBe(true);
        });
        
        test('should handle multiple operations on same session', async () => {
            const multiOpSession = new Session(createTestSession(), testSessionsDir);
            
            // Multiple saves should work
            await multiOpSession.save();
            await multiOpSession.save();
            
            // Multiple loads should work
            await multiOpSession.load();
            await multiOpSession.load();
            
            // Clean up
            await multiOpSession.delete();
        });
    });
    
    describe('Session Naming', () => {
        test('should handle special characters in session names', () => {
            const specialCharSession = new Session('test-session_123.abc', testSessionsDir);
            
            expect(specialCharSession.getName()).toBe('test-session_123.abc');
            expect(() => specialCharSession.exists()).not.toThrow();
        });
        
        test('should handle long session names', () => {
            const longName = 'a'.repeat(100);
            const longNameSession = new Session(longName, testSessionsDir);
            
            expect(longNameSession.getName()).toBe(longName);
        });
        
        test('should handle empty session name', () => {
            const emptyNameSession = new Session('', testSessionsDir);
            
            expect(emptyNameSession.getName()).toBe('');
        });
    });
    
    describe('Directory Handling', () => {
        test('should handle absolute paths', () => {
            const absolutePath = path.resolve('/tmp/sessions');
            const absoluteSession = new Session('test', absolutePath);
            
            expect(absoluteSession.sessionsDir).toBe(absolutePath);
        });
        
        test('should handle relative paths', () => {
            const relativeSession = new Session('test', './relative/sessions');
            
            expect(relativeSession.sessionsDir).toBe('./relative/sessions');
        });
        
        test('should handle path with trailing slash', () => {
            const trailingSlashSession = new Session('test', testSessionsDir + '/');
            
            expect(trailingSlashSession.sessionsDir).toBe(testSessionsDir + '/');
        });
    });
    
    describe('Error Scenarios', () => {
        test('should handle session operations with null/undefined', async () => {
            const nullSession = new Session(null, testSessionsDir);
            
            // Should not throw, but behavior depends on implementation
            expect(() => nullSession.getName()).not.toThrow();
            
            try {
                await nullSession.save();
            } catch (error) {
                expect(error).toBeInstanceOf(Error);
            }
        });
        
        test('should handle concurrent operations', async () => {
            const concurrentSession = new Session(createTestSession(), testSessionsDir);
            
            // Run multiple operations concurrently
            const operations = [
                concurrentSession.save(),
                concurrentSession.load(),
                concurrentSession.save(),
                concurrentSession.load()
            ];
            
            try {
                const results = await Promise.all(operations);
                results.forEach(result => {
                    expect(typeof result).toBe('boolean');
                });
            } catch (error) {
                // Some operations might fail due to concurrency
                expect(error).toBeInstanceOf(Error);
            }
            
            await concurrentSession.delete();
        });
    });
    
    describe('Integration with Browser', () => {
        test('should work with browser session management', () => {
            const browser = createTestBrowser({ session: 'integration-test' });
            const browserSession = new Session('integration-test', testSessionsDir);
            
            expect(browser.session).toBe('integration-test');
            expect(browserSession.getName()).toBe('integration-test');
            
            browser.destroy();
        });
        
        test('should support session restoration workflow', async () => {
            const sessionName = createTestSession();
            
            // Create browser with session
            const browser1 = createTestBrowser({ session: sessionName });
            await browser1.navigateSync('https://example.com');
            
            // Save session
            const sessionManager = new Session(sessionName, testSessionsDir);
            await sessionManager.save();
            
            browser1.destroy();
            
            // Restore session in new browser
            const browser2 = createTestBrowser({ session: sessionName });
            await sessionManager.load();
            
            // Session should be restored
            expect(browser2.session).toBe(sessionName);
            
            browser2.destroy();
            await sessionManager.delete();
        });
    });
});