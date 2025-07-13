#!/bin/bash

# Script to verify the Browser refactoring was successful
# Usage: ./verify_refactor.sh

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Browser Refactoring Verification${NC}"
echo "================================="

# Check if Browser directory exists
if [ ! -d "src/Browser" ]; then
    echo -e "${RED}Error: src/Browser directory not found${NC}"
    exit 1
fi

# Expected files
EXPECTED_FILES=(
    "Browser.h"
    "Browser.cpp"
    "BrowserCore.cpp"
    "BrowserJavaScript.cpp"
    "BrowserEvents.cpp"
    "BrowserDOM.cpp"
    "BrowserStorage.cpp"
    "BrowserSession.cpp"
    "BrowserScreenshot.cpp"
    "BrowserUtilities.cpp"
)

echo -e "\n${YELLOW}Checking for expected files...${NC}"
ALL_PRESENT=true
for file in "${EXPECTED_FILES[@]}"; do
    if [ -f "src/Browser/$file" ]; then
        echo -e "  ${GREEN}✓${NC} $file"
    else
        echo -e "  ${RED}✗${NC} $file - MISSING"
        ALL_PRESENT=false
    fi
done

if [ "$ALL_PRESENT" = false ]; then
    echo -e "${RED}Some files are missing!${NC}"
    exit 1
fi

echo -e "\n${YELLOW}Checking include statements...${NC}"

# Check Browser files include structure
echo -e "\n  ${BLUE}Browser internal includes:${NC}"
for file in src/Browser/*.cpp; do
    echo -n "    $(basename "$file"): "
    if grep -q '#include "Browser.h"' "$file"; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗ Missing Browser.h include${NC}"
    fi
done

# Check for parent directory includes
echo -e "\n  ${BLUE}Parent directory includes:${NC}"
PARENT_INCLUDES=("Session.h" "Debug.h")
for inc in "${PARENT_INCLUDES[@]}"; do
    echo -n "    Checking for ../$inc: "
    if grep -q "#include \"\.\.\/$inc\"" src/Browser/*.cpp 2>/dev/null; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}Not found (may not be needed)${NC}"
    fi
done

# Check external file includes
echo -e "\n  ${BLUE}External file includes of Browser:${NC}"
EXTERNAL_COUNT=$(find src -name "*.cpp" -o -name "*.h" | grep -v "src/Browser/" | xargs grep -l "Browser/Browser.h" 2>/dev/null | wc -l)
echo -e "    Found ${GREEN}$EXTERNAL_COUNT${NC} files including Browser/Browser.h"

# Check for old-style includes
echo -e "\n${YELLOW}Checking for old-style includes...${NC}"
OLD_STYLE=$(find src -name "*.cpp" -o -name "*.h" | xargs grep -n "\"Browser\.h\"" 2>/dev/null | grep -v "src/Browser/" | grep -v "Browser/Browser.h" || true)
if [ -n "$OLD_STYLE" ]; then
    echo -e "${RED}Found old-style includes:${NC}"
    echo "$OLD_STYLE"
else
    echo -e "${GREEN}No old-style includes found${NC}"
fi

# Try to compile a test file if possible
echo -e "\n${YELLOW}Compilation test...${NC}"
if command -v g++ &> /dev/null && command -v pkg-config &> /dev/null; then
    # Create a simple test file
    cat > /tmp/browser_test.cpp << 'EOF'
#include "Browser/Browser.h"
int main() {
    // Just test that Browser.h can be included
    return 0;
}
EOF

    echo -n "  Testing compilation of Browser.h include: "
    if g++ -c /tmp/browser_test.cpp -I src $(pkg-config --cflags gtk4 webkitgtk-6.0 jsoncpp 2>/dev/null || echo "") -o /tmp/browser_test.o 2>/dev/null; then
        echo -e "${GREEN}✓ Success${NC}"
        rm -f /tmp/browser_test.o
    else
        echo -e "${RED}✗ Failed${NC}"
        echo -e "${YELLOW}  Note: This might be due to missing dependencies${NC}"
    fi
    rm -f /tmp/browser_test.cpp
else
    echo -e "${YELLOW}  Skipping (g++ or pkg-config not found)${NC}"
fi

# Check CMakeLists.txt updates
if [ -f "CMakeLists.txt" ]; then
    echo -e "\n${YELLOW}Checking CMakeLists.txt...${NC}"
    echo -n "  Browser source paths updated: "
    if grep -q "src/Browser/" CMakeLists.txt || grep -q "Browser/" CMakeLists.txt; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
    fi
fi

# Summary
echo -e "\n${BLUE}Summary${NC}"
echo "======="
if [ "$ALL_PRESENT" = true ]; then
    echo -e "${GREEN}All Browser files are present in src/Browser/${NC}"
    
    # Count successful checks
    echo -e "\nRecommendations:"
    echo "1. Run your build system to ensure everything compiles"
    echo "2. Run your tests to ensure functionality is preserved"
    echo "3. Remove the backup directory once verified"
    
    if [ -n "$OLD_STYLE" ]; then
        echo -e "\n${YELLOW}Warning: Some old-style includes were found and should be updated${NC}"
    fi
else
    echo -e "${RED}Refactoring appears incomplete${NC}"
fi

# Optional: Show directory structure
echo -e "\n${YELLOW}Browser directory structure:${NC}"
tree src/Browser/ 2>/dev/null || ls -la src/Browser/
