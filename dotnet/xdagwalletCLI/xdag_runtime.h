#pragma once

#ifndef XDAG_RUNTIME_HEADER_H
#define XDAG_RUNTIME_HEADER_H

#include "stdafx.h"

#define _TIMESPEC_DEFINED

#include "../xdag/client/init.h"
#include "../xdag/client/commands.h"
#include "../xdag/client/utils/utils.h"
#include "../xdag/client/address.h"
#include "../xdag/dnet/dnet_crypt.h"

////---- Duplicated from dnet_crypt.c ----
#define KEYFILE	    "dnet_key.dat"
struct dnet_keys {
	struct dnet_key priv;
	struct dnet_key pub;
};
////------------------------------------


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
	return xdag_init(argc, argv, isGui);
}

int xdag_set_password_callback_wrap(int(*callback)(const char *prompt, char *buf, unsigned size))
{
	return xdag_set_password_callback(callback);
}

int xdag_transfer_wrap(const char* toAddress, const char* amountString)
{
	char* address = new char[strlen(toAddress) + 1];
	char* amount = new char[strlen(amountString) + 1];

	strcpy_s(address, strlen(toAddress) + 1, toAddress);
	strcpy_s(amount, strlen(amountString) + 1, amountString);

	int result = xdag_do_xfer(0, amount, address, NULL, 1);
	return result;
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