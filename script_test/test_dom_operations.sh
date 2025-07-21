#!/bin/bash

# HeadlessWeb DOM Operations Integration Tests
# Tests complex DOM manipulation, selectors, and interactions

set -e

# Test configuration
HWEB="../hweb"
TEST_SERVER_PORT=9876
TEST_SERVER_URL="http://localhost:$TEST_SERVER_PORT"
SERVER_SCRIPT="../test_server/upload-server.js"
TEMP_DIR="/tmp/hweb_dom_test_$$"

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

# Helper function to load complex test page
load_complex_test_page() {
    $HWEB --url "$TEST_SERVER_URL" \
          --execute-js "
              // Create a complex test page dynamically
              document.body.innerHTML = \`
                  <div id='container' class='main-container'>
                      <header id='header' class='page-header'>
                          <h1 id='title' data-test='main-title'>DOM Test Page</h1>
                          <nav class='navigation'>
                              <ul id='nav-list'>
                                  <li><a href='#section1' class='nav-link active'>Section 1</a></li>
                                  <li><a href='#section2' class='nav-link'>Section 2</a></li>
                                  <li><a href='#section3' class='nav-link'>Section 3</a></li>
                              </ul>
                          </nav>
                      </header>
                      
                      <main id='content' class='main-content'>
                          <section id='section1' class='content-section visible' data-index='1'>
                              <h2>Section 1</h2>
                              <p class='text-content'>First section content</p>
                              <button id='btn1' class='action-btn' type='button'>Button 1</button>
                              <input id='input1' class='text-input' type='text' placeholder='Enter text'/>
                          </section>
                          
                          <section id='section2' class='content-section hidden' data-index='2'>
                              <h2>Section 2</h2>
                              <div class='form-group'>
                                  <label for='checkbox1'>Checkbox:</label>
                                  <input id='checkbox1' type='checkbox' name='options' value='opt1'/>
                                  <label for='radio1'>Radio 1:</label>
                                  <input id='radio1' type='radio' name='radiogroup' value='r1'/>
                                  <label for='radio2'>Radio 2:</label>
                                  <input id='radio2' type='radio' name='radiogroup' value='r2'/>
                              </div>
                              <select id='select1' class='dropdown'>
                                  <option value=''>Select option...</option>
                                  <option value='option1'>Option 1</option>
                                  <option value='option2'>Option 2</option>
                                  <option value='option3'>Option 3</option>
                              </select>
                          </section>
                          
                          <section id='section3' class='content-section hidden' data-index='3'>
                              <h2>Section 3</h2>
                              <table id='data-table' class='data-table'>
                                  <thead>
                                      <tr>
                                          <th>Name</th>
                                          <th>Age</th>
                                          <th>City</th>
                                      </tr>
                                  </thead>
                                  <tbody>
                                      <tr class='data-row' data-id='1'>
                                          <td>John Doe</td>
                                          <td>30</td>
                                          <td>New York</td>
                                      </tr>
                                      <tr class='data-row' data-id='2'>
                                          <td>Jane Smith</td>
                                          <td>25</td>
                                          <td>Los Angeles</td>
                                      </tr>
                                  </tbody>
                              </table>
                              <ul id='dynamic-list' class='item-list'>
                                  <li class='list-item'>Item 1</li>
                                  <li class='list-item'>Item 2</li>
                                  <li class='list-item'>Item 3</li>
                              </ul>
                          </section>
                      </main>
                      
                      <footer id='footer' class='page-footer'>
                          <p>&copy; 2024 DOM Test</p>
                      </footer>
                  </div>
                  
                  <style>
                      .hidden { display: none; }
                      .visible { display: block; }
                      .highlight { background-color: yellow; }
                      .active { font-weight: bold; }
                      .error { color: red; border: 1px solid red; }
                      .success { color: green; border: 1px solid green; }
                  </style>
              \`;
              
              return 'Complex test page loaded';
          " 2>/dev/null
}

# ========== Element Selection Tests ==========

test_element_selection_basic() {
    run_test "Basic Element Selection"
    
    load_complex_test_page >/dev/null
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  const results = {
                      // ID selection
                      titleById: !!document.getElementById('title'),
                      containerById: !!document.getElementById('container'),
                      
                      // Class selection
                      navLinks: document.getElementsByClassName('nav-link').length,
                      contentSections: document.getElementsByClassName('content-section').length,
                      
                      // Tag selection
                      allSections: document.getElementsByTagName('section').length,
                      allButtons: document.getElementsByTagName('button').length,
                      
                      // Query selector
                      titleQuery: !!document.querySelector('#title'),
                      allNavLinksQuery: document.querySelectorAll('.nav-link').length,
                      visibleSections: document.querySelectorAll('.content-section.visible').length
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"titleById":true' && \
       echo "$output" | grep -q '"navLinks":3' && \
       echo "$output" | grep -q '"allSections":3' && \
       echo "$output" | grep -q '"visibleSections":1'; then
        log_success "Basic element selection works correctly"
    else
        log_error "Basic element selection failed: $output"
    fi
}

test_advanced_selectors() {
    run_test "Advanced CSS Selectors"
    
    load_complex_test_page >/dev/null
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  const results = {
                      // Attribute selectors
                      dataTestElements: document.querySelectorAll('[data-test]').length,
                      dataIndexElements: document.querySelectorAll('[data-index]').length,
                      typeTextInputs: document.querySelectorAll('input[type=\"text\"]').length,
                      
                      // Pseudo selectors
                      firstChild: document.querySelector('ul li:first-child')?.textContent,
                      lastChild: document.querySelector('ul li:last-child')?.textContent,
                      nthChild: document.querySelector('tr:nth-child(2)')?.dataset?.id,
                      
                      // Descendant selectors
                      navUlLi: document.querySelectorAll('nav ul li').length,
                      sectionH2: document.querySelectorAll('section h2').length,
                      
                      // Adjacent/sibling selectors
                      labelInput: document.querySelectorAll('label + input').length,
                      
                      // Complex selectors
                      visibleSectionButtons: document.querySelectorAll('section.visible button').length,
                      hiddenSectionInputs: document.querySelectorAll('section.hidden input').length
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"dataTestElements":1' && \
       echo "$output" | grep -q '"firstChild":"Item 1"' && \
       echo "$output" | grep -q '"navUlLi":3' && \
       echo "$output" | grep -q '"visibleSectionButtons":1'; then
        log_success "Advanced CSS selectors work correctly"
    else
        log_error "Advanced CSS selectors failed: $output"
    fi
}

test_xpath_selectors() {
    run_test "XPath Selectors"
    
    load_complex_test_page >/dev/null
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // XPath selection using evaluate
                  function selectByXPath(xpath) {
                      const result = document.evaluate(xpath, document, null, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);
                      return result.snapshotLength;
                  }
                  
                  function getByXPath(xpath) {
                      const result = document.evaluate(xpath, document, null, XPathResult.FIRST_ORDERED_NODE_TYPE, null);
                      return result.singleNodeValue;
                  }
                  
                  const results = {
                      // Basic XPath
                      titleByXPath: !!getByXPath('//h1[@id=\"title\"]'),
                      allH2s: selectByXPath('//h2'),
                      
                      // Attribute-based XPath
                      dataIndexElements: selectByXPath('//*[@data-index]'),
                      activeNavLinks: selectByXPath('//a[@class=\"nav-link active\"]'),
                      
                      // Text-based XPath
                      sectionOneByText: !!getByXPath('//section[h2[text()=\"Section 1\"]]'),
                      buttonByText: !!getByXPath('//button[text()=\"Button 1\"]'),
                      
                      // Parent/child XPath
                      navListItems: selectByXPath('//nav//li'),
                      tableCells: selectByXPath('//table//td'),
                      
                      // Complex XPath
                      hiddenSections: selectByXPath('//section[@class=\"content-section hidden\"]'),
                      inputsInHiddenSections: selectByXPath('//section[@class=\"content-section hidden\"]//input')
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"titleByXPath":true' && \
       echo "$output" | grep -q '"allH2s":3' && \
       echo "$output" | grep -q '"sectionOneByText":true' && \
       echo "$output" | grep -q '"hiddenSections":2'; then
        log_success "XPath selectors work correctly"
    else
        log_error "XPath selectors failed: $output"
    fi
}

# ========== Element Manipulation Tests ==========

test_element_content_manipulation() {
    run_test "Element Content Manipulation"
    
    load_complex_test_page >/dev/null
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Test innerHTML manipulation
                  const title = document.getElementById('title');
                  const originalTitle = title.innerHTML;
                  title.innerHTML = 'Modified <em>Title</em>';
                  
                  // Test textContent manipulation
                  const firstP = document.querySelector('.text-content');
                  const originalText = firstP.textContent;
                  firstP.textContent = 'Modified text content';
                  
                  // Test attribute manipulation
                  const input = document.getElementById('input1');
                  input.setAttribute('data-modified', 'true');
                  input.setAttribute('placeholder', 'Modified placeholder');
                  
                  // Test class manipulation
                  const section1 = document.getElementById('section1');
                  section1.classList.add('highlight');
                  section1.classList.remove('visible');
                  section1.classList.toggle('active');
                  
                  // Test style manipulation
                  const button = document.getElementById('btn1');
                  button.style.backgroundColor = 'lightblue';
                  button.style.padding = '10px';
                  
                  const results = {
                      titleChanged: title.innerHTML === 'Modified <em>Title</em>',
                      textChanged: firstP.textContent === 'Modified text content',
                      dataAttrSet: input.getAttribute('data-modified') === 'true',
                      placeholderChanged: input.getAttribute('placeholder') === 'Modified placeholder',
                      classAdded: section1.classList.contains('highlight'),
                      classRemoved: !section1.classList.contains('visible'),
                      classToggled: section1.classList.contains('active'),
                      styleSet: button.style.backgroundColor === 'lightblue'
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"titleChanged":true' && \
       echo "$output" | grep -q '"textChanged":true' && \
       echo "$output" | grep -q '"classAdded":true' && \
       echo "$output" | grep -q '"styleSet":true'; then
        log_success "Element content manipulation works correctly"
    else
        log_error "Element content manipulation failed: $output"
    fi
}

test_dom_tree_manipulation() {
    run_test "DOM Tree Manipulation"
    
    load_complex_test_page >/dev/null
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Create new elements
                  const newSection = document.createElement('section');
                  newSection.id = 'section4';
                  newSection.className = 'content-section';
                  newSection.innerHTML = '<h2>Dynamic Section</h2><p>Created dynamically</p>';
                  
                  const newListItem = document.createElement('li');
                  newListItem.className = 'list-item';
                  newListItem.textContent = 'Dynamic Item';
                  
                  // Insert elements
                  const content = document.getElementById('content');
                  content.appendChild(newSection);
                  
                  const list = document.getElementById('dynamic-list');
                  list.appendChild(newListItem);
                  
                  // Clone element
                  const button1 = document.getElementById('btn1');
                  const clonedButton = button1.cloneNode(true);
                  clonedButton.id = 'btn1-clone';
                  clonedButton.textContent = 'Cloned Button';
                  button1.parentNode.appendChild(clonedButton);
                  
                  // Move element
                  const footer = document.getElementById('footer');
                  const header = document.getElementById('header');
                  footer.appendChild(header); // Move header to footer
                  
                  // Remove element
                  const section2 = document.getElementById('section2');
                  section2.remove();
                  
                  // Replace element
                  const section3 = document.getElementById('section3');
                  const replacementDiv = document.createElement('div');
                  replacementDiv.id = 'replacement';
                  replacementDiv.innerHTML = '<p>Replacement content</p>';
                  section3.parentNode.replaceChild(replacementDiv, section3);
                  
                  const results = {
                      sectionAdded: !!document.getElementById('section4'),
                      listItemAdded: list.children.length === 4,
                      buttonCloned: !!document.getElementById('btn1-clone'),
                      headerMoved: footer.contains(header),
                      section2Removed: !document.getElementById('section2'),
                      section3Replaced: !!document.getElementById('replacement'),
                      totalSections: document.querySelectorAll('section').length
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"sectionAdded":true' && \
       echo "$output" | grep -q '"buttonCloned":true' && \
       echo "$output" | grep -q '"section2Removed":true' && \
       echo "$output" | grep -q '"section3Replaced":true'; then
        log_success "DOM tree manipulation works correctly"
    else
        log_error "DOM tree manipulation failed: $output"
    fi
}

# ========== Form Element Tests ==========

test_form_elements_interaction() {
    run_test "Form Elements Interaction"
    
    load_complex_test_page >/dev/null
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Text input
                  const textInput = document.getElementById('input1');
                  textInput.value = 'Test input value';
                  textInput.focus();
                  
                  // Checkbox
                  const checkbox = document.getElementById('checkbox1');
                  checkbox.checked = true;
                  
                  // Radio buttons
                  const radio1 = document.getElementById('radio1');
                  const radio2 = document.getElementById('radio2');
                  radio2.checked = true;
                  
                  // Select dropdown
                  const select = document.getElementById('select1');
                  select.value = 'option2';
                  
                  // Trigger events
                  textInput.dispatchEvent(new Event('input', { bubbles: true }));
                  checkbox.dispatchEvent(new Event('change', { bubbles: true }));
                  radio2.dispatchEvent(new Event('change', { bubbles: true }));
                  select.dispatchEvent(new Event('change', { bubbles: true }));
                  
                  const results = {
                      textInputValue: textInput.value,
                      textInputFocused: document.activeElement === textInput,
                      checkboxChecked: checkbox.checked,
                      radio1Checked: radio1.checked,
                      radio2Checked: radio2.checked,
                      selectValue: select.value,
                      selectSelectedIndex: select.selectedIndex,
                      selectedOptionText: select.options[select.selectedIndex].text
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"textInputValue":"Test input value"' && \
       echo "$output" | grep -q '"checkboxChecked":true' && \
       echo "$output" | grep -q '"radio2Checked":true' && \
       echo "$output" | grep -q '"selectValue":"option2"'; then
        log_success "Form elements interaction works correctly"
    else
        log_error "Form elements interaction failed: $output"
    fi
}

# ========== Event Handling Tests ==========

test_event_handling() {
    run_test "Event Handling"
    
    load_complex_test_page >/dev/null
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  let eventResults = {
                      clickEvent: false,
                      mouseoverEvent: false,
                      keydownEvent: false,
                      customEvent: false
                  };
                  
                  // Set up event listeners
                  const button = document.getElementById('btn1');
                  button.addEventListener('click', function(e) {
                      eventResults.clickEvent = true;
                      e.target.classList.add('clicked');
                  });
                  
                  button.addEventListener('mouseover', function(e) {
                      eventResults.mouseoverEvent = true;
                  });
                  
                  const input = document.getElementById('input1');
                  input.addEventListener('keydown', function(e) {
                      eventResults.keydownEvent = true;
                      e.target.classList.add('typed');
                  });
                  
                  // Custom event
                  document.addEventListener('customTest', function(e) {
                      eventResults.customEvent = true;
                  });
                  
                  // Trigger events
                  button.click();
                  button.dispatchEvent(new MouseEvent('mouseover', { bubbles: true }));
                  input.dispatchEvent(new KeyboardEvent('keydown', { key: 'a', bubbles: true }));
                  document.dispatchEvent(new CustomEvent('customTest', { detail: { test: true } }));
                  
                  // Check event effects
                  const results = {
                      ...eventResults,
                      buttonHasClickedClass: button.classList.contains('clicked'),
                      inputHasTypedClass: input.classList.contains('typed')
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"clickEvent":true' && \
       echo "$output" | grep -q '"mouseoverEvent":true' && \
       echo "$output" | grep -q '"customEvent":true' && \
       echo "$output" | grep -q '"buttonHasClickedClass":true'; then
        log_success "Event handling works correctly"
    else
        log_error "Event handling failed: $output"
    fi
}

# ========== Dynamic Content Tests ==========

test_dynamic_content_updates() {
    run_test "Dynamic Content Updates"
    
    load_complex_test_page >/dev/null
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  // Create dynamic content updates
                  function updateContent() {
                      // Add rows to table
                      const table = document.getElementById('data-table').getElementsByTagName('tbody')[0];
                      const newRow = table.insertRow();
                      newRow.className = 'data-row';
                      newRow.setAttribute('data-id', '3');
                      newRow.innerHTML = '<td>Bob Johnson</td><td>35</td><td>Chicago</td>';
                      
                      // Update navigation
                      const navList = document.getElementById('nav-list');
                      const newNavItem = document.createElement('li');
                      newNavItem.innerHTML = '<a href=\"#section4\" class=\"nav-link\">Section 4</a>';
                      navList.appendChild(newNavItem);
                      
                      // Toggle visibility
                      const section1 = document.getElementById('section1');
                      const section2 = document.getElementById('section2');
                      section1.classList.replace('visible', 'hidden');
                      section2.classList.replace('hidden', 'visible');
                      
                      // Update counter
                      let counter = parseInt(localStorage.getItem('updateCounter') || '0');
                      counter++;
                      localStorage.setItem('updateCounter', counter.toString());
                      
                      return {
                          tableRows: table.rows.length,
                          navItems: navList.children.length,
                          section1Visible: section1.classList.contains('visible'),
                          section2Visible: section2.classList.contains('visible'),
                          updateCounter: counter
                      };
                  }
                  
                  // Perform multiple updates
                  const result1 = updateContent();
                  const result2 = updateContent();
                  
                  const results = {
                      firstUpdate: result1,
                      secondUpdate: result2,
                      finalTableRows: document.querySelectorAll('#data-table tbody tr').length,
                      finalNavItems: document.querySelectorAll('#nav-list li').length
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"finalTableRows":4' && \
       echo "$output" | grep -q '"finalNavItems":5' && \
       echo "$output" | grep -q '"section2Visible":true'; then
        log_success "Dynamic content updates work correctly"
    else
        log_error "Dynamic content updates failed: $output"
    fi
}

# ========== Element Visibility Tests ==========

test_element_visibility() {
    run_test "Element Visibility Detection"
    
    load_complex_test_page >/dev/null
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  function isElementVisible(element) {
                      const rect = element.getBoundingClientRect();
                      const style = window.getComputedStyle(element);
                      
                      return (
                          rect.width > 0 &&
                          rect.height > 0 &&
                          style.display !== 'none' &&
                          style.visibility !== 'hidden' &&
                          style.opacity !== '0'
                      );
                  }
                  
                  function isElementInViewport(element) {
                      const rect = element.getBoundingClientRect();
                      return (
                          rect.top >= 0 &&
                          rect.left >= 0 &&
                          rect.bottom <= window.innerHeight &&
                          rect.right <= window.innerWidth
                      );
                  }
                  
                  const elements = {
                      header: document.getElementById('header'),
                      section1: document.getElementById('section1'),
                      section2: document.getElementById('section2'),
                      footer: document.getElementById('footer')
                  };
                  
                  // Test visibility
                  const visibility = {};
                  const viewport = {};
                  const dimensions = {};
                  
                  for (const [name, element] of Object.entries(elements)) {
                      if (element) {
                          visibility[name] = isElementVisible(element);
                          viewport[name] = isElementInViewport(element);
                          
                          const rect = element.getBoundingClientRect();
                          dimensions[name] = {
                              width: rect.width,
                              height: rect.height,
                              top: rect.top,
                              left: rect.left
                          };
                      }
                  }
                  
                  // Make section2 visible and test again
                  elements.section2.classList.remove('hidden');
                  elements.section2.classList.add('visible');
                  
                  const results = {
                      initialVisibility: visibility,
                      inViewport: viewport,
                      dimensions: dimensions,
                      section2VisibleAfterToggle: isElementVisible(elements.section2)
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"section1":true' && \
       echo "$output" | grep -q '"section2VisibleAfterToggle":true'; then
        log_success "Element visibility detection works correctly"
    else
        log_error "Element visibility detection failed: $output"
    fi
}

# ========== Performance Tests ==========

test_dom_performance() {
    run_test "DOM Performance"
    
    load_complex_test_page >/dev/null
    
    local output=$(
        $HWEB --url "$TEST_SERVER_URL" \
              --execute-js "
                  const iterations = 1000;
                  
                  // Test element creation performance
                  const createStart = performance.now();
                  const createdElements = [];
                  for (let i = 0; i < iterations; i++) {
                      const div = document.createElement('div');
                      div.className = 'perf-test';
                      div.textContent = 'Element ' + i;
                      createdElements.push(div);
                  }
                  const createEnd = performance.now();
                  
                  // Test DOM insertion performance
                  const insertStart = performance.now();
                  const container = document.getElementById('container');
                  const fragment = document.createDocumentFragment();
                  for (const element of createdElements) {
                      fragment.appendChild(element);
                  }
                  container.appendChild(fragment);
                  const insertEnd = performance.now();
                  
                  // Test query performance
                  const queryStart = performance.now();
                  for (let i = 0; i < iterations; i++) {
                      document.querySelectorAll('.perf-test');
                  }
                  const queryEnd = performance.now();
                  
                  // Test modification performance
                  const modifyStart = performance.now();
                  const perfElements = document.querySelectorAll('.perf-test');
                  for (let i = 0; i < perfElements.length; i++) {
                      perfElements[i].style.color = 'blue';
                  }
                  const modifyEnd = performance.now();
                  
                  const results = {
                      createTime: createEnd - createStart,
                      insertTime: insertEnd - insertStart,
                      queryTime: queryEnd - queryStart,
                      modifyTime: modifyEnd - modifyStart,
                      elementsCreated: createdElements.length,
                      elementsInDOM: document.querySelectorAll('.perf-test').length,
                      performanceTestCompleted: true
                  };
                  
                  return JSON.stringify(results);
              " 2>/dev/null
    )
    
    if echo "$output" | grep -q '"elementsCreated":1000' && \
       echo "$output" | grep -q '"elementsInDOM":1000' && \
       echo "$output" | grep -q '"performanceTestCompleted":true'; then
        log_success "DOM performance test completed successfully"
    else
        log_error "DOM performance test failed: $output"
    fi
}

# Main test execution
main() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  HeadlessWeb DOM Operations Integration Tests${NC}"
    echo -e "${BLUE}========================================${NC}\n"
    
    # Setup
    setup_test_env
    start_test_server
    
    # Run all tests
    test_element_selection_basic
    test_advanced_selectors
    test_xpath_selectors
    test_element_content_manipulation
    test_dom_tree_manipulation
    test_form_elements_interaction
    test_event_handling
    test_dynamic_content_updates
    test_element_visibility
    test_dom_performance
    
    # Print summary
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}  Test Summary${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Tests run: ${TESTS_RUN}"
    echo -e "${GREEN}Tests passed: ${TESTS_PASSED}${NC}"
    echo -e "${RED}Tests failed: ${TESTS_FAILED}${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}All DOM operations tests passed! 🎉${NC}"
        exit 0
    else
        echo -e "\n${RED}Some DOM operations tests failed! ❌${NC}"
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