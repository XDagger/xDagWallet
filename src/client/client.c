#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "system.h"
#include "../dus/dfslib_crypt.h"
#include "../dus/crc.h"
#include "crypt.h"
#include "wallet.h"
#include "address.h"
#include "block.h"
#include "common.h"
#include "client.h"
#include "storage.h"
#include "utils/log.h"
#include "commands.h"
#include "dnet_crypt.h"
#include "./utils/utils.h"
#include "version.h"

//#define __stand_alone_lib__ // if run as stand alone library

#if defined(_WIN32) || defined(_WIN64)
#if defined(_WIN64)
#define poll WSAPoll
#else
#define poll(a, b, c) ((a)->revents = (a)->events, (b))
#endif
#else
#include <poll.h>
#endif

#define DATA_SIZE          (sizeof(struct xdag_field) / sizeof(uint32_t))
#define BLOCK_HEADER_WORD  0x3fca9e2bu

pthread_mutex_t g_transport_mutex = PTHREAD_MUTEX_INITIALIZER;
time_t g_xdag_last_received = 0;

struct xdag_pool_task g_xdag_pool_task[2];
uint64_t g_xdag_pool_task_index;

struct dfslib_crypt *g_crypt;

#define MINERS_PWD             "minersgonnamine"
#define SECTOR0_BASE           0x1947f3acu
#define SECTOR0_OFFSET         0x82e9d1b5u
#define SEND_PERIOD            5                                  /* share period of sending shares */

struct miner {
	struct xdag_field id;
	uint64_t nfield_in;
	uint64_t nfield_out;
};

static struct miner g_local_miner;

/* a number of mining threads */
int g_xdag_mining_threads = 0;

static int g_socket = -1;


static int crypt_start(void)
{
	struct dfslib_string str;
	uint32_t sector0[128];
	int i;

	g_crypt = malloc(sizeof(struct dfslib_crypt));
	if(!g_crypt) return -1;
	dfslib_crypt_set_password(g_crypt, dfslib_utf8_string(&str, MINERS_PWD, strlen(MINERS_PWD)));

	for(i = 0; i < 128; ++i) {
		sector0[i] = SECTOR0_BASE + i * SECTOR0_OFFSET;
	}

	for(i = 0; i < 128; ++i) {
		dfslib_crypt_set_sector0(g_crypt, sector0);
		dfslib_encrypt_sector(g_crypt, sector0, SECTOR0_BASE + i * SECTOR0_OFFSET);
	}

	return 0;
}

/* pool_arg - pool parameters ip:port[:CFG] */
int xdag_client_init(const char *pool_arg)
{

#if (defined __android__) || (defined __ios__) || (defined__stand_alone_lib__)

#else
	if(!!client_init()) {
		return -1;
	}
#endif

	printf("%s\n", pool_arg);

	pthread_t th;
	int err = pthread_create(&th, 0, client_main_thread, (void*)pool_arg);
	if(err != 0) {
		printf("create client_main_thread failed, error : %s\n", strerror(err));
		return -1;
	}

	return 0;
}

static int can_send_share(time_t current_time, time_t task_time, time_t share_time)
{
	int can_send = (current_time - share_time >= SEND_PERIOD) && (current_time - task_time <= 64) && (share_time >= task_time);
	return can_send;
}

static int send_to_pool(struct xdag_field *fld, int nfld)
{
	struct xdag_field f[XDAG_BLOCK_FIELDS];
	xdag_hash_t h;
	struct miner *m = &g_local_miner;
	int todo = nfld * sizeof(struct xdag_field), done = 0;

	if(g_socket < 0) {
		return -1;
	}

	memcpy(f, fld, todo);

	if(nfld == XDAG_BLOCK_FIELDS) {
		f[0].transport_header = 0;

		xdag_hash(f, sizeof(struct xdag_block), h);

		f[0].transport_header = BLOCK_HEADER_WORD;

		uint32_t crc = crc_of_array((uint8_t*)f, sizeof(struct xdag_block));

		f[0].transport_header |= (uint64_t)crc << 32;
	}

	for(int i = 0; i < nfld; ++i) {
		dfslib_encrypt_array(g_crypt, (uint32_t*)(f + i), DATA_SIZE, m->nfield_out++);
	}

	while(todo) {
		struct pollfd p;

		p.fd = g_socket;
		p.events = POLLOUT;

		if(!poll(&p, 1, 1000)) continue;

		if(p.revents & (POLLHUP | POLLERR)) {
			return -1;
		}

		if(!(p.revents & POLLOUT)) continue;

		int res = (int)write(g_socket, (uint8_t*)f + done, todo);
		if(res <= 0) {
			return -1;
		}

		done += res;
		todo -= res;
	}

	if(nfld == XDAG_BLOCK_FIELDS) {
		xdag_info("Sent  : %016llx%016llx%016llx%016llx t=%llx res=%d",
			h[3], h[2], h[1], h[0], fld[0].time, 0);
	}

	return 0;
}

