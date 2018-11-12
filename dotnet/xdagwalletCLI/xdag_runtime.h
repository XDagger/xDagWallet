#pragma once

#pragma unmanaged

#ifndef XDAG_RUNTIME_HEADER_H
#define XDAG_RUNTIME_HEADER_H

#include "stdafx.h"

#define _TIMESPEC_DEFINED


// This is to disable security warning from Visual C++
//// #define _CRT_SECURE_NO_WARNINGS

#include "../../core/client/init.h"
#include "../../core/client/common.h"
#include "../../core/client/commands.h"
#include "../../core/client/client.h"
#include "../../core/client/events.h"
#include "../../core/client/utils/utils.h"
#include "../../core/client/address.h"
#include "../../core/client/dnet_crypt.h"
#include "../../core/client/xdag_wrapper.h"

////---- Duplicated from dnet_crypt.c ----
#define KEYFILE	    "dnet_key.dat"
struct dnet_keys {
	struct dnet_key priv;
	struct dnet_key pub;
};
////------------------------------------


////---- Duplicated from commands.c ----
struct account_callback_data {
	char out[128];
	int count;
};

struct xfer_callback_data {
	struct xdag_field fields[XFER_MAX_IN + 1];
	int keys[XFER_MAX_IN + 1];
	xdag_amount_t todo, done, remains;
	int fieldsCount, keysCount, outsig;
	xdag_hash_t transactionBlockHash;
};
////------------------------------------

int xdag_event_callback(void* thisObj, xdag_event *event);


////---- Exporting functions ----
NATIVE_LIB_EXPORT int xdag_init_wrap(int argc, char **argv, int isGui);
NATIVE_LIB_EXPORT int xdag_set_password_callback_wrap(int(*callback)(const char *prompt, char *buf, unsigned size));
NATIVE_LIB_EXPORT int xdag_transfer_wrap(char* toAddress, char* amountString);
NATIVE_LIB_EXPORT bool xdag_is_valid_wallet_address(const char* address);
NATIVE_LIB_EXPORT bool xdag_dnet_crpt_found();


////------------------------------------

////---- Exporting functions wrapping functions ----
int xdag_init_wrap(int argc, char **argv, int isGui)
{
	//// return xdag_init(argc, argv, isGui);

	xdag_init_path(argv[0]);

	const char *pool_arg = 0;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] != '-') {
			if ((!argv[i][1] || argv[i][2]) && strchr(argv[i], ':')) {
				pool_arg = argv[i];
			}
			else {
				return 0;
			}
			continue;
		}
	}

	xdag_set_event_callback(&xdag_event_callback);
	if (xdag_client_init(pool_arg)) return -1;

	return 0;
}

int xdag_event_callback(void* thisObj, xdag_event *event)
{
	if (!event) {
		return -1;
	}

	switch (event->event_id) {
	case event_id_log:
	{
		
		break;
	}

	case event_id_interact:
	{
		
		break;
	}

	//		case event_id_err:
	//		{
	//			fprintf(stdout, "error : %x, msg : %s\n", event->error_no, event->event_data);
	//			fflush(stdout);
	//			break;
	//		}

	case event_id_err_exit:
	{
		
		break;
	}

	case event_id_account_done:
	{
		
		break;
	}

	case event_id_address_done:
	{
		
		break;
	}

	case event_id_balance_done:
	{
		
		break;
	}

	case event_id_xfer_done:
	{
		
		break;
	}

	case event_id_level_done:
	{
		
		break;
	}

	case event_id_state_done:
	{
		
		break;
	}

	case event_id_exit_done:
	{
		
		break;
	}

	case event_id_passwd:
	{
		break;
	}

	case event_id_passwd_again:
	{
		break;
	}

	case event_id_random_key:
	{
		break;
	}

	case event_id_state_change:
	{
		
		break;
	}

	default:
	{
		
		break;
	}
	}
	return 0;
}

int xdag_set_password_callback_wrap(int(*callback)(const char *prompt, char *buf, unsigned size))
{
	//// return xdag_set_password_callback(callback);
	return xdag_user_crypt_action((uint32_t *)(void *)callback, 0, 0, 6);
}

int xdag_transfer_wrap(const char* toAddress, const char* amountString)
{
	char* address = new char[strlen(toAddress) + 1];
	char* amount = new char[strlen(amountString) + 1];

	strcpy_s(address, strlen(toAddress) + 1, toAddress);
	strcpy_s(amount, strlen(amountString) + 1, amountString);

	char *result = NULL;
	int err = processXferCommand(amount, address, &result);

	if (err != error_none) {
		xdag_wrapper_event(event_id_log, (xdag_error_no)err, result);
	}
	else {
		xdag_wrapper_event(event_id_xfer_done, (xdag_error_no)0, result);
	}

	return err;
}

bool xdag_is_valid_wallet_address(const char* address)
{
	struct xfer_callback_data xfer;
	if (xdag_address2hash(address, xfer.fields[XFER_MAX_IN].hash) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool xdag_dnet_crpt_found()
{
	FILE *f = NULL;
	struct dnet_keys *keys = (struct dnet_keys*)malloc(sizeof(struct dnet_keys));

	bool is_found = false;
	f = xdag_open_file(KEYFILE, "rb");

	if (f)
	{
		if (fread(keys, sizeof(struct dnet_keys), 1, f) == 1)
		{
			is_found = true;
		}
		xdag_close_file(f);
	}

	free(keys);
	return is_found;
}

#endif