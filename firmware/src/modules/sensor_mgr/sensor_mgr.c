#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

LOG_MODULE_REGISTER(sensor_mgr, LOG_LEVEL_INF);

#define SENSOR_POLL_INTERVAL K_MSEC(1000)

static const struct device *temp_sensor = DEVICE_DT_GET(DT_NODELABEL(sht21));

static int fetch_temp_as_float(const struct device *dev, float *out_temp);

static void sensor_mgr_entry(void) {
  int ret;

  if (!device_is_ready(temp_sensor)) {
    LOG_ERR("Temperature sensor not ready");
    return;
  }

  while (1) {
    struct sensor_data current_sensor = {0};
    ret = fetch_temp_as_float(temp_sensor, &current_sensor.temp);
    if (ret < 0) {
      current_sensor.temp = 0.0f;
      LOG_ERR("Sensor fetch error: %d", ret);
      continue;
    }

    ret = zbus_chan_pub(&temp_chan, &current_sensor, K_NO_WAIT);
    if (ret < 0) {
      LOG_ERR("ZBus temp chan publish error: %d", ret);
    }

    k_sleep(SENSOR_POLL_INTERVAL);
  }
}

static int fetch_temp_as_float(const struct device *dev, float *out_temp) {
  int ret;
  struct sensor_value raw;

  ret = sensor_sample_fetch(dev);
  if (ret < 0)
    return ret;

  ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &raw);
  if (ret < 0)
    return ret;

  *out_temp = sensor_value_to_float(&raw);
  return 0;
}

K_THREAD_DEFINE(sensor_mgr_id, CONFIG_SENSOR_MGR_STACK_SIZE, sensor_mgr_entry,
                NULL, NULL, NULL, CONFIG_SENSOR_MGR_PRIORITY, 0, 0);
