/**
 * @file tmp119.h
 * @brief TI TMP119 digital temperature sensor driver (I2C)
 *
 * Register map and behavior per TMP119 datasheet (SNIS236 – January 2024).
 * See docs/datasheets/TMP119.pdf: Register Map (Table 8-2, p.32),
 * Temperature register (Section 8.5.2, p.32), Configuration (8.5.3),
 * Limits (8.5.4–8.5.5), EEPROM/Unlock (8.5.6–8.5.10), Offset (8.5.9),
 * and Device ID (8.5.11, p.33).
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TMP119 register addresses (Table 8-2, p.32) */
#define TMP119_REG_TEMPERATURE 0x00
#define TMP119_REG_CONFIG      0x01
#define TMP119_REG_HIGH_LIMIT  0x02
#define TMP119_REG_LOW_LIMIT   0x03
#define TMP119_REG_EE_UNLOCK   0x04
#define TMP119_REG_EE1         0x05
#define TMP119_REG_EE2         0x06
#define TMP119_REG_TEMP_OFFSET 0x07
#define TMP119_REG_EE3         0x08
#define TMP119_REG_DEVICE_ID   0x0F

/* Default 7-bit I2C addresses depending on ADD0 (Table 7-2, p.23):
 * 0x48 (ADD0=GND), 0x49 (ADD0=V+), 0x4A (ADD0=SDA), 0x4B (ADD0=SCL)
 */

/**
 * @brief Initialize TMP119 driver dependencies (I2C ready).
 *
 * Does not validate any device. Use tmp119_boot_init() or per-address calls
 * to verify presence.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_init(void);

/**
 * @brief Probe and initialize TMP119 at boot.
 *
 * Scans addresses 0x48–0x4B (Table 7-2, p.23). For each responding address,
 * reads Device ID (Sec 8.5.11, p.33) and, on match (0x2117), applies default
 * configuration (Sec 8.5.3) to continuous conversion and marks the address as
 * initialized.
 *
 * @return number of devices initialized (>=0) or negative errno on failure.
 */
int grlc_tmp119_boot_init(void);

/**
 * @brief Read the 16-bit device ID register.
 *
 * Expected reset value is 0x2117 (Section 8.5.11, p.33).
 *
 * @param addr7 7-bit I2C address.
 * @param id_out Pointer to store device ID.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_read_device_id(uint8_t addr7, uint16_t *id_out);

/**
 * @brief Read raw temperature register (16-bit).
 *
 * See Temperature register (Section 8.5.2, p.32). The value is in two's
 * complement with LSB = 1/128 °C.
 *
 * @param addr7 7-bit I2C address.
 * @param raw_out Pointer to store raw register value (MSB:bit15).
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_read_temperature_raw(uint8_t addr7, uint16_t *raw_out);

/**
 * @brief Read temperature in milli-Celsius.
 *
 * Converts the signed raw value using LSB = 1/128 °C (Section 8.5.2) to m°C.
 *
 * @param addr7 7-bit I2C address.
 * @param mC_out Pointer to store temperature in milli-Celsius.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_read_temperature_mC(uint8_t addr7, int32_t *mC_out);

/**
 * @brief Read configuration register.
 *
 * Configuration bits documented in Section 8.5.3 (p.32–33).
 *
 * @param addr7 7-bit I2C address.
 * @param cfg_out Pointer to store configuration register.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_read_config(uint8_t addr7, uint16_t *cfg_out);

/**
 * @brief Write configuration register.
 *
 * @param addr7 7-bit I2C address.
 * @param cfg Configuration value to write.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_write_config(uint8_t addr7, uint16_t cfg);

/**
 * @brief Read high temperature limit register.
 *
 * Section 8.5.4.
 * @param addr7 7-bit I2C address.
 * @param val_out Pointer to store 16-bit value.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_read_high_limit(uint8_t addr7, uint16_t *val_out);

/**
 * @brief Write high temperature limit register.
 *
 * @param addr7 7-bit I2C address.
 * @param val 16-bit value to write.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_write_high_limit(uint8_t addr7, uint16_t val);

/**
 * @brief Read low temperature limit register.
 *
 * Section 8.5.5.
 * @param addr7 7-bit I2C address.
 * @param val_out Pointer to store 16-bit value.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_read_low_limit(uint8_t addr7, uint16_t *val_out);

/**
 * @brief Write low temperature limit register.
 *
 * @param addr7 7-bit I2C address.
 * @param val 16-bit value to write.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_write_low_limit(uint8_t addr7, uint16_t val);

/**
 * @brief Unlock EEPROM registers for programming.
 *
 * Write 0x0001 to EE_UNLOCK (0x04) before writing EE1/EE2/EE3 (p.32, Sections 8.5.6–8.5.10).
 *
 * @param addr7 7-bit I2C address.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_unlock_eeprom(uint8_t addr7);

/**
 * @brief Read EEPROM register by index (1..3 maps to EE1, EE2, EE3).
 *
 * EE1=0x05, EE2=0x06, EE3=0x08 (Table 8-2). Note EE3 is not contiguous.
 *
 * @param addr7 7-bit I2C address.
 * @param index EEPROM index (1..3).
 * @param val_out Pointer to store 16-bit value.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_read_eeprom(uint8_t addr7, uint8_t index, uint16_t *val_out);

/**
 * @brief Write EEPROM register by index (1..3) after unlock.
 *
 * @param addr7 7-bit I2C address.
 * @param index EEPROM index (1..3).
 * @param val 16-bit value to write.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_write_eeprom(uint8_t addr7, uint8_t index, uint16_t val);

/**
 * @brief Read temperature offset register (0x07).
 *
 * Section 8.5.9 describes the offset behavior.
 *
 * @param addr7 7-bit I2C address.
 * @param val_out Pointer to store 16-bit value.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_read_offset(uint8_t addr7, uint16_t *val_out);

/**
 * @brief Write temperature offset register (0x07).
 *
 * @param addr7 7-bit I2C address.
 * @param val 16-bit value to write.
 * @return 0 on success, negative errno on failure.
 */
int grlc_tmp119_write_offset(uint8_t addr7, uint16_t val);

/**
 * @brief Ensure the given TMP119 address has been initialized.
 *
 * Validates Device ID (0x2117) and applies default configuration if needed.
 * On validation failure, enters project fatal state (prints to RTT/UART and halts).
 *
 * @param addr7 7-bit I2C address.
 */
void grlc_tmp119_require_initialized(uint8_t addr7);

#ifdef __cplusplus
}
#endif
