#include "commands/inc/register_all.h"

void grlc_cmd_register_git_version(void);
void grlc_cmd_register_uptime(void);
void grlc_cmd_register_flash_read(void);
void grlc_cmd_register_reboot(void);
void grlc_cmd_register_echo(void);
void grlc_cmd_register_i2c(void);
void grlc_cmd_register_tmp119(void);
void grlc_cmd_register_ble_ctrl(void);

void grlc_cmd_register_builtin(void)
{
    grlc_cmd_register_git_version();
    grlc_cmd_register_uptime();
    grlc_cmd_register_flash_read();
    grlc_cmd_register_reboot();
    grlc_cmd_register_echo();
    grlc_cmd_register_i2c();
    grlc_cmd_register_tmp119();
    grlc_cmd_register_ble_ctrl();
}
