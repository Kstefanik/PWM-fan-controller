#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "message_channel.h"

LOG_MODULE_REGISTER(usb_mgr, LOG_LEVEL_INF);

#define REPORT_INTERVAL K_MSEC(500)
#define RX_BUF_SIZE 64

K_TIMER_DEFINE(usb_report_timer, NULL, NULL);

/* UART device for USB CDC ACM */
const struct device *usb_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

/* PC reception buffers */
static uint8_t rx_buf[RX_BUF_SIZE];
static int rx_buf_idx = 0;
static bool new_target_received = false;
static float received_target_temp = 25.0f;

/* UART interrupt handler for PC reception */
static void uart_interrupt_handler(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);
    int recv_len;
    uint8_t buffer[64];

    if (!uart_irq_update(dev)) {
        return;
    }

    if (uart_irq_rx_ready(dev)) {
        /* Read up to 64 bytes at once from FIFO */
        recv_len = uart_fifo_read(dev, buffer, sizeof(buffer));
        if (recv_len > 0) {
            for (int i = 0; i < recv_len; i++) {
                uint8_t c = buffer[i];

                /* Command complete on newline or carriage return */
                if (c == '\n' || c == '\r') {
                    if (rx_buf_idx > 0) {
                        rx_buf[rx_buf_idx] = '\0';
                        
                        /* Parse "SET:<float>" command */
                        if (strncmp((char *)rx_buf, "SET:", 4) == 0) {
                            received_target_temp = (float)atof((char *)&rx_buf[4]);
                            new_target_received = true;
                            LOG_INF("Parsed valid SET command: %.2f", (double)received_target_temp);
                        } else {
                            LOG_WRN("Unknown command: %s", rx_buf);
                        }
                        rx_buf_idx = 0;
                    }
                } else {
                    if (rx_buf_idx < (RX_BUF_SIZE - 1)) {
                        rx_buf[rx_buf_idx++] = c;
                    } else {
                        /* Prevent buffer overflow */
                        rx_buf_idx = 0;
                    }
                }
            }
        }
    }
}

static void usb_mgr_entry(void) {
    int ret;
    struct sensor_data current_sensor = {0};
    struct tacho_data current_tacho = {0};
    struct control_data current_control = {0};

    /* Initialize USB CDC ACM */
    if (!device_is_ready(usb_dev)) {
        LOG_ERR("CDC ACM device not ready");
        return;
    }

    ret = usb_enable(NULL);
    if (ret != 0) {
        LOG_ERR("Failed to enable USB");
        return;
    }

    uart_irq_callback_set(usb_dev, uart_interrupt_handler);
    uart_irq_rx_enable(usb_dev);

    LOG_INF("USB CDC ACM initialized");

    current_control.target_temp = 25.0f;

    k_timer_start(&usb_report_timer, REPORT_INTERVAL, REPORT_INTERVAL);

    while (1) {
        k_timer_status_sync(&usb_report_timer);

        /* Read current temperature and RPM from ZBUS */
        ret = zbus_chan_read(&temp_chan, &current_sensor, K_NO_WAIT);
        if (ret < 0) {
            current_sensor.temp = -1.0f;
        }

        ret = zbus_chan_read(&rpm_chan, &current_tacho, K_NO_WAIT);
        if (ret < 0) {
            current_tacho.rpm = 0;
        }

        /* Send RPM and Temp to PC via USB */
        char tx_buf[64];
        snprintf(tx_buf, sizeof(tx_buf), "TEMP:%.2f,RPM:%u\r\n", 
                 (double)current_sensor.temp, current_tacho.rpm);
        
        for (int i = 0; i < strlen(tx_buf); i++) {
            uart_poll_out(usb_dev, tx_buf[i]);
        }

        /* Process new target temperature from PC */
        if (new_target_received) {
            current_control.target_temp = received_target_temp;
            new_target_received = false;
            
            LOG_INF("New Target Temp received from USB: %.2f", (double)current_control.target_temp);

            /* Publish target temperature to PID controller */
            ret = zbus_chan_pub(&control_chan, &current_control, K_NO_WAIT);
            if (ret < 0) {
                LOG_ERR("ZBus control chan publish error: %d", ret);
            }
        }
    }
}

K_THREAD_DEFINE(usb_mgr_id, CONFIG_USB_MGR_STACK_SIZE, usb_mgr_entry, NULL,
                NULL, NULL, CONFIG_USB_MGR_PRIORITY, 0, 0);