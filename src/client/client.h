#ifndef XDAG_MINER_H
#define XDAG_MINER_H

#include <stdio.h>
#include "block.h"

struct xdag_pool_task {
	struct xdag_field task[2], lastfield, minhash, nonce;
	xdag_time_t task_time;
	void *ctx0, *ctx;
};

extern time_t g_xdag_last_received;

#ifdef __cplusplus
extern "C" {
#endif
	
	extern struct dfslib_crypt *g_crypt;

	/* a number of mining threads */
	extern int g_xdag_mining_threads;

	/* init client */
	extern int client_init(void);

	/* client main thread */
	extern void *client_main_thread(void *arg);

	/* send block to network via pool */
	extern int xdag_send_block_via_pool(struct xdag_block *block);

	extern struct xdag_pool_task g_xdag_pool_task[2];
	extern uint64_t g_xdag_pool_task_index; /* global variables are instantiated with 0 */

	/* initialization of the pool (g_xdag_pool = 1) or connecting the miner to pool (g_xdag_pool = 0; pool_arg - pool parameters ip:port[:CFG];
	 miner_addr - address of the miner, if specified */
	extern int xdag_client_init(const char *pool_arg);
#ifdef __cplusplus
};
#endif
		
#endif
