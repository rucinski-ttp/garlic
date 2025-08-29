#!/bin/bash

# Check code formatting with clang-format
# Usage: ./scripts/check_format.sh
# Returns: 0 if all files are formatted, 1 if formatting needed

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

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}Error: clang-format is not installed!${NC}"
    echo "Install it with:"
    echo "  Ubuntu/Debian: sudo apt-get install clang-format"
    echo "  macOS: brew install clang-format"
    exit 1
fi

# Show clang-format version
echo -e "${BLUE}Checking code format with: $(clang-format --version | head -n1)${NC}"
echo ""

# Find all C/C++ source files
FILES=$(find app -type f \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) \
    ! -path "*/build/*" \
    ! -path "*/.git/*" \
    ! -path "*/zephyr/*" \
    ! -path "*/modules/*" | sort)

if [ -z "$FILES" ]; then
    echo -e "${YELLOW}No source files found to check${NC}"
    exit 0
fi

echo "Checking files:"
FAILED=0
FAILED_FILES=""

for file in $FILES; do
    printf "  %-50s " "$file"
    if clang-format --dry-run --Werror "$file" 2>/dev/null; then
        echo -e "${GREEN}✓ OK${NC}"
    else
        echo -e "${RED}✗ NEEDS FORMATTING${NC}"
        FAILED=1
        FAILED_FILES="$FAILED_FILES\n    $file"
    fi
done

echo ""
echo "----------------------------------------"
if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All files are properly formatted!${NC}"
    echo "----------------------------------------"
    exit 0
else
    echo -e "${RED}✗ Format check failed!${NC}"
    echo -e "${RED}Files needing formatting:${NC}$FAILED_FILES"
    echo ""
    echo "To fix formatting, run:"
    echo -e "  ${YELLOW}./scripts/fix_format.sh${NC}"
    echo "----------------------------------------"
    exit 1
fi