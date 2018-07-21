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
	event_id_interact,
	event_id_xfer_done,
	event_id_passwd,
	event_id_passwd_again,
	event_id_random_key,

	// no error
	event_id_error_none						= 0x1000,

	// init errors
	event_id_error_pwd_inconsistent			= 0x1001,
	event_id_error_pwd_incorrect			= 0x1002,
	event_id_error_create_dnet_key_failed	= 0x1003,
	event_id_error_write_wallet_key_failed	= 0x1004,
	event_id_error_add_wallet_key_failed	= 0x1005,
	event_id_error_init_task_failed			= 0x1006,
	event_id_error_init_crypt_failed		= 0x1007,
	event_id_error_missing_param			= 0x1008,

	// storage
	event_id_error_storage_load_faild		= 0x2001,
	event_id_error_storage_sum_corrupted	= 0x2002,
	event_id_error_storage_create_file		= 0x2003,
	event_id_error_storage_write_file		= 0x2004,
	event_id_error_storage_corrupted			= 0x2005,

	// xfer
	event_id_error_xfer_nothing				= 0x3001,
	event_id_error_xfer_too_small			= 0x3002,
	event_id_error_xfer_address				= 0x3003,

	// block
	event_id_error_block_create				= 0x4001,
	event_id_error_block_not_found			= 0x4002,
	event_id_error_block_load_failed			= 0x4003,

	// network
	event_id_error_socket_create			= 0x5001,
	event_id_error_socket_host				= 0x5002,
	event_id_error_socket_port				= 0x5003,
	event_id_error_socket_connect			= 0x5004,
	event_id_error_socket_hangup			= 0x5005,
	event_id_error_socket_err				= 0x5006,
	event_id_error_socket_read				= 0x5007,
	event_id_error_socket_write				= 0x5008,
	event_id_error_socket_timeout			= 0x5009,
	event_id_error_socket_resolve_host		= 0x5010,
	event_id_error_socket_closed			= 0x5011,

	// general errors
	event_id_error_unknown					= 0x0001,
	event_id_error_malloc					= 0x0002,
	event_id_error_fatal					= 0x0003,
} xdag_event_id;

typedef struct {
	xdag_event_id event_id;
	xdag_error_no error_no;
	char *event_data;
} xdag_event;
#endif /* events_h */
