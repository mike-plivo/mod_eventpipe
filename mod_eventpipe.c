#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "eventpipe.h"

#define BUFF_SIZE 4096

SWITCH_MODULE_LOAD_FUNCTION(mod_eventpipe_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_eventpipe_shutdown);
SWITCH_MODULE_DEFINITION(mod_eventpipe, mod_eventpipe_load, mod_eventpipe_shutdown, NULL);



SWITCH_STANDARD_APP(eventpipe_function)
{
	int fromfs[2];
	int tofs[2];
	int pid;
	int status;
	char buffer[BUFF_SIZE];
	char buff[2];
	switch_memory_pool_t *pool = NULL;
	switch_channel_t *channel = switch_core_session_get_channel(session);
	const char *eventpipe_flag = switch_channel_get_variable(channel, "eventpipe_app");
	const char *eventpipe_script = switch_channel_get_variable(channel, "eventpipe_script");

	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "Eventpipe application started\n");

	/* create memory pool */
	pool = switch_core_session_get_pool(session);

	/*
	   If eventpipe_app not set to 'true', assume its the first time
	   this call enters eventpipe application and
	   set eventpipe_app=true and _eventpipe_run_count_=0 
	*/
	if ((zstr(eventpipe_flag)) || (strcmp(eventpipe_flag, "true"))) {
		switch_channel_set_variable(channel, "eventpipe_app", "true");
		switch_channel_set_variable(channel, "eventpipe_run_count", "0");
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "eventpipe_app set\n");
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Eventpipe is running for the first time for this call\n");
	/* else if already set, check if reach max redirect else increment redirect count */
	} else {
		if (!eventpipe_check_count(session)) {
			goto eventpipe_nok;
		}
	}
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "Eventpipe script %s\n", eventpipe_script);

	if (pipe(fromfs)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Eventpipe 'fromfs' pipe error\n");
		goto eventpipe_nok;;
	}
	if (pipe(tofs)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Eventpipe 'tofs' pipe error\n");
		goto eventpipe_nok;;
	}

	/* Append call to eventpipe call list */
	eventpipe_events_track_call(session);

	/* Run Eventpipe */
	pid = fork();
	if (pid < 0) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Eventpipe fork failure\n");
		goto eventpipe_nok;
	}
	/* child process */
	if (!pid) {
		/*dup2(fd[0], STDIN_FILENO);
		dup2(fd[1], STDOUT_FILENO);*/
		dup2(fromfs[0], STDIN_FILENO);
		close(fromfs[1]);
		dup2(tofs[1], STDOUT_FILENO);
		close(tofs[0]);
		setvbuf(stdout, (char*)NULL, _IONBF, 0);
		execl(eventpipe_script, (char *)NULL);
		exit(0);
	}

	/* parent child */
	dup2(tofs[0], STDIN_FILENO);
	close(tofs[1]);
	dup2(fromfs[1], STDOUT_FILENO);
	close(fromfs[0]);
	setvbuf(stdout, (char*)NULL, _IONBF, 0);
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "Eventpipe wait pid end\n");
	printf("connected\n");

	/* TODO this is where we need to get events (queue) from event_handler
		and push them to child
	*/

	/* TODO this is where we need to get commands from child */
	/* get one line from child */
	memset(buffer, 0, sizeof(buffer));
	while (read(tofs[0], buff, 1) != 0) {
		switch_snprintf(buffer, sizeof(buffer), "%s%c", buffer, buff[0]);
		if (buff[0] == '\n') {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "End of line %s\n", buffer);
			break;
		}
	}

	/* wait end of child */
	wait(&status);	
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "Eventpipe wait pid done (%d)\n", status);

	//eventpipe_run(session);

	/* Remove call from eventpipe call list */
	eventpipe_events_untrack_call(session);

	/* Eventpipe application ending here */
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "Eventpipe application ended\n");
	fflush(stdout);
        if (!session && pool) {
                switch_core_destroy_memory_pool(&pool);
        }
	return;	

eventpipe_nok:
	eventpipe_events_untrack_call(session);
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Eventpipe application failed\n");
	fflush(stdout);
        if (!session && pool) {
                switch_core_destroy_memory_pool(&pool);
        }
	return;	
}




SWITCH_MODULE_LOAD_FUNCTION(mod_eventpipe_load)
{
	switch_application_interface_t *app_interface;

	if (eventpipe_events_load(modname, pool) != SWITCH_STATUS_SUCCESS) {
		return SWITCH_STATUS_GENERR;
	}

	*module_interface = switch_loadable_module_create_module_interface(pool, modname);
	SWITCH_ADD_APP(app_interface, "eventpipe", "Eventpipe", "Eventpipe Application", eventpipe_function, "", SAF_NONE);

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

/*
  Called when the system shuts down
  Macro expands to: switch_status_t mod_eventpipe_shutdown() */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_eventpipe_shutdown)
{
	eventpipe_events_unload();
	return SWITCH_STATUS_SUCCESS;
}

