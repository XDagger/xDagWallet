//
//  xdag_wrapper.c
//  xDagWallet
//
//  Created by Rui Xie on 7/12/18.
//  Copyright © 2018 xrdavies. All rights reserved.
//

#include "xdag_wrapper.h"
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "commands.h"

#if !defined(NDBUG)
#define assert(p)    if(!(p)){fprintf(stderr,\
"Assertion failed: %s, file %s, line %d\n",\
#p, __FILE__, __LINE__);abort();}
#else
#define assert(p)
#endif


xdag_log_callback_t g_wrapper_log_callback = NULL;
xdag_event_callback_t g_wrapper_event_callback = NULL;
static void* g_thisObj = NULL;

int xdag_set_log_callback(xdag_log_callback_t callback)
{
	g_wrapper_log_callback = callback;
	return 0;
}

int xdag_set_event_callback(xdag_event_callback_t callback)
{
	g_wrapper_event_callback = callback;
	return 0;
}

int xdag_set_password_callback(xdag_password_callback_t callback)
{
	return xdag_user_crypt_action((uint32_t *)(void *)callback, 0, 0, 6);
}

int xdag_wrapper_init(void* thisObj, xdag_password_callback_t password, xdag_event_callback_t event, xdag_log_callback_t log)
{
	if(thisObj) g_thisObj = thisObj;
	if(password) xdag_set_password_callback(password);
	if(event) xdag_set_event_callback(event);
	if(log) xdag_set_log_callback(log);

	return 0;
}

int xdag_wrapper_xfer(const char *amount, const char *to)
{
	return xdag_do_xfer(NULL, amount, to, 0);
}

int xdag_wrapper_log(int level, xdag_error_no err, char *data)
{
	xdag_wrapper_event(event_id_log, err, data);

	return 0;
}

int xdag_wrapper_interact(char *data)
{
	xdag_wrapper_event(event_id_interact, 8, data);

	return 0;
}

int xdag_wrapper_event(xdag_event_id event_id, xdag_error_no err, char *data)
{
	if(!g_wrapper_event_callback) {
		assert(0);
	} else {
		xdag_event *evt = calloc(1, sizeof(xdag_event));
		evt->event_id = event_id;
		evt->error_no = err;
		evt->event_data = strdup(data);

		(*g_wrapper_event_callback)(g_thisObj, evt);

		if(evt->event_data) {
			free(evt->event_data);
		}
		free(evt);
	}

	return 0;
}



