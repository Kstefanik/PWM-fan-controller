#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"
#include "usb_handlers.h"
#include "usb_msg_types.h"
#include "system_state.h"

LOG_MODULE_REGISTER(usb_mgr, LOG_LEVEL_INF);

K_SEM_DEFINE(cmd_rx_sem, 0, 1);

const struct device *const usb_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

static void process_telemetry_tx_cmd()
{
	struct sensor_data s;
	struct tacho_data t;
	struct pid_data p;
	struct control_data c;

	int state = system_state_get();

	int ret = 0;
	ret |= zbus_chan_read(&temp_chan, &s, K_MSEC(10));
	ret |= zbus_chan_read(&rpm_chan, &t, K_MSEC(10));
	ret |= zbus_chan_read(&duty_chan, &p, K_MSEC(10));
	ret |= zbus_chan_read(&control_chan, &c, K_MSEC(10));

	if (ret < 0) {
		LOG_WRN("Failed to read one or more ZBus channels");
	} else {
		usb_tx_telemetry(usb_dev, &s, &t, &p, &c, state);
	}
}

static void process_duty_override_cmd(struct Command *cmd)
{
	int ret;

	if (system_state_get() == SYSTEM_STATE_SELF_TEST) {

		struct pid_data override_msg = {.duty = cmd->CommandType_m.cmd_override_pwm_duty};

		ret = zbus_chan_pub(&duty_override_chan, &override_msg, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("ZBus override publish failed: %d", ret);
		}
	} else {
		LOG_WRN("Cannot override PWM duty: not in SELF_TEST (state=%d)",
			system_state_get());
	}
}

static void process_set_target_temp_cmd(struct Command *cmd)
{
	int ret;
	struct control_data ctrl = {0};
	ctrl.target_temp = cmd->CommandType_m.cmd_set_target_temp;
	ret = zbus_chan_pub(&control_chan, &ctrl, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("ZBus control chan publish failed: %d", ret);
	}
}

static void process_calibrate_pid_cmd()
{
	int ret;
	struct pid_cal_data trigger = {};
	ret = zbus_chan_pub(&pid_cal_chan, &trigger, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("ZBus pid cal publish failed: %d", ret);
	}
}

static void usb_mgr_entry(void)
{
	int ret;

	ret = usb_init(usb_dev, &cmd_rx_sem);
	if (ret < 0) {
		return;
	}

	while (1) {
		k_sem_take(&cmd_rx_sem, K_FOREVER);

		struct Command cmd = {0};

		ret = usb_rx_cmd(usb_dev, &cmd);
		if (ret != 0) {
			LOG_ERR("RX command error: %d", ret);
		}

		switch (cmd.CommandType_m.CommandType_choice) {
		case CommandType_cmd_get_telemetry_c:
			LOG_DBG("Requesting telemetry");
			process_telemetry_tx_cmd();
			break;

		case CommandType_cmd_set_target_temp_c:
			LOG_INF("Requesting new target temp = %.2f",
				(double)cmd.CommandType_m.cmd_set_target_temp);
			process_set_target_temp_cmd(&cmd);
			break;

		case CommandType_cmd_enter_debug_c:
			LOG_INF("Requesting enter debug mode");
			system_state_post_event(SYSTEM_EVENT_ENTER_DEBUG);
			break;

		case CommandType_cmd_exit_debug_c:
			LOG_INF("Requesting exit debug mode");
			system_state_post_event(SYSTEM_EVENT_EXIT_DEBUG);
			break;

		case CommandType_cmd_self_test_c:
			LOG_INF("Requesting self test");
			system_state_post_event(SYSTEM_EVENT_SELF_TEST);
			break;

		case CommandType_cmd_override_pwm_duty_c:
			LOG_INF("Requesting pwm duty override = %u",
				cmd.CommandType_m.cmd_override_pwm_duty);
			process_duty_override_cmd(&cmd);
			break;

		case CommandType_cmd_calibrate_pid_c:
			LOG_INF("Requesting PID calibration");
			system_state_post_event(SYSTEM_EVENT_CALIBRATE_PID);
			process_calibrate_pid_cmd();
			break;

		default:
			LOG_WRN("Unknown command choice: %d", cmd.CommandType_m.CommandType_choice);
			break;
		}
	}
}

K_THREAD_DEFINE(usb_mgr_id, CONFIG_USB_MGR_STACK_SIZE, usb_mgr_entry, NULL, NULL, NULL,
		CONFIG_USB_MGR_PRIORITY, 0, 0);
