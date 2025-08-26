#!/bin/bash

# Serial monitor script for nRF52-DK board
# Usage: ./scripts/monitor.sh [baudrate] [port]
# Default: 115200 baud, auto-detect port

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default settings
BAUDRATE=${1:-115200}
PORT=${2:-""}

echo -e "${BLUE}nRF52-DK Serial Monitor${NC}"
echo "========================"

# Auto-detect serial port if not specified
if [ -z "$PORT" ]; then
    echo -e "${YELLOW}Auto-detecting serial port...${NC}"
    
    # Check common serial port patterns
    for pattern in "/dev/ttyACM*" "/dev/ttyUSB*" "/dev/tty.usbmodem*"; do
        for port in $pattern; do
            if [ -e "$port" ]; then
                PORT="$port"
                break 2
            fi
        done
    done
    
    if [ -z "$PORT" ]; then
        echo -e "${RED}Error: No serial port detected!${NC}"
        echo "Please ensure the nRF52-DK is connected and powered on."
        echo ""
        echo "Available serial devices:"
        ls -la /dev/tty* 2>/dev/null | grep -E "(ACM|USB|usbmodem)" || echo "  None found"
        echo ""
        echo "You can specify a port manually:"
        echo "  $0 115200 /dev/ttyACM0"
        exit 1
    fi
fi

# Check if port exists
if [ ! -e "$PORT" ]; then
    echo -e "${RED}Error: Serial port $PORT does not exist!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Using port: $PORT${NC}"
echo -e "${GREEN}✓ Baudrate: $BAUDRATE${NC}"
echo ""

# Check for available serial tools
MONITOR_TOOL=""

if command -v picocom &> /dev/null; then
    MONITOR_TOOL="picocom"
elif command -v minicom &> /dev/null; then
    MONITOR_TOOL="minicom"
elif command -v screen &> /dev/null; then
    MONITOR_TOOL="screen"
elif command -v cu &> /dev/null; then
    MONITOR_TOOL="cu"
else
    echo -e "${RED}Error: No serial monitor tool found!${NC}"
    echo "Please install one of: picocom, minicom, screen, or cu"
    echo "  Ubuntu/Debian: sudo apt-get install picocom"
    echo "  macOS: brew install picocom"
    exit 1
fi

echo -e "${BLUE}Starting serial monitor with $MONITOR_TOOL...${NC}"
echo -e "${YELLOW}Press Ctrl+A then Ctrl+X to exit (picocom)${NC}"
echo -e "${YELLOW}Press Ctrl+A then K to exit (screen)${NC}"
echo "----------------------------------------"
echo ""

# Start the appropriate monitor
case $MONITOR_TOOL in
    picocom)
        picocom -b $BAUDRATE $PORT
        ;;
    minicom)
        minicom -b $BAUDRATE -D $PORT
        ;;
    screen)
        screen $PORT $BAUDRATE
        ;;
    cu)
        cu -l $PORT -s $BAUDRATE
        ;;
esac

echo ""
echo -e "${GREEN}Serial monitor closed.${NC}"