#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

LOG_MODULE_REGISTER(usb_mgr, LOG_LEVEL_INF);

#define REPORT_INTERVAL K_MSEC(100)

K_TIMER_DEFINE(usb_report_timer, NULL, NULL);

static void usb_mgr_entry(void) {
  int ret;

  struct sensor_data current_sensor = {0};
  struct tacho_data current_tacho = {0};
  struct control_data current_control = {0};

  k_timer_start(&usb_report_timer, REPORT_INTERVAL, REPORT_INTERVAL);

  while (1) {
    k_timer_status_sync(&usb_report_timer);

    ret = zbus_chan_read(&temp_chan, &current_sensor, K_NO_WAIT);
    if (ret < 0) {
      LOG_ERR("ZBus temp chan read error: %d", ret);
    }

    ret = zbus_chan_read(&rpm_chan, &current_tacho, K_NO_WAIT);
    if (ret < 0) {
      LOG_ERR("ZBus rpm chan read error: %d", ret);
    }

    /*--- TODO: Sending RPM and Temp to PC via usb ---*/
    LOG_INF("USB OUT -> Temp: %.2f C | RPM: %u", (double)current_sensor.temp,
            current_tacho.rpm);

    /*--- TODO: Receiving target Temp from PC via usb ---*/
    current_control.target_temp = 25.0f;
    ret = zbus_chan_pub(&control_chan, &current_control, K_NO_WAIT);
    if (ret < 0) {
      LOG_ERR("ZBus control chan publish error: %d", ret);
    }
  }
}

K_THREAD_DEFINE(usb_mgr_id, CONFIG_USB_MGR_STACK_SIZE, usb_mgr_entry, NULL,
                NULL, NULL, CONFIG_USB_MGR_PRIORITY, 0, 0);