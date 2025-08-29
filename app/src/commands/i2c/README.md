# I2C Command (Generic Transfers)

- Command ID: `0x0100`
- Purpose: Perform generic I2C transfers for quick bring-up and debugging.
- Bus: `&i2c0` configured via `app/boards/nrf52dk_nrf52832.overlay` (SCL=P0.27, SDA=P0.26 @ 400kHz).

## Request/Response

Request payload:

- `op:uint8` — 0=write, 1=read, 2=write_read
- `addr7:uint8` — 7-bit I2C address
- `wlen:uint16_le`, `rlen:uint16_le`
- `wdata[wlen]` — present for `write` and `write_read`

Responses:

- write: empty
- read: `rdata[rlen]`
- write_read: `rdata[rlen]`

The implementation uses Zephyr's `i2c_transfer_cb` when available (`CONFIG_I2C_CALLBACK=y`) to avoid blocking the application.

## Extras

- `op=0x10` (scan): returns a variable-length list of responding addresses.
  - Request: `op=0x10, addr7 ignored`
  - Response: `[count:uint8, addr0:uint8, addr1:uint8, ...]`
  - Internally performs a 1-byte read probe per address after optional bus recovery.
