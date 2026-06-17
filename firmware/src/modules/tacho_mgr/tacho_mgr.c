#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

LOG_MODULE_REGISTER(tacho_mgr, LOG_LEVEL_INF);

#define PULSES_PER_REV  2
#define MINUTES_TO_USEC 60000000ULL
#define MAX_SANE_RPM    20000

#define TACHO_NODE DT_NODELABEL(tacho_input)
static const struct pwm_dt_spec tacho_input = PWM_DT_SPEC_GET(TACHO_NODE);

static uint32_t calculate_rpm(uint64_t period_usec, uint32_t last_val)
{
	if (period_usec == 0) {
		return 0;
	}

	/* Guard against division by zero and huge periods */
	uint64_t denominator = period_usec * PULSES_PER_REV;
	uint32_t raw_rpm = (uint32_t)(MINUTES_TO_USEC / denominator);

	if (raw_rpm > MAX_SANE_RPM) {
		LOG_WRN("Glitch: %u RPM. Keeping: %u", raw_rpm, last_val);
		return last_val;
	}

	return raw_rpm;
}

static void tacho_mgr_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int ret;

	uint32_t last_valid_rpm = 0;
	uint64_t period, pulse;

	if (!pwm_is_ready_dt(&tacho_input)) {
		LOG_ERR("Tachometer device %s not ready", tacho_input.dev->name);
		return;
	}

	while (1) {

		struct tacho_data msg = {.rpm = CONFIG_DEFAULT_RPM};
		bool do_publish = true;

		ret = pwm_capture_usec(tacho_input.dev, tacho_input.channel,
				       PWM_CAPTURE_TYPE_PERIOD, &period, &pulse,
				       K_MSEC(CONFIG_TACHO_CAPTURE_TIMEOUT_MS));
		if (ret == -EAGAIN || ret == -ETIMEDOUT) {
			/* Fan is likely stalled/stopped; this is a valid state */
			msg.rpm = 0;
		} else if (ret < 0) {
			LOG_ERR("Hardware capture failed: %d", ret);
			do_publish = false;
		} else {
			msg.rpm = calculate_rpm(period, last_valid_rpm);
			last_valid_rpm = msg.rpm;
		}

		if (do_publish) {
			ret = zbus_chan_pub(&rpm_chan, &msg, K_MSEC(10));
			if (ret < 0) {
				LOG_ERR("ZBus rpm chan pub failed: %d", ret);
			}
		}

		k_msleep(CONFIG_TACHO_POLL_INTERVAL_MS);
	}
}

K_THREAD_DEFINE(tacho_mgr_id, CONFIG_TACHO_MGR_STACK_SIZE, tacho_mgr_entry, NULL, NULL, NULL,
		CONFIG_TACHO_MGR_PRIORITY, 0, 0);
