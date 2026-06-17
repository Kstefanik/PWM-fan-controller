#ifndef MESSAGE_CHANNEL_H__
#define MESSAGE_CHANNEL_H__

#include <zephyr/zbus/zbus.h>

struct sensor_data {
	float temp;
};

struct tacho_data {
	uint16_t rpm;
};

struct control_data {
	float target_temp;
};

struct pid_data {
	uint8_t duty;
};

struct pid_cal_data {
};

ZBUS_CHAN_DECLARE(temp_chan, rpm_chan, control_chan, duty_chan, duty_override_chan, pid_cal_chan);

#endif /* MESSAGE_CHANNEL_H__ */
