#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "Session/Manager.h"
#include "Session/Session.h"
#include "FileOps/DownloadManager.h"
#include "FileOps/UploadManager.h"
#include "Assertion/Manager.h"
#include "../utils/test_helpers.h"
#include <memory>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

class ComplexWorkflowChainsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directories for testing
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("workflow_chains_tests");
        
        // Initialize components
        browser_ = std::make_unique<Browser>();
        session_manager_ = std::make_unique<SessionManager>(temp_dir->getPath());
        download_manager_ = std::make_unique<FileOps::DownloadManager>();
        upload_manager_ = std::make_unique<FileOps::UploadManager>();
        assertion_manager_ = std::make_unique<Assertion::Manager>();
        
        // Set up download directory
        download_manager_->setDownloadDirectory(temp_dir->getPath());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown() override {
        // Cleanup components
        assertion_manager_.reset();
        upload_manager_.reset();
        download_manager_.reset();
        session_manager_.reset();
        browser_.reset();
        temp_dir.reset();
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    
    void loadECommerceTestPage() {
        std::string ecommerce_html = R"(
            <!DOCTYPE html>
            <html>
            <head>
                <title>E-Commerce Test Site</title>
                <style>
                    .product { border: 1px solid #ccc; margin: 10px; padding: 10px; }
                    .cart { position: fixed; top: 10px; right: 10px; }
                    .cart-count { background: red; color: white; padding: 2px 6px; border-radius: 10px; }
                    .hidden { display: none; }
                    .checkout-form { margin: 20px 0; }
                </style>
                <script>
                    let cart = [];
                    let orderCounter = 1;
                    
                    function addToCart(productId, productName, price) {
                        cart.push({id: productId, name: productName, price: price});
                        updateCartDisplay();
                        
                        // Simulate adding to localStorage
                        localStorage.setItem('cart', JSON.stringify(cart));
                    }
                    
                    function updateCartDisplay() {
                        document.getElementById('cart-count').textContent = cart.length;
                        
                        let cartItems = document.getElementById('cart-items');
                        cartItems.innerHTML = '';
                        
                        cart.forEach(item => {
                            let div = document.createElement('div');
                            div.innerHTML = item.name + ' - $' + item.price;
                            cartItems.appendChild(div);
                        });
                        
                        // Show checkout button if cart has items
                        let checkoutBtn = document.getElementById('checkout-btn');
                        if (cart.length > 0) {
                            checkoutBtn.classList.remove('hidden');
                        }
                    }
                    
                    function showCheckout() {
                        document.getElementById('checkout-form').classList.remove('hidden');
                        document.getElementById('product-list').classList.add('hidden');
                    }
                    
                    function processCheckout() {
                        let form = document.getElementById('customer-form');
                        let formData = new FormData(form);
                        
                        // Simulate order processing
                        let orderDiv = document.getElementById('order-confirmation');
                        orderDiv.innerHTML = '<h3>Order #' + orderCounter + ' Confirmed!</h3>';
                        orderDiv.innerHTML += '<p>Customer: ' + formData.get('customer_name') + '</p>';
                        orderDiv.innerHTML += '<p>Email: ' + formData.get('customer_email') + '</p>';
                        orderDiv.innerHTML += '<p>Items: ' + cart.length + '</p>';
                        orderDiv.classList.remove('hidden');
                        
                        // Generate download link for receipt
                        let downloadLink = document.createElement('a');
                        downloadLink.href = 'data:text/plain;charset=utf-8,Order Receipt\\nOrder #' + orderCounter + '\\nCustomer: ' + formData.get('customer_name');
                        downloadLink.download = 'receipt_' + orderCounter + '.txt';
                        downloadLink.textContent = 'Download Receipt';
                        orderDiv.appendChild(downloadLink);
                        
                        orderCounter++;
                        cart = [];
                        localStorage.removeItem('cart');
                        updateCartDisplay();
                    }
                    
                    function searchProducts(query) {
                        let products = document.querySelectorAll('.product');
                        products.forEach(product => {
                            let name = product.querySelector('.product-name').textContent.toLowerCase();
                            if (name.includes(query.toLowerCase()) || query === '') {
                                product.style.display = 'block';
                            } else {
                                product.style.display = 'none';
                            }
                        });
                    }
                    
                    // Initialize cart from localStorage on page load
                    window.onload = function() {
                        let savedCart = localStorage.getItem('cart');
                        if (savedCart) {
                            cart = JSON.parse(savedCart);
                            updateCartDisplay();
                        }
                    };
                </script>
            </head>
            <body>
                <div class="cart">
                    🛒 <span id="cart-count" class="cart-count">0</span>
                    <div id="cart-items"></div>
                    <button id="checkout-btn" class="hidden" onclick="showCheckout()">Checkout</button>
                </div>
                
                <h1>Online Store</h1>
                
                <div id="search-area">
                    <input type="text" id="search-input" placeholder="Search products..." onkeyup="searchProducts(this.value)">
                </div>
                
                <div id="product-list">
                    <div class="product" data-id="1">
                        <h3 class="product-name">Laptop Computer</h3>
                        <p>Price: $999</p>
                        <button onclick="addToCart('1', 'Laptop Computer', 999)">Add to Cart</button>
                    </div>
                    
                    <div class="product" data-id="2">
                        <h3 class="product-name">Wireless Mouse</h3>
                        <p>Price: $29</p>
                        <button onclick="addToCart('2', 'Wireless Mouse', 29)">Add to Cart</button>
                    </div>
                    
                    <div class="product" data-id="3">
                        <h3 class="product-name">USB Keyboard</h3>
                        <p>Price: $79</p>
                        <button onclick="addToCart('3', 'USB Keyboard', 79)">Add to Cart</button>
                    </div>
                    
                    <div class="product" data-id="4">
                        <h3 class="product-name">Monitor Stand</h3>
                        <p>Price: $49</p>
                        <button onclick="addToCart('4', 'Monitor Stand', 49)">Add to Cart</button>
                    </div>
                </div>
                
                <div id="checkout-form" class="checkout-form hidden">
                    <h2>Checkout</h2>
                    <form id="customer-form">
                        <label for="customer_name">Full Name:</label>
                        <input type="text" id="customer_name" name="customer_name" required><br><br>
                        
                        <label for="customer_email">Email:</label>
                        <input type="email" id="customer_email" name="customer_email" required><br><br>
                        
                        <label for="customer_address">Address:</label>
                        <textarea id="customer_address" name="customer_address" rows="3" required></textarea><br><br>
                        
                        <label for="payment_method">Payment Method:</label>
                        <select id="payment_method" name="payment_method" required>
                            <option value="">Select payment method</option>
                            <option value="credit">Credit Card</option>
                            <option value="debit">Debit Card</option>
                            <option value="paypal">PayPal</option>
                        </select><br><br>
                        
                        <button type="button" onclick="processCheckout()">Complete Order</button>
                    </form>
                </div>
                
                <div id="order-confirmation" class="hidden"></div>
            </body>
            </html>
        )";
        
        browser_->loadUri("data:text/html;charset=utf-8," + ecommerce_html);
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    
    void createTestUploadFile(const std::string& filename, const std::string& content) {
        std::filesystem::path file_path = temp_dir->getPath() / filename;
        std::ofstream file(file_path);
        file << content;
        file.close();
    }

    std::unique_ptr<Browser> browser_;
    std::unique_ptr<SessionManager> session_manager_;
    std::unique_ptr<FileOps::DownloadManager> download_manager_;
    std::unique_ptr<FileOps::UploadManager> upload_manager_;
    std::unique_ptr<Assertion::Manager> assertion_manager_;
};

// ========== Complete E-Commerce Workflow Tests ==========

TEST_F(ComplexWorkflowChainsTest, ECommerceWorkflow_BrowseToCheckout) {
    loadECommerceTestPage();
    
    // Step 1: Browse products and verify initial state
    assertion_manager_->addAssertion("element-exists", "#search-input", "", "");
    assertion_manager_->addAssertion("element-text", "#cart-count", "0", "equals");
    bool initial_assertions = assertion_manager_->executeAssertions(*browser_);
    EXPECT_TRUE(initial_assertions);
    
    // Step 2: Search for products
    browser_->fillInput("#search-input", "laptop");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Verify search functionality
    EXPECT_TRUE(browser_->isElementVisible(".product[data-id='1']")); // Laptop should be visible
    
    // Step 3: Add items to cart
    browser_->clickElement(".product[data-id='1'] button"); // Add laptop
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    browser_->clickElement(".product[data-id='2'] button"); // Add mouse
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Verify cart updates
    std::string cart_count = browser_->getElementText("#cart-count");
    EXPECT_EQ(cart_count, "2");
    
    // Step 4: Proceed to checkout
    EXPECT_TRUE(browser_->isElementVisible("#checkout-btn"));
    browser_->clickElement("#checkout-btn");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Verify checkout form appears
    EXPECT_TRUE(browser_->isElementVisible("#checkout-form"));
    EXPECT_FALSE(browser_->isElementVisible("#product-list"));
    
    // Step 5: Fill checkout form
    browser_->fillInput("#customer_name", "Test Customer");
    browser_->fillInput("#customer_email", "test@customer.com");
    browser_->fillInput("#customer_address", "123 Test Street, Test City, TC 12345");
    browser_->selectOption("#payment_method", "credit");
    
    // Step 6: Complete order
    browser_->clickElement("button[onclick='processCheckout()']");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify order confirmation
    EXPECT_TRUE(browser_->isElementVisible("#order-confirmation"));
    std::string confirmation_text = browser_->getElementText("#order-confirmation");
    EXPECT_TRUE(confirmation_text.find("Order #1 Confirmed!") != std::string::npos);
    EXPECT_TRUE(confirmation_text.find("Test Customer") != std::string::npos);
}

TEST_F(ComplexWorkflowChainsTest, ECommerceWorkflow_WithSessionPersistence) {
    loadECommerceTestPage();
    
    // Step 1: Create session and add items to cart
    Session ecommerce_session;
    ecommerce_session.setUrl("data:text/html,ecommerce-test");
    
    browser_->clickElement(".product[data-id='1'] button"); // Add laptop
    browser_->clickElement(".product[data-id='3'] button"); // Add keyboard
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Step 2: Save session state
    browser_->updateSessionData(ecommerce_session);
    bool session_saved = session_manager_->saveSession(ecommerce_session, "ecommerce_workflow");
    EXPECT_TRUE(session_saved);
    
    // Step 3: Simulate browser restart by reloading page
    loadECommerceTestPage();
    
    // Step 4: Restore session (cart should be restored via localStorage simulation)
    // In real implementation, session restoration would reload localStorage
    
    // Step 5: Continue with checkout process
    browser_->clickElement("#checkout-btn");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    browser_->fillInput("#customer_name", "Returning Customer");
    browser_->fillInput("#customer_email", "returning@customer.com");
    browser_->fillInput("#customer_address", "456 Return Ave");
    browser_->selectOption("#payment_method", "paypal");
    
    browser_->clickElement("button[onclick='processCheckout()']");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify successful completion
    EXPECT_TRUE(browser_->isElementVisible("#order-confirmation"));
}

// ========== Multi-Page Navigation Workflows ==========

TEST_F(ComplexWorkflowChainsTest, MultiPageNavigation_WithFormData) {
    // Step 1: Load initial page with form
    std::string page1_html = R"(
        <html><body>
            <h1>Page 1 - Registration</h1>
            <form id="reg-form">
                <input type="text" id="username" name="username" placeholder="Username">
                <input type="email" id="email" name="email" placeholder="Email">
                <button type="button" onclick="goToPage2()">Continue to Page 2</button>
            </form>
            <script>
                function goToPage2() {
                    let username = document.getElementById('username').value;
                    let email = document.getElementById('email').value;
                    if (username && email) {
                        localStorage.setItem('userdata', JSON.stringify({username, email}));
                        window.location.href = '#page2';
                    }
                }
            </script>
        </body></html>
    )";
    
    browser_->loadUri("data:text/html;charset=utf-8," + page1_html);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Step 2: Fill form and navigate
    browser_->fillInput("#username", "testuser");
    browser_->fillInput("#email", "test@example.com");
    browser_->clickElement("button[onclick='goToPage2()']");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Step 3: Save session at this point
    Session multi_page_session;
    browser_->updateSessionData(multi_page_session);
    session_manager_->saveSession(multi_page_session, "multipage_workflow");
    
    // Step 4: Load second page
    std::string page2_html = R"(
        <html><body>
            <h1>Page 2 - Profile Setup</h1>
            <div id="user-info"></div>
            <form id="profile-form">
                <input type="text" id="fullname" placeholder="Full Name">
                <textarea id="bio" placeholder="Bio"></textarea>
                <button type="button" onclick="completeProfile()">Complete Profile</button>
            </form>
            <div id="completion-message" style="display:none;">
                <h2>Profile Complete!</h2>
                <p>Thank you for completing your profile.</p>
            </div>
            <script>
                window.onload = function() {
                    let userdata = localStorage.getItem('userdata');
                    if (userdata) {
                        let data = JSON.parse(userdata);
                        document.getElementById('user-info').innerHTML = 
                            '<p>Username: ' + data.username + '</p>' +
                            '<p>Email: ' + data.email + '</p>';
                    }
                };
                
                function completeProfile() {
                    let fullname = document.getElementById('fullname').value;
                    let bio = document.getElementById('bio').value;
                    if (fullname) {
                        document.getElementById('profile-form').style.display = 'none';
                        document.getElementById('completion-message').style.display = 'block';
                        
                        // Save complete profile
                        let userdata = JSON.parse(localStorage.getItem('userdata') || '{}');
                        userdata.fullname = fullname;
                        userdata.bio = bio;
                        localStorage.setItem('userdata', JSON.stringify(userdata));
                    }
                }
            </script>
        </body></html>
    )";
    
    browser_->loadUri("data:text/html;charset=utf-8," + page2_html);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Step 5: Complete profile form
    browser_->fillInput("#fullname", "Test User Full Name");
    browser_->fillInput("#bio", "This is a test user bio for workflow testing.");
    browser_->clickElement("button[onclick='completeProfile()']");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Step 6: Verify completion
    EXPECT_TRUE(browser_->isElementVisible("#completion-message"));
    std::string completion_text = browser_->getElementText("#completion-message");
    EXPECT_TRUE(completion_text.find("Profile Complete!") != std::string::npos);
    
    // Step 7: Update and save final session
    browser_->updateSessionData(multi_page_session);
    bool final_save = session_manager_->saveSession(multi_page_session, "multipage_workflow_complete");
    EXPECT_TRUE(final_save);
}

