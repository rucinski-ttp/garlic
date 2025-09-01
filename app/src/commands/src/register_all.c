#include "commands/inc/register_all.h"

void command_register_git_version(void);
void command_register_uptime(void);
void command_register_flash_read(void);
void command_register_reboot(void);
void command_register_echo(void);
void command_register_i2c(void);
void command_register_tmp119(void);
void command_register_ble_ctrl(void);

void command_register_builtin(void)
{
    command_register_git_version();
    command_register_uptime();
    command_register_flash_read();
    command_register_reboot();
    command_register_echo();
    command_register_i2c();
    command_register_tmp119();
    command_register_ble_ctrl();
}
