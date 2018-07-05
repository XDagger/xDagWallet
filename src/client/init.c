/* cheatcoin main, T13.654-T13.895 $DVS:time$ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <signal.h>
#endif
#include "system.h"
#include "address.h"
#include "block.h"
#include "crypt.h"
#include "version.h"
#include "wallet.h"
#include "init.h"
#include "client.h"
#include "commands.h"
#include "dnet_crypt.h"
#include "utils/log.h"
#include "utils/utils.h"
#include "json-rpc/rpc_service.h"

#define ARG_EQUAL(a,b,c) strcmp(c, "") == 0 ? strcmp(a, b) == 0 : (strcmp(a, b) == 0 || strcmp(a, c) == 0)

int g_xdag_state = XDAG_STATE_INIT;
int g_xdag_testnet = 0;
time_t g_xdag_xfer_last = 0;
enum xdag_field_type g_block_header_type = XDAG_FIELD_HEAD;
struct xdag_stats g_xdag_stats;
struct xdag_ext_stats g_xdag_extstats;

void printUsage(char* appName);

int xdag_init(int argc, char **argv, int isGui)
{
	xdag_init_path(argv[0]);

	const char *pool_arg = 0;
	int level, is_rpc = 0, rpc_port = 0;

//#if !defined(_WIN32) && !defined(_WIN64)
//	signal(SIGHUP, SIG_IGN);
//	signal(SIGPIPE, SIG_IGN);
//	signal(SIGWINCH, SIG_IGN);
//	signal(SIGINT, SIG_IGN);
//	signal(SIGTERM, SIG_IGN);
//#endif

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
		} else if(ARG_EQUAL(argv[i], "-t", "")) { /* connect test net */
			g_xdag_testnet = 1;
		} else if(ARG_EQUAL(argv[i], "-v", "")) { /* log level */
			if (++i < argc && sscanf(argv[i], "%d", &level) == 1) {
				xdag_set_log_level(level);
			} else {
				printf("Illevel use of option -v\n");
				return -1;
			}
		} else if(ARG_EQUAL(argv[i], "", "-rpc-enable")) { /* enable JSON-RPC service */
			is_rpc = 1;
		} else if(ARG_EQUAL(argv[i], "", "-rpc-port")) { /* set JSON-RPC service port */
			if(++i < argc && sscanf(argv[i], "%d", &rpc_port) == 1) {
				if(rpc_port < 0 || rpc_port > 65535) {
					printf("RPC port is invalid, set to default.\n");
					rpc_port = 0;
				}
			}
		} else {
			printUsage(argv[0]);
			return 0;
		}
	}
	
	if(g_xdag_testnet) {
		g_block_header_type = XDAG_FIELD_HEAD_TEST; //block header has the different type in the test network
	}

	memset(&g_xdag_stats, 0, sizeof(g_xdag_stats));
	memset(&g_xdag_extstats, 0, sizeof(g_xdag_extstats));

	xdag_mess("Starting xdag, version %s", XDAG_VERSION);
	xdag_mess("Starting dnet transport...");
	printf("Initialize...\n");
	if (dnet_crypt_init(DNET_VERSION)) {
		sleep(3);
		printf("Password incorrect.\n");
		return -1;
	}

	if (xdag_log_init()) return -1;

	xdag_mess("Initializing cryptography...");
	if (xdag_crypt_init(1)) return -1;
	xdag_mess("Reading wallet...");
	if (xdag_wallet_init()) return -1;
	xdag_mess("Initializing addresses...");
	if (xdag_address_init()) return -1;

	xdag_mess("Starting blocks engine...");
	if (xdag_blocks_start()) return -1;

	if(is_rpc) {
		xdag_mess("Initializing RPC service...");
		if(!!xdag_rpc_service_init(rpc_port)) return -1;
	}

	xdag_mess("Starting engine...");
	if(xdag_client_init(pool_arg)) return -1;

	if (!isGui) {
		startCommandProcessing();
	}

	return 0;
}

int xdag_set_password_callback(int(*callback)(const char *prompt, char *buf, unsigned size))
{
    return xdag_user_crypt_action((uint32_t *)(void *)callback, 0, 0, 6);
}

void printUsage(char* appName)
{
	printf("Usage: %s flags [pool_ip:port]\n"
		"If pool_ip:port argument is given, then the node operates as a miner.\n"
		"Flags:\n"
		"  -h             - print this help\n"
		"  -t             - connect to test net (default is main net)\n"
		"  -v N           - set loglevel to N\n"
		"  -rpc-enable    - enable JSON-RPC service\n"
		"  -rpc-port      - set HTTP JSON-RPC port (default is 7677)\n"
		, appName);
}
