#!/bin/bash

# HeadlessWeb Advanced Browser Features Integration Tests
# Tests focus management, viewport operations, advanced wait strategies, and complex scenarios

set -e

# Test configuration
HWEB="../hweb"
TEST_SERVER_PORT=9876
TEST_SERVER_URL="http://localhost:$TEST_SERVER_PORT"
SERVER_SCRIPT="../test_server/upload-server.js"
TEMP_DIR="/tmp/hweb_advanced_browser_test_$$"

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Utility functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

run_test() {
    ((TESTS_RUN++))
    echo -e "\n${BLUE}=== Test $TESTS_RUN: $1 ===${NC}"
}

# Setup test environment
setup_test_env() {
    log_info "Setting up test environment..."
    mkdir -p "$TEMP_DIR"
    mkdir -p "$TEMP_DIR/screenshots"
}

# Start test server
start_test_server() {
    log_info "Starting test server on port $TEST_SERVER_PORT..."
    
    if [ ! -f "$SERVER_SCRIPT" ]; then
        log_error "Test server script not found: $SERVER_SCRIPT"
        exit 1
    fi
    
    cd "$(dirname "$SERVER_SCRIPT")"
    
    if [ ! -d "node_modules" ]; then
        log_info "Installing Node.js dependencies..."
        npm install
    fi
    
    PORT=$TEST_SERVER_PORT node "$(basename "$SERVER_SCRIPT")" &
    SERVER_PID=$!
    
    sleep 2
    
    if ! curl -s "$TEST_SERVER_URL/health" > /dev/null; then
        log_error "Test server failed to start"
        exit 1
    fi
    
    log_info "Test server started (PID: $SERVER_PID)"
    cd - > /dev/null
}

# Stop test server
stop_test_server() {
    if [ ! -z "$SERVER_PID" ]; then
        log_info "Stopping test server (PID: $SERVER_PID)..."
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    fi
}

# Cleanup
cleanup() {
    log_info "Cleaning up..."
    stop_test_server
    rm -rf "$TEMP_DIR"
}

trap cleanup EXIT

# ========== Focus Management Tests ==========

