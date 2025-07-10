#!/bin/bash

echo "=== Debug limitless-adventures.com ==="

SESSION="debug_limitless"

# Clean start
./hweb-poc --session "$SESSION" --end 2>/dev/null

echo "1. Navigate to site..."
./hweb-poc --session "$SESSION" --url "https://limitless-adventures.com"

echo "2. Check what's in h1 using JavaScript..."
./hweb-poc --session "$SESSION" --js "document.querySelector('h1') ? 'H1 exists' : 'No H1'"

echo "3. Get h1 text length..."
./hweb-poc --session "$SESSION" --js "document.querySelector('h1') ? document.querySelector('h1').textContent.length : 0"

echo "4. Get first 50 chars of h1..."
./hweb-poc --session "$SESSION" --js "(function() { var h1 = document.querySelector('h1'); return h1 ? h1.textContent.substring(0, 50) : 'No H1'; })()"

echo "5. Check for special characters..."
./hweb-poc --session "$SESSION" --js "(function() { var h1 = document.querySelector('h1'); if (!h1) return 'No H1'; var text = h1.textContent; for (var i = 0; i < text.length && i < 50; i++) { if (text.charCodeAt(i) < 32 || text.charCodeAt(i) > 126) return 'Special char at ' + i + ': ' + text.charCodeAt(i); } return 'No special chars in first 50'; })()"

echo "6. Now try --text command..."
./hweb-poc --session "$SESSION" --text "h1"
echo "Exit code: $?"

echo "7. Try with a different selector..."
./hweb-poc --session "$SESSION" --text "title"
echo "Exit code: $?"

# Cleanup
./hweb-poc --session "$SESSION" --end

echo "=== Debug complete ==="
