#/**
 * @file usbd_init.c
 * @brief USB device initialization implementation for Zephyr-based projects.
 *
 * Provides the implementation for initializing a USB device context, adding descriptors,
 * registering classes and callbacks, and starting the USB device.
 */
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(usb_init);

USBD_DEVICE_DEFINE(usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2fe3, 0x0001);

USBD_DESC_LANG_DEFINE(usbd_lang);
USBD_DESC_MANUFACTURER_DEFINE(usbd_mfr, "PWM");
USBD_DESC_PRODUCT_DEFINE(usbd_product, "PWM fan controller");
USBD_DESC_SERIAL_NUMBER_DEFINE(usbd_sn);

USBD_CONFIGURATION_DEFINE(usbd_config, USB_SCD_RESERVED, 250, NULL);

struct usbd_context *usbd_init_device(usbd_msg_cb_t msg_cb) {
  int ret;

  ret = usbd_add_descriptor(&usbd, &usbd_lang);
  if(ret < 0) {
    LOG_ERR("USB language descriptor init error: %d", ret);
    return NULL;
  }
  ret = usbd_add_descriptor(&usbd, &usbd_mfr);
  if(ret < 0) {
    LOG_ERR("USB manufacturer descriptor init error: %d", ret);
    return NULL;
  }
  ret = usbd_add_descriptor(&usbd, &usbd_product);
  if(ret < 0) {
    LOG_ERR("USB product descriptor init error: %d", ret);
    return NULL;
  }
  ret = usbd_add_descriptor(&usbd, &usbd_sn);
  if(ret < 0) {
    LOG_ERR("USB serial number descriptor init error: %d", ret);
    return NULL;
  }

  ret = usbd_add_configuration(&usbd, USBD_SPEED_FS, &usbd_config);
  if(ret < 0) {
    LOG_ERR("USB add configuration error: %d", ret);
    return NULL;
  }

  ret = usbd_register_class(&usbd, "cdc_acm_0", USBD_SPEED_FS, 1);
  if(ret < 0) {
    LOG_ERR("USB register class error: %d", ret);
    return NULL;
  }

  if(msg_cb != NULL) {
    ret = usbd_msg_register_cb(&usbd, msg_cb);
    if(ret < 0) {
      LOG_ERR("USB callback register error: %d", ret);
      return NULL;
    }
  }

  ret = usbd_init(&usbd);
  if(ret < 0) {
    LOG_ERR("USB init error: %d", ret);
    return NULL;
  }

  return &usbd;
}
