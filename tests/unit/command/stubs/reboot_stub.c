#include "commands/inc/register_all.h"

// Provide a no-op reboot registration for host unit tests (no Zephyr)
void command_register_reboot(void) {}

