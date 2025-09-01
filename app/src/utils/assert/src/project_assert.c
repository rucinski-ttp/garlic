#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_USE_SEGGER_RTT)
#include <SEGGER_RTT.h>
#endif
#include "drivers/uart/inc/uart.h"
LOG_MODULE_REGISTER(project_assert, LOG_LEVEL_ERR);
#endif

void grlc_assert_fatal(const char *msg)
{
#ifdef __ZEPHYR__
    if (!msg)
        msg = "Fatal error";
    /* Print to RTT and printk (if RTT enabled) */
#if defined(CONFIG_USE_SEGGER_RTT)
    SEGGER_RTT_WriteString(0, "FATAL: ");
    SEGGER_RTT_WriteString(0, msg);
    SEGGER_RTT_WriteString(0, "\n");
#endif
    printk("FATAL: %s\r\n", msg);
    LOG_ERR("%s", msg);
    /* Try to emit over UART DMA as best effort */
    (void)grlc_uart_send((const uint8_t *)"FATAL: ", 7);
    (void)grlc_uart_send((const uint8_t *)msg, strlen(msg));
    (void)grlc_uart_send((const uint8_t *)"\r\n", 2);
    /* Halt loop: keep processing UART DMA to flush */
    while (1) {
        grlc_uart_process();
        k_msleep(100);
    }
#else
    /* Host build: print to stderr and abort */
    if (!msg)
        msg = "Fatal error";
    fprintf(stderr, "FATAL: %s\n", msg);
    abort();
#endif
}

void project_fatal(const char *msg)
{
    grlc_assert_fatal(msg);
}
