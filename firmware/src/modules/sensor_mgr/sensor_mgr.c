#/**
 * @file sensor_mgr.c
 * @brief Sensor manager module for temperature sensor polling and publishing.
 *
 * This module handles periodic polling of a temperature sensor and publishes
 * the results to a ZBus channel for use by other system components.
 */
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

LOG_MODULE_REGISTER(sensor_mgr, LOG_LEVEL_INF);

#define SENSOR_NODE DT_NODELABEL(temp_sensor)
static const struct device *const temp_dev = DEVICE_DT_GET(SENSOR_NODE);

/**
 * @brief Read temperature sensor data.
 *
 * Fetches a sample from the temperature sensor and fills the provided
 * sensor_data structure with the result.
 *
 * @param[out] data Pointer to sensor_data structure to fill.
 *
 * @retval 0 Success
 * @retval <0 Negative error code from sensor API
 */
static int read_sensor_data(struct sensor_data *data) {
  int ret;
  struct sensor_value raw;

  ret = sensor_sample_fetch(temp_dev);
  if(ret < 0) {
    LOG_ERR("Failed to fetch temperature: %d", ret);
    return ret;
  }

  ret = sensor_channel_get(temp_dev, SENSOR_CHAN_AMBIENT_TEMP, &raw);
  if(ret < 0) {
    LOG_ERR("Failed to get sensor channel: %d", ret);
    return ret;
  }

  data->temp = (float)sensor_value_to_double(&raw);
  data->is_valid = true;
  return 0;
}

/**
 * @brief Sensor manager thread entry point.
 *
 * Periodically polls the temperature sensor and publishes the data to the
 * ZBus channel. Handles sensor initialization and error logging.
 *
 * @param p1 Unused
 * @param p2 Unused
 * @param p3 Unused
 */
static void sensor_mgr_entry(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  int ret;

  if(!device_is_ready(temp_dev)) {
    LOG_ERR("Sensor not ready");
    return;
  }

  /* Give sensor internal logic time to stabilize post-init */
  k_msleep(500);

  LOG_INF("Sensor manager started");

  while(1) {
    struct sensor_data msg = {.temp = CONFIG_DEFAULT_TEMP, .is_valid = false};

    ret = read_sensor_data(&msg);
    if(ret < 0) {
      LOG_ERR("Sensor read failed: %d", ret);
    }

    ret = zbus_chan_pub(&temp_chan, &msg, K_MSEC(10));
    if(ret < 0) {
      LOG_ERR("ZBus temp chan pub failed: %d", ret);
    }

    k_msleep(CONFIG_SENSOR_POLL_INTERVAL_MS);
  }
}

/**
 * @brief Define and start the sensor manager thread.
 */
K_THREAD_DEFINE(sensor_mgr_id, CONFIG_SENSOR_MGR_STACK_SIZE, sensor_mgr_entry, NULL, NULL, NULL, CONFIG_SENSOR_MGR_PRIORITY, 0, 0);
