Legacy test scripts
===================

This folder contains legacy, ad-hoc UART test scripts that relied on
external helpers (e.g., ~/pyhelpers) or manual serial bridges. They are
kept for historical reference but are not part of the supported workflow.

Preferred flow:
- Use Zephyr RTT for debug logs.
- Use pytest integration tests in `tests/integration` with the
  `serial_garlic_device` fixture.
- Use `scripts/monitor.sh` for quick manual serial monitoring.

