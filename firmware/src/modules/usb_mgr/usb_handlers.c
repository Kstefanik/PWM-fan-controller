#/**
 * @file usb_handlers.c
 * @brief USB CDC ACM handlers for UART and telemetry/control communication.
 *
 * Implements USB transport initialization, host wait, telemetry transmission,
 * and float parsing for a Zephyr-based USB CDC ACM interface.
 */
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>

#include "usb_handlers.h"
#include "usbd_init.h"

LOG_MODULE_DECLARE(usb_mgr, LOG_LEVEL_INF);

static char rx_buffer[64];
static int rx_ptr = 0;
static struct k_sem *uart_sem;

static void usb_uart_cb(const struct device *dev, void *user_data) {
  if(!uart_irq_update(dev))
    return;

  if(uart_irq_rx_ready(dev)) {
    /* Stop interrupts so the thread has time to process the FIFO */
    uart_irq_rx_disable(dev);
    k_sem_give(uart_sem);
  }
}

int usb_transport_init(const struct device *dev, struct k_sem *rx_sem) {
  uart_sem = rx_sem;

  struct usbd_context *ctx = usbd_init_device(NULL);
  if(!ctx || usbd_enable(ctx) < 0) {
    LOG_ERR("USB Stack failed");
    return -1;
  }

  if(!device_is_ready(dev)) {
    LOG_ERR("UART device not ready");
    return -1;
  }

  uart_irq_callback_set(dev, usb_uart_cb);
  uart_irq_rx_enable(dev);
  return 0;
}

void usb_wait_for_host(const struct device *dev) {
  uint32_t dtr = 0;
  while(!dtr) {
    uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
    k_msleep(100);
  }
  LOG_INF("Terminal connected!");
}

void usb_tx_telemetry(const struct device *dev, struct sensor_data *s, struct tacho_data *t, struct pid_data *p, struct control_data *c) {
  char msg[128];
  int len = snprintf(msg, sizeof(msg), "T:%.2f,R:%d,D:%u,TRG:%.2f\r\n", (double)s->temp, t->rpm, p->duty, (double)c->target_temp);

  if(len > 0) {
    for(int i = 0; i < len; i++) {
      uart_poll_out(dev, msg[i]);
    }
  }
}

float usb_rx_parse_float(const struct device *dev) {
  uint8_t c;
  float val = -1.0f;

  while(uart_fifo_read(dev, &c, 1) > 0) {
    if(c == '\n' || c == '\r') {
      if(rx_ptr > 0) {
        rx_buffer[rx_ptr] = '\0';
        char *endptr;
        val = strtof(rx_buffer, &endptr);

        /* Ensure we actually parsed digits and didn't just get a newline */
        if(endptr == rx_buffer) {
          val = -1.0f;
        } else {
          /* Formatting for terminal feedback */
          uart_poll_out(dev, '\r');
          uart_poll_out(dev, '\n');
        }
        rx_ptr = 0; // Always reset pointer after a newline
        return val;
      }
    } else if((c >= '0' && c <= '9') || c == '.' || c == '-') {
      if(rx_ptr < (sizeof(rx_buffer) - 1)) {
        rx_buffer[rx_ptr++] = c;
        uart_poll_out(dev, c); // Echo digits/decimals only
      }
    }
  }
  return -1.0f;
}
