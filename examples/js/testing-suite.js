#!/usr/bin/env node

/**
 * HeadlessWeb Testing Suite Example
 * 
 * This example demonstrates how to use the TestSuite class for
 * automated testing with assertions and comprehensive reporting.
 */

const { Browser } = require('../../packages/js');

async function loginFlowTest() {
    console.log('=== Login Flow Test Example ===\n');
    
    const browser = new Browser({ session: 'login-test' });
    
    try {
        // Create a test suite
        const test = browser.createTest('Login Flow Test');
        
        console.log('Running login flow test...');
        
        // Execute test steps with assertions
        await test
            .message('Starting login flow test')
            .navigate('https://httpbin.org/forms/post')
            .screenshot('login-step1.png')
            .assertExists('form')
            .assertExists('input[name="custname"]')
            .assertExists('input[name="custtel"]')
            .type('input[name="custname"]', 'John Doe')
            .type('input[name="custtel"]', '555-1234')
            .screenshot('login-step2.png')
            .message('Form filled successfully')
            .assertAttribute('input[name="custname"]', 'value', 'John Doe')
            .click('input[type="submit"]')
            .wait(2000)
            .screenshot('login-step3.png')
            .assertUrl('httpbin.org');
        
        // Generate and display test report
        const report = test.generateReport();
        console.log('\n--- Test Report ---');
        console.log(test.generateTextReport());
        
        if (report.summary.success) {
            console.log('\n✅ Login flow test PASSED!');
        } else {
            console.log('\n❌ Login flow test FAILED!');
        }
        
        return report.summary.success;
        
    } catch (error) {
        console.error('❌ Test execution error:', error.message);
        return false;
    }
}

async function elementValidationTest() {
    console.log('\n=== Element Validation Test ===\n');
    
    const browser = new Browser({ session: 'validation-test' });
    
    try {
        const test = browser.createTest('Element Validation Test');
        
        console.log('Running element validation test...');
        
        await test
            .navigate('https://example.com')
            .assertExists('h1')
            .assertExists('p')
            .assertText('h1', 'Example Domain')
            .assertText('p', 'This domain is for use in illustrative examples')
            .assertNotExists('.non-existent-class')
            .assertUrl('example.com')
            .screenshot('validation-test.png');
        
        const report = test.generateReport();
        console.log('\n--- Validation Test Report ---');
        console.log(test.generateTextReport());
        
        return report.summary.success;
        
    } catch (error) {
        console.error('❌ Validation test error:', error.message);
        return false;
    }
}

async function performanceTest() {
    console.log('\n=== Performance Test ===\n');
    
    const browser = new Browser({ session: 'perf-test' });
    
    try {
        const test = browser.createTest('Performance Test');
        
        console.log('Running performance test...');
        
        const startTime = Date.now();
        
        await test
            .navigate('https://httpbin.org/delay/1')
            .wait(500)
            .assertExists('body')
            .navigate('https://httpbin.org/json')
            .assertExists('body')
            .executeJavaScript('JSON.parse(document.body.textContent)')
            .screenshot('perf-test.png');
        
        const endTime = Date.now();
        const totalTime = endTime - startTime;
        
        const report = test.generateReport();
        
        console.log('\n--- Performance Test Report ---');
        console.log(test.generateTextReport());
        console.log(`\nTotal execution time: ${totalTime}ms`);
        console.log(`Average step time: ${Math.round(totalTime / report.steps.length)}ms`);
        
        // Performance assertion
        if (totalTime > 10000) {
            console.log('⚠️  Performance warning: Test took longer than 10 seconds');
        }
        
        return report.summary.success;
        
    } catch (error) {
        console.error('❌ Performance test error:', error.message);
        return false;
    }
}

async function errorHandlingTest() {
    console.log('\n=== Error Handling Test ===\n');
    
    const browser = new Browser({ session: 'error-test' });
    
    try {
        const test = browser.createTest('Error Handling Test');
        
        console.log('Testing error handling...');
        
        // Test successful operations first
        await test
            .navigate('https://example.com')
            .assertExists('h1');
        
        // Test error conditions
        try {
            await test.assertExists('.definitely-does-not-exist');
        } catch (error) {
            console.log('✅ Caught expected assertion error:', error.message);
        }
        
        try {
            await test.assertText('h1', 'Wrong Text That Does Not Exist');
        } catch (error) {
            console.log('✅ Caught expected text assertion error:', error.message);
        }
        
        // Continue with successful operations
        await test
            .screenshot('error-handling.png')
            .message('Error handling test completed');
        
        const report = test.generateReport();
        console.log('\n--- Error Handling Report ---');
        console.log(test.generateTextReport());
        
        return true; // We expect some failures in this test
        
    } catch (error) {
        console.error('❌ Unexpected error in error handling test:', error.message);
        return false;
    }
}

async function runTestSuite() {
    console.log('🧪 HeadlessWeb Testing Suite\n');
    console.log('Running comprehensive test suite...\n');
    
    const results = [];
    
    // Run all tests
    results.push(await loginFlowTest());
    results.push(await elementValidationTest());
    results.push(await performanceTest());
    results.push(await errorHandlingTest());
    
    // Summary
    const passed = results.filter(r => r === true).length;
    const total = results.length;
    
    console.log('\n' + '='.repeat(50));
    console.log('TEST SUITE SUMMARY');
    console.log('='.repeat(50));
    console.log(`Tests passed: ${passed}/${total}`);
    console.log(`Success rate: ${Math.round((passed / total) * 100)}%`);
    
    if (passed === total) {
        console.log('\n🎉 All tests PASSED!');
    } else {
        console.log('\n⚠️  Some tests failed. Check the reports above.');
    }
    
    return passed === total;
}

// Execute if run directly
if (require.main === module) {
    runTestSuite().catch(error => {
        console.error('Fatal error:', error);
        process.exit(1);
    });
}

module.exports = { 
    loginFlowTest, 
    elementValidationTest, 
    performanceTest, 
    errorHandlingTest, 
    runTestSuite 
};