// ========== File Upload/Download Workflows ==========

TEST_F(ComplexWorkflowChainsTest, FileOperationWorkflow_UploadProcessDownload) {
    // Step 1: Create test files
    createTestUploadFile("test_document.txt", "This is a test document for upload workflow.");
    createTestUploadFile("test_data.csv", "Name,Age,City\nJohn,30,NYC\nJane,25,LA\nBob,35,Chicago");
    
    // Step 2: Load file processing page
    std::string file_processor_html = R"(
        <html><body>
            <h1>File Processor</h1>
            <form id="upload-form" enctype="multipart/form-data">
                <label for="file-input">Select file to process:</label>
                <input type="file" id="file-input" name="upload_file" accept=".txt,.csv">
                <button type="button" onclick="processFile()">Process File</button>
            </form>
            
            <div id="processing-status" style="display:none;">
                <p>Processing file...</p>
                <div id="progress-bar" style="width:100px; height:20px; border:1px solid #ccc;">
                    <div id="progress-fill" style="width:0%; height:100%; background:green;"></div>
                </div>
            </div>
            
            <div id="results" style="display:none;">
                <h2>Processing Results</h2>
                <div id="results-content"></div>
                <a id="download-link" href="#" style="display:none;">Download Processed File</a>
            </div>
            
            <script>
                function processFile() {
                    let fileInput = document.getElementById('file-input');
                    if (fileInput.files.length > 0) {
                        let file = fileInput.files[0];
                        
                        // Show processing status
                        document.getElementById('processing-status').style.display = 'block';
                        
                        // Simulate file processing with progress
                        let progress = 0;
                        let interval = setInterval(() => {
                            progress += 10;
                            document.getElementById('progress-fill').style.width = progress + '%';
                            
                            if (progress >= 100) {
                                clearInterval(interval);
                                showResults(file);
                            }
                        }, 200);
                    }
                }
                
                function showResults(file) {
                    document.getElementById('processing-status').style.display = 'none';
                    document.getElementById('results').style.display = 'block';
                    
                    let resultsContent = document.getElementById('results-content');
                    resultsContent.innerHTML = 
                        '<p>File processed successfully!</p>' +
                        '<p>Original file: ' + file.name + '</p>' +
                        '<p>File size: ' + file.size + ' bytes</p>' +
                        '<p>File type: ' + file.type + '</p>';
                    
                    // Create download link
                    let downloadLink = document.getElementById('download-link');
                    downloadLink.href = 'data:text/plain;charset=utf-8,Processed content from: ' + file.name;
                    downloadLink.download = 'processed_' + file.name;
                    downloadLink.style.display = 'inline';
                    downloadLink.textContent = 'Download Processed File';
                }
            </script>
        </body></html>
    )";
    
    browser_->loadUri("data:text/html;charset=utf-8," + file_processor_html);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Step 3: Upload file simulation (in real scenario, would use actual file upload)
    std::filesystem::path upload_file = temp_dir->getPath() / "test_document.txt";
    bool upload_prepared = upload_manager_->prepareUpload(upload_file.string());
    EXPECT_TRUE(upload_prepared);
    
    // Simulate file selection and processing
    browser_->executeJS("document.getElementById('file-input').setAttribute('data-file', 'test_document.txt');");
    browser_->clickElement("button[onclick='processFile()']");
    
    // Step 4: Wait for processing to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // Wait for progress animation
    
    // Step 5: Verify results are shown
    EXPECT_TRUE(browser_->isElementVisible("#results"));
    std::string results_text = browser_->getElementText("#results-content");
    EXPECT_TRUE(results_text.find("File processed successfully!") != std::string::npos);
    
    // Step 6: Attempt download (simulate)
    EXPECT_TRUE(browser_->isElementVisible("#download-link"));
    std::string download_href = browser_->getAttribute("#download-link", "href");
    EXPECT_TRUE(download_href.find("data:text/plain") != std::string::npos);
}

