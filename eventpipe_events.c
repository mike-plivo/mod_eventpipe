#include "eventpipe.h"

static struct globals_t globals;

static struct eventpipe_call *head;


void eventpipe_events_track_call(switch_core_session_t *session) {
	struct eventpipe_call *call;
	struct eventpipe_call *current = NULL;

	switch_assert(session != NULL);

	if (!(call = switch_core_session_alloc(session, sizeof(*call)))) {
                switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Memory Error\n");
                return;
        }

	call->session = session;	
	call->next = NULL;

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, 
				"Tracking call events\n");
	switch_mutex_lock(globals.eventpipe_call_list_mutex);

	if (!head) {
		head = call;
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, 
			"Track call events (first one)\n");
	} else {
		current = head;
		call->next = current;
		head = call;
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, 
			"Track call events\n");
	}
	switch_mutex_unlock(globals.eventpipe_call_list_mutex);
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, 
				"Tracking call events done\n");
}

void eventpipe_events_untrack_call(switch_core_session_t *session) {
	struct eventpipe_call *current, *next, *last;
	next = NULL;
	last = NULL;
	current = NULL;

	switch_assert(session != NULL);

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, 
				"Untracking call events\n");
	switch_mutex_lock(globals.eventpipe_call_list_mutex);

	if (head) {
		if (head->session == session) {
			head = NULL;
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_NOTICE, "No more call to track\n");
			goto eventpipe_remove_call_end;
		}
	} else {
		goto eventpipe_remove_call_end;
	}
	current = head;
	last = head;
	while (current) {
		if (current->session == session) {
			next = current->next;
			last->next = next;
			goto eventpipe_remove_call_end;
		} else {
			current = current->next;
		}
		last = current;
	}
eventpipe_remove_call_end:
	switch_mutex_unlock(globals.eventpipe_call_list_mutex);
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, 
				"Untracking call events done\n");
}

void eventpipe_events_on_dtmf(switch_core_session_t *session, switch_event_t *event) {
	char args[8192];
	const char *dtmf_digit = switch_str_nil(switch_event_get_header(event, "dtmf-digit"));
	switch_channel_t *channel = switch_core_session_get_channel(session);
	const char *hangup_on_star = switch_str_nil(switch_channel_get_variable(channel, "eventpipe_conference_hangup_on_star"));
	const char *conf_room = switch_str_nil(switch_channel_get_variable(channel, "eventpipe_conference_room"));
	const char *conf_member_id = switch_str_nil(switch_channel_get_variable(channel, "eventpipe_conference_member_id"));

	/* handle conference kick if channel is in conference and digit '*' was pressed */
	if ((!strcmp(hangup_on_star, "true")) && (!strcmp(dtmf_digit, "*")) && (!zstr(conf_room)) && (!zstr(conf_member_id))) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, 
				"Conference, dtmf '*' pressed, kick member %s from room %s\n",
				conf_member_id, conf_room);
		switch_snprintf(args, sizeof(args), "%s kick %s", conf_room, conf_member_id);
		eventpipe_execute_api(session, "conference", args);
	}
}

