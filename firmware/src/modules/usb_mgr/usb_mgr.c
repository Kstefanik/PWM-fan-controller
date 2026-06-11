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
#include "usb_msg_types.h"

LOG_MODULE_REGISTER(usb_mgr, LOG_LEVEL_INF);

/* --- Synchronisation --- */
K_SEM_DEFINE(telemetry_tx_sem, 0, 1);
K_SEM_DEFINE(cmd_rx_sem, 0, 1);

/* Timer triggers every 500ms to signal a telemetry update */
/**
 * @brief Timer handler for telemetry transmission.
 *
 * Signals the telemetry thread to send an update.
 *
 * @param dummy Unused
 */
void telemetry_tx_timer_handler(struct k_timer *dummy)
{
	k_sem_give(&telemetry_tx_sem);
}
K_TIMER_DEFINE(telemetry_tx_timer, telemetry_tx_timer_handler, NULL);

const struct device *const usb_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

/**
 * @brief Fetch and transmit latest telemetry data over USB.
 *
 * Reads the latest sensor, tacho, PID and control data from ZBus channels and sends
 * them over USB to the host. Logs a warning if any channel read fails.
 *
 */
static void process_telemetry_tx_event()
{
	if (!usb_is_host_connected(usb_dev)) {
	  LOG_ERR("Host disconnected, skipping telemetry transmission");
    k_msleep(100);
	  return;
	}

	struct sensor_data s;
	struct tacho_data t;
	struct pid_data p;
	struct control_data c;

	int ret = 0;
	ret |= zbus_chan_read(&temp_chan, &s, K_MSEC(10));
	ret |= zbus_chan_read(&rpm_chan, &t, K_MSEC(10));
	ret |= zbus_chan_read(&duty_chan, &p, K_MSEC(10));
	ret |= zbus_chan_read(&control_chan, &c, K_MSEC(10));

	if (ret < 0) {
		LOG_WRN("Failed to read one or more ZBus channels");
	} else {
		usb_tx_telemetry(usb_dev, &s, &t, &p, &c);
	}
}

/**
 * @brief Handle incoming serial data and publish updates to ZBus.
 *
 * Parses a new target temperature from the USB input and publishes it to the
 * control channel. Re-enables UART RX interrupts after processing.
 *
 */
static void process_cmd_rx_event(void)
{
	int ret;
	struct Command cmd = {0};
	struct control_data ctrl;

	ret = usb_rx_cmd(usb_dev, &cmd);
	if (ret != 0) {
		LOG_ERR("RX command error: %d", ret);
    k_msleep(100);
		return;
	}

	switch (cmd.CommandType_m.CommandType_choice) {
	case CommandType_cmd_set_target_temp_c:
		ctrl.target_temp = cmd.CommandType_m.cmd_set_target_temp;

		LOG_INF("New target temp = %.2f", (double)ctrl.target_temp);

		ret = zbus_chan_pub(&control_chan, &ctrl, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("ZBus publish failed: %d", ret);
		}
		break;

	case CommandType_cmd_enter_debug_mode_c:
		LOG_INF("Entering debug mode...");
		break;

	default:
		LOG_WRN("Unknown command choice: %d", cmd.CommandType_m.CommandType_choice);
		break;
	}
}

/**
 * @brief USB manager thread entry point.
 *
 * Initializes USB transport, waits for host connection, and manages telemetry
 * and control events using polling and semaphores. Handles both periodic
 * telemetry transmission and incoming control commands.
 */
static void usb_mgr_entry(void)
{
	int ret;

	struct k_poll_event events[] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
						&telemetry_tx_sem, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
						&cmd_rx_sem, 0),
	};

	/* Initialize hardware transport and wait for DTR (terminal connection) */
	ret = usb_init(usb_dev, &cmd_rx_sem);
	if (ret < 0) {
		return;
	}

	usb_wait_for_host(usb_dev);

	/* Signal system and start the 500ms telemetry heartbeat */
	k_timer_start(&telemetry_tx_timer, K_MSEC(1000), K_MSEC(500));

	LOG_INF("USB Manager started");

	while (1) {
		/* Wait for either Telemetry timer or Data from USB */
		k_poll(events, ARRAY_SIZE(events), K_FOREVER);

		if (events[0].state == K_POLL_STATE_SEM_AVAILABLE) {
			k_sem_take(&telemetry_tx_sem, K_NO_WAIT);
			process_telemetry_tx_event();
			events[0].state = K_POLL_STATE_NOT_READY;
		}

		if (events[1].state == K_POLL_STATE_SEM_AVAILABLE) {
			k_sem_take(&cmd_rx_sem, K_NO_WAIT);
			process_cmd_rx_event();
			events[1].state = K_POLL_STATE_NOT_READY;
		}
	}
}

/**
 * @brief Define and start the USB manager thread.
 */
K_THREAD_DEFINE(usb_mgr_id, CONFIG_USB_MGR_STACK_SIZE, usb_mgr_entry, NULL, NULL, NULL,
		CONFIG_USB_MGR_PRIORITY, 0, 0);
