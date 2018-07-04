/* транспорт, T13.654-T13.788 $DVS:time$ */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "transport.h"
#include "storage.h"
#include "block.h"
#include "init.h"
#include "miner.h"
#include "version.h"
#include "./dnet_main.h"

struct xdag_send_data {
	struct xdag_block b;
	void *connection;
};

/* starts the transport system */
int xdag_transport_start(int flags)
{
const char **argv = malloc((0 + 5) * sizeof(char *));
	int argc = 0, i, res;

	if (!argv) return -1;

	argv[argc++] = "dnet";
#if !defined(_WIN32) && !defined(_WIN64)
	if (flags & XDAG_DAEMON) {
		argv[argc++] = "-d";
	}
#endif
	argv[argc] = 0;

	res = dnet_init(argc, (char**)argv);

	return res;
}
