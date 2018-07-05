#include "commands.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include "init.h"
#include "address.h"
#include "wallet.h"
#include "utils/log.h"
#include "miner.h"
//#include "transport.h"
#include "memory.h"
#include "crypt.h"
#include "miner.h"
#include "storage.h"
#if !defined(_WIN32) && !defined(_WIN64)
#include "utils/linenoise.h"
#endif

#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#endif

#define Nfields(d) (2 + d->fieldsCount + 3 * d->keysCount + 2 * d->outsig)
#define COMMAND_HISTORY ".cmd.history"

struct account_callback_data {
	FILE *out;
	int count;
};

typedef int (*xdag_com_func_t)(char*, FILE *);
typedef struct {
	char *name;				/* command name */
	int avaibility;         /* 0 - both miner and pool, 1 - only miner, 2 - only pool */
	xdag_com_func_t func;	/* command function */
} XDAG_COMMAND;

// Function declarations
int account_callback(void *data, xdag_hash_t hash, xdag_amount_t amount, xdag_time_t time, int n_our_key);
//long double hashrate(xdag_diff_t *diff);
const char *get_state(void);

void processAccountCommand(char *nextParam, FILE *out);
void processBalanceCommand(char *nextParam, FILE *out);
void processKeyGenCommand(FILE *out);
void processLevelCommand(char *nextParam, FILE *out);
void processExitCommand(void);
void processXferCommand(char *nextParam, FILE *out, int ispwd, uint32_t* pwd);
void processHelpCommand(FILE *out);

int xdag_com_account(char *, FILE*);
int xdag_com_balance(char *, FILE*);
int xdag_com_keyGen(char *, FILE*);
int xdag_com_level(char *, FILE*);
int xdag_com_stats(char *, FILE*);
int xdag_com_state(char *, FILE*);
int xdag_com_cache(char *, FILE*);
int xdag_com_help(char *, FILE*);
int xdag_com_terminate(char *, FILE*);
int xdag_com_exit(char *, FILE*);

XDAG_COMMAND* find_xdag_command(char*);

XDAG_COMMAND commands[] = {
	{ "account"    , 0, xdag_com_account },
	{ "balance"    , 0, xdag_com_balance },
	{ "keyGen"     , 0, xdag_com_keyGen },
	{ "level"      , 0, xdag_com_level },
	{ "state"      , 0, xdag_com_state },
	{ "terminate"  , 0, xdag_com_terminate },
	{ "exit"       , 0, xdag_com_exit },
	{ "xfer"       , 0, (xdag_com_func_t)NULL},
	{ "help"       , 0, xdag_com_help},
	{ (char *)NULL , 0, (xdag_com_func_t)NULL}
};

int xdag_com_account(char* args, FILE* out)
{
	processAccountCommand(args, out);
	return 0;
}

int xdag_com_balance(char * args, FILE* out)
{
	processBalanceCommand(args, out);
	return 0;
}

int xdag_com_keyGen(char * args, FILE* out)
{
	processKeyGenCommand(out);
	return 0;
}

int xdag_com_level(char * args, FILE* out)
{
	processLevelCommand(args, out);
	return 0;
}

int xdag_com_state(char * args, FILE* out)
{
	fprintf(out, "%s\n", get_state());
	return 0;
}

