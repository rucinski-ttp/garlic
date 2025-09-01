# Stack Module

This module hosts the application-side glue between the byte-transport layer and the command registry.

- `cmd_transport`: Binds the transport parser/encoder to the command dispatcher. Incoming request
  messages are parsed and dispatched; responses are encoded and written via the provided lower
  interface.

Concurrency and safety:

- Each bound transport uses its own `cmd_transport_binding`, which carries a per-transport response
  buffer and (when running under Zephyr) a mutex to serialize response construction and sending.
  This avoids cross-link interference when UART and BLE are active at the same time and prevents
  data races that could corrupt response payloads.
- No dynamic allocation is used; buffers are statically sized and bounded by
  `TRANSPORT_REASSEMBLY_MAX`.

No hardware access is performed here; the UART and BLE adaptations are provided by the app runtime.
