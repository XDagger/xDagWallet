#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "version.h"
#include "init.h"
#include "common.h"
#include "client.h"
#include "terminal.h"
#include "utils/utils.h"
#include "utils/log.h"
#include "xdag_wrapper.h"

#if defined(_WIN32) || defined(_WIN64)
#else
#include <sys/termios.h>
#endif

void printUsage(char* appName);
int log_callback(int level, xdag_error_no err, char *buffer);
int event_callback(void* thisObj, xdag_event *event);
int password_callback(const char *prompt, char *buf, unsigned len);

int xdag_init(int argc, char **argv, int isGui)
{
	xdag_init_path(argv[0]);

	const char *pool_arg = 0;
	if(!isGui) {
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
		
		if(ARG_EQUAL(argv[i], "-h", "--help")) { /* help */
			printUsage(argv[0]);
			return 0;
		} else {
			printUsage(argv[0]);
			return 0;
		}
	}

	printf("Set log callback...\n");
	xdag_wrapper_init(NULL, &password_callback, &event_callback);

	printf("Starting...\n");
	if(xdag_client_init(pool_arg)) return -1;

	if (!isGui) {
		startCommandProcessing();
	}

	return 0;
}

#define XDAG_LOG_FILE "xdag.log"

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

		case event_id_interact:
		{
			fprintf(stdout, "%s\n", event->event_data);
			fflush(stdout);
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
			fprintf(stdout, "error : %x, msg : %s\n", event->error_no, event->event_data);
			fflush(stdout);
			xdag_wrapper_exit();
			pthread_cancel(g_client_thread);
			exit(1);
			break;
		}

		case event_id_account_done:
		{
			fprintf(stdout, "%s\n", event->event_data);
			fflush(stdout);
			break;
		}

		case event_id_address_done:
		{
			fprintf(stdout, "%s\n", event->event_data);
			fflush(stdout);
			break;
		}

		case event_id_balance_done:
		{
			fprintf(stdout, "%s\n", event->event_data);
			fflush(stdout);
			break;
		}

		case event_id_xfer_done:
		{
			fprintf(stdout, "%s\n", event->event_data);
			fflush(stdout);
			break;
		}

		case event_id_level_done:
		{
			fprintf(stdout, "%s\n", event->event_data);
			fflush(stdout);
			break;
		}

		case event_id_state_done:
		{
			fprintf(stdout, "%s\n", event->event_data);
			fflush(stdout);
			break;
		}

		case event_id_exit_done:
		{
			fprintf(stdout, "%s\n", event->event_data);
			fflush(stdout);
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
			fprintf(stdout, "state changed %s\n", event->event_data);
			fflush(stdout);
			break;
		}

		default:
		{
			if(event->event_data) {
				printf("%s\n", event->event_data);
			}
			break;
		}
	}
	return 0;
}

int password_callback(const char *prompt, char *buf, unsigned len)
{

#if !defined(_WIN32) && !defined(_WIN64)
	struct termios t[1];
	int noecho = !!strstr(prompt, "assword");
	fprintf(stdout,"%s: ", prompt); fflush(stdout);

	if (noecho) {
		tcgetattr(0, t);
		t->c_lflag &= ~ECHO;
		tcsetattr(0, TCSANOW, t);
	}
	fgets(buf, len, stdin);
	if (noecho) {
		t->c_lflag |= ECHO;
		tcsetattr(0, TCSANOW, t);
		fprintf(stdout,"\n");
	}
	len = (int)strlen(buf);
	if (len && buf[len - 1] == '\n') buf[len - 1] = 0;
#endif

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
