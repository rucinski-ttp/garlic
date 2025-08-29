# Stack Module

This module hosts the application-side glue between the byte-transport layer and the command registry.

- `cmd_transport`: Binds the transport parser/encoder to the command dispatcher. Incoming request messages are parsed and dispatched; responses are encoded and written via the provided lower interface.

No hardware access is performed here; the UART/RTT adaptation is provided by the app runtime.

