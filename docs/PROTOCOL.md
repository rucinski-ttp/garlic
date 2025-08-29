# Garlic Serial Protocol

This document specifies the transport framing and command payloads used over the UART DMA data channel.

The protocol is layered to ensure clear responsibilities and testability:

- UART DMA layer: non-blocking byte I/O to the HAL (already implemented in `uart_dma.c`).
- Transport layer (this document): reliable framing, CRC32 integrity, fragmentation and reassembly (no retransmissions).
- Command layer: request/response messages packed within transport payloads, handler dispatch per command ID.

No dynamic allocation is used. All buffers are bounded and configured at compile time.

## Transport Framing

Each frame carries a fragment of a single logical message. Large messages are split across multiple frames; small messages fit in one frame.

Byte order is little-endian for all multi-byte fields.

Frame layout (one fragment):

- `sync` (2 bytes): 0xA5 0x5A
- `ver` (1 byte): protocol version. Current: 1
- `flags` (1 byte):
  - bit0 START: first fragment
  - bit1 MIDDLE: middle fragment
  - bit2 END: last fragment
  - bit4 RESP: frame belongs to a response message (vs request)
- `session` (uint16): correlation ID across fragments and request/response pair
- `frag_index` (uint16): 0..frag_count-1
- `frag_count` (uint16): total number of fragments (>= 1)
- `payload_len` (uint16): number of payload bytes in this frame
- `payload` (`payload_len` bytes)
- `crc32` (uint32): computed over [ver..payload]

Constraints (defaults; configurable via macros):

- Max fragment payload: 128 bytes (`TRANSPORT_FRAME_MAX_PAYLOAD`)
- Max reassembled message: 2048 bytes (`TRANSPORT_REASSEMBLY_MAX`)
- Max fragments per message: 64 (`TRANSPORT_MAX_FRAGMENTS`)

Integrity: CRC32 is IEEE 802.3 (poly 0x04C11DB7), reflected in/out, init 0xFFFFFFFF, final XOR 0xFFFFFFFF.

Resynchronization: The receiver hunts for the sync pattern; invalid frames or CRC failures are dropped; reassembly state is reset on error or protocol violations.

## Command Payloads

The transport payload carries a single command request or response.

- Request:
  - `cmd_id` (uint16)
  - `payload_len` (uint16)
  - `payload` bytes
- Response:
  - `cmd_id` (uint16) — echoes request command
  - `status` (uint16) — 0 = OK; non-zero error codes
  - `payload_len` (uint16)
  - `payload` bytes (optional)

Correlation is provided by the transport `session` field; the command payload does not repeat it.

## Initial Commands

- `GET_GIT_VERSION` — returns compile-time `GARLIC_GIT_HASH`
- `GET_UPTIME` — returns system uptime in milliseconds (uint64)
- `ECHO` — returns the request payload unmodified (diagnostics)
- `FLASH_READ` — read a whitelisted flash region (reserved for future FW update support)
- `REBOOT` — request system reboot (acknowledge immediately; reboot is verified by integration tests)

Each command will be implemented in its own module under `app/src/app/commands/` with a corresponding header and unit tests.

## Notes

- The transport does not implement retransmissions or flow control.
- One in-progress reassembly is supported at a time by default.
- All sizes and bounds are validated before copying.
