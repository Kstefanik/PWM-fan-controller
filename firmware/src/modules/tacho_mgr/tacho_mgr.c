#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

LOG_MODULE_REGISTER(tacho_mgr, LOG_LEVEL_INF);

#define TACHO_POLL_INTERVAL K_MSEC(1000)
#define PULSES_PER_REV 2
#define MINUTES_TO_USEC 60000000
#define CAPTURE_TIMEOUT K_MSEC(500)

static const struct pwm_dt_spec tacho_input =
    PWM_DT_SPEC_GET(DT_NODELABEL(tacho_input));

static void tacho_mgr_entry(void) {
  int ret;

  if (!pwm_is_ready_dt(&tacho_input)) {
    LOG_ERR("Tachometer input device not ready");
    return;
  }
  uint32_t period_ticks, pulse_width_ticks;

  while (1) {
    struct tacho_data current_tacho = {0};
    ret = pwm_capture_cycles(tacho_input.dev, tacho_input.channel,
                             PWM_CAPTURE_TYPE_PERIOD, &period_ticks,
                             &pulse_width_ticks, CAPTURE_TIMEOUT);

    if (ret < 0) {
      current_tacho.rpm = 0;
      LOG_ERR("Capture error: %d", ret);
    }

    current_tacho.rpm = MINUTES_TO_USEC / (period_ticks * PULSES_PER_REV);

    ret = zbus_chan_pub(&rpm_chan, &current_tacho, K_NO_WAIT);
    if (ret < 0) {
      LOG_ERR("ZBus  rpm chan publish error: %d", ret);
    }

    k_sleep(TACHO_POLL_INTERVAL);
  }
}

K_THREAD_DEFINE(tacho_mgr_id, CONFIG_TACHO_MGR_STACK_SIZE, tacho_mgr_entry,
                NULL, NULL, NULL, CONFIG_TACHO_MGR_PRIORITY, 0, 0);