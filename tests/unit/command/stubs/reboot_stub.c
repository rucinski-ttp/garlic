#include "commands/inc/register_all.h"

// Provide a no-op reboot registration for host unit tests (no Zephyr)
void grlc_cmd_register_reboot(void) {}