void eventpipe_events_on_conference(switch_core_session_t *session, switch_event_t *event) {
	char args[8192];
	char record_args[8192];
	const char *action = switch_str_nil(switch_event_get_header(event, "action"));
	const char *member_id = switch_str_nil(switch_event_get_header(event, "member-id"));
	const char *conf_room = switch_str_nil(switch_event_get_header(event, "conference-name"));
	const char *calluuid = switch_core_session_get_uuid(session);
	switch_channel_t *channel = switch_core_session_get_channel(session);
	if (!channel) {
		return;
	}

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, 
				"Event-Name: %s, Event-Subclass: %s, Conference-Name: %s, Action: %s, Member-ID: %s, Unique-ID: %s\n", 
				switch_str_nil(switch_event_get_header(event, "event-name")), 
				switch_str_nil(switch_event_get_header(event, "event-subclass")), 
				conf_room, action, member_id, calluuid);

	if (!strcmp(action, "add-member")) {
		const char *enter_sound = switch_str_nil(switch_channel_get_variable(channel, "eventpipe_conference_enter_sound"));
		const char *record_file = switch_str_nil(switch_channel_get_variable(channel, "eventpipe_conference_record_file"));
		switch_channel_set_variable(channel, "eventpipe_conference_member_id", member_id);
		switch_channel_set_variable(channel, "eventpipe_conference_room", conf_room);
		if (!zstr(enter_sound)) {
			switch_snprintf(args, sizeof(args), "%s play %s async",
					conf_room, enter_sound);
			eventpipe_execute_api(session, "conference", args);
		}
		if (!zstr(record_file)) {
			switch_snprintf(record_args, sizeof(record_args), "%s record %s",
					conf_room, record_file);
			eventpipe_execute_api(session, "conference", record_args);
		}
	} else if (!strcmp(action, "del-member")) {
		const char *exit_sound = switch_str_nil(switch_channel_get_variable(channel, "eventpipe_conference_exit_sound"));
		switch_channel_set_variable(channel, "eventpipe_conference_member_id", "");
		switch_channel_set_variable(channel, "eventpipe_conference_room", "");
		switch_channel_set_variable(channel, "eventpipe_conference_record_file", "");
		if (!zstr(exit_sound)) {
			switch_snprintf(args, sizeof(args), "conference %s play %s async",
					conf_room, exit_sound);
			eventpipe_execute_api(session, "conference", args);
		}
	}
}


void eventpipe_events_handler(switch_event_t *event)
{
	struct eventpipe_call *current = NULL;
	const char *uuid1 = "";
	const char *uuid2 = "";

        switch_assert(event != NULL);

	if ((event->event_id != SWITCH_EVENT_CUSTOM) && (event->event_id != SWITCH_EVENT_DTMF)) {
		return;
	}

	uuid1 = switch_event_get_header(event, "unique-id");
	if (zstr(uuid1)) {
		return;
	}
	switch_mutex_lock(globals.eventpipe_call_list_mutex);

	if (!head) {
		goto eventpipe_events_handler_done;
	}
	if (!head->session) {
		goto eventpipe_events_handler_done;
	}
	current = head;
	while (current) {
		if (current->session) {
			uuid2 = "";
			uuid2 = switch_core_session_get_uuid(current->session);	

			/* check if event unique-id is matching a call uuid in eventpipe */
			if ((uuid2) && (!zstr(uuid2) && (!strcmp(uuid1, uuid2)))) {
				/* conference event case */
				if (event->event_id == SWITCH_EVENT_CUSTOM) {
					const char *event_subclass = switch_str_nil(switch_event_get_header(event, "event-subclass"));
					if (!strcmp(event_subclass, PLIVO_EVENT_CONFERENCE)) {
						eventpipe_events_on_conference(current->session, event);
					}
				/* dtmf event case */
				} else if (event->event_id == SWITCH_EVENT_DTMF) {
					eventpipe_events_on_dtmf(current->session, event);
				}
				goto eventpipe_events_handler_done;
			}
		}
		current = current->next;
	}
eventpipe_events_handler_done:
	switch_mutex_unlock(globals.eventpipe_call_list_mutex);
}


int eventpipe_events_load(const char *modname, switch_memory_pool_t *pool) {
        memset(&globals, 0, sizeof(globals));
	head = NULL;

        switch_mutex_init(&globals.eventpipe_call_list_mutex, SWITCH_MUTEX_NESTED, pool);

        if (switch_event_bind_removable(modname, SWITCH_EVENT_ALL, SWITCH_EVENT_SUBCLASS_ANY,
                                        eventpipe_events_handler, NULL, &globals.node) != SWITCH_STATUS_SUCCESS) {
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
                return SWITCH_STATUS_GENERR;
        }
	return SWITCH_STATUS_SUCCESS;
}


void eventpipe_events_unload() {
	if (&globals.node) {
		switch_event_unbind(&globals.node);
	}
}

