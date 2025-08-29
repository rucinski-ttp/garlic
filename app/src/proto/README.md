# Proto Module

Implements the byte transport and CRC primitives used across the UART link.

- `transport.c/h`: Framing, fragmentation, CRC checking, and reassembly.
- `crc32.c/h`: IEEE CRC32 implementation.

See `docs/PROTOCOL.md` for the full wire protocol specification.

