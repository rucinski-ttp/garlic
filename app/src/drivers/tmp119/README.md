# TMP119 Driver

- Summary: TI TMP119 digital temperature sensor I2C driver for Zephyr.
- Datasheet: docs/datasheets/TMP119.pdf (SNIS236 – January 2024)

- Registers (Table 8-2, p.32): `TEMP(0x00)`, `CONFIG(0x01)`, `T_HIGH(0x02)`, `T_LOW(0x03)`, `EE_UNLOCK(0x04)`, `EE1(0x05)`, `EE2(0x06)`, `TEMP_OFFSET(0x07)`, `EE3(0x08)`, `DEVICE_ID(0x0F)`.
- Temperature resolution: LSB = 1/128 °C; signed two's complement (Section 8.5.2, p.32).
- Device ID expected value: 0x2117 on reset (Section 8.5.11, p.33).
- EEPROM programming: write `0x0001` to `EE_UNLOCK` (0x04) before programming `EE1/EE2/EE3` (Sections 8.5.6–8.5.10).

The public API in `inc/tmp119.h` includes references to specific sections/pages.

## UART Command Mapping

Command ID `0x0119` (see `app/src/commands/inc/ids.h`). Request payload:

- `op:uint8`, `addr7:uint8`, then op-specific args.
- Ops:
  - `0x00 READ_ID` → resp `u16 id`
  - `0x01 READ_TEMP_mC` → resp `s32 milli-Celsius`
  - `0x02 READ_TEMP_RAW` → resp `u16`
  - `0x03 READ_CONFIG` → resp `u16`
  - `0x04 WRITE_CONFIG(u16)` → resp empty
  - `0x05 READ_HIGH_LIMIT` → resp `u16`
  - `0x06 WRITE_HIGH_LIMIT(u16)` → resp empty
  - `0x07 READ_LOW_LIMIT` → resp `u16`
  - `0x08 WRITE_LOW_LIMIT(u16)` → resp empty
  - `0x09 UNLOCK_EEPROM` → resp empty
  - `0x0A READ_EEPROM(idx)` → resp `u16`
  - `0x0B WRITE_EEPROM(idx,u16)` → resp empty
  - `0x0C READ_OFFSET` → resp `u16`
  - `0x0D WRITE_OFFSET(u16)` → resp empty

All numeric fields in UART payloads are little-endian for consistency with other commands.

