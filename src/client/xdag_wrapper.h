//
//  xdag_wrapper.h
//  xDagWallet
//
//  Created by Rui Xie on 7/12/18.
//  Copyright © 2018 xrdavies. All rights reserved.
//

#ifndef xdag_wrapper_h
#define xdag_wrapper_h

#include <stdio.h>
#include "errno.h"
#include "events.h"

typedef int(*xdag_log_callback_t)(int, xdag_error_no, char *);
typedef int(*xdag_event_callback_t)(void *, xdag_event *) ;
typedef int(*xdag_password_callback_t)(const char *prompt, char *buf, unsigned size);

extern int xdag_wrapper_init(void* thisObj, xdag_password_callback_t password, xdag_event_callback_t event, xdag_log_callback_t log);
extern int xdag_wrapper_xfer(const char *amount, const char *to);


#endif /* xdag_wrapper_h */
