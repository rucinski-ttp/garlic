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

# Prefer running the CMake format-check target for consistency with CI
if command -v cmake >/dev/null 2>&1; then
    echo -e "${BLUE}Running CMake format-check target...${NC}"
    # Prepare build system
    (cd app && cmake -S . -B build -G Ninja >/dev/null)
    if cmake --build app/build --target format-check; then
        echo -e "${GREEN}✓ All files are properly formatted!${NC}"
        exit 0
    else
        echo -e "${RED}✗ Format check failed!${NC}"
        echo "To fix formatting, you can run:"
        echo "  cmake --build app/build --target format"
        exit 1
    fi
fi

echo -e "${YELLOW}cmake not available; falling back to script-based check${NC}"
"$(dirname "$0")/format.sh" check
