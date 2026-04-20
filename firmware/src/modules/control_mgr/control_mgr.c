#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/pwm.h>

#include "message_channel.h"

LOG_MODULE_REGISTER(control_mgr, LOG_LEVEL_INF);

ZBUS_SUBSCRIBER_DEFINE(control_mgr, CONFIG_ZBUS_QUEUE_SIZE);

static const struct pwm_dt_spec fan = PWM_DT_SPEC_GET(DT_NODELABEL(fan_pwm));

static int apply_fan_speed(uint8_t duty_percent);

static void control_mgr_entry(void) {
  int ret;
  struct sensor_data current_sensor = {0};
  struct control_data current_control = {0};
  const struct zbus_channel *chan = {0};

  uint8_t target_duty = 50;

  if (!pwm_is_ready_dt(&fan)) {
    LOG_ERR("Fan PWM hardware not ready!");
    return;
  }

  while (1) {
    ret = zbus_sub_wait(&control_mgr, &chan, K_SECONDS(3));
    if (ret < 0) {
      LOG_ERR("ZBus subscriber error: %d", ret);
      continue;
    }

    if (chan == &temp_chan) {
      ret = zbus_chan_read(&temp_chan, &current_sensor, K_MSEC(100));
      if (ret < 0) {
        LOG_ERR("ZBus temp chan read error: %d", ret);
      }

      ret = zbus_chan_read(&control_chan, &current_control, K_NO_WAIT);
      if (ret < 0) {
        LOG_ERR("ZBus control chan read error: %d", ret);
      }

      /*--- TODO: PID calculation ---*/
      LOG_INF("Current Temp: %.2f C | Target Temp: %.2f C",
              (double)current_sensor.temp, (double)current_control.target_temp);

      ret = apply_fan_speed(target_duty);
      if (ret < 0) {
        LOG_ERR("Apply fan speed error: %d", ret);
      }
    }
  }
}

static int apply_fan_speed(uint8_t duty_percent) {
  int ret;

  duty_percent = MIN(duty_percent, 100);
  uint32_t pulse = (uint32_t)((fan.period * duty_percent) / 100);

  ret = pwm_set_dt(&fan, fan.period, pulse);
  if (ret < 0) {
    LOG_ERR("Set PWM duty cycle error: %d", ret);
    return ret;
  }
  return 0;
}

K_THREAD_DEFINE(control_mgr_id, CONFIG_CONTROL_MGR_STACK_SIZE,
                control_mgr_entry, NULL, NULL, NULL,
                CONFIG_CONTROL_MGR_PRIORITY, 0, 0);