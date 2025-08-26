#!/bin/bash

# Initial setup script for Garlic nRF52-DK project
# This script prepares the development environment for AI agents

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}==================================${NC}"
echo -e "${BLUE}  Garlic Project Setup Script${NC}"
echo -e "${BLUE}==================================${NC}"
echo ""

# Check if we're in the project root
if [ ! -f "app/CMakeLists.txt" ]; then
    echo -e "${RED}Error: Not in project root directory!${NC}"
    echo "Please run from /projects/garlic/"
    exit 1
fi

# Source the environment
echo -e "${YELLOW}Setting up Zephyr environment...${NC}"
source scripts/source.sh

# Check for required tools
echo ""
echo -e "${BLUE}Checking required tools...${NC}"

check_tool() {
    if command -v $1 &> /dev/null; then
        echo -e "  ${GREEN}✓${NC} $1 found: $(which $1)"
        return 0
    else
        echo -e "  ${RED}✗${NC} $1 not found"
        return 1
    fi
}

# Essential tools
MISSING_TOOLS=0
check_tool west || MISSING_TOOLS=1
check_tool cmake || MISSING_TOOLS=1
check_tool ninja || MISSING_TOOLS=1
check_tool ${CROSS_COMPILE}gcc || MISSING_TOOLS=1

# Flash tools (at least one required)
FLASH_TOOL_FOUND=0
echo ""
echo -e "${BLUE}Checking flash tools...${NC}"
check_tool openocd && FLASH_TOOL_FOUND=1
check_tool pyocd && FLASH_TOOL_FOUND=1
check_tool JLinkExe && FLASH_TOOL_FOUND=1

if [ $FLASH_TOOL_FOUND -eq 0 ]; then
    echo -e "${YELLOW}⚠ No flash tools found. Install at least one:${NC}"
    echo "  - OpenOCD: sudo apt-get install openocd"
    echo "  - pyOCD: pip install pyocd"
    echo "  - JLink: Download from SEGGER website"
fi

# Check for serial tools
echo ""
echo -e "${BLUE}Checking serial monitor tools...${NC}"
SERIAL_TOOL_FOUND=0
check_tool picocom && SERIAL_TOOL_FOUND=1
check_tool minicom && SERIAL_TOOL_FOUND=1
check_tool screen && SERIAL_TOOL_FOUND=1

if [ $SERIAL_TOOL_FOUND -eq 0 ]; then
    echo -e "${YELLOW}⚠ No serial tools found. Install one:${NC}"
    echo "  - picocom: sudo apt-get install picocom"
    echo "  - screen: sudo apt-get install screen"
fi

# Initialize/update West workspace if needed
echo ""
echo -e "${BLUE}Checking West workspace...${NC}"

if [ ! -d ".west" ]; then
    echo -e "${YELLOW}Initializing West workspace...${NC}"
    west init -l app/
fi

# Update modules (only if not already present)
if [ ! -d "modules/hal" ]; then
    echo -e "${YELLOW}Downloading required modules...${NC}"
    west update --narrow -o=--depth=1
else
    echo -e "${GREEN}✓ Modules already present${NC}"
fi

# Make all scripts executable
echo ""
echo -e "${BLUE}Setting script permissions...${NC}"
chmod +x scripts/*.sh
echo -e "${GREEN}✓ All scripts are executable${NC}"

# Create a build to verify everything works
echo ""
echo -e "${BLUE}Performing test build...${NC}"
if [ ! -f "app/build/zephyr/zephyr.hex" ]; then
    cd app
    if west build -b nrf52dk/nrf52832; then
        echo -e "${GREEN}✓ Test build successful!${NC}"
    else
        echo -e "${RED}✗ Test build failed${NC}"
        echo "Please check the error messages above"
        exit 1
    fi
    cd ..
else
    echo -e "${GREEN}✓ Build already exists${NC}"
fi

# Display summary
echo ""
echo -e "${GREEN}==================================${NC}"
echo -e "${GREEN}  Setup Complete!${NC}"
echo -e "${GREEN}==================================${NC}"
echo ""
echo "Next steps:"
echo "  1. Connect your nRF52-DK board via USB"
echo "  2. Build:  ./scripts/build.sh"
echo "  3. Flash:  ./scripts/flash.sh"
echo "  4. Monitor: ./scripts/monitor.sh"
echo ""
echo "For AI agents: See docs/AI_AGENT_GUIDE.md for detailed information"
echo ""

# Check if user needs to be added to dialout group
if ! groups | grep -q dialout; then
    echo -e "${YELLOW}Note: You may need to add your user to the dialout group:${NC}"
    echo "  sudo usermod -a -G dialout $USER"
    echo "  Then log out and back in"
fi

exit 0