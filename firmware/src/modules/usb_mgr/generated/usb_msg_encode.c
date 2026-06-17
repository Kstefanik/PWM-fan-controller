/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_encode.h"
#include "usb_msg_encode.h"
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

static bool encode_CommandType(zcbor_state_t *state, const struct CommandType_r *input);
static bool encode_Command(zcbor_state_t *state, const struct Command *input);
static bool encode_Telemetry(zcbor_state_t *state, const struct Telemetry *input);


static bool encode_CommandType(
		zcbor_state_t *state, const struct CommandType_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).CommandType_choice == CommandType_cmd_get_telemetry_c) ? (((zcbor_uint32_put(state, (1))))
	&& (zcbor_nil_put(state, NULL)))
	: (((*input).CommandType_choice == CommandType_cmd_set_target_temp_c) ? (((zcbor_uint32_put(state, (2))))
	&& (zcbor_float32_encode(state, (&(*input).cmd_set_target_temp))))
	: (((*input).CommandType_choice == CommandType_cmd_enter_debug_c) ? (((zcbor_uint32_put(state, (3))))
	&& (zcbor_nil_put(state, NULL)))
	: (((*input).CommandType_choice == CommandType_cmd_exit_debug_c) ? (((zcbor_uint32_put(state, (4))))
	&& (zcbor_nil_put(state, NULL)))
	: (((*input).CommandType_choice == CommandType_cmd_self_test_c) ? (((zcbor_uint32_put(state, (5))))
	&& (zcbor_nil_put(state, NULL)))
	: (((*input).CommandType_choice == CommandType_cmd_override_pwm_duty_c) ? (((zcbor_uint32_put(state, (6))))
	&& (zcbor_uint32_encode(state, (&(*input).cmd_override_pwm_duty))))
	: (((*input).CommandType_choice == CommandType_cmd_calibrate_pid_c) ? (((zcbor_uint32_put(state, (7))))
	&& (zcbor_nil_put(state, NULL)))
	: false)))))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_Command(
		zcbor_state_t *state, const struct Command *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 1) && ((((encode_CommandType(state, (&(*input).CommandType_m))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 1))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_Telemetry(
		zcbor_state_t *state, const struct Telemetry *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_map_start_encode(state, 5) && (((((zcbor_uint32_put(state, (1))))
	&& (zcbor_float32_encode(state, (&(*input).temp))))
	&& (((zcbor_uint32_put(state, (2))))
	&& (zcbor_int32_encode(state, (&(*input).rpm))))
	&& (((zcbor_uint32_put(state, (3))))
	&& (zcbor_uint32_encode(state, (&(*input).duty))))
	&& (((zcbor_uint32_put(state, (4))))
	&& (zcbor_float32_encode(state, (&(*input).target_temp))))
	&& (((zcbor_uint32_put(state, (5))))
	&& (zcbor_uint32_encode(state, (&(*input).system_state))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_map_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}



int cbor_encode_Telemetry(
		uint8_t *payload, size_t payload_len,
		const struct Telemetry *input,
		size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
		(zcbor_decoder_t *)encode_Telemetry, sizeof(states) / sizeof(zcbor_state_t), 1);
}


int cbor_encode_Command(
		uint8_t *payload, size_t payload_len,
		const struct Command *input,
		size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
		(zcbor_decoder_t *)encode_Command, sizeof(states) / sizeof(zcbor_state_t), 1);
}
