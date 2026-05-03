#/**
 * @file control_mgr.c
 * @brief Control manager module for fan speed regulation using PID control.
 *
 * This module manages fan speed based on temperature readings using a PID controller.
 * It subscribes to ZBus channels for sensor and control data, applies PID logic,
 * and sets the fan PWM duty cycle accordingly.
 */
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

LOG_MODULE_REGISTER(control_mgr, LOG_LEVEL_INF);

/* Register as a ZBus subscriber */
ZBUS_SUBSCRIBER_DEFINE(control_mgr, CONFIG_ZBUS_QUEUE_SIZE);

#define FAILSAFE_DUTY 100

#define FAN_PWM_NODE DT_NODELABEL(fan_pwm)
#define FAN_EN_NODE DT_NODELABEL(fan_enable)
static const struct pwm_dt_spec fan_pwm = PWM_DT_SPEC_GET(FAN_PWM_NODE);
static const struct gpio_dt_spec fan_en = GPIO_DT_SPEC_GET(FAN_EN_NODE, gpios);

/**
 * @brief PID controller configuration parameters.
 */
struct pid_config {
  float kp;             /**< Proportional gain */
  float ki;             /**< Integral gain */
  float kd;             /**< Derivative gain */
  float dt;             /**< Time step (seconds) */
  float min_output;     /**< Minimum output value */
  float max_output;     /**< Maximum output value */
  float integral_limit; /**< Clamp for integral term */
};

/**
 * @brief PID controller runtime state.
 */
struct pid_state {
  float integral;   /**< Accumulated integral value */
  float last_error; /**< Previous error value */
  bool initialized; /**< Initialization flag */
};

static const struct pid_config fan_pid_cfg = {
    .kp = 25.0f, .ki = 1.5f, .kd = 0.5f, .dt = 1.0f, .min_output = 0.0f, .max_output = 100.0f, .integral_limit = 40.0f};

/**
 * @brief Apply the specified fan speed as a PWM duty cycle.
 *
 * Sets the PWM output to control the fan speed.
 *
 * @param duty_percent Duty cycle percentage (0-100).
 *
 * @retval 0 Success
 * @retval <0 Negative error code from PWM API
 */
static int apply_fan_speed(uint8_t duty_percent) {
  int ret;
  uint8_t clamped_duty = MIN(duty_percent, 100);
  uint32_t pulse = (uint32_t)((fan_pwm.period * clamped_duty) / 100);
  ret = pwm_set_dt(&fan_pwm, fan_pwm.period, pulse);
  if(ret < 0) {
    LOG_ERR("Failed to set pwm duty: %d", ret);
    return ret;
  }
  return 0;
}

/**
 * @brief Perform a single PID control step.
 *
 * Calculates the new control output based on the setpoint and measured value.
 *
 * @param state Pointer to PID state structure (updated in-place).
 * @param cfg Pointer to PID configuration.
 * @param setpoint Desired target value.
 * @param measured Current measured value.
 *
 * @return Clamped output value (duty cycle percent).
 */
static uint8_t run_pid_step(struct pid_state *state, const struct pid_config *cfg, float setpoint, float measured) {
  if(!state->initialized) {
    state->integral = 0.0f;
    state->last_error = 0.0f;
    state->initialized = true;
  }

  float error = measured - setpoint;
  float p_term = cfg->kp * error;

  state->integral += cfg->ki * error * cfg->dt;
  state->integral = CLAMP(state->integral, -cfg->integral_limit, cfg->integral_limit);

  float d_term = cfg->kd * ((error - state->last_error) / cfg->dt);
  state->last_error = error;

  float output = p_term + state->integral + d_term;

  return (uint8_t)CLAMP(output, cfg->min_output, cfg->max_output);
}

/**
 * @brief Initialize fan control hardware (PWM and GPIO).
 *
 * Configures the PWM and GPIO for fan control and applies the default duty cycle.
 *
 * @retval 0 Success
 * @retval <0 Negative error code from hardware APIs
 */
static int hardware_init(void) {
  int ret;

  if(!pwm_is_ready_dt(&fan_pwm) || !gpio_is_ready_dt(&fan_en)) {
    LOG_ERR("Hardware not ready");
    return -ENODEV;
  }

  ret = gpio_pin_configure_dt(&fan_en, GPIO_OUTPUT_ACTIVE);
  if(ret < 0) {
    LOG_ERR("Failed to configure GPIO: %d", ret);
    return ret;
  }
  ret = apply_fan_speed(CONFIG_DEFAULT_DUTY);
  if(ret < 0) {
    LOG_ERR("Failed to apply fan speed: %d", ret);
  }

  return 0;
}

/**
 * @brief Control manager thread entry point.
 *
 * Waits for new sensor/control data, runs the PID loop, and updates the fan speed.
 * Handles hardware initialization and ZBus communication.
 *
 * @param p1 Unused
 * @param p2 Unused
 * @param p3 Unused
 */
static void control_mgr_entry(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  int ret;

  struct sensor_data current_sensor = {.temp = CONFIG_DEFAULT_TEMP, .is_valid = false};
  struct control_data current_control = {.target_temp = CONFIG_DEFAULT_TARGET_TEMP};
  struct pid_state fan_pid = {.initialized = false};
  struct pid_data duty_msg = {.duty = CONFIG_DEFAULT_DUTY};
  const struct zbus_channel *chan;

  ret = hardware_init();
  if(ret < 0) {
    LOG_ERR("Hardware failed to initalize: %d", ret);
    return;
  }

  LOG_INF("Control manager started");

  while(1) {
    ret = zbus_sub_wait(&control_mgr, &chan, K_MSEC(3000));
    if(ret < 0) {
      LOG_ERR("ZBus sub wait failed: %d", ret);
    } else {
      /* Trigger event for PID loop is new temperature reading */
      if(chan == &temp_chan) {
        ret = zbus_chan_read(&temp_chan, &current_sensor, K_MSEC(10));
        if(ret < 0) {
          LOG_ERR("ZBus temp chan read failed: %d", ret);
        }
        ret = zbus_chan_read(&control_chan, &current_control, K_MSEC(10));
        if(ret < 0) {
          LOG_ERR("ZBus control chan read failed: %d", ret);
        }
      }

      /* Perform PID calculation whenever we have valid sensor data */
      if(current_sensor.is_valid) {
        duty_msg.duty = run_pid_step(&fan_pid, &fan_pid_cfg, current_control.target_temp, current_sensor.temp);
      }

      ret = apply_fan_speed(duty_msg.duty);
      if(ret < 0) {
        LOG_ERR("Failed to apply fan speed: %d", ret);
      }

      ret = zbus_chan_pub(&duty_chan, &duty_msg, K_MSEC(10));
      if(ret < 0) {
        LOG_ERR("ZBus duty chan pub failed: %d", ret);
      }
    }
  }
}

/**
 * @brief Define and start the control manager thread.
 */
K_THREAD_DEFINE(control_mgr_id, CONFIG_CONTROL_MGR_STACK_SIZE, control_mgr_entry, NULL, NULL, NULL, CONFIG_CONTROL_MGR_PRIORITY, 0, 0);
