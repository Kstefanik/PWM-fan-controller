
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/zbus/zbus.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

ZBUS_CHAN_DECLARE(BTN_CHAN);
ZBUS_OBS_DECLARE(usb_thread_obs);

static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led_orange = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

const struct device *uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

// Zmienne do przechowywania stanu systemu
float global_t = 24.5f;
int global_r = 1200;
float global_d = 30.0f;
float global_trg = 40.0f;

struct button_msg {
    bool pressed;
};

void send_to_pc(const char *str) {
    if (!device_is_ready(uart_dev)) return;
    for (size_t i = 0; i < strlen(str); i++) {
        uart_poll_out(uart_dev, str[i]);
    }
}

void usb_thread_entry(void *p1, void *p2, void *p3) {
    uint8_t c;
    char rx_buf[128];
    int idx = 0;
    const struct zbus_channel *chan;
    struct button_msg btn_data;
    
    uint32_t last_send_time = 0;

    if (usb_enable(NULL)) return;

    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&led_orange, GPIO_OUTPUT_INACTIVE);

    while (!device_is_ready(uart_dev)) k_msleep(10);
    
    uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DTR, 1);
    uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_RTS, 1);

    while (true) {
        // 1. CZYTANIE RAMKI Z PC (Odbieramy TRG)
        while (uart_poll_in(uart_dev, &c) == 0) {
            if (c == '\n' || c == '\r') {
                if (idx > 0) {
                    rx_buf[idx] = '\0';
                    char *trg_ptr = strstr(rx_buf, "TRG:");
                    if (trg_ptr) {
                        global_trg = strtof(trg_ptr + 4, NULL);
                        
                        // Logika diod na podstawie TRG
                        if (global_trg >= 50.0f) {
                            gpio_pin_set_dt(&led_orange, 1);
                            gpio_pin_set_dt(&led_green, 0);
                        } else {
                            gpio_pin_set_dt(&led_orange, 0);
                            gpio_pin_set_dt(&led_green, 1);
                        }
                    }
                    idx = 0;
                }
            } else if (idx < sizeof(rx_buf) - 1) {
                rx_buf[idx++] = c;
            }
        }

        // 2. WYSYŁANIE RAMKI KOLEGI DO PC (Co 100ms)
        uint32_t now = k_uptime_get_32();
        if (now - last_send_time >= 100) {
            char tx_buf[128];
            // Format identyczny jak u kolegi: T, R, D, TRG
            snprintf(tx_buf, sizeof(tx_buf), "T:%.2f,R:%d,D:%.1f,TRG:%.2f\r\n", 
                     global_t, global_r, global_d, global_trg);
            send_to_pc(tx_buf);
            last_send_time = now;
        }

        // 3. OBSŁUGA PRZYCISKU (Osobno, poza ramką)
        if (!zbus_sub_wait(&usb_thread_obs, &chan, K_NO_WAIT)) {
            if (zbus_chan_read(&BTN_CHAN, &btn_data, K_NO_WAIT) == 0) {
                if (btn_data.pressed) send_to_pc("BTN:1\r\n");
                else send_to_pc("BTN:0\r\n");
            }
        }
        
        k_sleep(K_MSEC(1));
    }
}

// Sensors thread bez zmian (czyta fizyczny przycisk)
void sensors_thread_entry(void *p1, void *p2, void *p3) {
    bool last_state = false;
    struct button_msg msg;
    gpio_pin_configure_dt(&btn, GPIO_INPUT);
    while (true) {
        bool current_state = (bool)gpio_pin_get_dt(&btn);
        if (current_state != last_state) {
            msg.pressed = current_state;
            zbus_chan_pub(&BTN_CHAN, &msg, K_NO_WAIT);
            last_state = current_state;
        }
        k_msleep(20);
    }
}

K_THREAD_DEFINE(usb_tid, 2048, usb_thread_entry, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(sensors_tid, 2048, sensors_thread_entry, NULL, NULL, NULL, 7, 0, 0);
