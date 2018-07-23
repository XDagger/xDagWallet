#include "commands.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"
#include "address.h"
#include "wallet.h"
#include "utils/log.h"
#include "client.h"
#include "crypt.h"
#include "client.h"
#include "storage.h"
#include "errno.h"
#if !defined(_WIN32) && !defined(_WIN64)
#include "utils/linenoise.h"
#endif

#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#endif

#define Nfields(d) (2 + d->fieldsCount + 3 * d->keysCount + 2 * d->outsig)
#define COMMAND_HISTORY ".cmd.history"

struct account_callback_data {
	char out[128];
	int count;
};

typedef int (*xdag_com_func_t)(char*, FILE *);
typedef struct {
	char *name;				/* command name */
	xdag_com_func_t func;	/* command function */
} XDAG_COMMAND;

// Function declarations
int account_callback(void *data, xdag_hash_t hash, xdag_amount_t amount, xdag_time_t time, int n_our_key);
const char *get_state(void);
int read_command(char* cmd);
int xdag_command(char *cmd, FILE *out);

void processAccountCommand(int count, char **out);
void processBalanceCommand(char *address, char **out);
void processLevelCommand(char *level, char **out);
void processExitCommand(void);
void processXferCommand(char *address, char *amount, char **out);
void processHelpCommand(FILE *out);

int xdag_com_account(char *, FILE*);
int xdag_com_balance(char *, FILE*);
int xdag_com_level(char *, FILE*);
int xdag_com_xfer(char *, FILE*);
int xdag_com_stats(char *, FILE*);
int xdag_com_state(char *, FILE*);
int xdag_com_help(char *, FILE*);
int xdag_com_terminate(char *, FILE*);
int xdag_com_exit(char *, FILE*);

XDAG_COMMAND* find_xdag_command(char*);

XDAG_COMMAND commands[] = {
	{ "account"    , xdag_com_account },
	{ "balance"    , xdag_com_balance },
	{ "level"      , xdag_com_level },
	{ "xfer"       , xdag_com_xfer },
	{ "state"      , xdag_com_state },
	{ "exit"       , xdag_com_exit },
	{ "xfer"       , (xdag_com_func_t)NULL},
	{ "help"       , xdag_com_help},
	{ (char *)NULL , (xdag_com_func_t)NULL}
};

int xdag_com_account(char *args, FILE* out)
{
	int count = 1;
	char *result = NULL;
	char *cmd = strtok_r(args, " \t\r\n", &args);
	if(cmd) {
		sscanf(cmd, "%d", &count);
	}

	processAccountCommand(count, &result);

	if(result) {
		fprintf(out, "%s", result);
		free(result);
	}

	return 0;
}

int xdag_com_balance(char *args, FILE* out)
{
	char *address = strtok_r(args, " \t\r\n", &args);
	char *result = NULL;

	processBalanceCommand(address, &result);

	if(result) {
		fprintf(out, "%s", result);
		free(result);
	}

	return 0;
}

int xdag_com_xfer(char *args, FILE* out)
{
	char *result = NULL;
	char *amount = strtok_r(args, " \t\r\n", &args);
	char *address = strtok_r(0, " \t\r\n", &args);

	processXferCommand(amount, address, &result);

	if(result) {
		fprintf(out, "%s", result);
		free(result);
	}

	return 0;
}

int xdag_com_level(char *args, FILE* out)
{
	char *cmd = strtok_r(args, " \t\r\n", &args);
	char *result = NULL;
	processLevelCommand(cmd, &result);

	if(result) {
		fprintf(out, "%s", result);
		free(result);
	}
	return 0;
}

int xdag_com_state(char *args, FILE* out)
{
	fprintf(out, "%s\n", get_state());
	return 0;
}

int xdag_com_exit(char * args, FILE* out)
{
	processExitCommand();
	return -1;
}

int xdag_com_help(char *args, FILE* out)
{
	processHelpCommand(out);
	return 0;
}

XDAG_COMMAND* find_xdag_command(char *name)
{
	for(int i = 0; commands[i].name; i++) {
		if(strcmp(name, commands[i].name) == 0) {
			return (&commands[i]);
		}
	}
	return (XDAG_COMMAND *)NULL;
}

