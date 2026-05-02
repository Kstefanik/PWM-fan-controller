#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

ZBUS_CHAN_DEFINE(temp_chan, struct sensor_data, NULL, NULL, ZBUS_OBSERVERS(control_mgr), ZBUS_MSG_INIT(.temp = CONFIG_DEFAULT_TEMP, .is_valid = false));

ZBUS_CHAN_DEFINE(rpm_chan, struct tacho_data, NULL, NULL, ZBUS_OBSERVERS(), ZBUS_MSG_INIT(.rpm = CONFIG_DEFAULT_RPM, .is_valid = false));

ZBUS_CHAN_DEFINE(control_chan, struct control_data, NULL, NULL, ZBUS_OBSERVERS(control_mgr), ZBUS_MSG_INIT(.target_temp = CONFIG_DEFAULT_TARGET_TEMP));

ZBUS_CHAN_DEFINE(duty_chan, struct pid_data, NULL, NULL, ZBUS_OBSERVERS(), ZBUS_MSG_INIT(.duty = CONFIG_DEFAULT_DUTY));
