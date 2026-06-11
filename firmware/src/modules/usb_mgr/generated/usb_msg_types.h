/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef USB_MSG_TYPES_H__
#define USB_MSG_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY 3

struct Telemetry {
	float temp;
	int32_t rpm;
	uint32_t duty;
	float target_temp;
};

struct CommandType_r {
	union {
		struct {
			float cmd_set_target_temp;
		};
	};
	enum {
		CommandType_cmd_set_target_temp_c = 1,
		CommandType_cmd_enter_debug_mode_c = 2,
	} CommandType_choice;
};

struct Command {
	struct CommandType_r CommandType_m;
};

#ifdef __cplusplus
}
#endif

#endif /* USB_MSG_TYPES_H__ */
