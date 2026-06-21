#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"
#include "system_state.h"

LOG_MODULE_REGISTER(control_mgr, LOG_LEVEL_INF);

ZBUS_SUBSCRIBER_DEFINE(control_mgr, CONFIG_ZBUS_QUEUE_SIZE);

#define FAILSAFE_DUTY 100
#define FAN_IDLE_DUTY 50.0f

#define FAN_PWM_NODE DT_NODELABEL(fan_pwm)
static const struct pwm_dt_spec fan_pwm = PWM_DT_SPEC_GET(FAN_PWM_NODE);

struct pid_config {
	float kp;             /**< Proportional gain */
	float ki;             /**< Integral gain */
	float kd;             /**< Derivative gain */
	float dt;             /**< Time step (seconds) */
	float min_output;     /**< Minimum output value */
	float max_output;     /**< Maximum output value */
	float integral_limit; /**< Clamp for integral term */
};

struct pid_state {
	float integral;   /**< Accumulated integral value */
	float last_error; /**< Previous error value */
	bool initialized; /**< Initialization flag */
};

static struct pid_config fan_pid_cfg = {.kp = 8.0f,
					.ki = 0.3f,
					.kd = 2.0f,
					.dt = 1.0f,
					.min_output = 20.0f,
					.max_output = 100.0f,
					.integral_limit = 30.0f};

static int apply_fan_speed(uint8_t duty_percent)
{
	int ret;
	uint8_t clamped_duty = MIN(duty_percent, 100);
	uint32_t pulse = (uint32_t)((fan_pwm.period * clamped_duty) / 100);
	ret = pwm_set_dt(&fan_pwm, fan_pwm.period, pulse);
	if (ret < 0) {
		LOG_ERR("Failed to set pwm duty: %d", ret);
		return ret;
	}

	struct pid_data p = {.duty = clamped_duty};
	ret = zbus_chan_pub(&duty_chan, &p, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("ZBus duty chan publish failed: %d", ret);
	}

	return 0;
}

static uint8_t run_pid_step(struct pid_state *state, const struct pid_config *cfg, float setpoint,
			    float measured)
{
	if (!state->initialized) {
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

	float output = FAN_IDLE_DUTY + p_term + state->integral + d_term;

	return (uint8_t)CLAMP(output, cfg->min_output, cfg->max_output);
}

static void calibrate_pid(struct pid_config *cfg, struct pid_state *state)
{

  // PID calibration feature not implemented

	// Simulate PID calibration
	k_msleep(10000);

	system_state_post_event(SYSTEM_EVENT_DONE);
  // Delay to make sure the system_state thread catches the second system event
	k_msleep(20);
	system_state_post_event(SYSTEM_EVENT_EXIT_DEBUG);
}

static void process_pid_cal_chan_msg(struct pid_config *cfg, struct pid_state *state)
{
	if (system_state_get() != SYSTEM_STATE_PID_CAL) {
		LOG_WRN("Pid calibration cmd received not in SYSTEM_STATE_PID_CAL");
		return;
	}

	calibrate_pid(cfg, state);
}

static void process_duty_override_chan_msg(struct pid_data *duty_msg)
{
	int ret;

	struct pid_data override_data;
	ret = zbus_chan_read(&duty_override_chan, &override_data, K_MSEC(10));

	if (ret == 0 && system_state_get() == SYSTEM_STATE_SELF_TEST) {
		ret = apply_fan_speed(override_data.duty);
		if (ret < 0) {
			LOG_ERR("Failed to apply fan speed: %d", ret);
		}

		duty_msg->duty = override_data.duty;
	}
}

static void process_temp_chan_msg(struct sensor_data *current_sensor,
				  struct control_data *current_control, struct pid_data *duty_msg,
				  struct pid_state *fan_pid)
{
	int ret;

	ret = zbus_chan_read(&temp_chan, current_sensor, K_MSEC(10));
	if (ret < 0) {
		LOG_ERR("ZBus temp read failed: %d", ret);
		return;
	}

	ret = zbus_chan_read(&control_chan, current_control, K_MSEC(10));
	if (ret < 0) {
		LOG_ERR("ZBus control read failed: %d", ret);
	}

	if (system_state_get() == SYSTEM_STATE_NORMAL) {
		duty_msg->duty = run_pid_step(fan_pid, &fan_pid_cfg, current_control->target_temp,
					      current_sensor->temp);
		apply_fan_speed(duty_msg->duty);
	}
}

static void control_mgr_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int ret;

	struct sensor_data current_sensor = {.temp = CONFIG_DEFAULT_TEMP};
	struct control_data current_control = {.target_temp = CONFIG_DEFAULT_TARGET_TEMP};
	struct pid_data duty_msg = {.duty = CONFIG_DEFAULT_DUTY};
	struct pid_state fan_pid = {.initialized = false};
	const struct zbus_channel *chan;

	if (!pwm_is_ready_dt(&fan_pwm)) {
		LOG_ERR("Hardware not ready");
		return;
	}

	while (1) {
		ret = zbus_sub_wait(&control_mgr, &chan, K_MSEC(3000));
		if (ret == -EAGAIN) {
			if (system_state_get() == SYSTEM_STATE_NORMAL) {
				LOG_WRN("Sensor timeout - applying failsafe duty");
				apply_fan_speed(FAILSAFE_DUTY);
			}
			continue;
		} else if (ret < 0) {
			LOG_ERR("ZBus sub wait failed: %d", ret);
			continue;
		}

		if (chan == &temp_chan) {
			process_temp_chan_msg(&current_sensor, &current_control, &duty_msg,
					      &fan_pid);
		}
		if (chan == &duty_override_chan) {
			process_duty_override_chan_msg(&duty_msg);
		}
		if (chan == &pid_cal_chan) {
			process_pid_cal_chan_msg(&fan_pid_cfg, &fan_pid);
		}
	}
}

K_THREAD_DEFINE(control_mgr_id, CONFIG_CONTROL_MGR_STACK_SIZE, control_mgr_entry, NULL, NULL, NULL,
		CONFIG_CONTROL_MGR_PRIORITY, 0, 0);
