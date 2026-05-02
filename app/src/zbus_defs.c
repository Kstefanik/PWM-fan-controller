#include <zephyr/zbus/zbus.h>
#include <stdbool.h>

struct button_msg {
    bool pressed;
};

// 1. DEFINICJA KANAŁU[cite: 15]
ZBUS_CHAN_DEFINE(BTN_CHAN, 
                 struct button_msg, 
                 NULL, 
                 NULL, 
                 ZBUS_OBSERVERS(usb_thread_obs), 
                 { .pressed = false } 
);

// 2. DEFINICJA SUBSKRYBENTA[cite: 15]
ZBUS_SUBSCRIBER_DEFINE(usb_thread_obs, 4);