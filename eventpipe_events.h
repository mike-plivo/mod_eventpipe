#include <switch.h>
#include <switch_xml.h>
#define PLIVO_EVENTS_H 1


struct eventpipe_call {
	switch_core_session_t *session;
	struct eventpipe_call *next;
};

struct globals_t {
        switch_mutex_t *eventpipe_call_list_mutex;
        switch_event_node_t *node;
};


void eventpipe_events_track_call(switch_core_session_t *session);

void eventpipe_events_untrack_call(switch_core_session_t *session);

void eventpipe_events_on_conference(switch_core_session_t *session, switch_event_t *event);

void eventpipe_events_handler(switch_event_t *event);

int eventpipe_events_load(const char *modname, switch_memory_pool_t *pool);

void eventpipe_events_unload();
