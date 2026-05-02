/**
 * @file usb_handlers.h
 * @brief USB CDC ACM handler API for telemetry and control.
 *
 * Provides function declarations for initializing USB transport, waiting for host,
 * sending telemetry, and parsing float values from the RX buffer.
 */
#ifndef USB_HANDLERS_H
#define USB_HANDLERS_H

#include "message_channel.h" // For your data structs
#include <zephyr/device.h>

/**
 * @brief Initialize USB stack and UART interrupts.
 *
 * @param dev UART device pointer.
 * @param rx_sem Pointer to RX semaphore for signaling.
 * @retval 0 Success
 * @retval -1 Failure
 */
int usb_transport_init(const struct device *dev, struct k_sem *rx_sem);

/**
 * @brief Wait for a terminal to be connected (DTR).
 *
 * Blocks until the host asserts DTR, indicating a terminal is connected.
 *
 * @param dev UART device pointer.
 */
void usb_wait_for_host(const struct device *dev);

/**
 * @brief Format and send telemetry over UART.
 *
 * Formats and sends sensor, tacho, PID, and control data as a single line.
 *
 * @param dev UART device pointer.
 * @param s Pointer to sensor data.
 * @param t Pointer to tacho data.
 * @param p Pointer to PID data.
 * @param c Pointer to control data.
 */
void usb_tx_telemetry(const struct device *dev, struct sensor_data *s, struct tacho_data *t, struct pid_data *p, struct control_data *c);

/**
 * @brief Parse the RX buffer for a float value.
 *
 * Reads characters from the UART FIFO, parses a float on newline, and echoes input.
 * Returns -1.0f if parsing fails or no valid number is received.
 *
 * @param dev UART device pointer.
 * @return Parsed float value, or -1.0f on error/invalid input.
 */
float usb_rx_parse_float(const struct device *dev);

#endif
