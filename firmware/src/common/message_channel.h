#ifndef _MESSAGE_CHANNEL_H_
#define _MESSAGE_CHANNEL_H_

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

ZBUS_CHAN_DECLARE(temp_chan, rpm_chan, control_chan);

#endif /* _MESSAGE_CHANNEL_H_ */