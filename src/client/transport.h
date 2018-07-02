/* транспорт, T13.654-T13.788 $DVS:time$ */

#ifndef XDAG_TRANSPORT_H
#define XDAG_TRANSPORT_H

#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include "block.h"
#include "storage.h"

enum xdag_transport_flags {
	XDAG_DAEMON = 1,
};

#ifdef __cplusplus
extern "C" {
#endif
	/* starts the transport system; bindto - ip:port for a socket for external connections
	 * addr-port_pairs - array of pointers to strings with parameters of other host for connection (ip:port),
	 * npairs - count of the strings
	 */
	extern int xdag_transport_start(int flags, const char *bindto, int npairs, const char **addr_port_pairs);

#ifdef __cplusplus
};
#endif

#endif
