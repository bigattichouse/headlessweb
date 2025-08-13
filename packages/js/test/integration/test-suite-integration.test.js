/**
 * Integration tests for TestSuite functionality
 * Tests the complete testing framework with real browser operations
 */

const { Browser } = require('../../index');

describe('TestSuite Integration', () => {
    let browser;
    
    beforeEach(() => {
        browser = createTestBrowser();
    });
    
    afterEach(() => {
        if (browser) {
            browser.destroy();
        }
    });
    
    describe('Complete Test Workflows', () => {
        test('should execute a complete login test workflow', async () => {
            // Create a mock login page
            const loginPageHtml = `
                <!DOCTYPE html>
                <html>
                <head><title>Login Test Page</title></head>
                <body>
                    <h1>Login to Test App</h1>
                    <form id="login-form">
                        <div>
                            <label for="username">Username:</label>
                            <input type="text" id="username" name="username" required>
                        </div>
                        <div>
                            <label for="password">Password:</label>
                            <input type="password" id="password" name="password" required>
                        </div>
                        <button type="button" id="login-btn">Login</button>
                    </form>
                    <div id="dashboard" style="display: none;">
                        <h2>Welcome to Dashboard!</h2>
                        <p>You are successfully logged in.</p>
                    </div>
                    <div id="error-msg" style="display: none; color: red;">
                        Invalid credentials
                    </div>
                    
                    <script>
                        document.getElementById('login-btn').addEventListener('click', function() {
                            const username = document.getElementById('username').value;
                            const password = document.getElementById('password').value;
                            
                            if (username === 'testuser' && password === 'password123') {
                                document.getElementById('login-form').style.display = 'none';
                                document.getElementById('dashboard').style.display = 'block';
                                document.getElementById('error-msg').style.display = 'none';
                            } else {
                                document.getElementById('error-msg').style.display = 'block';
                            }
                        });
                    </script>
                </body>
                </html>
            `;
            
            const dataUrl = 'data:text/html;charset=utf-8,' + encodeURIComponent(loginPageHtml);
            
            // Create and execute test suite
            const test = browser.createTest('Login Workflow Test');
            
            await test
                .navigate(dataUrl)
                .assertExists('h1')
                .assertText('h1', 'Login to Test App')
                .assertExists('#login-form')
                .assertExists('#username')
                .assertExists('#password')
                .assertExists('#login-btn')
                .screenshot('login-page.png')
                .type('#username', 'testuser')
                .type('#password', 'password123')
                .click('#login-btn')
                .wait(100) // Wait for JavaScript to execute
                .assertExists('#dashboard')
                .assertText('#dashboard h2', 'Welcome to Dashboard!')
                .assertNotExists('#error-msg:visible')
                .screenshot('dashboard.png')
                .message('Login workflow completed successfully');
            
            const report = test.generateReport();
            
            expect(report).toBeTestReport();
            expect(report.summary.success).toBe(true);
            expect(report.summary.failed).toBe(0);
            expect(report.steps.length).toBeGreaterThan(10);
            
            // Verify specific steps
            const navigationStep = report.steps.find(step => step.action === 'navigate');
            expect(navigationStep.success).toBe(true);
            
            const typeSteps = report.steps.filter(step => step.action === 'type');
            expect(typeSteps).toHaveLength(2);
            typeSteps.forEach(step => expect(step.success).toBe(true));
            
            const assertSteps = report.steps.filter(step => step.action.startsWith('assert'));
            assertSteps.forEach(step => expect(step.success).toBe(true));
        });
        
        test('should handle test failures gracefully', async () => {
            const test = browser.createTest('Failure Handling Test');
            
            await browser.navigate('about:blank');
            
            try {
                await test
                    .assertExists('body') // This should pass
                    .assertExists('#non-existent-element') // This should fail
                    .message('This step should not be reached');
            } catch (error) {
                // Test should fail on the assertion
                expect(error.message).toContain('Element not found');
            }
            
            const report = test.generateReport();
            
            expect(report.summary.success).toBe(false);
            expect(report.summary.failed).toBeGreaterThan(0);
            
            // First assertion should pass, second should fail
            const assertSteps = report.steps.filter(step => step.action.startsWith('assert'));
            expect(assertSteps[0].success).toBe(true);
            expect(assertSteps[1].success).toBe(false);
        });
    });
    
    describe('E-commerce Test Scenario', () => {
        test('should test an e-commerce shopping flow', async () => {
            // Create a mock e-commerce page
            const ecommerceHtml = `
                <!DOCTYPE html>
                <html>
                <head><title>Test Shop</title></head>
                <body>
                    <h1>Welcome to Test Shop</h1>
                    <div id="products">
                        <div class="product" data-id="1">
                            <h3>Laptop</h3>
                            <p>Price: $999</p>
                            <button class="add-to-cart" data-id="1">Add to Cart</button>
                        </div>
                        <div class="product" data-id="2">
                            <h3>Mouse</h3>
                            <p>Price: $29</p>
                            <button class="add-to-cart" data-id="2">Add to Cart</button>
                        </div>
                    </div>
                    <div id="cart">
                        <h3>Shopping Cart</h3>
                        <span id="cart-count">0</span> items
                        <ul id="cart-items"></ul>
                        <button id="checkout-btn" style="display: none;">Checkout</button>
                    </div>
                    <div id="checkout-success" style="display: none;">
                        <h2>Order Completed!</h2>
                        <p>Thank you for your purchase.</p>
                    </div>
                    
                    <script>
                        let cartItems = [];
                        
                        document.querySelectorAll('.add-to-cart').forEach(button => {
                            button.addEventListener('click', function() {
                                const productId = this.dataset.id;
                                const productElement = this.closest('.product');
                                const productName = productElement.querySelector('h3').textContent;
                                
                                cartItems.push({ id: productId, name: productName });
                                updateCart();
                            });
                        });
                        
                        function updateCart() {
                            document.getElementById('cart-count').textContent = cartItems.length;
                            
                            const cartList = document.getElementById('cart-items');
                            cartList.innerHTML = '';
                            
                            cartItems.forEach(item => {
                                const li = document.createElement('li');
                                li.textContent = item.name;
                                cartList.appendChild(li);
                            });
                            
                            if (cartItems.length > 0) {
                                document.getElementById('checkout-btn').style.display = 'block';
                            }
                        }
                        
                        document.getElementById('checkout-btn').addEventListener('click', function() {
                            document.getElementById('checkout-success').style.display = 'block';
                            document.getElementById('cart').style.display = 'none';
                            document.getElementById('products').style.display = 'none';
                        });
                    </script>
                </body>
                </html>
            `;
            
            const dataUrl = 'data:text/html;charset=utf-8,' + encodeURIComponent(ecommerceHtml);
            
            const test = browser.createTest('E-commerce Shopping Flow');
            
            await test
                .navigate(dataUrl)
                .assertExists('h1')
                .assertText('h1', 'Welcome to Test Shop')
                .assertExists('#products')
                .assertExists('.product')
                .message('Verified shop page loaded')
                
                // Add first item to cart
                .click('.product[data-id="1"] .add-to-cart')
                .wait(100)
                .assertText('#cart-count', '1')
                .message('Added laptop to cart')
                
                // Add second item to cart
                .click('.product[data-id="2"] .add-to-cart')
                .wait(100)
                .assertText('#cart-count', '2')
                .assertExists('#checkout-btn')
                .message('Added mouse to cart')
                
                // Proceed to checkout
                .screenshot('cart-with-items.png')
                .click('#checkout-btn')
                .wait(100)
                .assertExists('#checkout-success')
                .assertText('#checkout-success h2', 'Order Completed!')
                .assertNotExists('#cart:visible')
                .screenshot('checkout-success.png')
                .message('Checkout completed successfully');
            
            const report = test.generateReport();
            
            expect(report.summary.success).toBe(true);
            expect(report.summary.total).toBeGreaterThan(15);
            expect(report.timing.totalTime).toBeGreaterThan(0);
            
            // Verify critical steps
            const clickSteps = report.steps.filter(step => step.action === 'click');
            expect(clickSteps.length).toBeGreaterThanOrEqual(3); // Two add-to-cart + checkout
            
            const screenshots = report.steps.filter(step => step.action === 'screenshot');
            expect(screenshots).toHaveLength(2);
        });
    });
    
    describe('Form Validation Test', () => {
        test('should test form validation scenarios', async () => {
            const formHtml = `
                <!DOCTYPE html>
                <html>
                <head><title>Form Validation Test</title></head>
                <body>
                    <h1>Contact Form</h1>
                    <form id="contact-form">
                        <div>
                            <label for="name">Name (required):</label>
                            <input type="text" id="name" required>
                            <span id="name-error" class="error"></span>
                        </div>
                        <div>
                            <label for="email">Email (required):</label>
                            <input type="email" id="email" required>
                            <span id="email-error" class="error"></span>
                        </div>
                        <div>
                            <label for="message">Message:</label>
                            <textarea id="message"></textarea>
                        </div>
                        <button type="button" id="submit-btn">Submit</button>
                    </form>
                    <div id="success-msg" style="display: none; color: green;">
                        Form submitted successfully!
                    </div>
                    
                    <style>
                        .error { color: red; font-size: 14px; }
                        .invalid { border: 2px solid red; }
                    </style>
                    
                    <script>
                        document.getElementById('submit-btn').addEventListener('click', function() {
                            const name = document.getElementById('name').value.trim();
                            const email = document.getElementById('email').value.trim();
                            let isValid = true;
                            
                            // Clear previous errors
                            document.querySelectorAll('.error').forEach(el => el.textContent = '');
                            document.querySelectorAll('.invalid').forEach(el => el.classList.remove('invalid'));
                            
                            // Validate name
                            if (!name) {
                                document.getElementById('name-error').textContent = 'Name is required';
                                document.getElementById('name').classList.add('invalid');
                                isValid = false;
                            }
                            
                            // Validate email
                            if (!email) {
                                document.getElementById('email-error').textContent = 'Email is required';
                                document.getElementById('email').classList.add('invalid');
                                isValid = false;
                            } else if (!email.includes('@')) {
                                document.getElementById('email-error').textContent = 'Invalid email format';
                                document.getElementById('email').classList.add('invalid');
                                isValid = false;
                            }
                            
                            if (isValid) {
                                document.getElementById('success-msg').style.display = 'block';
                                document.getElementById('contact-form').style.display = 'none';
                            }
                        });
                    </script>
                </body>
                </html>
            `;
            
            const dataUrl = 'data:text/html;charset=utf-8,' + encodeURIComponent(formHtml);
            
            const test = browser.createTest('Form Validation Test');
            
            await test
                .navigate(dataUrl)
                .assertExists('#contact-form')
                .message('Form loaded successfully')
                
                // Test empty form submission
                .click('#submit-btn')
                .wait(100)
                .assertExists('#name-error')
                .assertExists('#email-error')
                .message('Validated empty form shows errors')
                
                // Test invalid email
                .type('#name', 'John Doe')
                .type('#email', 'invalid-email')
                .click('#submit-btn')
                .wait(100)
                .assertText('#email-error', 'Invalid email format')
                .message('Validated invalid email format')
                
                // Test valid form
                .type('#email', 'john@example.com')
                .type('#message', 'This is a test message')
                .click('#submit-btn')
                .wait(100)
                .assertExists('#success-msg')
                .assertText('#success-msg', 'Form submitted successfully!')
                .assertNotExists('#contact-form:visible')
                .message('Form submitted successfully with valid data');
            
            const report = test.generateReport();
            
            expect(report.summary.success).toBe(true);
            expect(report.summary.failed).toBe(0);
            
            // Check that all validation scenarios were tested
            const messageSteps = report.steps.filter(step => step.action === 'message');
            expect(messageSteps.some(step => step.details.message.includes('empty form'))).toBe(true);
            expect(messageSteps.some(step => step.details.message.includes('invalid email'))).toBe(true);
            expect(messageSteps.some(step => step.details.message.includes('valid data'))).toBe(true);
        });
    });
    
    describe('Test Reporting', () => {
        test('should generate comprehensive test reports', async () => {
            const test = browser.createTest('Comprehensive Reporting Test');
            
            await browser.navigate('about:blank');
            
            await test
                .assertExists('body')
                .wait(50)
                .screenshot('report-test.png')
                .message('Test steps for reporting');
            
            const report = test.generateReport();
            const textReport = test.generateTextReport();
            
            // Validate report structure
            expect(report).toBeTestReport();
            expect(report.steps).toHaveLength(4);
            
            // Validate timing information
            expect(report.timing.totalTime).toBeGreaterThan(40); // At least wait time
            expect(report.timing.endTime).toBeGreaterThan(report.timing.startTime);
            
            // Validate text report
            expect(textReport).toContain('Comprehensive Reporting Test');
            expect(textReport).toContain('PASSED');
            expect(textReport).toContain('4/4 passed');
            expect(textReport).toContain('✓');
            
            // Each step should be documented
            expect(textReport).toContain('assertExists');
            expect(textReport).toContain('wait');
            expect(textReport).toContain('screenshot');
            expect(textReport).toContain('message');
        });
        
        test('should track performance metrics', async () => {
            const test = browser.createTest('Performance Tracking Test');
            
            const start = Date.now();
            
            await test
                .navigate('about:blank')
                .wait(100)
                .assertExists('body')
                .screenshot('performance-test.png');
            
            const report = test.generateReport();
            const totalTestTime = Date.now() - start;
            
            // Report timing should be reasonable
            expect(report.timing.totalTime).toBeLessThan(totalTestTime + 100); // Some tolerance
            expect(report.timing.totalTime).toBeGreaterThan(90); // At least wait time
            
            // Individual steps should have timing
            report.steps.forEach(step => {
                expect(step.duration).toBeGreaterThanOrEqual(0);
                expect(typeof step.timestamp).toBe('number');
            });
            
            // Wait step should show significant duration
            const waitStep = report.steps.find(step => step.action === 'wait');
            expect(waitStep.duration).toBeGreaterThanOrEqual(90);
        });
    });
});