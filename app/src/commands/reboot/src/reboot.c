// Reboot command: acknowledge immediately, then reboot shortly after so that
// the response can be flushed on the transport.

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include "commands/inc/command.h"
#include "commands/inc/ids.h"

static void do_reboot(struct k_work *work)
{
    ARG_UNUSED(work);
    sys_reboot(SYS_REBOOT_WARM);
}

static struct k_work_delayable reboot_work;

static command_status_t reboot_handler(const uint8_t *in, size_t in_len, uint8_t *out,
                                       size_t *out_len)
{
    (void)in;
    (void)in_len;
    (void)out;
    (void)out_len;
    // Schedule a warm reboot with a small delay to allow the response to
    // serialize over the transport and UART.
    k_work_schedule(&reboot_work, K_MSEC(300));
    return CMD_STATUS_OK;
}

void command_register_reboot(void)
{
    k_work_init_delayable(&reboot_work, do_reboot);
    (void)command_register(CMD_ID_REBOOT, reboot_handler);
}
