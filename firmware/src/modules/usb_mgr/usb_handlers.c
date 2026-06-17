#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/data/cobs.h>

#include "usb_handlers.h"
#include "usbd_init.h"

#include "usb_msg_encode.h"
#include "usb_msg_decode.h"

LOG_MODULE_REGISTER(usb_handlers, LOG_LEVEL_INF);

#define RING_BUF_SIZE  1024
#define FRAME_BUF_SIZE 64

uint8_t tx_rb_buffer[RING_BUF_SIZE];
struct ring_buf tx_ringbuf;

static bool rx_throttled;

static struct k_sem *rx_sem;

static uint8_t frame_buf[FRAME_BUF_SIZE];
static size_t frame_len;

static struct cobs_decoder dec;

static int cobs_decoder_cb(const uint8_t *buf, size_t len, void *user_data)
{
	ARG_UNUSED(user_data);

	if (buf == NULL) {
		/* NULL = end of COBS frame */
		k_sem_give(rx_sem);
		return 0;
	}

	if (frame_len + len > FRAME_BUF_SIZE) {
		LOG_ERR("Frame buffer overflow, dropping frame");
		frame_len = 0;
		return -ENOMEM;
	}

	memcpy(frame_buf + frame_len, buf, len);
	frame_len += len;

	return 0;
}

static void interrupt_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (true) {
		uart_irq_update(dev);

		if (uart_irq_is_pending(dev) <= 0) {
			break;
		}

		/* Handle receiving data */
		if (!rx_throttled && uart_irq_rx_ready(dev)) {
			int ret, recv_len;
			uint8_t rx_buffer[64];

			recv_len = uart_fifo_read(dev, rx_buffer, sizeof(rx_buffer));
			if (recv_len < 0) {
				LOG_ERR("Failed to read UART FIFO");
				continue;
			}

			ret = cobs_decoder_write(&dec, rx_buffer, recv_len);
			if (ret < 0) {
				LOG_ERR("COBS decode error: %d, resetting decoder", ret);
				cobs_decoder_init(&dec, cobs_decoder_cb, rx_sem,
						  COBS_FLAG_TRAILING_DELIMITER);
				frame_len = 0;
			}
		}

		/* Handle transmiting data */
		if (uart_irq_tx_ready(dev)) {
			uint8_t buffer[64];
			size_t rb_len, send_len;

			rb_len = ring_buf_get(&tx_ringbuf, buffer, sizeof(buffer));
			if (!rb_len) {
				LOG_DBG("Ring buffer empty, disable TX IRQ");
				uart_irq_tx_disable(dev);
				continue;
			}

			send_len = uart_fifo_fill(dev, buffer, rb_len);
			if (send_len < rb_len) {
				LOG_ERR("Drop %d bytes", rb_len - send_len);
			}
		}
	}
}

int usb_init(const struct device *dev, struct k_sem *sem)
{
	int ret;
	rx_sem = sem;

	if (!device_is_ready(dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	struct usbd_context *ctx = usbd_init_device(NULL);
	if (ctx == NULL) {
		LOG_ERR("Failed to initalize USB device");
		return -ENODEV;
	}

	ret = usbd_enable(ctx);
	if (ret < 0) {
		LOG_ERR("Failed to enable device support: %d", ret);
	}
	LOG_INF("USB device support enabled");

	ring_buf_init(&tx_ringbuf, sizeof(tx_rb_buffer), tx_rb_buffer);

	ret = cobs_decoder_init(&dec, cobs_decoder_cb, rx_sem, COBS_FLAG_TRAILING_DELIMITER);
	if (ret != 0) {
		LOG_ERR("COBS decoder init failed: %d", ret);
		return ret;
	}

	/* Wait 100ms for the host to do all settings */
	k_msleep(100);

	uart_irq_callback_set(dev, interrupt_handler);
	uart_irq_rx_enable(dev);

	return 0;
}

static bool usb_is_host_connected(const struct device *dev)
{
	uint32_t dtr = 0;

	if (uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr) == 0) {
		return (dtr != 0);
	}

	return false;
}

static int usb_send_data(const struct device *dev, const uint8_t *buf, size_t len)
{
	if (!usb_is_host_connected(dev)) {
		ring_buf_reset(&tx_ringbuf);
		return 0;
	}

	size_t rb_len = ring_buf_put(&tx_ringbuf, buf, len);

	if (rb_len < len) {
		LOG_ERR("TX Ring Buffer full! Dropped %d bytes", (int)(len - rb_len));
	}

	uart_irq_tx_enable(dev);

	return rb_len;
}

static int cobs_encoder_cb(const uint8_t *buf, size_t len, void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	usb_send_data(dev, buf, len);

	return 0;
}

void usb_tx_telemetry(const struct device *dev, const struct sensor_data *s,
		      const struct tacho_data *t, const struct pid_data *p,
		      const struct control_data *c, const int state)
{
	int ret;
	uint8_t cbor_buf[32];
	size_t encoded_len = 0;

	/* CBOR encoding*/
	struct Telemetry tx_data = {
		.temp = s->temp,
		.rpm = t->rpm,
		.duty = p->duty,
		.target_temp = c->target_temp,
		.system_state = state,
	};

	ret = cbor_encode_Telemetry(cbor_buf, sizeof(cbor_buf), &tx_data, &encoded_len);
	if (ret != 0) {
		LOG_ERR("Telemetry CBOR encoding failed: %d", ret);
		return;
	}

	/* COBS encoding*/
	struct cobs_encoder enc;

	ret = cobs_encoder_init(&enc, cobs_encoder_cb, (void *)dev, COBS_FLAG_TRAILING_DELIMITER);
	if (ret != 0) {
		LOG_ERR("COBS encoder init failed: %d", ret);
		return;
	}

	ret = cobs_encoder_write(&enc, cbor_buf, encoded_len);
	if (ret < 0) {
		LOG_ERR("COBS encoder write failed: %d", ret);
		return;
	}

	ret = cobs_encoder_close(&enc);
	if (ret != 0) {
		LOG_ERR("COBS encoder close failed: %d", ret);
	}
}

static size_t usb_read_data(const struct device *dev, uint8_t *buf, size_t len)
{
	ARG_UNUSED(dev);

	size_t copy_len = MIN(len, frame_len);
	memcpy(buf, frame_buf, copy_len);
	frame_len = 0;

	return copy_len;
}

int usb_rx_cmd(const struct device *dev, struct Command *cmd)
{
	int ret;
	uint8_t cbor_buf[FRAME_BUF_SIZE];

	size_t len = usb_read_data(dev, cbor_buf, sizeof(cbor_buf));
	if (len <= 0) {
		LOG_ERR("Failed to read frame data");
		return -EIO;
	}

	LOG_DBG("COBS decoding successful! Raw CBOR payload length: %d bytes", len);
	LOG_HEXDUMP_DBG(cbor_buf, len, "Raw CBOR Buffer Payload:");

	ret = cbor_decode_Command(cbor_buf, len, cmd, NULL);
	if (ret != 0) {
		LOG_ERR("CBOR decode failed: %d", ret);
		return ret;
	}

	return 0;
}