int client_init(void)
{
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

	//	if(is_rpc) {
	//		xdag_mess("Initializing RPC service...");
	//		if(!!xdag_rpc_service_init(rpc_port)) return -1;
	//	}

	for(int i = 0; i < 2; ++i) {
		g_xdag_pool_task[i].ctx0 = malloc(xdag_hash_ctx_size());
		g_xdag_pool_task[i].ctx = malloc(xdag_hash_ctx_size());

		if(!g_xdag_pool_task[i].ctx0 || !g_xdag_pool_task[i].ctx) {
			xdag_err(error_init_task_failed,"init task failed.");
			return -1;
		}
	}

	if(crypt_start()) {
		xdag_err(error_init_crypt_failed,"crypt start failed.");
		return -1;
	}

	return 0;
}

void *client_main_thread(void *arg)
{
	if(!arg) {
		xdag_err(error_missing_param, "need pool arguments!");
		return 0;
	}

	const char *str = (const char*)arg;
	char pool_param[0x100];
	strcpy(pool_param, str);

#if (defined __android__) || (defined __ios__) || (defined __stand_alone_lib__)
	if(!!client_init()) {
		return (void *)1;
	}
#else
	int err = pthread_detach(pthread_self());
	if(err != 0) {
		printf("detach client_main_thread failed, error : %s\n", strerror(err));
		return 0;
	}
#endif

	xdag_mess("Initialize miner...");

//	const char *mess = "", *mess1 = "";

	struct xdag_block b;
	struct xdag_field data[2];
	xdag_hash_t hash;
	xdag_time_t t;

	struct sockaddr_in peeraddr;
	char *lasts;
	int res = 0, reuseaddr = 1;
	struct linger linger_opt = { 1, 0 }; // Linger active, timeout 0

	xdag_mess("Entering main cycle...");
	char pool_arg[0x100];

begin:
	strcpy(pool_arg, pool_param);
	memset(&g_local_miner, 0, sizeof(struct miner));
	xdag_get_our_block(g_local_miner.id.data);

	struct miner *m = &g_local_miner;
	m->nfield_in = m->nfield_out = 0;

	memcpy(hash, g_local_miner.id.data, sizeof(xdag_hash_t));

	int ndata = 0;
	int maxndata = sizeof(struct xdag_field);
	time_t share_time = 0;
	time_t task_time = 0;

	const int64_t pos = xdag_get_block_pos(hash, &t);
	if(pos < 0) {
		xdag_err(error_block_not_found, "can't find the block");
		goto err;
	}

	struct xdag_block *blk = xdag_storage_load(hash, t, pos, &b);
	if(!blk) {
		xdag_err(error_block_load_failed, "can't load the block");
		goto end;
	}
	if(blk != &b) {
		memcpy(&b, blk, sizeof(struct xdag_block));
	}

	// Create a socket
	g_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(g_socket == INVALID_SOCKET) {
		xdag_err(error_socket_create, "cannot create a socket");
		goto end;
	}
	if(fcntl(g_socket, F_SETFD, FD_CLOEXEC) == -1) {
		xdag_err(error_socket_create, "pool  : can't set FD_CLOEXEC flag on socket %d, %s\n", g_socket, strerror(errno));
	}

	// Fill in the address of server
	memset(&peeraddr, 0, sizeof(peeraddr));
	peeraddr.sin_family = AF_INET;

	// Resolve the server address (convert from symbolic name to IP number)
	const char *s = strtok_r(pool_arg, " \t\r\n:", &lasts);
	if(!s) {
		xdag_err(error_missing_param, "host is not given");
		goto end;
	}
	if(!strcmp(s, "any")) {
		peeraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else if(!inet_aton(s, &peeraddr.sin_addr)) {
		struct hostent *host = gethostbyname(s);
		if(host == NULL || host->h_addr_list[0] == NULL) {
			xdag_err(error_socket_resolve_host, "cannot resolve host %s", s);
			res = h_errno;
			goto end;
		}
		// Write resolved IP address of a server to the address structure
		memmove(&peeraddr.sin_addr.s_addr, host->h_addr_list[0], 4);
	}

	// Resolve port
	s = strtok_r(0, " \t\r\n:", &lasts);
	if(!s) {
		xdag_err(error_missing_param, "port is not given");
		goto end;
	}
	peeraddr.sin_port = htons(atoi(s));

	// Set the "LINGER" timeout to zero, to close the listen socket
	// immediately at program termination.
	setsockopt(g_socket, SOL_SOCKET, SO_LINGER, (char*)&linger_opt, sizeof(linger_opt));
	setsockopt(g_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseaddr, sizeof(int));

	// Now, connect to a pool
	res = connect(g_socket, (struct sockaddr*)&peeraddr, sizeof(peeraddr));
	if(res) {
		xdag_err(error_socket_connect, "cannot connect to the pool");
		g_xdag_state = g_xdag_testnet ? XDAG_STATE_TTST : XDAG_STATE_TRYP;
		goto err;
	}

	if(send_to_pool(b.field, XDAG_BLOCK_FIELDS) < 0) {
		xdag_err(error_socket_closed, "socket is closed");
		g_xdag_state = g_xdag_testnet ? XDAG_STATE_TTST : XDAG_STATE_TRYP;
		goto err;
	}

	for(;;) {
		if(get_timestamp() - t > 1024) {
			t = get_timestamp();
			if (g_xdag_state == XDAG_STATE_REST) {
				xdag_err(error_block_load_failed, "block reset!!!");
			} else {
				if (t > (g_xdag_last_received << 10) && t - (g_xdag_last_received << 10) > 3 * MAIN_CHAIN_PERIOD) {
					g_xdag_state = g_xdag_testnet ? XDAG_STATE_TTST : XDAG_STATE_TRYP;
				} else {
					if (t - (g_xdag_xfer_last << 10) <= 2 * MAIN_CHAIN_PERIOD + 4) {
						g_xdag_state = XDAG_STATE_XFER;
					} else {
						g_xdag_state = g_xdag_testnet ? XDAG_STATE_PTST : XDAG_STATE_POOL;
					}
				}
			}
		}

		struct pollfd p;

		if(g_socket < 0) {
			xdag_err(error_socket_closed, "socket is closed");
			goto err;
		}

		p.fd = g_socket;
		time_t current_time = time(0);
		p.events = POLLIN | (can_send_share(current_time, task_time, share_time) ? POLLOUT : 0);

		if(!poll(&p, 1, 0)) {
			sleep(1);
			continue;
		}

		if(p.revents & POLLHUP) {
			xdag_err(error_socket_hangup, "socket hangup");
			goto err;
		}

		if(p.revents & POLLERR) {
			xdag_err(error_socket_err, "socket error");
			goto err;
		}

		if(p.revents & POLLIN) {
			res = (int)read(g_socket, (uint8_t*)data + ndata, maxndata - ndata);
			if(res < 0) {
				xdag_err(error_socket_read, "read error on socket");
				goto err;
			}
			ndata += res;
			if(ndata == maxndata) {
				struct xdag_field *last = data + (ndata / sizeof(struct xdag_field) - 1);

				dfslib_uncrypt_array(g_crypt, (uint32_t*)last->data, DATA_SIZE, m->nfield_in++);

				if(!memcmp(last->data, hash, sizeof(xdag_hashlow_t))) {
					xdag_set_balance(hash, last->amount);

					pthread_mutex_lock(&g_transport_mutex);
					g_xdag_last_received = current_time;
					pthread_mutex_unlock(&g_transport_mutex);

					ndata = 0;

					maxndata = sizeof(struct xdag_field);
				} else if(maxndata == 2 * sizeof(struct xdag_field)) {
					const uint64_t task_index = g_xdag_pool_task_index + 1;
					struct xdag_pool_task *task = &g_xdag_pool_task[task_index & 1];

					task->task_time = xdag_main_time();
					xdag_hash_set_state(task->ctx, data[0].data,
						sizeof(struct xdag_block) - 2 * sizeof(struct xdag_field));
					xdag_hash_update(task->ctx, data[1].data, sizeof(struct xdag_field));
					xdag_hash_update(task->ctx, hash, sizeof(xdag_hashlow_t));

					dnet_generate_random_array(task->nonce.data, sizeof(xdag_hash_t));

					memcpy(task->nonce.data, hash, sizeof(xdag_hashlow_t));
					memcpy(task->lastfield.data, task->nonce.data, sizeof(xdag_hash_t));

					xdag_hash_final(task->ctx, &task->nonce.amount, sizeof(uint64_t), task->minhash.data);

					g_xdag_pool_task_index = task_index;
					task_time = time(0);

					xdag_info("Task  : t=%llx N=%llu", task->task_time << 16 | 0xffff, task_index);

					ndata = 0;
					maxndata = sizeof(struct xdag_field);
				} else {
					maxndata = 2 * sizeof(struct xdag_field);
				}
			}
		}

		if(p.revents & POLLOUT) {
			const uint64_t task_index = g_xdag_pool_task_index;
			struct xdag_pool_task *task = &g_xdag_pool_task[task_index & 1];
			uint64_t *h = task->minhash.data;

			share_time = time(0);
			res = send_to_pool(&task->lastfield, 1);

			xdag_info("Share : %016llx%016llx%016llx%016llx t=%llx res=%d",
				h[3], h[2], h[1], h[0], task->task_time << 16 | 0xffff, res);

			if(res) {
				xdag_err(error_socket_write, "write error on socket");
				goto err;
			}
		}
	}

	return 0;

err:
	if(g_socket != INVALID_SOCKET) {
		close(g_socket);
		g_socket = INVALID_SOCKET;
	}
	sleep(5);
	goto begin;

end:
	if(g_socket != INVALID_SOCKET) {
		close(g_socket);
		g_socket = INVALID_SOCKET;
	}
	return 0;
}

/* send block to network via pool */
int xdag_send_block_via_pool(struct xdag_block *b)
{
	if(g_socket < 0) return -1;
	int ret = send_to_pool(b->field, XDAG_BLOCK_FIELDS);
	return ret;
}
