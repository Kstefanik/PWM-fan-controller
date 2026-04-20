#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

ZBUS_CHAN_DEFINE(temp_chan, struct sensor_data, NULL, NULL,
                 ZBUS_OBSERVERS(control_mgr), ZBUS_MSG_INIT(.temp = 25.0f));

ZBUS_CHAN_DEFINE(rpm_chan, struct tacho_data, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.rpm = 0));

ZBUS_CHAN_DEFINE(control_chan, struct control_data, NULL, NULL,
                 ZBUS_OBSERVERS(control_mgr),
                 ZBUS_MSG_INIT(.target_temp = 25.0f));