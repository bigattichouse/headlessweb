#!/bin/bash

echo "=== Minimal test case ==="

# Create a simple HTML file
cat > /tmp/test.html << 'EOF'
<!DOCTYPE html>
<html>
<head><title>Test Page</title></head>
<body>
<h1>Hello World</h1>
</body>
</html>
EOF

# Test with local file
echo "Testing with local file..."
./hweb-poc --session minimal --url "file:///tmp/test.html"
echo "waiting on h1"
./hweb-poc --session minimal --wait "h1"
echo "reading text from h1"
./hweb-poc --session minimal --text "h1"
echo "Exit code: $?"

# Cleanup
echo "Cleanup session"
./hweb-poc --session minimal --end
rm /tmp/test.html

echo "=== Test complete ==="
