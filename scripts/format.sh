#!/bin/bash

# Code formatting script using clang-format
# Usage: ./scripts/format.sh [check|fix|file <filename>]
# Default: check mode

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Project root
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"

# Default mode
MODE=${1:-check}

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}Error: clang-format is not installed!${NC}"
    echo "Install it with:"
    echo "  Ubuntu/Debian: sudo apt-get install clang-format"
    echo "  macOS: brew install clang-format"
    echo "  Or install specific version: sudo apt-get install clang-format-14"
    exit 1
fi

# Show clang-format version
CLANG_VERSION=$(clang-format --version | head -n1)
echo -e "${BLUE}Using: $CLANG_VERSION${NC}"
echo ""

# Find all C/C++ source files
find_source_files() {
    find app -type f \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) \
        ! -path "*/build/*" \
        ! -path "*/.git/*" \
        ! -path "*/zephyr/*" \
        ! -path "*/modules/*" | sort
}

case "$MODE" in
    check)
        echo -e "${YELLOW}Checking code formatting...${NC}"
        echo "Files to check:"
        
        FAILED=0
        FILES=$(find_source_files)
        
        if [ -z "$FILES" ]; then
            echo -e "${YELLOW}No source files found to check${NC}"
            exit 0
        fi
        
        for file in $FILES; do
            echo -n "  Checking $file... "
            if clang-format --dry-run --Werror "$file" 2>/dev/null; then
                echo -e "${GREEN}✓${NC}"
            else
                echo -e "${RED}✗ needs formatting${NC}"
                FAILED=1
            fi
        done
        
        echo ""
        if [ $FAILED -eq 0 ]; then
            echo -e "${GREEN}✓ All files are properly formatted!${NC}"
        else
            echo -e "${RED}✗ Some files need formatting${NC}"
            echo ""
            echo "To fix formatting issues, run:"
            echo "  ./scripts/format.sh fix"
            exit 1
        fi
        ;;
        
    fix)
        echo -e "${YELLOW}Formatting code...${NC}"
        FILES=$(find_source_files)
        
        if [ -z "$FILES" ]; then
            echo -e "${YELLOW}No source files found to format${NC}"
            exit 0
        fi
        
        echo "Files to format:"
        for file in $FILES; do
            echo -n "  Formatting $file... "
            clang-format -i "$file"
            echo -e "${GREEN}✓${NC}"
        done
        
        echo ""
        echo -e "${GREEN}✓ All files have been formatted!${NC}"
        echo ""
        echo "Changes made:"
        git diff --stat -- '*.c' '*.h' '*.cpp' '*.hpp' 2>/dev/null || echo "  (no git repository or no changes)"
        ;;
        
    file)
        if [ -z "$2" ]; then
            echo -e "${RED}Error: Please specify a file to format${NC}"
            echo "Usage: $0 file <filename>"
            exit 1
        fi
        
        FILE="$2"
        if [ ! -f "$FILE" ]; then
            echo -e "${RED}Error: File '$FILE' not found${NC}"
            exit 1
        fi
        
        echo -e "${YELLOW}Formatting single file: $FILE${NC}"
        clang-format -i "$FILE"
        echo -e "${GREEN}✓ File formatted!${NC}"
        
        echo ""
        echo "Changes:"
        git diff "$FILE" 2>/dev/null || echo "  (no git repository or file not tracked)"
        ;;
        
    diff)
        echo -e "${YELLOW}Showing formatting differences...${NC}"
        FILES=$(find_source_files)
        
        if [ -z "$FILES" ]; then
            echo -e "${YELLOW}No source files found${NC}"
            exit 0
        fi
        
        for file in $FILES; do
            echo -e "${BLUE}=== $file ===${NC}"
            clang-format "$file" | diff -u "$file" - || true
            echo ""
        done
        ;;
        
    *)
        echo -e "${RED}Unknown mode: $MODE${NC}"
        echo ""
        echo "Usage: $0 [check|fix|file <filename>|diff]"
        echo ""
        echo "Modes:"
        echo "  check - Check if files are properly formatted (default)"
        echo "  fix   - Fix formatting for all source files"
        echo "  file  - Format a single file"
        echo "  diff  - Show what changes would be made"
        echo ""
        echo "Examples:"
        echo "  $0                    # Check formatting"
        echo "  $0 check             # Check formatting"
        echo "  $0 fix               # Fix all files"
        echo "  $0 file app/src/main.c   # Format single file"
        echo "  $0 diff              # Preview changes"
        exit 1
        ;;
esac

exit 0