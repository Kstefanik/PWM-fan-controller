/**
 * @file usbd_init.h
 * @brief USB device initialization API for Zephyr-based projects.
 *
 * Provides an interface for initializing a USB device context with a message callback.
 */
/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_SUBSYS_USB_COMMON_SAMPLE_USBD_H
#define ZEPHYR_SAMPLES_SUBSYS_USB_COMMON_SAMPLE_USBD_H

#include <stdint.h>
#include <zephyr/usb/usbd.h>

/**
 * @brief Initialize the USB device context.
 *
 * Sets up the USB device and registers a message callback for USB events.
 *
 * @param msg_cb Callback function for USB device messages/events.
 * @return Pointer to the initialized usbd_context structure, or NULL on failure.
 */
struct usbd_context *usbd_init_device(usbd_msg_cb_t msg_cb);

#endif /* ZEPHYR_SAMPLES_SUBSYS_USB_COMMON_SAMPLE_USBD_H */
