# AI Agent Development Guide

## 🤖 Essential Context for AI Agents

This document provides critical information for AI agents working on the Garlic embedded project.

## System Architecture

### Hardware
- **Board**: Nordic nRF52-DK
- **MCU**: nRF52832 (ARM Cortex-M4, 64KB RAM, 512KB Flash)
- **Debugger**: On-board SEGGER J-Link
- **LEDs**: 4 user LEDs (LED1-4)
- **Buttons**: 4 user buttons
- **Serial**: Virtual COM port over USB

### Software Stack
- **RTOS**: Zephyr 3.7.0
- **Build System**: West + CMake + Ninja
- **Toolchain**: Zephyr SDK (GCC ARM Embedded)
- **Flash Tools**: SEGGER J-Link

## Critical File Locations

```
/projects/garlic/
├── app/prj.conf                      # Zephyr configuration
├── app/CMakeLists.txt                # App-level CMake
└── app/src/
    ├── app/src/app_runtime.c         # Runtime thread (boot prints, LED, transport hookup)
    ├── commands/                     # Command handlers (each with its own CMakeLists.txt)
    ├── proto/                        # Transport + CRC32
    ├── stack/                        # Transport↔command glue
    └── drivers/uart_dma/             # UART DMA driver + headers
```

## Environment Variables

Set by `scripts/source.sh`:
- `ZEPHYR_BASE`: Points to zephyr directory
- `ZEPHYR_SDK_INSTALL_DIR`: SDK location
- `PATH`: Includes toolchain and West

## Common Commands Reference

### Essential Operations
```bash
# Always source environment first (scripts do this automatically)
source scripts/source.sh

# Code quality
# Formatting (clang-format)
./scripts/format.sh check   # Check formatting (matches CI)
./scripts/format.sh fix     # Auto-fix formatting
# Static analysis (clang-tidy)
./scripts/format.sh tidy        # Tidy checks (no fixes)
./scripts/format.sh tidy-fix    # Apply tidy fixes, then reformat

# Build cycle
./scripts/build.sh          # Build application
./scripts/flash.sh          # Flash to board
./scripts/monitor.sh        # View serial output

# Clean operations
./scripts/build.sh clean    # Clean rebuild
rm -rf app/build/          # Full clean
```

### Working with Git
```bash
# Check status
git status

# Stage changes
git add -A

# Commit (with co-author for AI)
git commit -m "Your message

Co-Authored-By: Claude <noreply@anthropic.com>"

# Push to remote
git push origin main
```

## Code Patterns

### Adding GPIO Output
```c
#include <zephyr/drivers/gpio.h>

#define MY_GPIO_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec my_gpio = GPIO_DT_SPEC_GET(MY_GPIO_NODE, gpios);

// In main:
gpio_is_ready_dt(&my_gpio);
gpio_pin_configure_dt(&my_gpio, GPIO_OUTPUT_ACTIVE);
gpio_pin_set_dt(&my_gpio, 1);  // Set high
```

### Adding Serial Output
```c
#include <zephyr/sys/printk.h>

printk("Hello from nRF52-DK!\n");
```

### Using Timers
```c
#include <zephyr/kernel.h>

k_msleep(1000);  // Sleep 1 second
k_usleep(100);   // Sleep 100 microseconds
```

## Configuration (prj.conf)

### Common Options
```conf
# Console / logging (project defaults)
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=n           # UART kept for data; debug logs via RTT
CONFIG_LOG_BACKEND_RTT=y
CONFIG_PRINTK=y                 # Enable printk for early boot lines

# GPIO
CONFIG_GPIO=y

# Debugging
CONFIG_DEBUG=y
CONFIG_ASSERT=y

# Shell (for interactive commands)
CONFIG_SHELL=y
CONFIG_SHELL_BACKEND_SERIAL=y
```

## Build System

### Adding Source Files
Prefer editing the module-local `CMakeLists.txt` (e.g., `app/src/commands/echo/CMakeLists.txt`) to register new sources. The app build gathers module targets.

If you need to add a new module, mirror the existing directory structure and minimal CMakeLists from a peer module.

## Debugging Issues

### Common Problems and Solutions

1. **West not found**
   - Run: `source scripts/source.sh`

2. **Board not detected**
   - Check: `lsusb | grep SEGGER`
   - Solution: Reconnect USB, check cable

3. **Flash fails**
   - Ensure J-Link tools are installed (JLinkExe, JLinkGDBServer)
   - Kill hanging sessions: close Ozone/VSCode; `pkill JLink`
   - Reconnect the board USB and retry

4. **Build errors**
   - Clean: `./scripts/build.sh clean`
   - Check: `app/prj.conf` for missing configs

5. **Serial port not found**
   - Check: `ls /dev/ttyACM*`
   - May need: `sudo usermod -a -G dialout $USER`

## Testing Approach

### Manual Testing
1. Build and flash application
2. Monitor serial output
3. Verify LED behavior
4. Test button inputs (if implemented)

### Future Automated Testing
- Unit tests: Google Test framework
- Integration tests: pytest with hardware-in-loop
- Static analysis: clang-tidy, cppcheck

## Important Conventions

1. **Always use scripts** - Don't run west/cmake directly
2. **Check formatting** - Run `./scripts/check_format.sh` before commit
3. **Check hardware** - Ensure board is connected before flashing
4. **Clean builds** - When in doubt, do a clean build
5. **Document changes** - Update README for new features
6. **Test before commit** - Always test on hardware

## Flash Memory Map

```
0x00000000 - 0x0007FFFF : Flash (512KB)
  0x00000000 - 0x00001000 : MBR/Bootloader
  0x00001000 - 0x0007FFFF : Application
  
0x20000000 - 0x2000FFFF : RAM (64KB)
```

## Peripheral Access

### Available Peripherals
- UART0, UART1
- SPI0, SPI1, SPI2
- I2C0, I2C1
- PWM0, PWM1, PWM2
- ADC (8 channels)
- Timer0-4
- RTC0, RTC1

### Device Tree References
```c
// Get device from devicetree
const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

// Check if device is ready
if (!device_is_ready(uart)) {
    printk("UART device not ready\n");
    return -1;
}
```

## Performance Considerations

- **Stack Size**: Default 1024 bytes (increase in prj.conf if needed)
- **Heap Size**: Configure with `CONFIG_HEAP_MEM_POOL_SIZE`
- **Flash Wear**: Minimize frequent writes to same sectors
- **Power**: Use `k_sleep()` instead of busy-waiting

## Version Control Best Practices

1. **Commit Messages**: Be specific about hardware changes
2. **Branch Names**: Use feature/component-description
3. **Testing**: Note tested configurations in commit
4. **Documentation**: Update docs with code changes

## Resources for AI Agents

- Device tree bindings: `zephyr/dts/bindings/`
- Sample applications: `zephyr/samples/`
- Board definition: `zephyr/boards/nordic/nrf52dk/`
- HAL documentation: `modules/hal/nordic/nrfx/`

---

**Remember**: This is an AI-agent-first project. All tools and documentation are designed for autonomous development. When stuck, examine script output carefully and check this guide.
