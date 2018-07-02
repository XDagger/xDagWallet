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
#include "../dnet/dnet_main.h"
//#include "common.h"

#define NEW_BLOCK_TTL   5
#define REQUEST_WAIT    64
#define N_CONNS         4096

static void **connections = 0;

struct xdag_send_data {
	struct xdag_block b;
	void *connection;
};

/* external interface */

/* starts the transport system; bindto - ip:port for a socket for external connections
* addr-port_pairs - array of pointers to strings with parameters of other host for connection (ip:port),
* npairs - count of the strings
*/
int xdag_transport_start(int flags, const char *bindto, int npairs, const char **addr_port_pairs)
{
	const char **argv = malloc((npairs + 5) * sizeof(char *)), *version;
	int argc = 0, i, res;

	if (!argv) return -1;

	argv[argc++] = "dnet";
#if !defined(_WIN32) && !defined(_WIN64)
	if (flags & XDAG_DAEMON) {
		argv[argc++] = "-d";
	}
#endif
	
	if (bindto) {
		argv[argc++] = "-s"; argv[argc++] = bindto;
	}

	for (i = 0; i < npairs; ++i) {
		argv[argc++] = addr_port_pairs[i];
	}
	argv[argc] = 0;
	
	dnet_set_xdag_callback(NULL);
	dnet_connection_open_check = NULL;
	dnet_connection_close_notify = NULL;
	
	connections = (void**)malloc(N_CONNS * sizeof(void *));
	if (!connections) return -1;
	
	res = dnet_init(argc, (char**)argv);
	if (!res) {
		version = strchr(XDAG_VERSION, '-');
		if (version) dnet_set_self_version(version + 1);
	}
	
	return res;
}
