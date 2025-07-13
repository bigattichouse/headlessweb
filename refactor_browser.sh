#!/bin/bash

# Script to refactor Browser files into a Browser subdirectory
# Usage: ./refactor_browser.sh

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Browser Refactoring Script${NC}"
echo "============================="

# Check if we're in the right directory
if [ ! -d "src" ]; then
    echo -e "${RED}Error: 'src' directory not found. Please run this script from the project root.${NC}"
    exit 1
fi

# List of Browser files to move
BROWSER_FILES=(
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

# Create backup directory
BACKUP_DIR="backup_$(date +%Y%m%d_%H%M%S)"
echo -e "${YELLOW}Creating backup in ${BACKUP_DIR}...${NC}"
mkdir -p "$BACKUP_DIR"
cp -r src "$BACKUP_DIR/"

# Create Browser subdirectory
echo -e "${YELLOW}Creating src/Browser directory...${NC}"
mkdir -p src/Browser

# Move Browser files
echo -e "${YELLOW}Moving Browser files...${NC}"
for file in "${BROWSER_FILES[@]}"; do
    if [ -f "src/$file" ]; then
        echo "  Moving $file"
        mv "src/$file" "src/Browser/"
    else
        echo -e "${RED}  Warning: $file not found in src/${NC}"
    fi
done

# Update includes in Browser files themselves
echo -e "${YELLOW}Updating includes in Browser files...${NC}"
for file in "${BROWSER_FILES[@]}"; do
    if [ -f "src/Browser/$file" ]; then
        echo "  Updating $file"
        
        # Update includes of Browser.h to use quotes instead of brackets if they were using brackets
        sed -i 's/#include <Browser\.h>/#include "Browser.h"/g' "src/Browser/$file"
        
        # Browser files including each other should use relative paths
        sed -i 's/#include "Browser\.h"/#include "Browser.h"/g' "src/Browser/$file"
        
        # Update includes of other project headers that are in parent directory
        sed -i 's/#include "Session\.h"/#include "..\/Session.h"/g' "src/Browser/$file"
        sed -i 's/#include "Debug\.h"/#include "..\/Debug.h"/g' "src/Browser/$file"
    fi
done

# Update includes in all other source files
echo -e "${YELLOW}Updating includes in other source files...${NC}"

# Find all .cpp and .h files in src/ (excluding Browser/ subdirectory)
find src -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | grep -v "src/Browser/" | while read -r file; do
    # Check if file contains Browser.h include
    if grep -q "Browser\.h" "$file"; then
        echo "  Updating $(basename "$file")"
        
        # Update Browser.h include to use subdirectory
        sed -i 's/#include "Browser\.h"/#include "Browser\/Browser.h"/g' "$file"
        sed -i 's/#include <Browser\.h>/#include "Browser\/Browser.h"/g' "$file"
    fi
done

# Update CMakeLists.txt if it exists
if [ -f "CMakeLists.txt" ]; then
    echo -e "${YELLOW}Updating CMakeLists.txt...${NC}"
    
    # Backup CMakeLists.txt
    cp CMakeLists.txt "$BACKUP_DIR/"
    
    # Update source file paths in CMakeLists.txt
    for file in "${BROWSER_FILES[@]}"; do
        if [[ $file == *.cpp ]]; then
            sed -i "s|src/$file|src/Browser/$file|g" CMakeLists.txt
            sed -i "s|$file|Browser/$file|g" CMakeLists.txt
        fi
    done
    
    # Add Browser subdirectory to include paths if not already there
    if ! grep -q "src/Browser" CMakeLists.txt; then
        # Try to find target_include_directories and add Browser path
        sed -i '/target_include_directories/,/)/s|${CMAKE_CURRENT_SOURCE_DIR}/src|${CMAKE_CURRENT_SOURCE_DIR}/src\n    ${CMAKE_CURRENT_SOURCE_DIR}/src/Browser|' CMakeLists.txt
    fi
fi

# Create a simple module CMakeLists.txt for the Browser subdirectory
echo -e "${YELLOW}Creating src/Browser/CMakeLists.txt...${NC}"
cat > src/Browser/CMakeLists.txt << 'EOF'
# Browser module CMakeLists.txt

set(BROWSER_SOURCES
    Browser.cpp
    BrowserCore.cpp
    BrowserJavaScript.cpp
    BrowserEvents.cpp
    BrowserDOM.cpp
    BrowserStorage.cpp
    BrowserSession.cpp
    BrowserScreenshot.cpp
    BrowserUtilities.cpp
)

# Create list with full paths
set(BROWSER_MODULE_SOURCES "")
foreach(source ${BROWSER_SOURCES})
    list(APPEND BROWSER_MODULE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()

# Export to parent scope
set(BROWSER_MODULE_SOURCES ${BROWSER_MODULE_SOURCES} PARENT_SCOPE)

# Optional: Create a static library
# add_library(browser_module STATIC ${BROWSER_SOURCES})
# target_include_directories(browser_module PUBLIC 
#     ${CMAKE_CURRENT_SOURCE_DIR}/..
#     ${GTK_INCLUDE_DIRS}
#     ${WEBKIT_INCLUDE_DIRS}
# )
EOF

# Update any Makefile if it exists
if [ -f "Makefile" ] && [ ! -L "Makefile" ]; then
    echo -e "${YELLOW}Updating Makefile...${NC}"
    cp Makefile "$BACKUP_DIR/"
    
    for file in "${BROWSER_FILES[@]}"; do
        if [[ $file == *.cpp ]]; then
            sed -i "s|src/$file|src/Browser/$file|g" Makefile
        fi
    done
fi

# Summary
echo -e "\n${GREEN}Refactoring completed!${NC}"
echo "====================="
echo -e "Backup created in: ${YELLOW}$BACKUP_DIR/${NC}"
echo -e "\nMoved files:"
for file in "${BROWSER_FILES[@]}"; do
    if [ -f "src/Browser/$file" ]; then
        echo -e "  ${GREEN}✓${NC} $file"
    fi
done

echo -e "\n${YELLOW}Next steps:${NC}"
echo "1. Review the changes"
echo "2. Update your build system if needed"
echo "3. Recompile and test your project"
echo "4. If everything works, you can remove the backup: rm -rf $BACKUP_DIR"

echo -e "\n${YELLOW}Note:${NC} If you have other build files (like .pro files for Qt),"
echo "      you'll need to update those manually."

# Check for any remaining references
echo -e "\n${YELLOW}Checking for any remaining references...${NC}"
if grep -r "Browser\.h" src/ --include="*.cpp" --include="*.h" --include="*.hpp" | grep -v "src/Browser/" | grep -v "Browser/Browser.h"; then
    echo -e "${RED}Warning: Found additional Browser.h references that may need manual updating${NC}"
else
    echo -e "${GREEN}No additional references found${NC}"
fi
