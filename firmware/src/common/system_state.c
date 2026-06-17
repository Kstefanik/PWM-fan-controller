#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/smf.h>

#include "system_state.h"

LOG_MODULE_REGISTER(system_state, LOG_LEVEL_INF);

#define EVENT_NEW_TRANSITION BIT(0)

struct system_state_object {
	struct smf_ctx ctx;
	struct k_mutex lock;
	struct k_event smf_event;
	SystemEvent pending_event;
	bool has_pending;
};

static struct system_state_object s_obj;

static void normal_entry(void *o);
static enum smf_state_result normal_run(void *o);

static void debug_idle_entry(void *o);
static enum smf_state_result debug_idle_run(void *o);

static void self_test_entry(void *o);
static enum smf_state_result self_test_run(void *o);

static void pid_cal_entry(void *o);
static enum smf_state_result pid_cal_run(void *o);

enum system_smf_state {
	STATE_NORMAL,
	STATE_DEBUG_IDLE,
	STATE_SELF_TEST,
	STATE_PID_CAL,
};

static const struct smf_state system_states[] = {
	[STATE_NORMAL] = SMF_CREATE_STATE(normal_entry, normal_run, NULL, NULL, NULL),
	[STATE_DEBUG_IDLE] = SMF_CREATE_STATE(debug_idle_entry, debug_idle_run, NULL, NULL, NULL),
	[STATE_SELF_TEST] = SMF_CREATE_STATE(self_test_entry, self_test_run, NULL, NULL, NULL),
	[STATE_PID_CAL] = SMF_CREATE_STATE(pid_cal_entry, pid_cal_run, NULL, NULL, NULL),
};

static SystemState smf_to_system_state(enum system_smf_state s)
{
	switch (s) {
	case STATE_NORMAL:
		return SYSTEM_STATE_NORMAL;
	case STATE_DEBUG_IDLE:
		return SYSTEM_STATE_DEBUG_IDLE;
	case STATE_SELF_TEST:
		return SYSTEM_STATE_SELF_TEST;
	case STATE_PID_CAL:
		return SYSTEM_STATE_PID_CAL;
	default:
		return SYSTEM_STATE_NORMAL;
	}
}

static void normal_entry(void *o)
{
	LOG_INF("State -> NORMAL");
}

static enum smf_state_result normal_run(void *o)
{
	struct system_state_object *s = o;

	if (s->pending_event == SYSTEM_EVENT_ENTER_DEBUG) {
		smf_set_state(SMF_CTX(s), &system_states[STATE_DEBUG_IDLE]);
	} else {
		LOG_WRN("Event %d invalid in NORMAL", s->pending_event);
	}

	return SMF_EVENT_HANDLED;
}

static void debug_idle_entry(void *o)
{
	LOG_INF("State -> DEBUG_IDLE");
}

static enum smf_state_result debug_idle_run(void *o)
{
	struct system_state_object *s = o;

	switch (s->pending_event) {
	case SYSTEM_EVENT_EXIT_DEBUG:
		smf_set_state(SMF_CTX(s), &system_states[STATE_NORMAL]);
		break;
	case SYSTEM_EVENT_SELF_TEST:
		smf_set_state(SMF_CTX(s), &system_states[STATE_SELF_TEST]);
		break;
	case SYSTEM_EVENT_CALIBRATE_PID:
		smf_set_state(SMF_CTX(s), &system_states[STATE_PID_CAL]);
		break;
	default:
		LOG_WRN("Event %d invalid in DEBUG_IDLE", s->pending_event);
		break;
	}

	return SMF_EVENT_HANDLED;
}

static void self_test_entry(void *o)
{
	LOG_INF("State -> SELF_TEST");
}

static enum smf_state_result self_test_run(void *o)
{
	struct system_state_object *s = o;

	switch (s->pending_event) {
	case SYSTEM_EVENT_DONE:
	case SYSTEM_EVENT_EXIT_DEBUG:
		smf_set_state(SMF_CTX(s), &system_states[STATE_DEBUG_IDLE]);
		break;
	default:
		LOG_WRN("Event %d invalid in SELF_TEST", s->pending_event);
		break;
	}

	return SMF_EVENT_HANDLED;
}

static void pid_cal_entry(void *o)
{
	LOG_INF("State -> PID_CAL");
}

static enum smf_state_result pid_cal_run(void *o)
{
	struct system_state_object *s = o;

	switch (s->pending_event) {
	case SYSTEM_EVENT_DONE:
	case SYSTEM_EVENT_EXIT_DEBUG:
		smf_set_state(SMF_CTX(s), &system_states[STATE_DEBUG_IDLE]);
		break;
	default:
		LOG_WRN("Event %d invalid in PID_CAL", s->pending_event);
		break;
	}

	return SMF_EVENT_HANDLED;
}

void system_state_init(void)
{
	k_mutex_init(&s_obj.lock);
	k_event_init(&s_obj.smf_event);
	s_obj.has_pending = false;

	smf_set_initial(SMF_CTX(&s_obj), &system_states[STATE_NORMAL]);
}

void system_state_run(void)
{
	while (1) {
		k_event_wait(&s_obj.smf_event, EVENT_NEW_TRANSITION, true, K_FOREVER);

		k_mutex_lock(&s_obj.lock, K_FOREVER);
		int32_t ret = smf_run_state(SMF_CTX(&s_obj));
		s_obj.has_pending = false;
		k_mutex_unlock(&s_obj.lock);

		if (ret != 0) {
			LOG_ERR("State machine terminated unexpectedly: %d", ret);
			return;
		}
	}
}

SystemState system_state_get(void)
{
	const struct smf_state *leaf;
	enum system_smf_state idx;

	k_mutex_lock(&s_obj.lock, K_FOREVER);
	leaf = smf_get_current_leaf_state(SMF_CTX(&s_obj));
	k_mutex_unlock(&s_obj.lock);

	idx = (enum system_smf_state)(leaf - system_states);

	return smf_to_system_state(idx);
}

void system_state_post_event(SystemEvent event)
{
	k_mutex_lock(&s_obj.lock, K_FOREVER);
	s_obj.pending_event = event;
	s_obj.has_pending = true;
	k_mutex_unlock(&s_obj.lock);

	k_event_post(&s_obj.smf_event, EVENT_NEW_TRANSITION);
}

static void system_state_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	system_state_init();
	system_state_run();
}

K_THREAD_DEFINE(system_state_id, CONFIG_SYSTEM_STATE_STACK_SIZE, system_state_thread_entry, NULL,
		NULL, NULL, CONFIG_SYSTEM_STATE_PRIORITY, 0, 0);
