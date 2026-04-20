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
  uint32_t period_ticks, pulse_width_ticks;
  uint32_t last_valid_rpm = 0;

  if (!pwm_is_ready_dt(&tacho_input)) {
    LOG_ERR("Tachometer input device not ready");
    return;
  }

  while (1) {
    struct tacho_data current_tacho = {0};

    // Capture the period
    ret = pwm_capture_cycles(tacho_input.dev, tacho_input.channel,
                             PWM_CAPTURE_TYPE_PERIOD, &period_ticks,
                             &pulse_width_ticks, CAPTURE_TIMEOUT);

    if (ret == -EAGAIN || ret == -ETIMEDOUT) {
      current_tacho.rpm = 0;
    } else if (ret < 0) {
      LOG_DBG("Capture failed: %d", ret);
      current_tacho.rpm = 0;
    } else {
      if (period_ticks > 0) {
        uint32_t raw_rpm =
            (uint32_t)(MINUTES_TO_USEC / (period_ticks * PULSES_PER_REV));

        // --- GLITCH FILTERING ---
        if (raw_rpm > 20000) {
          current_tacho.rpm = last_valid_rpm;
          LOG_WRN("Discarded glitch: %u RPM", raw_rpm);
        } else {
          current_tacho.rpm = raw_rpm;
          last_valid_rpm = raw_rpm;
        }
      }
    }

    zbus_chan_pub(&rpm_chan, &current_tacho, K_NO_WAIT);
    k_sleep(TACHO_POLL_INTERVAL);
  }
}

K_THREAD_DEFINE(tacho_mgr_id, CONFIG_TACHO_MGR_STACK_SIZE, tacho_mgr_entry,
                NULL, NULL, NULL, CONFIG_TACHO_MGR_PRIORITY, 0, 0);