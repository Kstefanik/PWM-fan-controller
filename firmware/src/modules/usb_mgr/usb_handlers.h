#ifndef USB_HANDLERS_H__
#define USB_HANDLERS_H__

#include "message_channel.h"
#include "usb_msg_types.h"

int usb_init(const struct device *dev, struct k_sem *rx_sem);

void usb_tx_telemetry(const struct device *dev, const struct sensor_data *s,
		      const struct tacho_data *t, const struct pid_data *p,
		      const struct control_data *c, const int state);

int usb_rx_cmd(const struct device *dev, struct Command *cmd);

#endif /* USB_HANDLERS_H__ */