int xdag_com_terminate(char * args, FILE* out)
{
	processExitCommand();
	return -1;
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

void startCommandProcessing(int deamonFlags)
{
	char cmd[XDAG_COMMAND_MAX];
	if(!(deamonFlags & 0x1)) printf("Type command, help for example.\n");

	xdag_init_commands();

	for(;;) {
		if(deamonFlags & 0x1) {
			sleep(100);
		} else {
			read_command(cmd);
			if(strlen(cmd) > 0) {
				int ret = xdag_command(cmd, stdout);
				if(ret < 0) {
					break;
				}
			}
		}
	}
}

int xdag_command(char *cmd, FILE *out)
{
	uint32_t pwd[4];
	char *nextParam;
	int ispwd = 0;

	cmd = strtok_r(cmd, " \t\r\n", &nextParam);
	if(!cmd) return 0;
	if(sscanf(cmd, "pwd=%8x%8x%8x%8x", pwd, pwd + 1, pwd + 2, pwd + 3) == 4) {
		ispwd = 1;
		cmd = strtok_r(0, " \t\r\n", &nextParam);
	}

	XDAG_COMMAND *command = find_xdag_command(cmd);

	if(!command || (command->avaibility == 1 && !g_is_miner) || (command->avaibility == 2 && g_is_miner)) {
		fprintf(out, "Illegal command.\n");
	} else {
		if(!strcmp(command->name, "xfer")) {
			processXferCommand(nextParam, out, ispwd, pwd);
		} else {
			return (*(command->func))(nextParam, out);
		}
	}
	return 0;
}

void processAccountCommand(char *nextParam, FILE *out)
{
	struct account_callback_data d;
	d.out = out;
	d.count = (g_is_miner ? 1 : 20);
	char *cmd = strtok_r(nextParam, " \t\r\n", &nextParam);
	if(cmd) {
		sscanf(cmd, "%d", &d.count);
	}
	if(g_xdag_state < XDAG_STATE_XFER) {
		fprintf(out, "Not ready to show balances. Type 'state' command to see the reason.\n");
	}
	xdag_traverse_our_blocks(&d, &account_callback);
}

void processBalanceCommand(char *nextParam, FILE *out)
{
	if(g_xdag_state < XDAG_STATE_XFER) {
		fprintf(out, "Not ready to show a balance. Type 'state' command to see the reason.\n");
	} else {
		xdag_hash_t hash;
		xdag_amount_t balance;
		char *cmd = strtok_r(nextParam, " \t\r\n", &nextParam);
		if(cmd) {
			xdag_address2hash(cmd, hash);
			balance = xdag_get_balance(hash);
		} else {
			balance = xdag_get_balance(0);
		}
		fprintf(out, "Balance: %.9Lf %s\n", amount2xdags(balance), COINNAME);
	}
}


void processKeyGenCommand(FILE *out)
{
	const int res = xdag_wallet_new_key();
	if(res < 0) {
		fprintf(out, "Can't generate new key pair.\n");
	} else {
		fprintf(out, "Key %d generated and set as default.\n", res);
	}
}

void processLevelCommand(char *nextParam, FILE *out)
{
	unsigned level;
	char *cmd = strtok_r(nextParam, " \t\r\n", &nextParam);
	if(!cmd) {
		fprintf(out, "%d\n", xdag_set_log_level(-1));
	} else if(sscanf(cmd, "%u", &level) != 1 || level > XDAG_TRACE) {
		fprintf(out, "Illegal level.\n");
	} else {
		xdag_set_log_level(level);
	}
}

void processExitCommand()
{
	xdag_wallet_finish();
//	xdag_netdb_finish();
	xdag_storage_finish();
	xdag_mem_finish();
}

void processXferCommand(char *nextParam, FILE *out, int ispwd, uint32_t* pwd)
{
	char *amount = strtok_r(nextParam, " \t\r\n", &nextParam);
	if(!amount) {
		fprintf(out, "Xfer: amount not given.\n");
		return;
	}
	char *address = strtok_r(0, " \t\r\n", &nextParam);
	if(!address) {
		fprintf(out, "Xfer: destination address not given.\n");
		return;
	}
	if(out == stdout ? xdag_user_crypt_action(0, 0, 0, 3) : (ispwd ? xdag_user_crypt_action(pwd, 0, 4, 5) : 1)) {
		sleep(3);
		fprintf(out, "Password incorrect.\n");
	} else {
		xdag_do_xfer(out, amount, address, 0);
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

int account_callback(void *data, xdag_hash_t hash, xdag_amount_t amount, xdag_time_t time, int n_our_key)
{
	char address[33];
	struct account_callback_data *d = (struct account_callback_data *)data;
	if(!d->count--) {
		return -1;
	}
	xdag_hash2address(hash, address);
	if(g_xdag_state < XDAG_STATE_XFER)
		fprintf(d->out, "%s  key %d\n", address, n_our_key);
	else
		fprintf(d->out, "%s %20.9Lf  key %d\n", address, amount2xdags(amount), n_our_key);
	return 0;
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
		xdag_err("FAILED: to %s xfer %.9Lf %s, error %d",
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

int xdag_do_xfer(void *outv, const char *amount, const char *address, int isGui)
{
	char address_buf[33];
	struct xfer_callback_data xfer;
	FILE *out = (FILE *)outv;

	if(isGui && xdag_user_crypt_action(0, 0, 0, 3)) {
		sleep(3);
		return 1;
	}

	memset(&xfer, 0, sizeof(xfer));
	xfer.remains = xdags2amount(amount);
	if(!xfer.remains) {
		if(out) {
			fprintf(out, "Xfer: nothing to transfer.\n");
		}
		return 1;
	}
	if(xfer.remains > xdag_get_balance(0)) {
		if(out) {
			fprintf(out, "Xfer: balance too small.\n");
		}
		return 1;
	}
	if(xdag_address2hash(address, xfer.fields[XFER_MAX_IN].hash)) {
		if(out) {
			fprintf(out, "Xfer: incorrect address.\n");
		}
		return 1;
	}
	xdag_wallet_default_key(&xfer.keys[XFER_MAX_IN]);
	xfer.outsig = 1;
	g_xdag_state = XDAG_STATE_XFER;
	g_xdag_xfer_last = time(0);
	xdag_traverse_our_blocks(&xfer, &xfer_callback);
	if(out) {
		xdag_hash2address(xfer.fields[XFER_MAX_IN].hash, address_buf);
		fprintf(out, "Xfer: transferred %.9Lf %s to the address %s.\n", amount2xdags(xfer.done), COINNAME, address_buf);
		xdag_hash2address(xfer.transactionBlockHash, address_buf);
		fprintf(out, "Transaction address is %s, it will take several minutes to complete the transaction.\n", address_buf);
	}
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
	if(!g_is_miner && xdag_main_time() < (time >> 16) + 2 * 16) {
		return 0;
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

double xdagGetHashRate(void)
{
	return g_xdag_extstats.hashrate_s / (1024 * 1024);
}

int read_command(char *cmd)
{
#if !defined(_WIN32) && !defined(_WIN64)
	char* line = linenoise("xdag> ");
	if(line == NULL) return 0;

	if(strlen(line) > XDAG_COMMAND_MAX) {
		printf("exceed max length\n");
		strncpy(cmd, line, XDAG_COMMAND_MAX - 1);
		cmd[XDAG_COMMAND_MAX - 1] = '\0';
	} else {
		strcpy(cmd, line);
	}
	free(line);

	if(strlen(cmd) > 0) {
		linenoiseHistoryAdd(cmd);
		linenoiseHistorySave(COMMAND_HISTORY);
	}
#else
	printf("%s> ", g_progname);
	fflush(stdout);
	fgets(cmd, XDAG_COMMAND_MAX, stdin);
#endif

	return 0;
}

#if !defined(_WIN32) && !defined(_WIN64)
static void xdag_com_completion(const char *buf, linenoiseCompletions *lc)
{
	for(int index = 0; commands[index].name; index++) {
		if(!strncmp(buf, commands[index].name, strlen(buf))) {
			linenoiseAddCompletion(lc, commands[index].name);
		}
	}
}
#endif

void xdag_init_commands(void)
{
#if !defined(_WIN32) && !defined(_WIN64)
	linenoiseSetCompletionCallback(xdag_com_completion); //set completion
	linenoiseHistorySetMaxLen(50); //set max line for history
	linenoiseHistoryLoad(COMMAND_HISTORY); //load history
#endif
}