void startCommandProcessing(void)
{
	char cmd[XDAG_COMMAND_MAX];
	printf("Type command, help for example.\n");

	for(;;) {
		read_command(cmd);
		if(strlen(cmd) > 0) {
			int ret = xdag_command(cmd, stdout);
			if(ret < 0) {
				break;
			}
		}
	}
}

int xdag_command(char *cmd, FILE *out)
{
	char *nextParam;

	cmd = strtok_r(cmd, " \t\r\n", &nextParam);
	if(!cmd) return 0;

	XDAG_COMMAND *command = find_xdag_command(cmd);

	if(!command) {
		fprintf(out, "Illegal command.\n");
	} else {
		return (*(command->func))(nextParam, out);
	}
	return 0;
}

int account_callback(void *data, xdag_hash_t hash, xdag_amount_t amount, xdag_time_t time, int n_our_key)
{
	char address[33];
	struct account_callback_data *d = (struct account_callback_data *)data;
	if(!d->count--) {
		return -1;
	}
	xdag_hash2address(hash, address);

	if(g_xdag_state < XDAG_STATE_XFER)
		sprintf(d->out, "%s  key %d\n", address, n_our_key);
	else
		sprintf(d->out, "%s %20.9Lf  key %d\n", address, amount2xdags(amount), n_our_key);
	return 0;
}

void processAccountCommand(int count, char **out)
{
	struct account_callback_data d;
	d.count = count;

	char tmp[128] = {0};
	if(g_xdag_state < XDAG_STATE_XFER) {
		sprintf(tmp, "Not ready to show balances. Type 'state' command to see the reason.\n");
	}
	xdag_traverse_our_blocks(&d, &account_callback);

	*out = strdup(strcat(tmp, d.out));
}

void processBalanceCommand(char *address, char **out)
{
	if(g_xdag_state < XDAG_STATE_XFER) {
		*out = strdup("Not ready to show a balance. Type 'state' command to see the reason.\n");
	} else {
		xdag_hash_t hash;
		xdag_amount_t balance;
		if(address) {
			xdag_address2hash(address, hash);
			balance = xdag_get_balance(hash);
		} else {
			balance = xdag_get_balance(0);
		}
		char result[128] = {0};
		sprintf(result, "Balance: %.9Lf\n", amount2xdags(balance));
		*out = strdup(result);
	}
}

void processLevelCommand(char *level, char **out)
{
	unsigned lv;
	if(!level) {
		char tmp[16];
		sprintf(tmp, "%d\n", xdag_set_log_level(-1));
		*out = strdup(tmp);
	} else if(sscanf(level, "%u", &lv) != 1 || lv > XDAG_TRACE) {
		*out = strdup("Illegal level.\n");
	} else {
		xdag_set_log_level(lv);
	}
}

void processXferCommand(char *amount, char *address, char **out)
{
	if(!amount) {
		*out = strdup("Xfer: amount not given.\n");
		return;
	}
	if(!address) {
		*out = strdup("Xfer: destination address not given.\n");
		return;
	}
	if(xdag_user_crypt_action(0, 0, 0, 3)) {
		sleep(3);
		*out = strdup("Password incorrect.\n");
	} else {
		xdag_do_xfer(amount, address, out);
	}
}

const char *get_state()
{
	static const char *states[] = {
#define xdag_state(n,s) s ,
#include "state.h"
#undef xdag_state
	};
	return states[g_xdag_state];
}

static int make_transaction_block(struct xfer_callback_data *xferData)
{
	char address[33];
	if(xferData->fieldsCount != XFER_MAX_IN) {
		memcpy(xferData->fields + xferData->fieldsCount, xferData->fields + XFER_MAX_IN, sizeof(xdag_hashlow_t));
	}
	xferData->fields[xferData->fieldsCount].amount = xferData->todo;
	int res = xdag_create_block(xferData->fields, xferData->fieldsCount, 1, 0, 0, xferData->transactionBlockHash);
	if(res) {
		xdag_hash2address(xferData->fields[xferData->fieldsCount].hash, address);
		xdag_err(error_block_create, "FAILED: to %s xfer %.9Lf %s, error %d",
			address, amount2xdags(xferData->todo), COINNAME, res);
		return -1;
	}
	xferData->done += xferData->todo;
	xferData->todo = 0;
	xferData->fieldsCount = 0;
	xferData->keysCount = 0;
	xferData->outsig = 1;
	return 0;
}

