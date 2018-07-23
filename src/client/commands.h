#ifndef XDAG_COMMANDS_H
#define XDAG_COMMANDS_H

#include <time.h>
#include "block.h"

#define XDAG_COMMAND_MAX	0x1000

#ifdef __cplusplus
extern "C" {
#endif

#define XFER_MAX_IN		11

	extern void startCommandProcessing(void);

	struct xfer_callback_data {
		struct xdag_field fields[XFER_MAX_IN + 1];
		int keys[XFER_MAX_IN + 1];
		xdag_amount_t todo, done, remains;
		int fieldsCount, keysCount, outsig;
		xdag_hash_t transactionBlockHash;
	};

	extern int xdag_do_xfer(const char *amount, const char *address, char **out);
	extern int xfer_callback(void *data, xdag_hash_t hash, xdag_amount_t amount, xdag_time_t time, int n_our_key);

	extern void xdag_log_xfer(xdag_hash_t from, xdag_hash_t to, xdag_amount_t amount);
#ifdef __cplusplus
};
#endif

#endif // !XDAG_COMMANDS_H
