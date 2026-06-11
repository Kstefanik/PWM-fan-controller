/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "usb_msg_decode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

#define log_result(state, result, func) do { \
	if (!result) { \
		zcbor_trace_file(state); \
		zcbor_log("%s error: %s\r\n", func, zcbor_error_str(zcbor_peek_error(state))); \
	} else { \
		zcbor_log("%s success\r\n", func); \
	} \
} while(0)

static bool decode_CommandType(zcbor_state_t *state, struct CommandType_r *result);
static bool decode_Command(zcbor_state_t *state, struct Command *result);
static bool decode_Telemetry(zcbor_state_t *state, struct Telemetry *result);


static bool decode_CommandType(
		zcbor_state_t *state, struct CommandType_r *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint_decode(state, &(*result).CommandType_choice, sizeof((*result).CommandType_choice)))) && ((((((*result).CommandType_choice == CommandType_cmd_set_target_temp_c) && (((1)
	&& (zcbor_float32_decode(state, (&(*result).cmd_set_target_temp))))))
	|| (((*result).CommandType_choice == CommandType_cmd_enter_debug_mode_c) && (((1)
	&& (zcbor_nil_expect(state, NULL)))))) || (zcbor_error(state, ZCBOR_ERR_WRONG_VALUE), false))))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_Command(
		zcbor_state_t *state, struct Command *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_decode(state) && ((((decode_CommandType(state, (&(*result).CommandType_m))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_Telemetry(
		zcbor_state_t *state, struct Telemetry *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_decode(state) && ((((zcbor_float32_decode(state, (&(*result).temp))))
	&& ((zcbor_int32_decode(state, (&(*result).rpm))))
	&& ((zcbor_uint32_decode(state, (&(*result).duty))))
	&& ((zcbor_float32_decode(state, (&(*result).target_temp))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}



int cbor_decode_Telemetry(
		const uint8_t *payload, size_t payload_len,
		struct Telemetry *result,
		size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
		(zcbor_decoder_t *)decode_Telemetry, sizeof(states) / sizeof(zcbor_state_t), 1);
}


int cbor_decode_Command(
		const uint8_t *payload, size_t payload_len,
		struct Command *result,
		size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
		(zcbor_decoder_t *)decode_Command, sizeof(states) / sizeof(zcbor_state_t), 1);
}
