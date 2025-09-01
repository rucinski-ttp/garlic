/**
 * @file project_assert.h
 * @brief Project-wide fatal assert utility.
 *
 * Prints a fatal message to RTT and UART, then halts forever. Intended for
 * unrecoverable conditions (e.g., required hardware missing).
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enter fatal state with message (namespaced API).
 *
 * Prints the message to RTT and UART (DMA) if available, then loops forever
 * (Zephyr build) or aborts the process (host build). This function never
 * returns on Zephyr.
 *
 * @param msg Null-terminated message string (may be NULL).
 */
void grlc_assert_fatal(const char *msg);

/**
 * @brief Deprecated alias for grlc_assert_fatal.
 * @deprecated Use grlc_assert_fatal() instead to follow naming convention.
 */
void project_fatal(const char *msg);

/**
 * @brief Assert condition or enter fatal state.
 * @param cond Condition expected to be true.
 * @param msg Message to print if condition is false.
 */
#define PROJECT_ASSERT(cond, msg)                                              \
    do {                                                                       \
        if (!(cond)) {                                                         \
            grlc_assert_fatal(msg);                                            \
        }                                                                      \
    } while (0)

#ifdef __cplusplus
}
#endif
