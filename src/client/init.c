#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "version.h"
#include "init.h"
#include "common.h"
#include "client.h"
#include "commands.h"
#include "utils/utils.h"
#include "utils/log.h"
#include <sys/termios.h>
#include "xdag_wrapper.h"

void printUsage(char* appName);
int log_callback(int level, xdag_error_no err, char *buffer);
int event_callback(void* thisObj, xdag_event *event);
int password_callback(const char *prompt, char *buf, unsigned len);

int xdag_init(int argc, char **argv, int isGui)
{
	xdag_init_path(argv[0]);

	const char *pool_arg = 0;
//	int level = 0;

	if (!isGui) {
		printf("xdag client/server, version %s.\n", XDAG_VERSION);
	}

	if (argc <= 1) {
		printUsage(argv[0]);
		return 0;
	}

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] != '-') {
			if ((!argv[i][1] || argv[i][2]) && strchr(argv[i], ':')) {
				pool_arg = argv[i];
			} else {
				printUsage(argv[0]);
				return 0;
			}
			continue;
		}
		
		if(ARG_EQUAL(argv[i], "-h", "")) { /* help */
			printUsage(argv[0]);
			return 0;
		} else {
			printUsage(argv[0]);
			return 0;
		}
	}

	printf("Set log callback...\n");
	xdag_wrapper_init(NULL, &password_callback, &event_callback, &log_callback);

	printf("Starting...\n");
	if(xdag_client_init(pool_arg)) return -1;

	if (!isGui) {
		startCommandProcessing();
	}

	return 0;
}

#define XDAG_LOG_FILE "xdag.log"
int log_callback(int level, xdag_error_no err, char *buffer)
{
	FILE *f;
	char buf[64] = {0};
	sprintf(buf, XDAG_LOG_FILE);
	f = xdag_open_file(buf, "a");
	if (f) {
		fprintf(f, "%s\n", buffer);
	}

	xdag_close_file(f);
	return 0;
}

int event_callback(void* thisObj, xdag_event *event)
{
	if(!event) {
		return -1;
	}

	switch (event->event_id) {
		case event_id_log:
		{
			printf("%s\n", event->event_data);
			FILE *f;
			char buf[64] = {0};
			sprintf(buf, XDAG_LOG_FILE);
			f = xdag_open_file(buf, "a");
			if (f) {
				fprintf(f, "%s\n", event->event_data);
			}

			xdag_close_file(f);
			break;
		}

		default:
			break;
	}
	return 0;
}

int password_callback(const char *prompt, char *buf, unsigned len)
{
	struct termios t[1];
	int noecho = !!strstr(prompt, "assword");
	printf("%s: ", prompt); fflush(stdout);
	if (noecho) {
		tcgetattr(0, t);
		t->c_lflag &= ~ECHO;
		tcsetattr(0, TCSANOW, t);
	}
	fgets(buf, len, stdin);
	if (noecho) {
		t->c_lflag |= ECHO;
		tcsetattr(0, TCSANOW, t);
		printf("\n");
	}
	len = (int)strlen(buf);
	if (len && buf[len - 1] == '\n') buf[len - 1] = 0;
	return 0;
}

void printUsage(char* appName)
{
	printf("Usage: %s flags [pool_ip:port]\n"
		"If pool_ip:port argument is given, then the node operates as a miner.\n"
		"Flags:\n"
		"  -h             - print this help\n"
		"  -t             - connect to test net (default is main net)\n"
		"  -v N           - set loglevel to N\n"
		, appName);
}
