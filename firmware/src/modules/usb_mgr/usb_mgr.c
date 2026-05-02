#/**
 * @file usb_mgr.c
 * @brief USB manager module for telemetry and control via USB CDC ACM.
 *
 * This module handles USB communication for telemetry transmission and control
 * commands using Zephyr's CDC ACM UART driver. It synchronizes with other
 * system modules via ZBus channels and manages periodic and event-driven USB I/O.
 */
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"
#include "usb_handlers.h"

LOG_MODULE_REGISTER(usb_mgr, LOG_LEVEL_INF);

/* --- Synchronisation --- */
K_SEM_DEFINE(telemetry_tx_sem, 0, 1);
K_SEM_DEFINE(data_rx_sem, 0, 1);

/* Timer triggers every 500ms to signal a telemetry update */
/**
 * @brief Timer handler for telemetry transmission.
 *
 * Signals the telemetry thread to send an update.
 *
 * @param dummy Unused
 */
void telemetry_tx_timer_handler(struct k_timer *dummy) { k_sem_give(&telemetry_tx_sem); }
K_TIMER_DEFINE(telemetry_tx_timer, telemetry_tx_timer_handler, NULL);

const struct device *const usb_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

/**
 * @brief Fetch and transmit latest telemetry data over USB.
 *
 * Reads the latest sensor, tacho, and PID data from ZBus channels and sends
 * them over USB to the host. Logs a warning if any channel read fails.
 *
 * @param ctrl Pointer to current control data.
 */
static void process_telemetry_event(struct control_data *ctrl) {
  struct sensor_data s;
  struct tacho_data t;
  struct pid_data p;

  /* Read latest snapshots from channels */
  int ret = 0;
  ret |= zbus_chan_read(&temp_chan, &s, K_MSEC(10));
  ret |= zbus_chan_read(&rpm_chan, &t, K_MSEC(10));
  ret |= zbus_chan_read(&duty_chan, &p, K_MSEC(10));

  if(ret == 0) {
    usb_tx_telemetry(usb_dev, &s, &t, &p, ctrl);
  } else {
    LOG_WRN("Failed to read one or more ZBus channels");
  }
}

/**
 * @brief Handle incoming serial data and publish updates to ZBus.
 *
 * Parses a new target temperature from the USB input and publishes it to the
 * control channel if valid. Re-enables UART RX interrupts after processing.
 *
 * @param ctrl Pointer to current control data (updated in-place).
 */
static void process_rx_event(struct control_data *ctrl) {
  float new_temp = usb_rx_parse_float(usb_dev);

  /* Only publish if we got a valid command (positive float) */
  if(new_temp >= 0) {
    ctrl->target_temp = new_temp;
    if(zbus_chan_pub(&control_chan, ctrl, K_NO_WAIT) != 0) {
      LOG_ERR("ZBus publish failed");
    }
  }

  /* Re-enable interrupts after buffer processing is complete */
  uart_irq_rx_enable(usb_dev);
}

/**
 * @brief USB manager thread entry point.
 *
 * Initializes USB transport, waits for host connection, and manages telemetry
 * and control events using polling and semaphores. Handles both periodic
 * telemetry transmission and incoming control commands.
 */
static void usb_mgr_entry(void) {
  struct control_data current_control = {.target_temp = CONFIG_DEFAULT_TARGET_TEMP};

  struct k_poll_event events[] = {
      K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &telemetry_tx_sem, 0),
      K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &data_rx_sem, 0),
  };

  /* Initialize hardware transport and wait for DTR (terminal connection) */
  if(usb_transport_init(usb_dev, &data_rx_sem) != 0) {
    return;
  }
  usb_wait_for_host(usb_dev);

  /* Signal system and start the 500ms telemetry heartbeat */
  k_timer_start(&telemetry_tx_timer, K_MSEC(1000), K_MSEC(500));

  LOG_INF("USB Manager started");

  while(1) {
    /* Wait for either timer (Telemetry) or IRQ (Data RX) */
    k_poll(events, ARRAY_SIZE(events), K_FOREVER);

    if(events[0].state == K_POLL_STATE_SEM_AVAILABLE) {
      k_sem_take(&telemetry_tx_sem, K_NO_WAIT);
      process_telemetry_event(&current_control);
      events[0].state = K_POLL_STATE_NOT_READY;
    }

    if(events[1].state == K_POLL_STATE_SEM_AVAILABLE) {
      k_sem_take(&data_rx_sem, K_NO_WAIT);
      process_rx_event(&current_control);
      events[1].state = K_POLL_STATE_NOT_READY;
    }
  }
}

/**
 * @brief Define and start the USB manager thread.
 */
K_THREAD_DEFINE(usb_mgr_id, CONFIG_USB_MGR_STACK_SIZE, usb_mgr_entry, NULL, NULL, NULL, CONFIG_USB_MGR_PRIORITY, 0, 0);
