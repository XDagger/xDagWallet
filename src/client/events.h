//
//  events.h
//  xDagWallet
//
//  Created by Rui Xie on 7/12/18.
//  Copyright © 2018 xrdavies. All rights reserved.
//

#ifndef events_h
#define events_h

#include <stdio.h>
#include "errno.h"

typedef enum {
	event_id_log,
	event_id_xfer_done,
	event_id_passwd,
	event_id_passwd_again,
	event_id_random_key
} xdag_event_id;

typedef struct {
	xdag_event_id event_id;
	xdag_error_no error_no;
	char *event_data;
} xdag_event;
#endif /* events_h */
