#include "eventpipe.h"



/* TODO add an helper function eventpipe_xml_attr2bool */
/* TODO add an helper function eventpipe_xml_attr2str */
/* TODO add an helper for doing http HEAD request */
/* TODO add a urlencode func */


int eventpipe_filepath_end_with_sep(char *filepath) {
        int len = 0;
        int x = 0;
        char *s = NULL;

        if (!filepath) {
                return 0;
        }

        s = strstr(filepath, SWITCH_PATH_SEPARATOR);
        if (!s) {
                return 0;
        }
        s = strdup(filepath);
        len = strlen(filepath) + 1;
        x = strlen(SWITCH_PATH_SEPARATOR);
        s = s+(len-1-x);
        if (!strncmp(s, SWITCH_PATH_SEPARATOR, x)) {
                return 1;
        }
        return 0;
}


char *eventpipe_lstrip(char *in, char c) {
	if (*in == 0) {
		return in;
	}
	while (*in == c) {
		++in;
	}
	return in;
}

char *eventpipe_get_substring(const char *in, char s, char e, char lstrip) {
        int begin, end, len;
        char *sp;
        char *ep;
	char *res;
        sp = strchr(in, s);
        ep = strchr(in, e);
        if ((!sp) || (!ep)) {
                return NULL;
        }
        begin = sp - in + 1;
        end = ep - in;
        if (begin > end) {
                return NULL;
        }
        len = end - begin;
        if (len <= 0) {
                return NULL;
        }
	res = strndup(in + begin, len);
	if (lstrip != 0) {
		res = eventpipe_lstrip(res, lstrip);
	}
	/* must be freed ! */
	return res;
}

int eventpipe_check_count(switch_core_session_t *session) {
	const char *check_str = NULL;
	int check_count = 0;
	char vbuf[10] = "";

	switch_channel_t *channel = switch_core_session_get_channel(session);
	check_str = switch_channel_get_variable(channel, "eventpipe_run_count");

	if (!zstr(check_str)) {
		/* check if reach max count */
		check_count = atoi(check_str);
		if (check_count >= PLIVO_MAX_RUN) {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), 
				SWITCH_LOG_ERROR, "Abort, Max eventpipe applications reached for a call (%d)!\n", PLIVO_MAX_RUN);
			return 0;
		/* increment redirect count */
		} else {
			check_count++;
			switch_snprintf(vbuf, sizeof(vbuf), "%d", check_count);
			switch_channel_set_variable(channel, "eventpipe_run_count", vbuf);
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "eventpipe_run_count %d\n", check_count);
			return 1;
		}
	}
	/* Hu ?! must never be here */
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "Hu ?! must never be here !\n");
	return 0;
}

/* Answer helper */
switch_status_t eventpipe_answer(switch_core_session_t *session) {
	switch_channel_t *channel = switch_core_session_get_channel(session);
	return switch_channel_answer(channel);
} 

/* Api helper */
switch_status_t eventpipe_execute_api(switch_core_session_t *session, char *cmd, char *args) {
	switch_status_t status;
	switch_stream_handle_t stream = { 0 };
	SWITCH_STANDARD_STREAM(stream);

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "api: cmd %s, args %s\n", 
			  cmd, switch_str_nil(args));
	status = switch_api_execute(cmd, args, session, &stream);
	if (status != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, 
				"api: %s(%s) failed\n", cmd, switch_str_nil(args));
	} else {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, 
				"api: %s(%s) %s\n", cmd, switch_str_nil(args), (char *) stream.data);
	}
	free(stream.data);
	return status;
}


