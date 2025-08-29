#!/bin/bash

# Fix code formatting with clang-format
# Usage: ./scripts/fix_format.sh [--check-only]
# Options:
#   --check-only: Only show what would be changed without modifying files

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Project root
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"

# Parse arguments
CHECK_ONLY=false
if [ "$1" == "--check-only" ]; then
    CHECK_ONLY=true
fi

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}Error: clang-format is not installed!${NC}"
    echo "Install it with:"
    echo "  Ubuntu/Debian: sudo apt-get install clang-format"
    echo "  macOS: brew install clang-format"
    exit 1
fi

# Show clang-format version
echo -e "${BLUE}Formatting code with: $(clang-format --version | head -n1)${NC}"
echo ""

# Find all C/C++ source files
FILES=$(find app -type f \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) \
    ! -path "*/build/*" \
    ! -path "*/.git/*" \
    ! -path "*/zephyr/*" \
    ! -path "*/modules/*" | sort)

if [ -z "$FILES" ]; then
    echo -e "${YELLOW}No source files found to format${NC}"
    exit 0
fi

if [ "$CHECK_ONLY" == "true" ]; then
    echo -e "${CYAN}Preview mode - no files will be modified${NC}"
    echo ""
    for file in $FILES; do
        echo -e "${BLUE}=== $file ===${NC}"
        DIFF=$(clang-format "$file" | diff -u "$file" - 2>/dev/null || true)
        if [ -z "$DIFF" ]; then
            echo "  No changes needed"
        else
            echo "$DIFF"
        fi
        echo ""
    done
else
    echo "Formatting files:"
    MODIFIED=0
    MODIFIED_FILES=""
    
    for file in $FILES; do
        printf "  %-50s " "$file"
        
        # Check if file needs formatting first
        if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
            clang-format -i "$file"
            echo -e "${GREEN}✓ FORMATTED${NC}"
            MODIFIED=1
            MODIFIED_FILES="$MODIFIED_FILES\n    $file"
        else
            echo -e "${CYAN}○ Already formatted${NC}"
        fi
    done
    
    echo ""
    echo "----------------------------------------"
    if [ $MODIFIED -eq 1 ]; then
        echo -e "${GREEN}✓ Formatting complete!${NC}"
        echo -e "${YELLOW}Modified files:${NC}$MODIFIED_FILES"
        echo ""
        
        # Show git status if in a git repo
        if [ -d .git ]; then
            echo "Git changes:"
            git diff --stat -- '*.c' '*.h' '*.cpp' '*.hpp' 2>/dev/null || true
        fi
    else
        echo -e "${GREEN}✓ All files were already properly formatted!${NC}"
    fi
    echo "----------------------------------------"
    
    # Run check to verify
    echo ""
    echo "Running format check to verify..."
    if ./scripts/check_format.sh > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Format verification passed${NC}"
    else
        echo -e "${RED}✗ Format verification failed - this shouldn't happen${NC}"
        exit 1
    fi
fi

exit 0