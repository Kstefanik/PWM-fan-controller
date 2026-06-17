#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdint.h>

typedef enum {
	SYSTEM_STATE_NORMAL,
	SYSTEM_STATE_DEBUG_IDLE,
	SYSTEM_STATE_SELF_TEST,
	SYSTEM_STATE_PID_CAL,
} SystemState;

typedef enum {
	SYSTEM_EVENT_ENTER_DEBUG,
	SYSTEM_EVENT_EXIT_DEBUG,
	SYSTEM_EVENT_SELF_TEST,
	SYSTEM_EVENT_CALIBRATE_PID,
	SYSTEM_EVENT_DONE, /* self-test / pid-cal completed -> back to DEBUG_IDLE */
} SystemEvent;

void system_state_init(void);

void system_state_run(void);

SystemState system_state_get(void);

void system_state_post_event(SystemEvent event);

#endif /* SYSTEM_STATE_H */
