//
//  terminal.c
//  xDagWallet
//
//  Created by Rui Xie on 7/23/18.
//  Copyright © 2018 xrdavies. All rights reserved.
//

#include "terminal.h"
#include "commands.h"
#include <string.h>
#include <stdlib.h>

typedef int (*xdag_com_func_t)(char *, FILE *);
typedef struct {
	char *name;				/* command name */
	xdag_com_func_t func;	/* command function */
} XDAG_COMMAND;


int read_command(char *cmd);
int xdag_command(char *cmd, FILE *out);

int xdag_com_account(char *, FILE *);
int xdag_com_address(char *, FILE *);
int xdag_com_balance(char *, FILE *);
int xdag_com_level(char *, FILE *);
int xdag_com_xfer(char *, FILE *);
int xdag_com_state(char *, FILE *);
int xdag_com_help(char *, FILE *);
int xdag_com_exit(char *, FILE *);

XDAG_COMMAND* find_xdag_command(char*);

XDAG_COMMAND commands[] = {
	{ "account"    , xdag_com_account },
	{ "address"    , xdag_com_address },
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
	char *result = NULL;
	processAccountCommand(&result);

	if(result) {
		fprintf(out, "%s\n", result);
		free(result);
	}

	return 0;
}

int xdag_com_address(char *args, FILE* out)
{
	char *result = NULL;
	processAddressCommand(&result);

	if(result) {
		fprintf(out, "%s\n", result);
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
		fprintf(out, "%s\n", result);
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
		fprintf(out, "%s\n", result);
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
		fprintf(out, "%s\n", result);
		free(result);
	}
	return 0;
}

int xdag_com_state(char *args, FILE* out)
{
	char *result = NULL;
	processStateCommand(&result);
	if(result) {
		fprintf(out, "%s\n", result);
		free(result);
	}

	return 0;
}

int xdag_com_exit(char * args, FILE* out)
{
	processExitCommand();
	return -1;
}

int xdag_com_help(char *args, FILE* out)
{
	char *result = NULL;

	processHelpCommand(&result);

	if(result) {
		fprintf(out, "%s\n", result);
		free(result);
	}
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

int read_command(char *cmd)
{
	printf("xdag> ");
	fflush(stdout);
	fgets(cmd, XDAG_COMMAND_MAX, stdin);
	return 0;
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

