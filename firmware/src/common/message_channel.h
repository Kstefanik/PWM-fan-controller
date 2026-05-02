/**
 * @file message_channel.h
 * @brief Data structures and ZBus channel declarations for inter-module messaging.
 *
 * Defines the data types and ZBus channels used for communication between modules
 * such as sensor, tacho, control, and PID in the PWM fan controller project.
 */
#ifndef _MESSAGE_CHANNEL_H_
#define _MESSAGE_CHANNEL_H_

#include <zephyr/zbus/zbus.h>

/**
 * @brief Temperature sensor data structure.
 */
struct sensor_data {
  float temp;
  bool is_valid;
};
/**
 * @brief Tachometer data structure.
 */
struct tacho_data {
  uint16_t rpm;
  bool is_valid;
};
/**
 * @brief Control data structure for target temperature.
 */
struct control_data {
  float target_temp;
};
/**
 * @brief PID controller output data structure.
 */
struct pid_data {
  uint8_t duty;
};

/**
 * @brief Declare ZBus channels for inter-module communication.
 *
 * - temp_chan: Publishes sensor_data (temperature readings)
 * - rpm_chan: Publishes tacho_data (fan RPM)
 * - control_chan: Publishes control_data (target temperature)
 * - duty_chan: Publishes pid_data (PWM duty cycle)
 */
ZBUS_CHAN_DECLARE(temp_chan, rpm_chan, control_chan, duty_chan);

#endif /* _MESSAGE_CHANNEL_H_ */