int xdag_do_xfer(const char *amount, const char *address, char **out)
{
	char address_buf[33];
	char result[256] = {0};
	struct xfer_callback_data xfer;

	memset(&xfer, 0, sizeof(xfer));
	xfer.remains = xdags2amount(amount);
	if(!xfer.remains) {
		*out = strdup("Xfer: nothing to transfer.\n");
		return 1;
	}
	if(xfer.remains > xdag_get_balance(0)) {
		*out = strdup("Xfer: balance too small.\n");
		return 1;
	}
	if(xdag_address2hash(address, xfer.fields[XFER_MAX_IN].hash)) {
		*out = strdup("Xfer: incorrect address.\n");
		return 1;
	}
	xdag_wallet_default_key(&xfer.keys[XFER_MAX_IN]);
	xfer.outsig = 1;
	g_xdag_state = XDAG_STATE_XFER;
	g_xdag_xfer_last = time(0);
	xdag_traverse_our_blocks(&xfer, &xfer_callback);
//	xdag_hash2address(xfer.fields[XFER_MAX_IN].hash, address_buf);
	xdag_hash2address(xfer.transactionBlockHash, address_buf);
	sprintf(result, "Xfer: transferred %.9Lf %s to the address %s\nTx block address is %s\n", amount2xdags(xfer.done), COINNAME, address, address_buf);

	*out = strdup(result);
	return 0;
}

int xfer_callback(void *data, xdag_hash_t hash, xdag_amount_t amount, xdag_time_t time, int n_our_key)
{
	struct xfer_callback_data *xferData = (struct xfer_callback_data*)data;
	xdag_amount_t todo = xferData->remains;
	int i;
	if(!amount) {
		return -1;
	}
	for(i = 0; i < xferData->keysCount; ++i) {
		if(n_our_key == xferData->keys[i]) {
			break;
		}
	}
	if(i == xferData->keysCount) {
		xferData->keys[xferData->keysCount++] = n_our_key;
	}
	if(xferData->keys[XFER_MAX_IN] == n_our_key) {
		xferData->outsig = 0;
	}
	if(Nfields(xferData) > XDAG_BLOCK_FIELDS) {
		if(make_transaction_block(xferData)) {
			return -1;
		}
		xferData->keys[xferData->keysCount++] = n_our_key;
		if(xferData->keys[XFER_MAX_IN] == n_our_key) {
			xferData->outsig = 0;
		}
	}
	if(amount < todo) {
		todo = amount;
	}
	memcpy(xferData->fields + xferData->fieldsCount, hash, sizeof(xdag_hashlow_t));
	xferData->fields[xferData->fieldsCount++].amount = todo;
	xferData->todo += todo;
	xferData->remains -= todo;
	xdag_log_xfer(hash, xferData->fields[XFER_MAX_IN].hash, todo);
	if(!xferData->remains || Nfields(xferData) == XDAG_BLOCK_FIELDS) {
		if(make_transaction_block(xferData)) {
			return -1;
		}
		if(!xferData->remains) {
			return 1;
		}
	}
	return 0;
}

void xdag_log_xfer(xdag_hash_t from, xdag_hash_t to, xdag_amount_t amount)
{
	char address_from[33], address_to[33];
	xdag_hash2address(from, address_from);
	xdag_hash2address(to, address_to);
	xdag_mess("Xfer : from %s to %s xfer %.9Lf %s", address_from, address_to, amount2xdags(amount), COINNAME);
}

void processExitCommand()
{
	xdag_wallet_finish();
	xdag_storage_finish();
}

void processHelpCommand(FILE *out)
{
	fprintf(out, "Commands:\n"
		"  account [N]         - print first N (20 by default) our addresses with their amounts\n"
		"  balance [A]         - print balance of the address A or total balance for all our addresses\n"
		"  exit                - exit this program (not the daemon)\n"
		"  help                - print this help\n"
		"  keygen              - generate new private/public key pair and set it by default\n"
		"  level [N]           - print level of logging or set it to N (0 - nothing, ..., 9 - all)\n"
		"  state               - print the program state\n"
		"  terminate           - terminate both daemon and this program\n"
		"  xfer S A            - transfer S our %s to the address A\n"
		, COINNAME);
}

int read_command(char *cmd)
{
	printf("xdag> ");
	fflush(stdout);
	fgets(cmd, XDAG_COMMAND_MAX, stdin);
	return 0;
}
