#include <switch.h>
#include <switch_xml.h>
#define PLIVO_HELPER_H 1


/* Return 1 if filepath ending with os separator else 0 */
int eventpipe_filepath_end_with_sep(char *filepath);

/* Lstrip a string */
char *eventpipe_lstrip(char *in, char c);

/* Get substring from first character s to last character e 
   If lstrip is not 0, lstrip this character
*/
char *eventpipe_get_substring(const char *in, char s, char e, char lstrip);

/* Check and validate max eventpipe app run by a call */
int eventpipe_check_count(switch_core_session_t *session);

/* Answer helper */
switch_status_t eventpipe_answer(switch_core_session_t *session);

/* Api helper */
switch_status_t eventpipe_execute_api(switch_core_session_t *session, char *cmd, char *args);

