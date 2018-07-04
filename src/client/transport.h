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
	/* starts the transport system */
	extern int xdag_transport_start(int flags);

#ifdef __cplusplus
};
#endif

#endif
