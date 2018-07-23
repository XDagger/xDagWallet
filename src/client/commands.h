#ifndef XDAG_COMMANDS_H
#define XDAG_COMMANDS_H

#include <time.h>
#include "block.h"

#define XDAG_COMMAND_MAX	0x1000

#ifdef __cplusplus
extern "C" {
#endif

#define XFER_MAX_IN		11
	extern void processAccountCommand(int count, char **out);
	extern void processBalanceCommand(char *address, char **out);
	extern void processLevelCommand(char *level, char **out);
	extern void processXferCommand(char *address, char *amount, char **out);
	extern void processStateCommand(char **out);
	extern void processExitCommand(void);
	extern void processHelpCommand(char **out);

	extern int xdag_do_xfer(const char *amount, const char *address, char **out);

	extern void xdag_log_xfer(xdag_hash_t from, xdag_hash_t to, xdag_amount_t amount);
#ifdef __cplusplus
};
#endif

#endif // !XDAG_COMMANDS_H
