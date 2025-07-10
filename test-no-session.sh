#!/bin/bash

echo "=== Test without session restoration ==="

# Test 1: Navigate and execute JS in same invocation
echo "Test 1: Navigate and JS in same command..."
./hweb-poc --session fresh1 --url "https://example.com" --js "document.title"

# Test 2: Navigate to limitless-adventures.com and execute JS in same invocation
echo -e "\nTest 2: Navigate to limitless-adventures and JS in same command..."
./hweb-poc --session fresh2 --url "https://limitless-adventures.com" --js "document.title"

# Test 3: Test session restoration with example.com
echo -e "\nTest 3: Session restoration with example.com..."
./hweb-poc --session restore1 --url "https://example.com"
./hweb-poc --session restore1 --js "document.title"

# Test 4: Test session restoration with limitless-adventures.com
echo -e "\nTest 4: Session restoration with limitless-adventures.com..."
./hweb-poc --session restore2 --url "https://limitless-adventures.com"
echo "Navigated, now trying JS..."
./hweb-poc --session restore2 --js "document.title"

# Cleanup
./hweb-poc --session fresh1 --end
./hweb-poc --session fresh2 --end
./hweb-poc --session restore1 --end
./hweb-poc --session restore2 --end

echo "=== Test complete ==="
