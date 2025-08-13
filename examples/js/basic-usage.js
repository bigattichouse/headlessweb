#!/usr/bin/env node

/**
 * Basic HeadlessWeb JavaScript Usage Example
 * 
 * This example demonstrates the core functionality of the HeadlessWeb
 * JavaScript API including navigation, DOM interaction, and data extraction.
 */

const { Browser } = require('../../packages/js');

async function basicExample() {
    console.log('=== HeadlessWeb Basic Usage Example ===\n');
    
    // Create a browser instance
    const browser = new Browser({ 
        session: 'basic-example',
        headless: true 
    });
    
    try {
        // Navigate to a webpage
        console.log('1. Navigating to example.com...');
        await browser.navigate('https://example.com');
        
        // Get page information
        console.log('2. Getting page information...');
        const pageInfo = browser.getPageInfo();
        console.log(`   URL: ${pageInfo.url}`);
        console.log(`   Title: ${pageInfo.title}`);
        console.log(`   Ready State: ${pageInfo.readyState}`);
        
        // Check if elements exist
        console.log('3. Checking for elements...');
        const hasH1 = browser.exists('h1');
        const hasP = browser.exists('p');
        console.log(`   H1 element exists: ${hasH1}`);
        console.log(`   P element exists: ${hasP}`);
        
        // Get text content
        if (hasH1) {
            const h1Text = browser.getText('h1');
            console.log(`   H1 text: "${h1Text}"`);
        }
        
        if (hasP) {
            const pText = browser.getText('p');
            console.log(`   P text: "${pText}"`);
        }
        
        // Execute custom JavaScript
        console.log('4. Executing custom JavaScript...');
        const linksCount = await browser.executeJavaScript('document.querySelectorAll("a").length');
        console.log(`   Number of links: ${linksCount}`);
        
        // Take a screenshot
        console.log('5. Taking screenshot...');
        await browser.screenshot('basic-example.png');
        console.log('   Screenshot saved as: basic-example.png');
        
        console.log('\n✅ Basic example completed successfully!');
        
    } catch (error) {
        console.error('❌ Error in basic example:', error.message);
    }
}

async function chainedOperationsExample() {
    console.log('\n=== Chained Operations Example ===\n');
    
    const browser = new Browser({ session: 'chained-example' });
    
    try {
        // Use fluent API for chained operations
        console.log('Executing chained operations...');
        
        const results = await browser.chain()
            .navigate('https://httpbin.org/html')
            .wait(1000)
            .screenshot('chained-step1.png')
            .execute();
        
        console.log('Chain results:', results);
        
        // Get some information after the chain
        const title = browser.getText('h1');
        console.log(`Page title: "${title}"`);
        
        console.log('\n✅ Chained operations example completed!');
        
    } catch (error) {
        console.error('❌ Error in chained operations:', error.message);
    }
}

async function eventHandlingExample() {
    console.log('\n=== Event Handling Example ===\n');
    
    const browser = new Browser({ session: 'events-example' });
    
    // Set up event listeners
    browser.on('navigated', (data) => {
        console.log(`📍 Navigated to: ${data.url}`);
    });
    
    browser.on('clicked', (data) => {
        console.log(`👆 Clicked: ${data.selector}`);
    });
    
    browser.on('screenshot', (data) => {
        console.log(`📸 Screenshot taken: ${data.filename}`);
    });
    
    browser.on('error', (error) => {
        console.error(`🚨 Browser error: ${error.message}`);
    });
    
    try {
        console.log('Performing actions with event monitoring...');
        
        await browser.navigate('https://example.com');
        await browser.wait(500);
        await browser.screenshot('events-example.png');
        
        console.log('\n✅ Event handling example completed!');
        
    } catch (error) {
        console.error('❌ Error in event handling:', error.message);
    }
}

// Run all examples
async function runAllExamples() {
    await basicExample();
    await chainedOperationsExample();
    await eventHandlingExample();
    
    console.log('\n🎉 All examples completed!');
}

// Execute if run directly
if (require.main === module) {
    runAllExamples().catch(error => {
        console.error('Fatal error:', error);
        process.exit(1);
    });
}

module.exports = { basicExample, chainedOperationsExample, eventHandlingExample };