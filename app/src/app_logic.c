// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/uart.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/usb/usb_device.h>
// #include <string.h>

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

// #define BUF_SIZE 64

// int main(void)
// {
//     const struct device *dev;
//     uint8_t c;
//     char buf[BUF_SIZE];
//     int idx = 0;

//     if (!gpio_is_ready_dt(&led)) return 1;
//     gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

//     if (usb_enable(NULL)) return 1;

//     dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
//     if (!device_is_ready(dev)) return 1;

//     // Wymuszenie gotowości linii
//     uart_line_ctrl_set(dev, UART_LINE_CTRL_DTR, 1);
//     uart_line_ctrl_set(dev, UART_LINE_CTRL_RTS, 1);

//     memset(buf, 0, BUF_SIZE);

//     while (1) {
//         if (uart_poll_in(dev, &c) == 0) {
//             if (c == '\n' || c == '\r') {
//                 if (idx > 0) {
//                     buf[idx] = '\0';
//                     if (strstr(buf, "LED:1")) {
//                         gpio_pin_set_dt(&led, 1);
//                     } else if (strstr(buf, "LED:0")) {
//                         gpio_pin_set_dt(&led, 0);
//                     }
//                     idx = 0;
//                     memset(buf, 0, BUF_SIZE);
//                 }
//             } else {
//                 if (idx < BUF_SIZE - 1) {
//                     buf[idx++] = c;
//                 } else {
//                     idx = 0; // Zabezpieczenie przed przepełnieniem
//                     memset(buf, 0, BUF_SIZE);
//                 }
//             }
//         }
//         k_msleep(1);
//     }
//     return 0;
// }













// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/uart.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/usb/usb_device.h>
// #include <string.h>
// #include <stdio.h> // Potrzebne do sprintf

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
// #define BUF_SIZE 64

// int main(void) {
//     const struct device *dev;
//     uint8_t c;
//     char buf[BUF_SIZE];
//     char tx_buf[64];
//     int idx = 0;
//     uint32_t uptime = 0;

//     gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
//     if (usb_enable(NULL)) return 1;

//     dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
//     uart_line_ctrl_set(dev, UART_LINE_CTRL_DTR, 1);
//     uart_line_ctrl_set(dev, UART_LINE_CTRL_RTS, 1);

//     while (1) {
//         // 1. ODBIERANIE (Toggle LED)
//         if (uart_poll_in(dev, &c) == 0) {
//             if (c == '\n' || c == '\r') {
//                 if (idx > 0) {
//                     buf[idx] = '\0';
//                     if (strstr(buf, "LED:1")) gpio_pin_set_dt(&led, 1);
//                     else if (strstr(buf, "LED:0")) gpio_pin_set_dt(&led, 0);
//                     idx = 0;
//                 }
//             } else if (idx < BUF_SIZE - 1) {
//                 buf[idx++] = c;
//             }
//         }

//         // 2. WYSYŁANIE DANYCH (Co 1 sekundę - prosta metoda)
//         // Używamy licznika pętli lub k_uptime_get()
//         if (k_uptime_get() % 1000 < 10) { 
//             uptime = k_uptime_get() / 1000;
//             // Generujemy udawaną temperaturę (np. 20-30 stopni)
//             float temp = 20.0f + (uptime % 10); 
            
//             sprintf(tx_buf, "DATA:T=%.1f;U=%d\r\n", (double)temp, uptime);
            
//             for (int i = 0; i < strlen(tx_buf); i++) {
//                 uart_poll_out(dev, tx_buf[i]);
//             }
//             k_msleep(10); // Mały odstęp, żeby nie słać serii ramek w 10ms
//         }

//         k_msleep(1);
//     }
//     return 0;
// }











// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/uart.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/usb/usb_device.h>
// #include <stdio.h>
// #include <string.h>

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
// static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
// const struct device *uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

// void send_str(const char *str) {
//     for (size_t i = 0; i < strlen(str); i++) {
//         uart_poll_out(uart_dev, str[i]);
//     }
// }

// int main(void) {
//     uint8_t c;
//     char rx_buf[64];
//     int idx = 0;
//     bool last_state = false;

