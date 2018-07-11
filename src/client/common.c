//
//  common.c
//  xDagWallet
//
//  Created by Rui Xie on 7/11/18.
//  Copyright © 2018 xrdavies. All rights reserved.
//

#include "common.h"
#include <math.h>

enum xdag_field_type g_block_header_type = XDAG_FIELD_HEAD;
int g_xdag_state = XDAG_STATE_INIT;
int g_xdag_testnet = 0;
time_t g_xdag_xfer_last = 0;
struct xdag_stats g_xdag_stats;
struct xdag_ext_stats g_xdag_extstats;


xdag_amount_t xdags2amount(const char *str)
{
	long double sum;
	if(sscanf(str, "%Lf", &sum) != 1 || sum <= 0) {
		return 0;
	}
	long double flr = floorl(sum);
	xdag_amount_t res = (xdag_amount_t)flr << 32;
	sum -= flr;
	sum = ldexpl(sum, 32);
	flr = ceill(sum);
	return res + (xdag_amount_t)flr;
}

// convert xdag_amount_t to long double
long double amount2xdags(xdag_amount_t amount)
{
	return xdag_amount2xdag(amount) + (long double)xdag_amount2cheato(amount) / 1000000000;
}

int xdag_set_password_callback(int(*callback)(const char *prompt, char *buf, unsigned size))
{
	return xdag_user_crypt_action((uint32_t *)(void *)callback, 0, 0, 6);
}
