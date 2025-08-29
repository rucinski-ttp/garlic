# Commands Module

Contains self-contained request/response command handlers and their registration glue.

- Core: command registry and pack/parse helpers.
- Built-ins: `git_version`, `uptime`, `flash_read`, `reboot`, `echo`.

Each command has its own folder (`inc/` and `src/`) and a small CMake file.

