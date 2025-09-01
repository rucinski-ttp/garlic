// Reboot command: perform a cold reboot as a controlled operation.

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include "commands/inc/command.h"
#include "commands/inc/ids.h"

static void do_reboot(struct k_work *work)
{
    ARG_UNUSED(work);
    sys_reboot(SYS_REBOOT_COLD);
}

static command_status_t
reboot_handler(const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len)
{
    (void)in;
    (void)in_len;
    (void)out;
    (void)out_len;
    /* Schedule a cold reboot shortly to allow GATT/serial write to complete. */
    static struct k_work_delayable w;
    static bool inited;
    if (!inited) {
        k_work_init_delayable(&w, do_reboot);
        inited = true;
    }
    k_work_schedule(&w, K_MSEC(50));
    return CMD_STATUS_OK;
}

void grlc_cmd_register_reboot(void)
{
    (void)grlc_cmd_register(CMD_ID_REBOOT, reboot_handler);
}
