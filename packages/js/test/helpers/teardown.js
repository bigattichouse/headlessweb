/**
 * Jest teardown file for HeadlessWeb tests
 */

module.exports = async () => {
    // Cleanup test sessions
    if (global.cleanupTestSessions) {
        global.cleanupTestSessions();
    }
    
    // Additional cleanup
    console.log('HeadlessWeb test teardown complete');
};