// ========== Screenshot + Session + Assertion Workflows ==========

TEST_F(ComplexWorkflowChainsTest, ScreenshotSessionAssertionWorkflow) {
    // Step 1: Load complex visual page
    std::string visual_test_html = R"(
        <html>
        <head>
            <style>
                .visual-element { 
                    width: 200px; height: 100px; 
                    background: linear-gradient(45deg, #ff0000, #00ff00);
                    margin: 20px;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    color: white;
                    font-weight: bold;
                }
                .dynamic-content {
                    transition: all 0.5s ease;
                }
                .changed {
                    background: linear-gradient(45deg, #0000ff, #ffff00);
                    transform: scale(1.1);
                }
            </style>
        </head>
        <body>
            <h1>Visual Test Page</h1>
            <div class="visual-element" id="element1">Element 1</div>
            <div class="visual-element dynamic-content" id="element2">Element 2</div>
            <button onclick="changeElements()">Change Visual Elements</button>
            
            <form id="state-form">
                <input type="text" id="state-input" value="initial state">
                <select id="state-select">
                    <option value="state1" selected>State 1</option>
                    <option value="state2">State 2</option>
                </select>
            </form>
            
            <div id="status-indicator">Ready</div>
            
            <script>
                function changeElements() {
                    document.getElementById('element2').classList.add('changed');
                    document.getElementById('status-indicator').textContent = 'Changed';
                    document.getElementById('state-input').value = 'changed state';
                    document.getElementById('state-select').value = 'state2';
                }
            </script>
        </body>
        </html>
    )";
    
    browser_->loadUri("data:text/html;charset=utf-8," + visual_test_html);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Step 2: Create session and take initial screenshot
    Session visual_session;
    browser_->updateSessionData(visual_session);
    
    std::filesystem::path screenshot1_path = temp_dir->getPath() / "initial_screenshot.png";
    bool screenshot1_taken = browser_->takeScreenshot(screenshot1_path.string());
    EXPECT_TRUE(screenshot1_taken);
    EXPECT_TRUE(std::filesystem::exists(screenshot1_path));
    
    // Step 3: Add assertions for initial state
    assertion_manager_->addAssertion("element-text", "#status-indicator", "Ready", "equals");
    assertion_manager_->addAssertion("element-value", "#state-input", "initial state", "equals");
    assertion_manager_->addAssertion("element-value", "#state-select", "state1", "equals");
    
    bool initial_assertions = assertion_manager_->executeAssertions(*browser_);
    EXPECT_TRUE(initial_assertions);
    
    // Step 4: Save initial session state
    bool session_saved = session_manager_->saveSession(visual_session, "visual_workflow_initial");
    EXPECT_TRUE(session_saved);
    
    // Step 5: Trigger changes
    browser_->clickElement("button[onclick='changeElements()']");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Wait for transitions
    
    // Step 6: Take screenshot after changes
    std::filesystem::path screenshot2_path = temp_dir->getPath() / "changed_screenshot.png";
    bool screenshot2_taken = browser_->takeScreenshot(screenshot2_path.string());
    EXPECT_TRUE(screenshot2_taken);
    EXPECT_TRUE(std::filesystem::exists(screenshot2_path));
    
    // Step 7: Add assertions for changed state
    assertion_manager_->addAssertion("element-text", "#status-indicator", "Changed", "equals");
    assertion_manager_->addAssertion("element-value", "#state-input", "changed state", "equals");
    assertion_manager_->addAssertion("element-value", "#state-select", "state2", "equals");
    
    bool changed_assertions = assertion_manager_->executeAssertions(*browser_);
    EXPECT_TRUE(changed_assertions);
    
    // Step 8: Update and save final session
    browser_->updateSessionData(visual_session);
    bool final_session_saved = session_manager_->saveSession(visual_session, "visual_workflow_final");
    EXPECT_TRUE(final_session_saved);
    
    // Step 9: Verify we have different screenshots (different file sizes indicate different content)
    auto size1 = std::filesystem::file_size(screenshot1_path);
    auto size2 = std::filesystem::file_size(screenshot2_path);
    EXPECT_NE(size1, size2); // Screenshots should be different
}

// ========== Error Recovery Workflows ==========

TEST_F(ComplexWorkflowChainsTest, ErrorRecoveryWorkflow_NavigationFailureRecovery) {
    // Step 1: Start successful workflow
    Session recovery_session;
    
    std::string stable_html = "<html><body><h1>Stable Page</h1><input id='test-input' value='stable'></body></html>";
    browser_->loadUri("data:text/html;charset=utf-8," + stable_html);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    browser_->updateSessionData(recovery_session);
    session_manager_->saveSession(recovery_session, "stable_state");
    
    // Step 2: Attempt navigation that might fail
    bool navigation_attempted = browser_->navigate("invalid://malformed-url");
    EXPECT_FALSE(navigation_attempted); // Should fail
    
    // Step 3: Verify browser state is still recoverable
    std::string current_content = browser_->getPageText();
    if (current_content.empty()) {
        // Recovery: reload last known good state
        std::optional<Session> loaded_session = session_manager_->loadSession("stable_state");
        EXPECT_TRUE(loaded_session.has_value());
        
        if (loaded_session) {
            // In real implementation, would restore browser to session state
            browser_->loadUri("data:text/html;charset=utf-8," + stable_html); // Simulate recovery
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }
    
    // Step 4: Verify recovery was successful
    EXPECT_TRUE(browser_->elementExists("#test-input"));
    std::string recovered_value = browser_->getValue("#test-input");
    EXPECT_EQ(recovered_value, "stable");
    
    // Step 5: Continue with valid workflow after recovery
    browser_->fillInput("#test-input", "recovered and continuing");
    
    assertion_manager_->addAssertion("element-value", "#test-input", "recovered and continuing", "equals");
    bool recovery_assertion = assertion_manager_->executeAssertions(*browser_);
    EXPECT_TRUE(recovery_assertion);
}

// ========== Performance Stress Workflows ==========

TEST_F(ComplexWorkflowChainsTest, PerformanceStressWorkflow_RapidOperations) {
    // Step 1: Load page suitable for rapid operations
    std::string stress_test_html = R"(
        <html><body>
            <h1>Stress Test Page</h1>
            <div id="counter">0</div>
            <button id="increment-btn" onclick="increment()">Increment</button>
            <div id="log"></div>
            <script>
                let counter = 0;
                function increment() {
                    counter++;
                    document.getElementById('counter').textContent = counter;
                    
                    let log = document.getElementById('log');
                    let entry = document.createElement('div');
                    entry.textContent = 'Operation ' + counter + ' at ' + new Date().toISOString();
                    log.appendChild(entry);
                    
                    // Keep only last 100 log entries for performance
                    if (log.children.length > 100) {
                        log.removeChild(log.firstChild);
                    }
                }
            </script>
        </body></html>
    )";
    
    browser_->loadUri("data:text/html;charset=utf-8," + stress_test_html);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Step 2: Perform rapid operations
    auto start_time = std::chrono::high_resolution_clock::now();
    
    const int num_operations = 50;
    for (int i = 0; i < num_operations; i++) {
        browser_->clickElement("#increment-btn");
        
        // Add assertions periodically
        if (i % 10 == 0) {
            assertion_manager_->addAssertion("element-exists", "#counter", "", "");
            assertion_manager_->executeAssertions(*browser_);
        }
        
        // Small delay to prevent overwhelming
        if (i % 5 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Step 3: Verify all operations completed successfully
    std::string final_counter = browser_->getElementText("#counter");
    EXPECT_EQ(final_counter, std::to_string(num_operations));
    
    // Performance expectation: should complete within reasonable time
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds for 50 operations
    
    // Step 4: Take final screenshot for verification
    std::filesystem::path stress_screenshot = temp_dir->getPath() / "stress_test_final.png";
    bool screenshot_taken = browser_->takeScreenshot(stress_screenshot.string());
    EXPECT_TRUE(screenshot_taken);
    
    // Step 5: Save session state after stress test
    Session stress_session;
    browser_->updateSessionData(stress_session);
    bool stress_session_saved = session_manager_->saveSession(stress_session, "stress_test_complete");
    EXPECT_TRUE(stress_session_saved);
}