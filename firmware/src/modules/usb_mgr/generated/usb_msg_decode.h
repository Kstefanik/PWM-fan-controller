/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef USB_MSG_DECODE_H__
#define USB_MSG_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "usb_msg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif


int cbor_decode_Telemetry(
		const uint8_t *payload, size_t payload_len,
		struct Telemetry *result,
		size_t *payload_len_out);


int cbor_decode_Command(
		const uint8_t *payload, size_t payload_len,
		struct Command *result,
		size_t *payload_len_out);


#ifdef __cplusplus
}
#endif

#endif /* USB_MSG_DECODE_H__ */
