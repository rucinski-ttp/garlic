#!/bin/bash

# All-in-one script for quick build, flash, and monitor
# Usage: ./scripts/all.sh

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(dirname "$0")"

echo -e "${BLUE}==================================${NC}"
echo -e "${BLUE}  All-in-One Build & Flash${NC}"
echo -e "${BLUE}==================================${NC}"
echo ""

# Build
echo -e "${YELLOW}Step 1/3: Building...${NC}"
if ${SCRIPT_DIR}/build.sh; then
    echo -e "${GREEN}✓ Build complete${NC}"
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

echo ""

# Flash
echo -e "${YELLOW}Step 2/3: Flashing...${NC}"
if ${SCRIPT_DIR}/flash.sh; then
    echo -e "${GREEN}✓ Flash complete${NC}"
else
    echo -e "${RED}✗ Flash failed${NC}"
    exit 1
fi

echo ""

# Monitor (optional)
echo -e "${YELLOW}Step 3/3: Starting monitor...${NC}"
echo -e "${BLUE}Press Ctrl+C to skip monitoring${NC}"
sleep 2
${SCRIPT_DIR}/monitor.sh || true

echo ""
echo -e "${GREEN}Done!${NC}"