#!/bin/bash

set -e

echo "=== Screenshot Feature Test ==="
echo ""

# Create test HTML with long content
TEST_FILE="/tmp/screenshot_test.html"
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

echo "Test HTML created at: $TEST_FILE"
echo ""

# Test 1: Basic visible area screenshot
echo "Test 1: Basic visible area screenshot"
./hweb-poc --session screenshot_test --url "file://$TEST_FILE" --screenshot test_visible.png

if [[ -f "test_visible.png" ]]; then
    SIZE=$(stat -f%z "test_visible.png" 2>/dev/null || stat -c%s "test_visible.png" 2>/dev/null || echo "0")
    echo "✓ Visible area screenshot created: test_visible.png (${SIZE} bytes)"
else
    echo "✗ Failed to create visible area screenshot"
fi
echo ""

# Test 2: Screenshot with default filename
echo "Test 2: Screenshot with default filename"
./hweb-poc --session screenshot_test --screenshot

if [[ -f "screenshot.png" ]]; then
    echo "✓ Default screenshot created: screenshot.png"
    rm screenshot.png
else
    echo "✗ Failed to create default screenshot"
fi
echo ""

# Test 3: Full page screenshot
echo "Test 3: Full page screenshot"
./hweb-poc --session screenshot_test --screenshot-full test_fullpage.png

if [[ -f "test_fullpage.png" ]]; then
    SIZE=$(stat -f%z "test_fullpage.png" 2>/dev/null || stat -c%s "test_fullpage.png" 2>/dev/null || echo "0")
    echo "✓ Full page screenshot created: test_fullpage.png (${SIZE} bytes)"
    
    # Compare sizes - full page should be larger
    VISIBLE_SIZE=$(stat -f%z "test_visible.png" 2>/dev/null || stat -c%s "test_visible.png" 2>/dev/null || echo "0")
    if [[ $SIZE -gt $VISIBLE_SIZE ]]; then
        echo "✓ Full page screenshot is larger than visible area (expected)"
    else
        echo "? Full page screenshot is not larger - may need to check"
    fi
else
    echo "✗ Failed to create full page screenshot"
fi
echo ""

# Test 4: Screenshot with scroll position
echo "Test 4: Screenshot after scrolling"
./hweb-poc --session screenshot_test --js "window.scrollTo(0, 800); 'scrolled'"
./hweb-poc --session screenshot_test --screenshot test_scrolled.png

if [[ -f "test_scrolled.png" ]]; then
    echo "✓ Screenshot after scroll created: test_scrolled.png"
else
    echo "✗ Failed to create screenshot after scroll"
fi
echo ""

# Test 5: Screenshot of remote site
echo "Test 5: Screenshot of remote website"
./hweb-poc --session screenshot_remote --url "https://example.com" --wait "h1" --screenshot test_remote.png

if [[ -f "test_remote.png" ]]; then
    echo "✓ Remote website screenshot created: test_remote.png"
else
    echo "✗ Failed to create remote website screenshot"
fi
echo ""

# Test 6: Screenshot in subdirectory
echo "Test 6: Screenshot in subdirectory"
mkdir -p screenshots
./hweb-poc --session screenshot_test --screenshot screenshots/test_subdir.png

if [[ -f "screenshots/test_subdir.png" ]]; then
    echo "✓ Screenshot in subdirectory created: screenshots/test_subdir.png"
else
    echo "✗ Failed to create screenshot in subdirectory"
fi
echo ""

# Test 7: Error handling - invalid path
echo "Test 7: Error handling for invalid path"
if ./hweb-poc --session screenshot_test --screenshot /invalid/path/screenshot.png 2>&1 | grep -q "Error"; then
    echo "✓ Invalid path error handled correctly"
else
    echo "✗ Invalid path error not handled properly"
fi
echo ""

# Test 8: Screenshot as part of command chain
echo "Test 8: Screenshot in command chain"
./hweb-poc --session screenshot_chain \
    --url "file://$TEST_FILE" \
    --wait "h1" \
    --js "document.body.style.background = 'lightblue'" \
    --screenshot test_chain.png \
    --js "document.querySelector('h1').innerText"

if [[ -f "test_chain.png" ]]; then
    echo "✓ Screenshot in command chain created: test_chain.png"
else
    echo "✗ Failed to create screenshot in command chain"
fi
echo ""

# Cleanup
echo "=== Cleanup ==="
./hweb-poc --session screenshot_test --end
./hweb-poc --session screenshot_remote --end
./hweb-poc --session screenshot_chain --end

# Show created files
echo ""
echo "Screenshots created:"
ls -la *.png screenshots/*.png 2>/dev/null || echo "No screenshots found"

echo ""
echo "To view screenshots:"
echo "  - Visible area: test_visible.png"
echo "  - Full page: test_fullpage.png"
echo "  - After scroll: test_scrolled.png"
echo "  - Remote site: test_remote.png"

echo ""
echo "=== Screenshot Test Complete ==="

# Optional: Clean up screenshots
echo ""
read -p "Delete test screenshots? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -f test_*.png screenshot.png
    rm -rf screenshots
    rm -f "$TEST_FILE"
    echo "Test files cleaned up"
fi
