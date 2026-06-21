#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

LOG_MODULE_REGISTER(sensor_mgr, LOG_LEVEL_INF);

#define SENSOR_NODE DT_NODELABEL(temp_sensor)
static const struct device *const temp_dev = DEVICE_DT_GET(SENSOR_NODE);

static int read_sensor_data(struct sensor_data *data)
{
	int ret;
	struct sensor_value raw;

	ret = sensor_sample_fetch(temp_dev);
	if (ret < 0) {
		LOG_ERR("Failed to fetch temperature: %d", ret);
		return ret;
	}

	ret = sensor_channel_get(temp_dev, SENSOR_CHAN_AMBIENT_TEMP, &raw);
	if (ret < 0) {
		LOG_ERR("Failed to get sensor channel: %d", ret);
		return ret;
	}

	data->temp = (float)sensor_value_to_double(&raw);
	return 0;
}

static void sensor_mgr_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int ret;

	if (!device_is_ready(temp_dev)) {
		LOG_ERR("Sensor not ready");
		return;
	}

	while (1) {
		struct sensor_data msg = {.temp = CONFIG_DEFAULT_TEMP};

		ret = read_sensor_data(&msg);
		if (ret == 0) {
			ret = zbus_chan_pub(&temp_chan, &msg, K_MSEC(10));
			if (ret < 0) {
				LOG_ERR("ZBus temp chan publish failed: %d", ret);
			}
		}

		k_msleep(CONFIG_SENSOR_POLL_INTERVAL_MS);
	}
}

K_THREAD_DEFINE(sensor_mgr_id, CONFIG_SENSOR_MGR_STACK_SIZE, sensor_mgr_entry, NULL, NULL, NULL,
		CONFIG_SENSOR_MGR_PRIORITY, 0, 0);