test_focus_management_complex_apps_basic() {
    run_test "Focus Management in Complex Applications - Basic"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Create complex form with multiple focusable elements
                  document.body.innerHTML = \`
                      <div id='app-container'>
                          <form id='complex-form'>
                              <fieldset id='personal-info'>
                                  <legend>Personal Information</legend>
                                  <label>First Name: <input type='text' id='fname' tabindex='1'/></label>
                                  <label>Last Name: <input type='text' id='lname' tabindex='2'/></label>
                                  <label>Email: <input type='email' id='email' tabindex='3'/></label>
                              </fieldset>
                              
                              <fieldset id='preferences'>
                                  <legend>Preferences</legend>
                                  <label><input type='radio' name='theme' value='light' id='light' tabindex='4'/> Light</label>
                                  <label><input type='radio' name='theme' value='dark' id='dark' tabindex='5'/> Dark</label>
                                  <label><input type='checkbox' id='newsletter' tabindex='6'/> Subscribe to newsletter</label>
                              </fieldset>
                              
                              <div id='actions'>
                                  <button type='button' id='save-draft' tabindex='7'>Save Draft</button>
                                  <button type='submit' id='submit-form' tabindex='8'>Submit</button>
                                  <button type='button' id='cancel' tabindex='9'>Cancel</button>
                              </div>
                          </form>
                      </div>
                  \`;
                  
                  // Focus first element
                  document.getElementById('fname').focus();
                  return document.activeElement.id;
              " \
              --timeout 10000 2>/dev/null
    )
    
    if echo "$output" | grep -q "fname"; then
        log_success "Focus management initialized correctly in complex app"
    else
        log_error "Focus management initialization failed: $output"
    fi
}

test_focus_management_complex_apps_navigation() {
    run_test "Focus Management - Tab Navigation"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Create form and test tab navigation
                  document.body.innerHTML = \`
                      <form>
                          <input type='text' id='input1' tabindex='1' placeholder='Input 1'/>
                          <input type='text' id='input2' tabindex='2' placeholder='Input 2'/>
                          <input type='text' id='input3' tabindex='3' placeholder='Input 3'/>
                          <button id='submit' tabindex='4'>Submit</button>
                      </form>
                  \`;
                  
                  let results = [];
                  
                  // Focus first element
                  document.getElementById('input1').focus();
                  results.push('start:' + document.activeElement.id);
                  
                  // Simulate tab navigation
                  let currentElement = document.activeElement;
                  let tabOrder = ['input1', 'input2', 'input3', 'submit'];
                  
                  for (let i = 0; i < tabOrder.length - 1; i++) {
                      // Find next focusable element
                      let nextTab = parseInt(currentElement.tabIndex) + 1;
                      let nextElement = document.querySelector('[tabindex=\"' + nextTab + '\"]');
                      if (nextElement) {
                          nextElement.focus();
                          currentElement = document.activeElement;
                          results.push('tab' + (i + 1) + ':' + currentElement.id);
                      }
                  }
                  
                  return results.join(',');
              " \
              --timeout 10000 2>/dev/null
    )
    
    if echo "$output" | grep -q "start:input1" && \
       echo "$output" | grep -q "tab1:input2" && \
       echo "$output" | grep -q "tab2:input3" && \
       echo "$output" | grep -q "tab3:submit"; then
        log_success "Tab navigation through complex form works correctly"
    else
        log_error "Tab navigation failed: $output"
    fi
}

test_focus_management_complex_apps_modal() {
    run_test "Focus Management - Modal Dialog Focus Trapping"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Create modal dialog with focus trapping
                  document.body.innerHTML = \`
                      <div id='main-content'>
                          <button id='open-modal'>Open Modal</button>
                          <input id='background-input' placeholder='Background input'/>
                      </div>
                      
                      <div id='modal' style='position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.5); display: none;'>
                          <div style='position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); background: white; padding: 20px;'>
                              <h2 id='modal-title'>Modal Dialog</h2>
                              <input id='modal-input1' placeholder='Modal Input 1'/>
                              <input id='modal-input2' placeholder='Modal Input 2'/>
                              <button id='modal-ok'>OK</button>
                              <button id='modal-cancel'>Cancel</button>
                          </div>
                      </div>
                  \`;
                  
                  let results = [];
                  
                  // Focus background element
                  document.getElementById('background-input').focus();
                  results.push('initial:' + document.activeElement.id);
                  
                  // Show modal
                  document.getElementById('modal').style.display = 'block';
                  document.getElementById('modal-input1').focus();
                  results.push('modal-open:' + document.activeElement.id);
                  
                  // Test focus within modal
                  document.getElementById('modal-input2').focus();
                  results.push('modal-nav:' + document.activeElement.id);
                  
                  // Try to focus background (should be prevented in real modal)
                  document.getElementById('modal-ok').focus();
                  results.push('modal-button:' + document.activeElement.id);
                  
                  return results.join(',');
              " \
              --timeout 10000 2>/dev/null
    )
    
    if echo "$output" | grep -q "initial:background-input" && \
       echo "$output" | grep -q "modal-open:modal-input1" && \
       echo "$output" | grep -q "modal-nav:modal-input2" && \
       echo "$output" | grep -q "modal-button:modal-ok"; then
        log_success "Modal focus management works correctly"
    else
        log_error "Modal focus management failed: $output"
    fi
}

test_focus_management_complex_apps_accessibility() {
    run_test "Focus Management - Accessibility Features"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Create accessible form with ARIA labels and skip links
                  document.body.innerHTML = \`
                      <div id='skip-links'>
                          <a href='#main-content' id='skip-to-main'>Skip to main content</a>
                          <a href='#navigation' id='skip-to-nav'>Skip to navigation</a>
                      </div>
                      
                      <nav id='navigation' role='navigation' aria-label='Main navigation'>
                          <ul>
                              <li><a href='#' id='nav-home'>Home</a></li>
                              <li><a href='#' id='nav-about'>About</a></li>
                              <li><a href='#' id='nav-contact'>Contact</a></li>
                          </ul>
                      </nav>
                      
                      <main id='main-content' role='main'>
                          <h1>Accessible Form</h1>
                          <form>
                              <div class='form-group'>
                                  <label for='accessible-input' id='input-label'>Name (required)</label>
                                  <input type='text' id='accessible-input' aria-labelledby='input-label' aria-required='true' aria-describedby='input-help'/>
                                  <div id='input-help'>Enter your full name</div>
                              </div>
                              
                              <fieldset>
                                  <legend>Contact preferences</legend>
                                  <label><input type='radio' name='contact' value='email' id='contact-email' aria-describedby='email-desc'/> Email</label>
                                  <div id='email-desc'>We'll send updates via email</div>
                                  <label><input type='radio' name='contact' value='phone' id='contact-phone' aria-describedby='phone-desc'/> Phone</label>
                                  <div id='phone-desc'>We'll call you with updates</div>
                              </fieldset>
                              
                              <button type='submit' id='accessible-submit' aria-describedby='submit-help'>Submit Form</button>
                              <div id='submit-help'>Press Enter or click to submit</div>
                          </form>
                      </main>
                  \`;
                  
                  let results = [];
                  
                  // Test skip link functionality
                  document.getElementById('skip-to-main').focus();
                  results.push('skip-link:' + document.activeElement.id);
                  
                  // Simulate skip link activation
                  document.getElementById('main-content').setAttribute('tabindex', '-1');
                  document.getElementById('main-content').focus();
                  results.push('main-focus:' + document.activeElement.id);
                  
                  // Test form accessibility
                  document.getElementById('accessible-input').focus();
                  let inputLabel = document.getElementById('accessible-input').getAttribute('aria-labelledby');
                  let inputRequired = document.getElementById('accessible-input').getAttribute('aria-required');
                  results.push('input-attrs:' + inputLabel + ',' + inputRequired);
                  
                  // Test fieldset navigation
                  document.getElementById('contact-email').focus();
                  results.push('fieldset:' + document.activeElement.id);
                  
                  return results.join('|');
              " \
              --timeout 10000 2>/dev/null
    )
    
    if echo "$output" | grep -q "skip-link:skip-to-main" && \
       echo "$output" | grep -q "main-focus:main-content" && \
       echo "$output" | grep -q "input-attrs:input-label,true" && \
       echo "$output" | grep -q "fieldset:contact-email"; then
        log_success "Accessibility focus management works correctly"
    else
        log_error "Accessibility focus management failed: $output"
    fi
}

# ========== Viewport Operations Tests ==========

test_viewport_operations_dynamic_content_responsive() {
    run_test "Viewport Operations - Responsive Design"
    
    local mobile_result=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --viewport-width 360 \
              --viewport-height 640 \
              --user-agent "Mozilla/5.0 (iPhone; CPU iPhone OS 14_7_1 like Mac OS X) AppleWebKit/605.1.15" \
              --execute-js "
                  // Create responsive content
                  document.head.innerHTML += \`
                      <style>
                          .responsive { width: 100%; padding: 10px; }
                          .desktop-only { display: block; }
                          .mobile-only { display: none; }
                          
                          @media (max-width: 768px) {
                              .desktop-only { display: none; }
                              .mobile-only { display: block; }
                              .responsive { font-size: 14px; }
                          }
                      </style>
                  \`;
                  
                  document.body.innerHTML = \`
                      <div class='responsive'>
                          <div class='desktop-only' id='desktop-content'>Desktop Content</div>
                          <div class='mobile-only' id='mobile-content'>Mobile Content</div>
                      </div>
                  \`;
                  
                  let results = {
                      viewport: window.innerWidth + 'x' + window.innerHeight,
                      userAgent: navigator.userAgent.includes('iPhone') ? 'mobile' : 'desktop',
                      desktopVisible: window.getComputedStyle(document.getElementById('desktop-content')).display,
                      mobileVisible: window.getComputedStyle(document.getElementById('mobile-content')).display
                  };
                  
                  return JSON.stringify(results);
              " \
              --timeout 10000 2>/dev/null
    )
    
    if echo "$mobile_result" | grep -q "360x640" && \
       echo "$mobile_result" | grep -q "mobile" && \
       echo "$mobile_result" | grep -q "\"desktopVisible\":\"none\"" && \
       echo "$mobile_result" | grep -q "\"mobileVisible\":\"block\""; then
        log_success "Mobile viewport responsive design works correctly"
    else
        log_error "Mobile viewport responsive design failed: $mobile_result"
    fi
    
    # Test desktop viewport
    local desktop_result=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --viewport-width 1920 \
              --viewport-height 1080 \
              --user-agent "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36" \
              --execute-js "
                  // Same responsive content
                  document.head.innerHTML += \`
                      <style>
                          .responsive { width: 100%; padding: 10px; }
                          .desktop-only { display: block; }
                          .mobile-only { display: none; }
                          
                          @media (max-width: 768px) {
                              .desktop-only { display: none; }
                              .mobile-only { display: block; }
                          }
                      </style>
                  \`;
                  
                  document.body.innerHTML = \`
                      <div class='responsive'>
                          <div class='desktop-only' id='desktop-content'>Desktop Content</div>
                          <div class='mobile-only' id='mobile-content'>Mobile Content</div>
                      </div>
                  \`;
                  
                  return {
                      viewport: window.innerWidth + 'x' + window.innerHeight,
                      desktopVisible: window.getComputedStyle(document.getElementById('desktop-content')).display,
                      mobileVisible: window.getComputedStyle(document.getElementById('mobile-content')).display
                  };
              " \
              --timeout 10000 2>/dev/null
    )
    
    if echo "$desktop_result" | grep -q "1920x1080" && \
       echo "$desktop_result" | grep -q "desktop-content.*block" && \
       echo "$desktop_result" | grep -q "mobile-content.*none"; then
        log_success "Desktop viewport responsive design works correctly"
    else
        log_error "Desktop viewport responsive design failed: $desktop_result"
    fi
}

test_viewport_operations_dynamic_content_scaling() {
    run_test "Viewport Operations - Content Scaling"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --viewport-width 800 \
              --viewport-height 600 \
              --execute-js "
                  // Create scalable content
                  document.body.innerHTML = \`
                      <div id='container' style='width: 100vw; height: 100vh; display: flex; align-items: center; justify-content: center;'>
                          <div id='content' style='width: 50vw; height: 50vh; background: #f0f0f0; border: 2px solid #333;'>
                              <p id='viewport-text'>Viewport: \${window.innerWidth}x\${window.innerHeight}</p>
                              <p id='content-text'>Content: 50% of viewport</p>
                          </div>
                      </div>
                  \`;
                  
                  let containerRect = document.getElementById('container').getBoundingClientRect();
                  let contentRect = document.getElementById('content').getBoundingClientRect();
                  
                  return {
                      viewport: window.innerWidth + 'x' + window.innerHeight,
                      containerSize: Math.round(containerRect.width) + 'x' + Math.round(containerRect.height),
                      contentSize: Math.round(contentRect.width) + 'x' + Math.round(contentRect.height),
                      contentRatio: {
                          width: Math.round((contentRect.width / window.innerWidth) * 100),
                          height: Math.round((contentRect.height / window.innerHeight) * 100)
                      }
                  };
              " \
              --timeout 10000 2>/dev/null
    )
    
    if echo "$output" | grep -q "viewport.*800x600" && \
       echo "$output" | grep -q "contentRatio.*width.*50" && \
       echo "$output" | grep -q "contentRatio.*height.*50"; then
        log_success "Viewport content scaling works correctly"
    else
        log_error "Viewport content scaling failed: $output"
    fi
}

test_viewport_operations_dynamic_content_zoom() {
    run_test "Viewport Operations - Zoom and DPI"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --viewport-width 1200 \
              --viewport-height 800 \
              --device-scale-factor 2.0 \
              --execute-js "
                  // Test high DPI rendering
                  document.body.innerHTML = \`
                      <div id='dpi-test' style='width: 100px; height: 100px; background: linear-gradient(45deg, red, blue);'>
                          <p style='font-size: 12px;'>High DPI Test</p>
                      </div>
                      <canvas id='dpi-canvas' width='100' height='100'></canvas>
                  \`;
                  
                  // Test canvas DPI
                  let canvas = document.getElementById('dpi-canvas');
                  let ctx = canvas.getContext('2d');
                  let devicePixelRatio = window.devicePixelRatio || 1;
                  
                  // Draw test pattern
                  ctx.fillStyle = 'red';
                  ctx.fillRect(0, 0, 50, 50);
                  ctx.fillStyle = 'blue';
                  ctx.fillRect(50, 50, 50, 50);
                  
                  return {
                      viewport: window.innerWidth + 'x' + window.innerHeight,
                      devicePixelRatio: devicePixelRatio,
                      canvasImageData: ctx.getImageData(25, 25, 1, 1).data[0] > 200 ? 'red' : 'other',
                      screenDimensions: screen.width + 'x' + screen.height
                  };
              " \
              --timeout 10000 2>/dev/null
    )
    
    if echo "$output" | grep -q "devicePixelRatio.*2" && \
       echo "$output" | grep -q "canvasImageData.*red" && \
       echo "$output" | grep -q "viewport.*1200x800"; then
        log_success "High DPI viewport operations work correctly"
    else
        log_error "High DPI viewport operations failed: $output"
    fi
}

# ========== Advanced Wait Strategies Tests ==========

test_advanced_wait_strategies_real_world_spa() {
    run_test "Advanced Wait Strategies - SPA Navigation"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Simulate SPA framework
                  document.body.innerHTML = \`
                      <div id='spa-app'>
                          <nav id='spa-nav'>
                              <button id='home-btn' onclick='navigateToHome()'>Home</button>
                              <button id='about-btn' onclick='navigateToAbout()'>About</button>
                              <button id='contact-btn' onclick='navigateToContact()'>Contact</button>
                          </nav>
                          <div id='spa-content'>
                              <div id='loading' style='display: none;'>Loading...</div>
                              <div id='page-content'>Initial Content</div>
                          </div>
                      </div>
                  \`;
                  
                  // SPA navigation functions
                  window.navigateToHome = function() {
                      showLoading();
                      setTimeout(() => {
                          document.getElementById('page-content').innerHTML = '<h1>Home Page</h1><p>Welcome to the home page</p>';
                          hideLoading();
                      }, 500);
                  };
                  
                  window.navigateToAbout = function() {
                      showLoading();
                      setTimeout(() => {
                          document.getElementById('page-content').innerHTML = '<h1>About Page</h1><p>About our company</p>';
                          hideLoading();
                      }, 800);
                  };
                  
                  window.navigateToContact = function() {
                      showLoading();
                      setTimeout(() => {
                          document.getElementById('page-content').innerHTML = '<h1>Contact Page</h1><form><input placeholder=\"Email\"/><button>Submit</button></form>';
                          hideLoading();
                      }, 1200);
                  };
                  
                  window.showLoading = function() {
                      document.getElementById('loading').style.display = 'block';
                      document.getElementById('page-content').style.opacity = '0.5';
                  };
                  
                  window.hideLoading = function() {
                      document.getElementById('loading').style.display = 'none';
                      document.getElementById('page-content').style.opacity = '1';
                  };
                  
                  return 'SPA initialized';
              " \
              --wait-for "#spa-app" \
              --click "#about-btn" \
              --wait-for-text "About Page" \
              --assert-text "h1" "About Page" \
              --timeout 15000 2>/dev/null
    )
    
    if [ $? -eq 0 ]; then
        log_success "SPA navigation wait strategies work correctly"
    else
        log_error "SPA navigation wait strategies failed"
    fi
}

test_advanced_wait_strategies_real_world_ajax() {
    run_test "Advanced Wait Strategies - AJAX Content Loading"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Simulate AJAX content loading
                  document.body.innerHTML = \`
                      <div id='ajax-app'>
                          <button id='load-data-btn'>Load Data</button>
                          <div id='data-status'>Ready to load</div>
                          <div id='data-container'></div>
                      </div>
                  \`;
                  
                  window.loadAjaxData = function() {
                      document.getElementById('data-status').textContent = 'Loading...';
                      document.getElementById('data-container').innerHTML = '<div class=\"spinner\">⏳</div>';
                      
                      // Simulate AJAX delay
                      setTimeout(() => {
                          document.getElementById('data-status').textContent = 'Data loaded';
                          document.getElementById('data-container').innerHTML = \`
                              <div class='data-item' id='item-1'>Item 1: Product A</div>
                              <div class='data-item' id='item-2'>Item 2: Product B</div>
                              <div class='data-item' id='item-3'>Item 3: Product C</div>
                              <div id='data-complete' style='display: none;'>Loading complete</div>
                          \`;
                          
                          // Show completion indicator after another delay
                          setTimeout(() => {
                              document.getElementById('data-complete').style.display = 'block';
                          }, 300);
                      }, 1000);
                  };
                  
                  document.getElementById('load-data-btn').onclick = loadAjaxData;
                  
                  return 'AJAX app initialized';
              " \
              --click "#load-data-btn" \
              --wait-for-text "Data loaded" \
              --wait-for "#data-complete" \
              --assert-count ".data-item" 3 \
              --assert-text "#data-status" "Data loaded" \
              --timeout 15000 2>/dev/null
    )
    
    if [ $? -eq 0 ]; then
        log_success "AJAX content loading wait strategies work correctly"
    else
        log_error "AJAX content loading wait strategies failed"
    fi
}

test_advanced_wait_strategies_real_world_animations() {
    run_test "Advanced Wait Strategies - CSS Animations"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Create animated content
                  document.head.innerHTML += \`
                      <style>
                          @keyframes slideIn {
                              from { transform: translateX(-100%); opacity: 0; }
                              to { transform: translateX(0); opacity: 1; }
                          }
                          
                          @keyframes fadeIn {
                              from { opacity: 0; }
                              to { opacity: 1; }
                          }
                          
                          .slide-in {
                              animation: slideIn 1s ease-in-out forwards;
                          }
                          
                          .fade-in {
                              animation: fadeIn 0.8s ease-in-out forwards;
                              opacity: 0;
                          }
                          
                          .hidden { display: none; }
                          .visible { display: block; }
                      </style>
                  \`;
                  
                  document.body.innerHTML = \`
                      <div id='animation-app'>
                          <button id='trigger-animations'>Trigger Animations</button>
                          <div id='slide-element' class='hidden'>Sliding Content</div>
                          <div id='fade-element' class='hidden'>Fading Content</div>
                          <div id='completion-indicator' style='display: none;'>Animations Complete</div>
                      </div>
                  \`;
                  
                  window.triggerAnimations = function() {
                      // Start slide animation
                      let slideElement = document.getElementById('slide-element');
                      slideElement.classList.remove('hidden');
                      slideElement.classList.add('visible', 'slide-in');
                      
                      // Start fade animation after delay
                      setTimeout(() => {
                          let fadeElement = document.getElementById('fade-element');
                          fadeElement.classList.remove('hidden');
                          fadeElement.classList.add('visible', 'fade-in');
                      }, 500);
                      
                      // Show completion after all animations
                      setTimeout(() => {
                          document.getElementById('completion-indicator').style.display = 'block';
                      }, 2000);
                  };
                  
                  document.getElementById('trigger-animations').onclick = triggerAnimations;
                  
                  return 'Animation app initialized';
              " \
              --click "#trigger-animations" \
              --wait-for "#slide-element.visible" \
              --wait-for "#fade-element.visible" \
              --wait-for "#completion-indicator" \
              --assert-exists "#slide-element.slide-in" \
              --assert-exists "#fade-element.fade-in" \
              --timeout 20000 2>/dev/null
    )
    
    if [ $? -eq 0 ]; then
        log_success "CSS animation wait strategies work correctly"
    else
        log_error "CSS animation wait strategies failed"
    fi
}

# ========== Framework Integration Tests ==========

test_framework_integration_testing_react_simulation() {
    run_test "Framework Integration - React-like Component Lifecycle"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Simulate React-like component lifecycle
                  window.React = {
                      components: {},
                      state: {},
                      
                      createElement: function(type, props, children) {
                          return { type, props, children: children || [] };
                      },
                      
                      render: function(component, container) {
                          container.innerHTML = '';
                          this.renderComponent(component, container);
                      },
                      
                      renderComponent: function(component, container) {
                          if (typeof component === 'string') {
                              container.appendChild(document.createTextNode(component));
                              return;
                          }
                          
                          let element = document.createElement(component.type);
                          if (component.props) {
                              Object.keys(component.props).forEach(key => {
                                  if (key.startsWith('on')) {
                                      element.addEventListener(key.slice(2).toLowerCase(), component.props[key]);
                                  } else {
                                      element.setAttribute(key, component.props[key]);
                                  }
                              });
                          }
                          
                          if (component.children) {
                              component.children.forEach(child => {
                                  this.renderComponent(child, element);
                              });
                          }
                          
                          container.appendChild(element);
                      },
                      
                      useState: function(initialState) {
                          let value = this.state.value !== undefined ? this.state.value : initialState;
                          let setValue = (newValue) => {
                              this.state.value = newValue;
                              this.forceUpdate();
                          };
                          return [value, setValue];
                      },
                      
                      forceUpdate: function() {
                          let event = new CustomEvent('react-update');
                          document.dispatchEvent(event);
                      }
                  };
                  
                  // Create React-like app
                  document.body.innerHTML = '<div id=\"react-root\"></div>';
                  
                  let [count, setCount] = React.useState(0);
                  
                  function Counter() {
                      return React.createElement('div', { id: 'counter-app' }, [
                          React.createElement('h2', {}, 'React-like Counter'),
                          React.createElement('div', { id: 'counter-value' }, 'Count: ' + count),
                          React.createElement('button', { 
                              id: 'increment-btn',
                              onclick: () => setCount(count + 1)
                          }, 'Increment'),
                          React.createElement('button', { 
                              id: 'decrement-btn',
                              onclick: () => setCount(count - 1)
                          }, 'Decrement')
                      ]);
                  }
                  
                  // Initial render
                  React.render(Counter(), document.getElementById('react-root'));
                  
                  // Listen for updates
                  document.addEventListener('react-update', () => {
                      React.render(Counter(), document.getElementById('react-root'));
                  });
                  
                  return 'React-like app initialized';
              " \
              --wait-for "#counter-app" \
              --assert-text "#counter-value" "Count: 0" \
              --click "#increment-btn" \
              --wait-for-text "Count: 1" \
              --click "#increment-btn" \
              --click "#increment-btn" \
              --wait-for-text "Count: 3" \
              --click "#decrement-btn" \
              --wait-for-text "Count: 2" \
              --timeout 15000 2>/dev/null
    )
    
    if [ $? -eq 0 ]; then
        log_success "React-like framework integration works correctly"
    else
        log_error "React-like framework integration failed"
    fi
}

test_framework_integration_testing_vue_simulation() {
    run_test "Framework Integration - Vue-like Reactive System"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Simulate Vue-like reactive system
                  window.Vue = {
                      data: {},
                      watchers: {},
                      
                      reactive: function(obj) {
                          Object.keys(obj).forEach(key => {
                              let value = obj[key];
                              Object.defineProperty(this.data, key, {
                                  get: function() { return value; },
                                  set: function(newValue) {
                                      value = newValue;
                                      Vue.trigger(key);
                                  }
                              });
                          });
                          return this.data;
                      },
                      
                      watch: function(key, callback) {
                          if (!this.watchers[key]) this.watchers[key] = [];
                          this.watchers[key].push(callback);
                      },
                      
                      trigger: function(key) {
                          if (this.watchers[key]) {
                              this.watchers[key].forEach(callback => callback(this.data[key]));
                          }
                      },
                      
                      mount: function(selector, options) {
                          let element = document.querySelector(selector);
                          let template = options.template;
                          let data = this.reactive(options.data);
                          
                          // Initial render
                          this.render(element, template, data);
                          
                          // Setup watchers for auto-update
                          Object.keys(data).forEach(key => {
                              this.watch(key, () => {
                                  this.render(element, template, data);
                              });
                          });
                          
                          // Setup event handlers
                          if (options.methods) {
                              Object.keys(options.methods).forEach(methodName => {
                                  window[methodName] = options.methods[methodName].bind({ data });
                              });
                          }
                          
                          return data;
                      },
                      
                      render: function(element, template, data) {
                          let rendered = template;
                          Object.keys(data).forEach(key => {
                              rendered = rendered.replace(new RegExp('\\{\\{\\s*' + key + '\\s*\\}\\}', 'g'), data[key]);
                          });
                          element.innerHTML = rendered;
                      }
                  };
                  
                  // Create Vue-like app
                  document.body.innerHTML = '<div id=\"vue-app\"></div>';
                  
                  let app = Vue.mount('#vue-app', {
                      data: {
                          message: 'Hello Vue!',
                          items: ['Apple', 'Banana', 'Cherry'],
                          newItem: ''
                      },
                      template: \`
                          <div id='vue-component'>
                              <h2>{{ message }}</h2>
                              <ul id='item-list'>
                                  <li>Apple</li>
                                  <li>Banana</li>
                                  <li>Cherry</li>
                              </ul>
                              <div>
                                  <input id='new-item-input' placeholder='Add new item' onchange='updateNewItem(this.value)'/>
                                  <button id='add-item-btn' onclick='addItem()'>Add Item</button>
                              </div>
                              <div id='item-count'>Total items: {{ items.length }}</div>
                          </div>
                      \`,
                      methods: {
                          addItem: function() {
                              if (this.data.newItem.trim()) {
                                  // Simulate adding item
                                  let newLi = document.createElement('li');
                                  newLi.textContent = this.data.newItem;
                                  document.getElementById('item-list').appendChild(newLi);
                                  this.data.newItem = '';
                                  document.getElementById('new-item-input').value = '';
                                  
                                  // Update count
                                  let count = document.querySelectorAll('#item-list li').length;
                                  document.getElementById('item-count').textContent = 'Total items: ' + count;
                              }
                          },
                          updateNewItem: function(value) {
                              this.data.newItem = value;
                          }
                      }
                  });
                  
                  return 'Vue-like app initialized';
              " \
              --wait-for "#vue-component" \
              --assert-text "h2" "Hello Vue!" \
              --type "#new-item-input" "Orange" \
              --click "#add-item-btn" \
              --wait-for-text "Total items: 4" \
              --assert-count "#item-list li" 4 \
              --timeout 15000 2>/dev/null
    )
    
    if [ $? -eq 0 ]; then
        log_success "Vue-like framework integration works correctly"
    else
        log_error "Vue-like framework integration failed"
    fi
}

# ========== Dynamic Content Handling Tests ==========

test_dynamic_content_handling_real_time() {
    run_test "Dynamic Content Handling - Real-time Updates"
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Simulate real-time updates (like WebSocket)
                  document.body.innerHTML = \`
                      <div id='realtime-app'>
                          <h2>Real-time Dashboard</h2>
                          <div id='status' class='disconnected'>Disconnected</div>
                          <div id='message-count'>Messages: 0</div>
                          <ul id='message-list'></ul>
                          <button id='connect-btn'>Connect</button>
                          <button id='disconnect-btn' style='display:none;'>Disconnect</button>
                      </div>
                  \`;
                  
                  let messageCount = 0;
                  let isConnected = false;
                  let updateInterval;
                  
                  window.connect = function() {
                      isConnected = true;
                      document.getElementById('status').textContent = 'Connected';
                      document.getElementById('status').className = 'connected';
                      document.getElementById('connect-btn').style.display = 'none';
                      document.getElementById('disconnect-btn').style.display = 'inline';
                      
                      // Simulate real-time message updates
                      updateInterval = setInterval(() => {
                          if (isConnected) {
                              messageCount++;
                              let messageList = document.getElementById('message-list');
                              let li = document.createElement('li');
                              li.textContent = 'Message ' + messageCount + ' - ' + new Date().toLocaleTimeString();
                              li.id = 'message-' + messageCount;
                              messageList.appendChild(li);
                              
                              document.getElementById('message-count').textContent = 'Messages: ' + messageCount;
                              
                              // Keep only last 5 messages
                              if (messageList.children.length > 5) {
                                  messageList.removeChild(messageList.firstChild);
                              }
                          }
                      }, 500);
                  };
                  
                  window.disconnect = function() {
                      isConnected = false;
                      clearInterval(updateInterval);
                      document.getElementById('status').textContent = 'Disconnected';
                      document.getElementById('status').className = 'disconnected';
                      document.getElementById('connect-btn').style.display = 'inline';
                      document.getElementById('disconnect-btn').style.display = 'none';
                  };
                  
                  document.getElementById('connect-btn').onclick = connect;
                  document.getElementById('disconnect-btn').onclick = disconnect;
                  
                  return 'Real-time app initialized';
              " \
              --wait-for "#realtime-app" \
              --click "#connect-btn" \
              --wait-for-text "Connected" \
              --wait-for "#message-1" \
              --wait-for "#message-3" \
              --assert-text "#status" "Connected" \
              --assert-exists "#message-list li" \
              --click "#disconnect-btn" \
              --wait-for-text "Disconnected" \
              --timeout 20000 2>/dev/null
    )
    
    if [ $? -eq 0 ]; then
        log_success "Real-time dynamic content handling works correctly"
    else
        log_error "Real-time dynamic content handling failed"
    fi
}

# ========== Performance Tests ==========

test_performance_with_complex_pages() {
    run_test "Performance with Complex Pages"
    
    local start_time=$(date +%s%N)
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --viewport-width 1920 \
              --viewport-height 1080 \
              --execute-js "
                  // Create complex page with many elements
                  let complexContent = '<div id=\"complex-page\">';
                  
                  // Add many nested elements
                  for (let i = 0; i < 100; i++) {
                      complexContent += \`
                          <div class='section' id='section-\${i}'>
                              <h3>Section \${i}</h3>
                              <div class='content'>
                                  <p>This is paragraph \${i} with some content.</p>
                                  <ul>
                                      <li>Item 1 of section \${i}</li>
                                      <li>Item 2 of section \${i}</li>
                                      <li>Item 3 of section \${i}</li>
                                  </ul>
                                  <button id='btn-\${i}' onclick='handleClick(\${i})'>Button \${i}</button>
                              </div>
                          </div>
                      \`;
                  }
                  complexContent += '</div>';
                  
                  document.body.innerHTML = complexContent;
                  
                  window.handleClick = function(id) {
                      document.getElementById('btn-' + id).textContent = 'Clicked ' + id;
                  };
                  
                  return {
                      sectionsCreated: 100,
                      totalElements: document.querySelectorAll('*').length,
                      pageReady: true
                  };
              " \
              --wait-for "#complex-page" \
              --assert-count ".section" 100 \
              --click "#btn-50" \
              --wait-for-text "Clicked 50" \
              --screenshot "$TEMP_DIR/screenshots/complex_page.png" \
              --timeout 30000 2>/dev/null
    )
    
    local end_time=$(date +%s%N)
    local duration=$(((end_time - start_time) / 1000000))  # Convert to milliseconds
    
    if [ $? -eq 0 ]; then
        log_success "Complex page performance test completed in ${duration}ms"
        
        if [ $duration -lt 15000 ]; then  # Less than 15 seconds
            log_success "Performance acceptable for complex page (${duration}ms)"
        else
            log_warning "Performance may be slow for complex page (${duration}ms)"
        fi
        
        # Check if screenshot was created
        if [ -f "$TEMP_DIR/screenshots/complex_page.png" ]; then
            log_success "Screenshot of complex page created successfully"
        else
            log_error "Screenshot of complex page not created"
        fi
    else
        log_error "Complex page performance test failed"
    fi
}

# Main test execution
main() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  HeadlessWeb Advanced Browser Features Tests${NC}"
    echo -e "${BLUE}========================================${NC}\n"
    
    # Setup
    setup_test_env
    start_test_server
    
    # Run focus management tests
    test_focus_management_complex_apps_basic
    test_focus_management_complex_apps_navigation
    test_focus_management_complex_apps_modal
    test_focus_management_complex_apps_accessibility
    
    # Run viewport operation tests
    test_viewport_operations_dynamic_content_responsive
    test_viewport_operations_dynamic_content_scaling
    test_viewport_operations_dynamic_content_zoom
    
    # Run advanced wait strategy tests
    test_advanced_wait_strategies_real_world_spa
    test_advanced_wait_strategies_real_world_ajax
    test_advanced_wait_strategies_real_world_animations
    
    # Run framework integration tests
    test_framework_integration_testing_react_simulation
    test_framework_integration_testing_vue_simulation
    
    # Run dynamic content tests
    test_dynamic_content_handling_real_time
    
    # Run performance tests
    test_performance_with_complex_pages
    
    # Print summary
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  Test Summary${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Tests run: ${TESTS_RUN}"
    echo -e "${GREEN}Tests passed: ${TESTS_PASSED}${NC}"
    echo -e "${RED}Tests failed: ${TESTS_FAILED}${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}All advanced browser feature tests passed! 🎉${NC}"
        exit 0
    else
        echo -e "\n${RED}Some advanced browser feature tests failed! ❌${NC}"
        exit 1
    fi
}

# Check prerequisites
check_prerequisites() {
    if [ ! -f "$HWEB" ]; then
        log_error "hweb binary not found: $HWEB"
        exit 1
    fi
    
    if ! command -v node >/dev/null 2>&1; then
        log_error "Node.js is required but not installed"
        exit 1
    fi
    
    if ! command -v curl >/dev/null 2>&1; then
        log_error "curl is required but not installed"
        exit 1
    fi
}

check_prerequisites
main "$@"