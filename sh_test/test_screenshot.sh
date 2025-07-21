#!/bin/bash

# Screenshot Feature Test
# Tests visible area and full page screenshot functionality

set -e

# Helper functions
source "$(dirname "$0")/test_helpers.sh"

# Configuration
TEST_FILE="/tmp/hweb_screenshot_test.html"

# Create test HTML with visual content
create_screenshot_test_html() {
    cat > "$TEST_FILE" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>Screenshot Test Page</title>
    <style>
        body { margin: 20px; font-family: Arial, sans-serif; }
        .header { background: #3498db; color: white; padding: 20px; }
        .content { padding: 20px; }
        .box { 
            width: 200px; 
            height: 200px; 
            margin: 20px; 
            display: inline-block;
            text-align: center;
            line-height: 200px;
            font-size: 24px;
            color: white;
        }
        .red { background: #e74c3c; }
        .green { background: #2ecc71; }
        .blue { background: #3498db; }
        .yellow { background: #f1c40f; }
        .footer { 
            background: #34495e; 
            color: white; 
            padding: 20px; 
            margin-top: 100px;
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>Screenshot Test Page</h1>
        <p>This page tests both visible area and full page screenshots</p>
    </div>
    
    <div class="content">
        <h2>Visible Area Content</h2>
        <div class="box red">Red Box</div>
        <div class="box green">Green Box</div>
        <div class="box blue">Blue Box</div>
        <div class="box yellow">Yellow Box</div>
        
        <h2>Below the Fold Content</h2>
        <p>This content requires scrolling to see...</p>
        <div style="height: 1000px; background: linear-gradient(to bottom, #ecf0f1, #bdc3c7); padding: 20px;">
            <h3>Long scrollable area</h3>
            <p>Scroll position: 500px</p>
            <p style="margin-top: 400px;">Scroll position: 900px</p>
            <p style="margin-top: 400px;">Scroll position: 1300px</p>
        </div>
        
        <div class="footer">
            <h3>Footer Content</h3>
            <p>This should only appear in full page screenshots</p>
        </div>
    </div>
</body>
</html>
EOF
    echo "Created screenshot test HTML: $TEST_FILE"
}

# Test basic visible area screenshot
test_visible_screenshot() {
    echo "=== Test: Visible Area Screenshot ==="
    
    create_screenshot_test_html
    
    check_command "./hweb --session 'screenshot_test' --url 'file://$TEST_FILE' --screenshot 'test_visible.png'" "Visible area screenshot"
    
    if [[ -f "test_visible.png" ]]; then
        SIZE=$(stat -f%z "test_visible.png" 2>/dev/null || stat -c%s "test_visible.png" 2>/dev/null || echo "0")
        echo -e "${GREEN}✓ SUCCESS${NC}: Screenshot file created (${SIZE} bytes)"
    else
        echo -e "${RED}✗ FAIL${NC}: Screenshot file not created"
    fi
    echo ""
}

# Test full page screenshot
test_fullpage_screenshot() {
    echo "=== Test: Full Page Screenshot ==="
    
    check_command "./hweb --session 'screenshot_test' --screenshot-full 'test_fullpage.png'" "Full page screenshot"
    
    if [[ -f "test_fullpage.png" ]]; then
        SIZE=$(stat -f%z "test_fullpage.png" 2>/dev/null || stat -c%s "test_fullpage.png" 2>/dev/null || echo "0")
        echo -e "${GREEN}✓ SUCCESS${NC}: Full page screenshot created (${SIZE} bytes)"
        
        # Compare sizes - full page should be larger
        VISIBLE_SIZE=$(stat -f%z "test_visible.png" 2>/dev/null || stat -c%s "test_visible.png" 2>/dev/null || echo "0")
        if [[ $SIZE -gt $VISIBLE_SIZE ]]; then
            echo -e "${GREEN}✓ PASS${NC}: Full page screenshot is larger than visible area"
        else
            echo -e "${YELLOW}? INFO${NC}: Full page screenshot is not larger - may need to check"
        fi
    else
        echo -e "${RED}✗ FAIL${NC}: Full page screenshot file not created"
    fi
    echo ""
}

# Test default filename
test_default_filename() {
    echo "=== Test: Default Filename Screenshot ==="
    
    check_command "./hweb --session 'screenshot_test' --screenshot" "Screenshot with default filename"
    
    if [[ -f "screenshot.png" ]]; then
        echo -e "${GREEN}✓ SUCCESS${NC}: Default screenshot created"
        rm -f screenshot.png
    else
        echo -e "${RED}✗ FAIL${NC}: Default screenshot not created"
    fi
    echo ""
}

# Test screenshot after scrolling
test_screenshot_with_scroll() {
    echo "=== Test: Screenshot After Scrolling ==="
    
    check_command "./hweb --session 'screenshot_test' --js 'window.scrollTo(0, 800); \"scrolled\"'" "Scroll page"
    check_command "./hweb --session 'screenshot_test' --screenshot 'test_scrolled.png'" "Screenshot after scroll"
    
    if [[ -f "test_scrolled.png" ]]; then
        echo -e "${GREEN}✓ SUCCESS${NC}: Screenshot after scroll created"
    else
        echo -e "${RED}✗ FAIL${NC}: Screenshot after scroll not created"
    fi
    echo ""
}

# Test screenshot in command chain
test_screenshot_command_chain() {
    echo "=== Test: Screenshot in Command Chain ==="
    
    check_command "./hweb --session 'screenshot_chain' \
        --url 'file://$TEST_FILE' \
        --wait 'h1' \
        --js 'document.body.style.background = \"lightblue\"' \
        --screenshot 'test_chain.png' \
        --js 'document.querySelector(\"h1\").innerText'" "Screenshot in command chain"
    
    if [[ -f "test_chain.png" ]]; then
        echo -e "${GREEN}✓ SUCCESS${NC}: Screenshot in command chain created"
    else
        echo -e "${RED}✗ FAIL${NC}: Screenshot in command chain not created"
    fi
    echo ""
}

# Test screenshot with remote site
test_remote_screenshot() {
    echo "=== Test: Remote Website Screenshot ==="
    
    check_command "./hweb --session 'screenshot_remote' --url 'https://example.com' --wait 'h1' --screenshot 'test_remote.png'" "Remote website screenshot"
    
    if [[ -f "test_remote.png" ]]; then
        echo -e "${GREEN}✓ SUCCESS${NC}: Remote website screenshot created"
    else
        echo -e "${RED}✗ FAIL${NC}: Remote website screenshot not created"
    fi
    echo ""
}

# Test error handling
test_screenshot_errors() {
    echo "=== Test: Screenshot Error Handling ==="
    
    # Test invalid path
    if ./hweb --session 'screenshot_test' --screenshot '/invalid/path/screenshot.png' 2>&1 | grep -q "Error"; then
        echo -e "${GREEN}✓ PASS${NC}: Invalid path error handled correctly"
    else
        echo -e "${RED}✗ FAIL${NC}: Invalid path error not handled properly"
    fi
    echo ""
}

# Test screenshot subdirectory
test_screenshot_subdirectory() {
    echo "=== Test: Screenshot in Subdirectory ==="
    
    mkdir -p screenshots_test
    check_command "./hweb --session 'screenshot_test' --screenshot 'screenshots_test/test_subdir.png'" "Screenshot in subdirectory"
    
    if [[ -f "screenshots_test/test_subdir.png" ]]; then
        echo -e "${GREEN}✓ SUCCESS${NC}: Screenshot in subdirectory created"
    else
        echo -e "${RED}✗ FAIL${NC}: Screenshot in subdirectory not created"
    fi
    echo ""
}

# Cleanup function
cleanup_screenshots() {
    echo "=== Screenshot Cleanup ==="
    ./hweb --session 'screenshot_test' --end >/dev/null 2>&1 || true
    ./hweb --session 'screenshot_remote' --end >/dev/null 2>&1 || true  
    ./hweb --session 'screenshot_chain' --end >/dev/null 2>&1 || true
    
    rm -f test_*.png screenshot.png
    rm -rf screenshots_test
    rm -f "$TEST_FILE"
    echo "Screenshot test files cleaned up"
    echo ""
}

# Main test execution
main() {
    echo "=== Screenshot Feature Test Suite ==="
    echo ""
    
    test_visible_screenshot
    test_fullpage_screenshot
    test_default_filename
    test_screenshot_with_scroll
    test_screenshot_command_chain
    test_remote_screenshot
    test_screenshot_errors
    test_screenshot_subdirectory
    
    cleanup_screenshots
    
    echo "=== Screenshot Tests Complete ==="
    echo ""
    echo "Screenshot features tested:"
    echo "✓ Visible area screenshots"
    echo "✓ Full page screenshots"
    echo "✓ Default filename handling"
    echo "✓ Screenshots with scrolling"
    echo "✓ Screenshots in command chains"
    echo "✓ Remote website screenshots"
    echo "✓ Error handling"
    echo "✓ Subdirectory creation"
}

# Run tests if called directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main
fi