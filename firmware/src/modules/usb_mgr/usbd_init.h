#ifndef USBD_INIT_H__
#define USBD_INIT_H__

#include <stdint.h>
#include <zephyr/usb/usbd.h>

struct usbd_context *usbd_init_device(usbd_msg_cb_t msg_cb);

#endif /* USBD_INIT_H__ */