//     gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
//     gpio_pin_configure_dt(&btn, GPIO_INPUT);

//     usb_enable(NULL);
//     uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DTR, 1);

//     while (1) {
//         // 1. Obsługa LED (PC -> STM32)
//         if (uart_poll_in(uart_dev, &c) == 0) {
//             if (c == '\n' || c == '\r') {
//                 if (idx > 0) {
//                     rx_buf[idx] = '\0';
//                     if (strstr(rx_buf, "LED:1")) gpio_pin_set_dt(&led, 1);
//                     else if (strstr(rx_buf, "LED:0")) gpio_pin_set_dt(&led, 0);
//                     idx = 0;
//                 }
//             } else if (idx < 63) {
//                 rx_buf[idx++] = c;
//             }
//         }

//         // 2. Obsługa Przycisku (STM32 -> PC)
//         bool current_state = gpio_pin_get_dt(&btn);
//         if (current_state != last_state) {
//             if (current_state) {
//                 send_str("BTN:1\r\n");
//             } else {
//                 send_str("BTN:0\r\n");
//             }
//             last_state = current_state;
//         }

//         k_msleep(20); // Mały delay dla stabilności
//     }
//     return 0;
// }




















// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/uart.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/usb/usb_device.h>
// #include <string.h>
// #include <stdio.h>

// // Komunikacja C <-> C++
// extern "C" {
//     #include <zephyr/zbus/zbus.h>
//     ZBUS_CHAN_DECLARE(BTN_CHAN);
//     ZBUS_OBS_DECLARE(usb_thread_obs);
// }

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
// static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
// const struct device *uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

// struct button_msg {
//     bool pressed;
// };

// // Funkcja pomocnicza do wysyłania do PC
// void send_to_pc(const char *str) {
//     if (!uart_dev) return;
//     for (size_t i = 0; i < strlen(str); i++) {
//         uart_poll_out(uart_dev, str[i]);
//     }
// }

// // Wątek USB (Obsługuje ODBIERANIE rozkazów i WYSYŁANIE powiadomień)
// void usb_thread_entry(void *, void *, void *) {
//     uint8_t c;
//     char rx_buf[64];
//     int idx = 0;
//     const struct zbus_channel *chan;
//     struct button_msg btn_data;

//     usb_enable(NULL);
//     k_msleep(500);
//     uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DTR, 1);
//     uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_RTS, 1);

//     while (true) {
//         // --- [RX] ODBIERANIE z PC ---
//         if (uart_poll_in(uart_dev, &c) == 0) {
//             if (c == '\n' || c == '\r') {
//                 if (idx > 0) {
//                     rx_buf[idx] = '\0';
//                     if (strstr(rx_buf, "LED:1")) gpio_pin_set_dt(&led, 1);
//                     else if (strstr(rx_buf, "LED:0")) gpio_pin_set_dt(&led, 0);
//                     idx = 0;
//                 }
//             } else if (idx < 63) {
//                 rx_buf[idx++] = c;
//             }
//         }

//         // --- [TX] WYSYŁANIE do PC (przez ZBus) ---
//         if (!zbus_sub_wait(&usb_thread_obs, &chan, K_MSEC(10))) {
//             if (zbus_chan_read(&BTN_CHAN, &btn_data, K_NO_WAIT) == 0) {
//                 if (btn_data.pressed) send_to_pc("BTN:1\r\n");
//                 else send_to_pc("BTN:0\r\n");
//             }
//         }
//         k_yield();
//     }
// }

// // Wątek przycisku
// void sensors_thread_entry(void *, void *, void *) {
//     bool last_state = false;
//     struct button_msg msg;
//     gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
//     gpio_pin_configure_dt(&btn, GPIO_INPUT);

//     while (true) {
//         bool current_state = (bool)gpio_pin_get_dt(&btn);
//         if (current_state != last_state) {
//             msg.pressed = current_state;
//             zbus_chan_pub(&BTN_CHAN, &msg, K_NO_WAIT);
//             last_state = current_state;
//         }
//         k_msleep(20);
//     }
// }

// K_THREAD_DEFINE(usb_tid, 2048, usb_thread_entry, NULL, NULL, NULL, 7, 0, 0);
// K_THREAD_DEFINE(sensors_tid, 2048, sensors_thread_entry, NULL, NULL, NULL, 7, 0, 0);











// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/uart.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/usb/usb_device.h>
// #include <zephyr/zbus/zbus.h>
// #include <string.h>
// #include <stdio.h>
// #include <stdbool.h>

// // Deklaracje kanałów ZBus (muszą być zgodne z zbus_defs.c)[cite: 11]
// ZBUS_CHAN_DECLARE(BTN_CHAN);
// ZBUS_OBS_DECLARE(usb_thread_obs);

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
// static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
// const struct device *uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

// struct button_msg {
//     bool pressed;
// };

// // Funkcja pomocnicza do wysyłania do PC przez UART (USB CDC)
// void send_to_pc(const char *str) {
//     if (!uart_dev) return;
//     for (size_t i = 0; i < strlen(str); i++) {
//         uart_poll_out(uart_dev, str[i]);
//     }
// }

// // Wątek USB (Odbieranie rozkazów i wysyłanie powiadomień)
// void usb_thread_entry(void *p1, void *p2, void *p3) {
//     uint8_t c;
//     char rx_buf[64];
//     int idx = 0;
//     const struct zbus_channel *chan;
//     struct button_msg btn_data;

//     // --- KLUCZOWA POPRAWKA ---
//     // Aktywacja stosu USB. To sprawia, że w Linuksie pojawia się /dev/ttyACM0[cite: 10]
//     if (usb_enable(NULL)) {
//         return; 
//     }

//     // Czekamy chwilę na enumerację urządzenia w systemie[cite: 10]
//     k_msleep(500);
    
//     // Wymuszenie sygnałów DTR/RTS, aby terminal (Python) mógł się połączyć[cite: 10]
//     if (device_is_ready(uart_dev)) {
//         uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DTR, 1);
//         uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_RTS, 1);
//     }

//     while (true) {
//         // --- [RX] ODBIERANIE z PC (rozkazy LED:1 / LED:0) ---[cite: 10]
//         if (uart_poll_in(uart_dev, &c) == 0) {
//             if (c == '\n' || c == '\r') {
//                 if (idx > 0) {
//                     rx_buf[idx] = '\0';
//                     if (strstr(rx_buf, "LED:1")) gpio_pin_set_dt(&led, 1);
//                     else if (strstr(rx_buf, "LED:0")) gpio_pin_set_dt(&led, 0);
//                     idx = 0;
//                 }
//             } else if (idx < 63) {
//                 rx_buf[idx++] = c;
//             }
//         }

//         // --- [TX] WYSYŁANIE do PC (wiadomości BTN:1 / BTN:0 przez ZBus) ---[cite: 10, 11]
//         if (!zbus_sub_wait(&usb_thread_obs, &chan, K_MSEC(10))) {
//             if (zbus_chan_read(&BTN_CHAN, &btn_data, K_NO_WAIT) == 0) {
//                 if (btn_data.pressed) send_to_pc("BTN:1\r\n");
//                 else send_to_pc("BTN:0\r\n");
//             }
//         }
//         k_yield();
//     }
// }

// // Wątek sensora (sprawdzanie przycisku co 20ms)[cite: 10]
// void sensors_thread_entry(void *p1, void *p2, void *p3) {
//     bool last_state = false;
//     struct button_msg msg;
    
//     gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
//     gpio_pin_configure_dt(&btn, GPIO_INPUT);

//     while (true) {
//         bool current_state = (bool)gpio_pin_get_dt(&btn);
//         if (current_state != last_state) {
//             msg.pressed = current_state;
//             // Publikacja zmiany stanu przycisku na szynę ZBus[cite: 10, 11]
//             zbus_chan_pub(&BTN_CHAN, &msg, K_NO_WAIT);
//             last_state = current_state;
//         }
//         k_msleep(20);
//     }
// }

// // Definicja wątków w systemie Zephyr[cite: 10]
// K_THREAD_DEFINE(usb_tid, 2048, usb_thread_entry, NULL, NULL, NULL, 7, 0, 0);
// K_THREAD_DEFINE(sensors_tid, 2048, sensors_thread_entry, NULL, NULL, NULL, 7, 0, 0);










// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/uart.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/usb/usb_device.h>
// #include <zephyr/zbus/zbus.h>
// #include <string.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <stdbool.h>

// ZBUS_CHAN_DECLARE(BTN_CHAN);
// ZBUS_OBS_DECLARE(usb_thread_obs);

// // Pobieramy specyfikację dla diody zielonej (led0) i pomarańczowej (led1)
// static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
// static const struct gpio_dt_spec led_orange = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
// static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

// const struct device *uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

// struct button_msg {
//     bool pressed;
// };

// void send_to_pc(const char *str) {
//     if (!device_is_ready(uart_dev)) return;
//     for (size_t i = 0; i < strlen(str); i++) {
//         uart_poll_out(uart_dev, str[i]);
//     }
// }

// void usb_thread_entry(void *p1, void *p2, void *p3) {
//     uint8_t c;
//     char rx_buf[128]; // Zwiększony bufor, aby pomieścić całą ramkę T,R,D,TRG
//     int idx = 0;
//     const struct zbus_channel *chan;
//     struct button_msg btn_data;

//     if (usb_enable(NULL)) return;

//     // Konfiguracja obu diod
//     gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_ACTIVE);   // Startujemy z zieloną (poniżej 50 st)
//     gpio_pin_configure_dt(&led_orange, GPIO_OUTPUT_INACTIVE);

//     while (!device_is_ready(uart_dev)) k_msleep(10);
    
//     uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DTR, 1);
//     uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_RTS, 1);

//     while (true) {
//         // 1. CZYTANIE UART
//         while (uart_poll_in(uart_dev, &c) == 0) {
//             if (c == '\n' || c == '\r') {
//                 if (idx > 0) {
//                     rx_buf[idx] = '\0';
                    
//                     // Szukamy frazy "TRG:" w ramce od PC
//                     char *trg_ptr = strstr(rx_buf, "TRG:");
//                     if (trg_ptr) {
//                         float target_temp = strtof(trg_ptr + 4, NULL);
                        
//                         // LOGIKA TESTOWA:
//                         if (target_temp >= 50.0f) {
//                             gpio_pin_set_dt(&led_orange, 1); // Zapal pomarańczową
//                             gpio_pin_set_dt(&led_green, 0);  // Zgaś zieloną
//                         } else {
//                             gpio_pin_set_dt(&led_orange, 0); // Zgaś pomarańczową
//                             gpio_pin_set_dt(&led_green, 1);  // Zapal zieloną
//                         }
//                     }
//                     idx = 0;
//                 }
//             } else if (idx < sizeof(rx_buf) - 1) {
//                 rx_buf[idx++] = c;
//             }
//         }

//         // 2. ZBUS (Przyciski)
//         if (!zbus_sub_wait(&usb_thread_obs, &chan, K_NO_WAIT)) {
//             if (zbus_chan_read(&BTN_CHAN, &btn_data, K_NO_WAIT) == 0) {
//                 // Wysyłamy status BTN (Python to obsłuży poza ramką T,R,D,TRG)
//                 if (btn_data.pressed) send_to_pc("BTN:1\r\n");
//                 else send_to_pc("BTN:0\r\n");
//             }
//         }
        
//         k_sleep(K_MSEC(1));
//     }
// }

// // Reszta kodu (sensors_thread_entry i definicje wątków) bez zmian...
// void sensors_thread_entry(void *p1, void *p2, void *p3) {
//     bool last_state = false;
//     struct button_msg msg;
//     gpio_pin_configure_dt(&btn, GPIO_INPUT);

//     while (true) {
//         bool current_state = (bool)gpio_pin_get_dt(&btn);
//         if (current_state != last_state) {
//             msg.pressed = current_state;
//             zbus_chan_pub(&BTN_CHAN, &msg, K_NO_WAIT);
//             last_state = current_state;
//         }
//         k_msleep(20);
//     }
// }

// K_THREAD_DEFINE(usb_tid, 2048, usb_thread_entry, NULL, NULL, NULL, 7, 0, 0);
// K_THREAD_DEFINE(sensors_tid, 2048, sensors_thread_entry, NULL, NULL, NULL, 7, 0, 0);






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