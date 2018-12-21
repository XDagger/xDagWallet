#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "system.h"
#include "../ldus/rbtree.h"
#include "block.h"
#include "crypt.h"
#include "wallet.h"
#include "storage.h"
#include "utils/log.h"
#include "common.h"
#include "client.h"
#include "address.h"
#include "commands.h"
#include "utils/utils.h"

#if defined(_WIN32) || defined(_WIN64)
#include "../win/unistd.h"
#else
#include <unistd.h>
#endif

enum bi_flags {
	BI_MAIN       = 0x01,
	BI_MAIN_CHAIN = 0x02,
	BI_APPLIED    = 0x04,
	BI_MAIN_REF   = 0x08,
	BI_REF        = 0x10,
	BI_OURS       = 0x20,
};

struct block_backrefs;

struct block_internal {
	struct ldus_rbtree node;
	xdag_hash_t hash;
	xdag_diff_t difficulty;
	xdag_amount_t amount, linkamount[MAX_LINKS], fee;
	xdag_time_t time;
	uint64_t storage_pos;
	struct block_internal *ref, *link[MAX_LINKS];
	struct block_backrefs *backrefs;
	uint8_t flags, nlinks, max_diff_link, reserved;
	uint16_t in_mask;
	uint16_t n_our_key;
};

#define N_BACKREFS      (sizeof(struct block_internal) / sizeof(struct block_internal *) - 1)

struct block_backrefs {
	struct block_internal *backrefs[N_BACKREFS];
	struct block_backrefs *next;
};

#define ourprev link[MAX_LINKS - 2]
#define ournext link[MAX_LINKS - 1]

static xdag_amount_t g_balance = 0;
static xdag_time_t time_limit = DEF_TIME_LIMIT;
xdag_time_t g_xdag_era = XDAG_MAIN_ERA;
static struct ldus_rbtree *root = 0;
static struct block_internal *volatile top_main_chain = 0, *volatile pretop_main_chain = 0;
static struct block_internal *ourfirst = 0, *ourlast = 0, *noref_first = 0, *noref_last = 0;
static pthread_mutex_t block_mutex;
//TODO: this variable duplicates existing global variable g_is_pool. Probably should be removed
static int g_light_mode = 0;

//functions
int32_t check_signature_out_cached(struct block_internal*, struct xdag_public_key*, const int, int32_t*, int32_t*);
int32_t check_signature_out(struct block_internal*, struct xdag_public_key*, const int);
static int32_t find_and_verify_signature_out(struct xdag_block*, struct xdag_public_key*, const int);

// returns a time period index, where a period is 64 seconds long
xdag_time_t xdag_main_time(void)
{
	return MAIN_TIME(get_timestamp());
}

// returns the time period index corresponding to the start of the network
xdag_time_t xdag_start_main_time(void)
{
	return MAIN_TIME(XDAG_ERA);
}

static inline int lessthan(struct ldus_rbtree *l, struct ldus_rbtree *r)
{
	return memcmp(l + 1, r + 1, 24) < 0;
}

ldus_rbtree_define_prefix(lessthan, static inline, )

static inline struct block_internal *block_by_hash(const xdag_hashlow_t hash)
{
	return (struct block_internal *)ldus_rbtree_find(root, (struct ldus_rbtree *)hash - 1);
}

static void log_block(const char *mess, xdag_hash_t h, xdag_time_t t, uint64_t pos)
{
	/* Do not log blocks as we are loading from local storage */
	if(xdag_get_state() != XDAG_STATE_LOAD) {
		xdag_info("%s: %016llx%016llx%016llx%016llx t=%llx pos=%llx", mess,
			((uint64_t*)h)[3], ((uint64_t*)h)[2], ((uint64_t*)h)[1], ((uint64_t*)h)[0], t, pos);
	}
}

static inline void accept_amount(struct block_internal *bi, xdag_amount_t sum)
{
	if (!sum) {
		return;
	}

	bi->amount += sum;
	if (bi->flags & BI_OURS) {
		struct block_internal *ti;
		g_balance += sum;

		while ((ti = bi->ourprev) && bi->amount > ti->amount) {
			bi->ourprev = ti->ourprev;
			ti->ournext = bi->ournext;
			bi->ournext = ti;
			ti->ourprev = bi;
			*(bi->ourprev ? &bi->ourprev->ournext : &ourfirst) = bi;
			*(ti->ournext ? &ti->ournext->ourprev : &ourlast) = ti;
		}

		while ((ti = bi->ournext) && bi->amount < ti->amount) {
			bi->ournext = ti->ournext;
			ti->ourprev = bi->ourprev;
			bi->ourprev = ti;
			ti->ournext = bi;
			*(bi->ournext ? &bi->ournext->ourprev : &ourlast) = bi;
			*(ti->ourprev ? &ti->ourprev->ournext : &ourfirst) = ti;
		}
	}
}

static uint64_t apply_block(struct block_internal *bi)
{
	xdag_amount_t sum_in, sum_out;

	if (bi->flags & BI_MAIN_REF) {
		return -1l;
	}

	bi->flags |= BI_MAIN_REF;

	for (int i = 0; i < bi->nlinks; ++i) {
		xdag_amount_t ref_amount = apply_block(bi->link[i]);
		if (ref_amount == -1l) {
			continue;
		}
		bi->link[i]->ref = bi;
		if (bi->amount + ref_amount >= bi->amount) {
			accept_amount(bi, ref_amount);
		}
	}

	sum_in = 0;
	sum_out = bi->fee;

	for (int i = 0; i < bi->nlinks; ++i) {
		if (1 << i & bi->in_mask) {
			if (bi->link[i]->amount < bi->linkamount[i]) {
				return 0;
			}
			if (sum_in + bi->linkamount[i] < sum_in) {
				return 0;
			}
			sum_in += bi->linkamount[i];
		} else {
			if (sum_out + bi->linkamount[i] < sum_out) {
				return 0;
			}
			sum_out += bi->linkamount[i];
		}
	}

	if (sum_in + bi->amount < sum_in || sum_in + bi->amount < sum_out) {
		return 0;
	}

	for (int i = 0; i < bi->nlinks; ++i) {
		if (1 << i & bi->in_mask) {
			accept_amount(bi->link[i], (xdag_amount_t)0 - bi->linkamount[i]);
		} else {
			accept_amount(bi->link[i], bi->linkamount[i]);
		}
	}

	accept_amount(bi, sum_in - sum_out);
	bi->flags |= BI_APPLIED;

	return bi->fee;
}

static uint64_t unapply_block(struct block_internal *bi)
{
	int i;

	if (bi->flags & BI_APPLIED) {
		xdag_amount_t sum = bi->fee;

		for (i = 0; i < bi->nlinks; ++i) {
			if (1 << i & bi->in_mask) {
				accept_amount(bi->link[i], bi->linkamount[i]);
				sum -= bi->linkamount[i];
			} else {
				accept_amount(bi->link[i], (xdag_amount_t)0 - bi->linkamount[i]);
				sum += bi->linkamount[i];
			}
		}

		accept_amount(bi, sum);
		bi->flags &= ~BI_APPLIED;
	}

	bi->flags &= ~BI_MAIN_REF;
	bi->ref = 0;

	for (i = 0; i < bi->nlinks; ++i) {
		if (bi->link[i]->ref == bi && bi->link[i]->flags & BI_MAIN_REF) {
			accept_amount(bi, unapply_block(bi->link[i]));
		}
	}

	return (xdag_amount_t)0 - bi->fee;
}

// calculates current supply by specified count of main blocks
xdag_amount_t xdag_get_supply(uint64_t nmain)
{
	xdag_amount_t res = 0, amount = MAIN_START_AMOUNT;

	while (nmain >> MAIN_BIG_PERIOD_LOG) {
		res += (1l << MAIN_BIG_PERIOD_LOG) * amount;
		nmain -= 1l << MAIN_BIG_PERIOD_LOG;
		amount >>= 1;
	}
	res += nmain * amount;
	return res;
}

static void set_main(struct block_internal *m)
{
	xdag_amount_t amount = MAIN_START_AMOUNT >> (g_xdag_stats.nmain >> MAIN_BIG_PERIOD_LOG);

	m->flags |= BI_MAIN;
	accept_amount(m, amount);
	g_xdag_stats.nmain++;

	if (g_xdag_stats.nmain > g_xdag_stats.total_nmain) {
		g_xdag_stats.total_nmain = g_xdag_stats.nmain;
	}

	accept_amount(m, apply_block(m));
	m->ref = m;
	log_block((m->flags & BI_OURS ? "MAIN +" : "MAIN  "), m->hash, m->time, m->storage_pos);
}

static void unset_main(struct block_internal *m)
{
	g_xdag_stats.nmain--;
	g_xdag_stats.total_nmain--;
	xdag_amount_t amount = MAIN_START_AMOUNT >> (g_xdag_stats.nmain >> MAIN_BIG_PERIOD_LOG);
	m->flags &= ~BI_MAIN;
	accept_amount(m, (xdag_amount_t)0 - amount);
	accept_amount(m, unapply_block(m));
	log_block("UNMAIN", m->hash, m->time, m->storage_pos);
}

static void check_new_main(void)
{
	struct block_internal *b, *p = 0;
	int i;

	for (b = top_main_chain, i = 0; b && !(b->flags & BI_MAIN); b = b->link[b->max_diff_link]) {
		if (b->flags & BI_MAIN_CHAIN) {
			p = b;
			++i;
		}
	}

	if (p && i > MAX_WAITING_MAIN && get_timestamp() >= p->time + 2 * 1024) {
		set_main(p);
	}
}

static void unwind_main(struct block_internal *b)
{
	for (struct block_internal *t = top_main_chain; t != b; t = t->link[t->max_diff_link]) {
		t->flags &= ~BI_MAIN_CHAIN;
		if (t->flags & BI_MAIN) {
			unset_main(t);
		}
	}
}

static inline void hash_for_signature(struct xdag_block b[2], const struct xdag_public_key *key, xdag_hash_t hash)
{
	memcpy((uint8_t*)(b + 1) + 1, (void*)((uintptr_t)key->pub & ~1l), sizeof(xdag_hash_t));

	*(uint8_t*)(b + 1) = ((uintptr_t)key->pub & 1) | 0x02;

	xdag_hash(b, sizeof(struct xdag_block) + sizeof(xdag_hash_t) + 1, hash);

	xdag_debug("Hash  : hash=[%s] data=[%s]", xdag_log_hash(hash),
		xdag_log_array(b, sizeof(struct xdag_block) + sizeof(xdag_hash_t) + 1));
}

xdag_diff_t xdag_hash_difficulty(xdag_hash_t hash)
{
	xdag_diff_t res = ((xdag_diff_t*)hash)[1];
	xdag_diff_t max = xdag_diff_max;

	xdag_diff_shr32(&res);

#if !defined(_WIN32) && !defined(_WIN64)
	if(!res) {
		xdag_warn("hash_difficulty higher part of hash is equal zero");
		return max;
	}
#endif
	return xdag_diff_div(max, res);
}

// returns a number of public key from 'keys' array with lengh 'keysLength', which conforms to the signature starting from field signo_r of the block b
// returns -1 if nothing is found
static int valid_signature(const struct xdag_block *b, int signo_r, int keysLength, struct xdag_public_key *keys)
{
	struct xdag_block buf[2];
	xdag_hash_t hash;
	int i, signo_s = -1;

	memcpy(buf, b, sizeof(struct xdag_block));

	for(i = signo_r; i < XDAG_BLOCK_FIELDS; ++i) {
		if(xdag_type(b, i) == XDAG_FIELD_SIGN_IN || xdag_type(b, i) == XDAG_FIELD_SIGN_OUT) {
			memset(&buf[0].field[i], 0, sizeof(struct xdag_field));
			if(i > signo_r && signo_s < 0 && xdag_type(b, i) == xdag_type(b, signo_r)) {
				signo_s = i;
			}
		}
	}

	if(signo_s >= 0) {
		for(i = 0; i < keysLength; ++i) {
			hash_for_signature(buf, keys + i, hash);
			if(!xdag_verify_signature(keys[i].key, hash, b->field[signo_r].data, b->field[signo_s].data)) {
				return i;
			}
		}
	}

	return -1;
}

#define set_pretop(b) if ((b) && MAIN_TIME((b)->time) < MAIN_TIME(timestamp) && \
		(!pretop_main_chain || xdag_diff_gt((b)->difficulty, pretop_main_chain->difficulty))) { \
		pretop_main_chain = (b); \
		log_block("Pretop", (b)->hash, (b)->time, (b)->storage_pos); \
}

/* checks and adds a new block to the storage
 * returns:
 *		>0 = block was added
 *		0  = block exists
 *		<0 = error
 */
static int add_block_nolock(struct xdag_block *newBlock, xdag_time_t limit)
{
	const uint64_t timestamp = get_timestamp();
	uint64_t sum_in = 0, sum_out = 0, *psum;
	const uint64_t transportHeader = newBlock->field[0].transport_header;
	struct xdag_public_key public_keys[16], *our_keys = 0;
	int i, j;
	int keysCount = 0, ourKeysCount = 0;
	int signInCount = 0, signOutCount = 0;
	int signinmask = 0, signoutmask = 0;
	int inmask = 0, outmask = 0;
	int verified_keys_mask = 0, err, type;
	struct block_internal tmpNodeBlock, *blockRef, *blockRef0;
	xdag_diff_t diff0, diff;

	memset(&tmpNodeBlock, 0, sizeof(struct block_internal));
	newBlock->field[0].transport_header = 0;
	xdag_hash(newBlock, sizeof(struct xdag_block), tmpNodeBlock.hash);

	if(block_by_hash(tmpNodeBlock.hash)) return 0;

	if(xdag_type(newBlock, 0) != g_block_header_type) {
		i = xdag_type(newBlock, 0);
		err = 1;
		goto end;
	}

	tmpNodeBlock.time = newBlock->field[0].time;

	if(tmpNodeBlock.time > timestamp + MAIN_CHAIN_PERIOD / 4 || tmpNodeBlock.time < XDAG_ERA
		|| (limit && timestamp - tmpNodeBlock.time > limit)) {
		i = 0;
		err = 2;
		goto end;
	}

	if(!g_light_mode) {
		check_new_main();
	}

	for(i = 1; i < XDAG_BLOCK_FIELDS; ++i) {
		switch((type = xdag_type(newBlock, i))) {
			case XDAG_FIELD_NONCE:
				break;
			case XDAG_FIELD_IN:
				inmask |= 1 << i;
				break;
			case XDAG_FIELD_OUT:
				outmask |= 1 << i;
				break;
			case XDAG_FIELD_SIGN_IN:
				if(++signInCount & 1) {
					signinmask |= 1 << i;
				}
				break;
			case XDAG_FIELD_SIGN_OUT:
				if(++signOutCount & 1) {
					signoutmask |= 1 << i;
				}
				break;
			case XDAG_FIELD_PUBLIC_KEY_0:
			case XDAG_FIELD_PUBLIC_KEY_1:
				if((public_keys[keysCount].key = xdag_public_to_key(newBlock->field[i].data, type - XDAG_FIELD_PUBLIC_KEY_0))) {
					public_keys[keysCount++].pub = (uint64_t*)((uintptr_t)&newBlock->field[i].data | (type - XDAG_FIELD_PUBLIC_KEY_0));
				}
				break;
			default:
				err = 3;
				goto end;
		}
	}

	if(g_light_mode) {
		outmask = 0;
	}

	if(signOutCount & 1) {
		i = signOutCount;
		err = 4;
		goto end;
	}

	if(signOutCount) {
		our_keys = xdag_wallet_our_keys(&ourKeysCount);
	}

	for(i = 1; i < XDAG_BLOCK_FIELDS; ++i) {
		if(1 << i & (signinmask | signoutmask)) {
			int keyNumber = valid_signature(newBlock, i, keysCount, public_keys);
			if(keyNumber >= 0) {
				verified_keys_mask |= 1 << keyNumber;
			}
			if(1 << i & signoutmask && !(tmpNodeBlock.flags & BI_OURS) && (keyNumber = valid_signature(newBlock, i, ourKeysCount, our_keys)) >= 0) {
				tmpNodeBlock.flags |= BI_OURS;
				tmpNodeBlock.n_our_key = keyNumber;
			}
		}
	}

	for(i = j = 0; i < keysCount; ++i) {
		if(1 << i & verified_keys_mask) {
			if(i != j) {
				xdag_free_key(public_keys[j].key);
			}
			memcpy(public_keys + j++, public_keys + i, sizeof(struct xdag_public_key));
		}
	}

	keysCount = j;
	tmpNodeBlock.difficulty = diff0 = xdag_hash_difficulty(tmpNodeBlock.hash);
	sum_out += newBlock->field[0].amount;
	tmpNodeBlock.fee = newBlock->field[0].amount;

	for(i = 1; i < XDAG_BLOCK_FIELDS; ++i) {
		if(1 << i & (inmask | outmask)) {
			blockRef = block_by_hash(newBlock->field[i].hash);
			if(!blockRef) {
				err = 5;
				goto end;
			}
			if(blockRef->time >= tmpNodeBlock.time) {
				err = 6;
				goto end;
			}
			if(tmpNodeBlock.nlinks >= MAX_LINKS) {
				err = 7;
				goto end;
			}
			if(1 << i & inmask) {
				if(newBlock->field[i].amount) {
					int32_t res = check_signature_out(blockRef, public_keys, keysCount);
					if(res) {
						err = res;
						goto end;
					}
				}
				psum = &sum_in;
				tmpNodeBlock.in_mask |= 1 << tmpNodeBlock.nlinks;
			} else {
				psum = &sum_out;
			}

			if(*psum + newBlock->field[i].amount < *psum) {
				err = 0xA;
				goto end;
			}

			*psum += newBlock->field[i].amount;
			tmpNodeBlock.link[tmpNodeBlock.nlinks] = blockRef;
			tmpNodeBlock.linkamount[tmpNodeBlock.nlinks] = newBlock->field[i].amount;

			if(MAIN_TIME(blockRef->time) < MAIN_TIME(tmpNodeBlock.time)) {
				diff = xdag_diff_add(diff0, blockRef->difficulty);
			} else {
				diff = blockRef->difficulty;

				while(blockRef && MAIN_TIME(blockRef->time) == MAIN_TIME(tmpNodeBlock.time)) {
					blockRef = blockRef->link[blockRef->max_diff_link];
				}
				if(blockRef && xdag_diff_gt(xdag_diff_add(diff0, blockRef->difficulty), diff)) {
					diff = xdag_diff_add(diff0, blockRef->difficulty);
				}
			}

			if(xdag_diff_gt(diff, tmpNodeBlock.difficulty)) {
				tmpNodeBlock.difficulty = diff;
				tmpNodeBlock.max_diff_link = tmpNodeBlock.nlinks;
			}

			tmpNodeBlock.nlinks++;
		}
	}

	if(tmpNodeBlock.in_mask ? sum_in < sum_out : sum_out != newBlock->field[0].amount) {
		err = 0xB;
		goto end;
	}

	struct block_internal *nodeBlock = malloc(sizeof(struct block_internal));
	if(!nodeBlock) {
		err = 0xC;
		goto end;
	}

	if(!(transportHeader & (sizeof(struct xdag_block) - 1))) {
		tmpNodeBlock.storage_pos = transportHeader;
	} else {
		tmpNodeBlock.storage_pos = xdag_storage_save(newBlock);
	}

	memcpy(nodeBlock, &tmpNodeBlock, sizeof(struct block_internal));
	ldus_rbtree_insert(&root, &nodeBlock->node);
	g_xdag_stats.nblocks++;

	if(g_xdag_stats.nblocks > g_xdag_stats.total_nblocks) {
		g_xdag_stats.total_nblocks = g_xdag_stats.nblocks;
	}

	set_pretop(nodeBlock);
	set_pretop(top_main_chain);

	if(xdag_diff_gt(tmpNodeBlock.difficulty, g_xdag_stats.difficulty)) {
		/* Only log this if we are NOT loading state */
		if(xdag_get_state() != XDAG_STATE_LOAD)
			xdag_info("Diff  : %llx%016llx (+%llx%016llx)", xdag_diff_args(tmpNodeBlock.difficulty), xdag_diff_args(diff0));

		for(blockRef = nodeBlock, blockRef0 = 0; blockRef && !(blockRef->flags & BI_MAIN_CHAIN); blockRef = blockRef->link[blockRef->max_diff_link]) {
			if((!blockRef->link[blockRef->max_diff_link] || xdag_diff_gt(blockRef->difficulty, blockRef->link[blockRef->max_diff_link]->difficulty))
				&& (!blockRef0 || MAIN_TIME(blockRef0->time) > MAIN_TIME(blockRef->time))) {
				blockRef->flags |= BI_MAIN_CHAIN;
				blockRef0 = blockRef;
			}
		}

		if(blockRef && blockRef0 && blockRef != blockRef0 && MAIN_TIME(blockRef->time) == MAIN_TIME(blockRef0->time)) {
			blockRef = blockRef->link[blockRef->max_diff_link];
		}

		unwind_main(blockRef);
		top_main_chain = nodeBlock;
		g_xdag_stats.difficulty = tmpNodeBlock.difficulty;

		if(xdag_diff_gt(g_xdag_stats.difficulty, g_xdag_stats.max_difficulty)) {
			g_xdag_stats.max_difficulty = g_xdag_stats.difficulty;
		}
	}

	if(tmpNodeBlock.flags & BI_OURS) {
		nodeBlock->ourprev = ourlast;
		*(ourlast ? &ourlast->ournext : &ourfirst) = nodeBlock;
		ourlast = nodeBlock;
	}

	for(i = 0; i < tmpNodeBlock.nlinks; ++i) {
		if(!(tmpNodeBlock.link[i]->flags & BI_REF)) {
			for(blockRef0 = 0, blockRef = noref_first; blockRef != tmpNodeBlock.link[i]; blockRef0 = blockRef, blockRef = blockRef->ref) {
				;
			}

			*(blockRef0 ? &blockRef0->ref : &noref_first) = blockRef->ref;

			if(blockRef == noref_last) {
				noref_last = blockRef0;
			}

			blockRef->ref = 0;
			tmpNodeBlock.link[i]->flags |= BI_REF;
			g_xdag_extstats.nnoref--;
		}

		if(tmpNodeBlock.linkamount[i]) {
			blockRef = tmpNodeBlock.link[i];
			if(!blockRef->backrefs || blockRef->backrefs->backrefs[N_BACKREFS - 1]) {
				struct block_backrefs *back = malloc(sizeof(struct block_backrefs));
				if(!back) continue;
				memset(back, 0, sizeof(struct block_backrefs));
				back->next = blockRef->backrefs;
				blockRef->backrefs = back;
			}

			for(j = 0; blockRef->backrefs->backrefs[j]; ++j);

			blockRef->backrefs->backrefs[j] = nodeBlock;
		}
	}

	*(noref_last ? &noref_last->ref : &noref_first) = nodeBlock;
	noref_last = nodeBlock;
	g_xdag_extstats.nnoref++;

	log_block((tmpNodeBlock.flags & BI_OURS ? "Good +" : "Good  "), tmpNodeBlock.hash, tmpNodeBlock.time, tmpNodeBlock.storage_pos);

	i = MAIN_TIME(nodeBlock->time) & (HASHRATE_LAST_MAX_TIME - 1);
	if(MAIN_TIME(nodeBlock->time) > MAIN_TIME(g_xdag_extstats.hashrate_last_time)) {
		memset(g_xdag_extstats.hashrate_total + i, 0, sizeof(xdag_diff_t));
		memset(g_xdag_extstats.hashrate_ours + i, 0, sizeof(xdag_diff_t));
		g_xdag_extstats.hashrate_last_time = nodeBlock->time;
	}

	if(xdag_diff_gt(diff0, g_xdag_extstats.hashrate_total[i])) {
		g_xdag_extstats.hashrate_total[i] = diff0;
	}

	if(tmpNodeBlock.flags & BI_OURS && xdag_diff_gt(diff0, g_xdag_extstats.hashrate_ours[i])) {
		g_xdag_extstats.hashrate_ours[i] = diff0;
	}

	err = -1;

end:
	for(j = 0; j < keysCount; ++j) {
		xdag_free_key(public_keys[j].key);
	}

	if(err > 0) {
		char buf[32];
		err |= i << 4;
		sprintf(buf, "Err %2x", err & 0xff);
		log_block(buf, tmpNodeBlock.hash, tmpNodeBlock.time, transportHeader);
	}

	return -err;
}

static void *add_block_callback(void *block, void *data)
{
	struct xdag_block *b = (struct xdag_block *)block;
	xdag_time_t *t = (xdag_time_t*)data;
	int res;

	pthread_mutex_lock(&block_mutex);

	if(*t < XDAG_ERA) {
		(res = add_block_nolock(b, *t));
	} else if((res = add_block_nolock(b, 0)) >= 0 && b->field[0].time > *t) {
		*t = b->field[0].time;
	}

	pthread_mutex_unlock(&block_mutex);
	return 0;
}

/* checks and adds block to the storage. Returns non-zero value in case of error. */
int xdag_add_block(struct xdag_block *b)
{
	pthread_mutex_lock(&block_mutex);
	int res = add_block_nolock(b, time_limit);
	pthread_mutex_unlock(&block_mutex);

	return res;
}

#define setfld(fldtype, src, hashtype) ( \
		block[0].field[0].type |= (uint64_t)(fldtype) << (i << 2), \
			memcpy(&block[0].field[i++], (void*)(src), sizeof(hashtype)) \
		)

#define pretop_block() (top_main_chain && MAIN_TIME(top_main_chain->time) == MAIN_TIME(send_time) ? pretop_main_chain : top_main_chain)

/* create and publish a block
 * The first 'ninput' field 'fields' contains the addresses of the inputs and the corresponding quantity of XDAG,
 * in the following 'noutput' fields similarly - outputs, fee; send_time (time of sending the block);
 * if it is greater than the current one, then the mining is performed to generate the most optimal hash
 */
int xdag_create_block(struct xdag_field *fields, int inputsCount, int outputsCount, xdag_amount_t fee,
	xdag_time_t send_time, xdag_hash_t newBlockHashResult)
{
	struct xdag_block block[2];
	int i, j, res, mining, defkeynum, keysnum[XDAG_BLOCK_FIELDS], nkeys, nkeysnum = 0, outsigkeyind = -1;
	struct xdag_public_key *defkey = xdag_wallet_default_key(&defkeynum), *keys = xdag_wallet_our_keys(&nkeys), *key;
	xdag_hash_t signatureHash;
	xdag_hash_t newBlockHash;
	struct block_internal *ref, *pretop = pretop_block();

	for (i = 0; i < inputsCount; ++i) {
		ref = block_by_hash(fields[i].hash);
		if (!ref || !(ref->flags & BI_OURS)) {
			return -1;
		}

		for (j = 0; j < nkeysnum && ref->n_our_key != keysnum[j]; ++j);

		if (j == nkeysnum) {
			if (outsigkeyind < 0 && ref->n_our_key == defkeynum) {
				outsigkeyind = nkeysnum;
			}
			keysnum[nkeysnum++] = ref->n_our_key;
		}
	}

	int res0 = 1 + inputsCount + outputsCount + 3 * nkeysnum + (outsigkeyind < 0 ? 2 : 0);

	if (res0 > XDAG_BLOCK_FIELDS) {
		return -1;
	}

	if (!send_time) {
		send_time = get_timestamp();
		mining = 0;
	} else {
		mining = (send_time > get_timestamp() && res0 + 1 <= XDAG_BLOCK_FIELDS);
	}

	res0 += mining;

 begin:
	res = res0;
	memset(block, 0, sizeof(struct xdag_block));
	i = 1;
	block[0].field[0].type = g_block_header_type | (mining ? (uint64_t)XDAG_FIELD_SIGN_IN << ((XDAG_BLOCK_FIELDS - 1) * 4) : 0);
	block[0].field[0].time = send_time;
	block[0].field[0].amount = fee;

	if (g_light_mode) {
		if (res < XDAG_BLOCK_FIELDS && ourfirst) {
			setfld(XDAG_FIELD_OUT, ourfirst->hash, xdag_hashlow_t);
			res++;
		}
	} else {
		if (res < XDAG_BLOCK_FIELDS && mining && pretop && pretop->time < send_time) {
			log_block("Mintop", pretop->hash, pretop->time, pretop->storage_pos);
			setfld(XDAG_FIELD_OUT, pretop->hash, xdag_hashlow_t); res++;
		}

		for (ref = noref_first; ref && res < XDAG_BLOCK_FIELDS; ref = ref->ref) {
			if (ref->time < send_time) {
				setfld(XDAG_FIELD_OUT, ref->hash, xdag_hashlow_t); res++;
			}
		}
	}

	for (j = 0; j < inputsCount; ++j) {
		setfld(XDAG_FIELD_IN, fields + j, xdag_hash_t);
	}

	for (j = 0; j < outputsCount; ++j) {
		setfld(XDAG_FIELD_OUT, fields + inputsCount + j, xdag_hash_t);
	}

	for (j = 0; j < nkeysnum; ++j) {
		key = keys + keysnum[j];
		block[0].field[0].type |= (uint64_t)((j == outsigkeyind ? XDAG_FIELD_SIGN_OUT : XDAG_FIELD_SIGN_IN) * 0x11) << ((i + j + nkeysnum) * 4);
		setfld(XDAG_FIELD_PUBLIC_KEY_0 + ((uintptr_t)key->pub & 1), (uintptr_t)key->pub & ~1l, xdag_hash_t);
	}

	if(outsigkeyind < 0) {
		block[0].field[0].type |= (uint64_t)(XDAG_FIELD_SIGN_OUT * 0x11) << ((i + j + nkeysnum) * 4);
	}

	for (j = 0; j < nkeysnum; ++j, i += 2) {
		key = keys + keysnum[j];
		hash_for_signature(block, key, signatureHash);
		xdag_sign(key->key, signatureHash, block[0].field[i].data, block[0].field[i + 1].data);
	}

	if (outsigkeyind < 0) {
		hash_for_signature(block, defkey, signatureHash);
		xdag_sign(defkey->key, signatureHash, block[0].field[i].data, block[0].field[i + 1].data);
	}


	xdag_hash(block, sizeof(struct xdag_block), newBlockHash);
	block[0].field[0].transport_header = 1;

	log_block("Create", newBlockHash, block[0].field[0].time, 1);

	res = xdag_add_block(block);
	if (res > 0) {
		xdag_send_block_via_pool(block);

		if(newBlockHashResult != NULL) {
			memcpy(newBlockHashResult, newBlockHash, sizeof(xdag_hash_t));
		}
		res = 0;
	}

	return res;
}

/* start of regular block processing */
int xdag_blocks_start(void)
{
	pthread_mutexattr_t attr;

	if (g_xdag_testnet) {
		g_xdag_era = XDAG_TEST_ERA;
	} else {
		g_xdag_era = XDAG_MAIN_ERA;
	}

	g_light_mode = 1;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&block_mutex, &attr);

	// loading block from the local storage
	xdag_time_t t = XDAG_ERA;
	xdag_set_state(XDAG_STATE_LOAD);
	xdag_mess("Loading blocks from local storage...");
	uint64_t start = get_timestamp();
	xdag_load_blocks(t, get_timestamp(), &t, &add_block_callback);
	xdag_mess("Finish loading blocks, time cost %ldms", get_timestamp() - start);

	return 0;
}

/* returns our first block. If there is no blocks yet - the first block is created. */
int xdag_get_our_block(xdag_hash_t hash)
{
	pthread_mutex_lock(&block_mutex);
	struct block_internal *bi = ourfirst;
	pthread_mutex_unlock(&block_mutex);

	if (!bi) {
		xdag_create_block(0, 0, 0, 0, 0, NULL);
		pthread_mutex_lock(&block_mutex);
		bi = ourfirst;
		pthread_mutex_unlock(&block_mutex);
		if (!bi) {
			return -1;
		}
	}

	memcpy(hash, bi->hash, sizeof(xdag_hash_t));

	return 0;
}

/* calls callback for each own block */
int xdag_traverse_our_blocks(void *data, int (*callback)(void*, xdag_hash_t, xdag_amount_t, xdag_time_t, int))
{
	int res = 0;

	pthread_mutex_lock(&block_mutex);

	for (struct block_internal *bi = ourfirst; !res && bi; bi = bi->ournext) {
		res = (*callback)(data, bi->hash, bi->amount, bi->time, bi->n_our_key);
	}

	pthread_mutex_unlock(&block_mutex);

	return res;
}

static int (*g_traverse_callback)(void *data, xdag_hash_t hash, xdag_amount_t amount, xdag_time_t time);
static void *g_traverse_data;

static void traverse_all_callback(struct ldus_rbtree *node)
{
	struct block_internal *bi = (struct block_internal*)node;

	(*g_traverse_callback)(g_traverse_data, bi->hash, bi->amount, bi->time);
}

/* calls callback for each block */
int xdag_traverse_all_blocks(void *data, int (*callback)(void *data, xdag_hash_t hash,
	xdag_amount_t amount, xdag_time_t time))
{
	pthread_mutex_lock(&block_mutex);
	g_traverse_callback = callback;
	g_traverse_data = data;
	ldus_rbtree_walk_right(root, traverse_all_callback);
	pthread_mutex_unlock(&block_mutex);
	return 0;
}

/* returns current balance for specified address or balance for all addresses if hash == 0 */
xdag_amount_t xdag_get_balance(xdag_hash_t hash)
{
	if (!hash) {
		return g_balance;
	}

	pthread_mutex_lock(&block_mutex);
	struct block_internal *bi = block_by_hash(hash);
	pthread_mutex_unlock(&block_mutex);

	if (!bi) {
		return 0;
	}

	return bi->amount;
}

/* sets current balance for the specified address */
int xdag_set_balance(xdag_hash_t hash, xdag_amount_t balance)
{
	if (!hash) return -1;

	pthread_mutex_lock(&block_mutex);

	struct block_internal *bi = block_by_hash(hash);
	if (bi->flags & BI_OURS && bi != ourfirst) {
		if (bi->ourprev) {
			bi->ourprev->ournext = bi->ournext;
		} else {
			ourfirst = bi->ournext;
		}

		if (bi->ournext) {
			bi->ournext->ourprev = bi->ourprev;
		} else {
			ourlast = bi->ourprev;
		}

		bi->ourprev = 0;
		bi->ournext = ourfirst;

		if (ourfirst) {
			ourfirst->ourprev = bi;
		} else {
			ourlast = bi;
		}

		ourfirst = bi;
	}

	pthread_mutex_unlock(&block_mutex);

	if (!bi) return -1;

	if (bi->amount != balance) {
		xdag_hash_t hash0;
		xdag_amount_t diff;

		memset(hash0, 0, sizeof(xdag_hash_t));

		if (balance > bi->amount) {
			diff = balance - bi->amount;
			xdag_log_xfer(hash0, hash, diff);
			if (bi->flags & BI_OURS) {
				g_balance += diff;
			}
		} else {
			diff = bi->amount - balance;
			xdag_log_xfer(hash, hash0, diff);
			if (bi->flags & BI_OURS) {
				g_balance -= diff;
			}
		}

		bi->amount = balance;
	}

	return 0;
}

// returns position and time of block by hash
int64_t xdag_get_block_pos(const xdag_hash_t hash, xdag_time_t *t)
{
	pthread_mutex_lock(&block_mutex);
	struct block_internal *bi = block_by_hash(hash);
	pthread_mutex_unlock(&block_mutex);

	if (!bi) {
		return -1;
	}

	*t = bi->time;

	return bi->storage_pos;
}

//returns a number of key by hash of block, or -1 if block is not ours
int xdag_get_key(xdag_hash_t hash)
{
	pthread_mutex_lock(&block_mutex);
	struct block_internal *bi = block_by_hash(hash);
	pthread_mutex_unlock(&block_mutex);

	if (!bi || !(bi->flags & BI_OURS)) {
		return -1;
	}

	return bi->n_our_key;
}

/* reinitialization of block processing */
int xdag_blocks_reset(void)
{
	pthread_mutex_lock(&block_mutex);
	if (xdag_get_state() != XDAG_STATE_REST) {
		xdag_crit(error_storage_corrupted, "The local storage is corrupted. Resetting blocks engine.");
		xdag_set_state(XDAG_STATE_REST);
	}
	pthread_mutex_unlock(&block_mutex);

	return 0;
}

static void reset_callback(struct ldus_rbtree *node)
{
	free(node);
}

/* cleanup blocks */
int xdag_blocks_finish(void)
{
	pthread_mutex_lock(&block_mutex);
	ldus_rbtree_walk_up(root, reset_callback);

	root = 0;
	g_balance = 0;
	top_main_chain = pretop_main_chain = 0;
	ourfirst = ourlast = noref_first = noref_last = 0;
	memset(&g_xdag_stats, 0, sizeof(g_xdag_stats));
	memset(&g_xdag_extstats, 0, sizeof(g_xdag_extstats));

	return 0;
}

#define pramount(amount) xdag_amount2xdag(amount), xdag_amount2cheato(amount)

static int bi_compar(const void *l, const void *r)
{
	xdag_time_t tl = (*(struct block_internal **)l)->time, tr = (*(struct block_internal **)r)->time;

	return (tl < tr) - (tl > tr);
}

int32_t check_signature_out(struct block_internal* blockRef, struct xdag_public_key *public_keys, const int keysCount)
{
	struct xdag_block buf;
	struct xdag_block *bref = xdag_storage_load(blockRef->hash, blockRef->time, blockRef->storage_pos, &buf);
	if(!bref) {
		return 8;
	}
	return find_and_verify_signature_out(bref, public_keys, keysCount);
}

static int32_t find_and_verify_signature_out(struct xdag_block* bref, struct xdag_public_key *public_keys, const int keysCount)
{
	int j = 0;
	for(int k = 0; j < XDAG_BLOCK_FIELDS; ++j) {
		if(xdag_type(bref, j) == XDAG_FIELD_SIGN_OUT && (++k & 1)
			&& valid_signature(bref, j, keysCount, public_keys) >= 0) {
			break;
		}
	}
	if(j == XDAG_BLOCK_FIELDS) {
		return 9;
	}
	return 0;
}

int xdag_get_transactions(xdag_hash_t hash, void *data, int (*callback)(void*, int, xdag_hash_t, xdag_amount_t, xdag_time_t))
{
	pthread_mutex_lock(&block_mutex);
	struct block_internal *bi = block_by_hash(hash);
	pthread_mutex_unlock(&block_mutex);
	
	if (!bi) {
		return -1;
	}
	
	int size = 0x10000; 
	int n = 0;
	struct block_internal **block_array = malloc(size * sizeof(struct block_internal *));
	
	if (!block_array) return -1;
	
	int i;
	for (struct block_backrefs *br = bi->backrefs; br; br = br->next) {
		for (i = N_BACKREFS; i && !br->backrefs[i - 1]; i--);
		
		if (!i) {
			continue;
		}
		
		if (n + i > size) {
			size *= 2;
			struct block_internal **tmp_array = realloc(block_array, size * sizeof(struct block_internal *));
			if (!tmp_array) {
				free(block_array);
				return -1;
			}
			
			block_array = tmp_array;
		}
		
		memcpy(block_array + n, br->backrefs, i * sizeof(struct block_internal *));
		n += i;
	}
	
	if (!n) {
		free(block_array);
		return 0;
	}
	
	qsort(block_array, n, sizeof(struct block_internal *), bi_compar);
	
	for (i = 0; i < n; ++i) {
		if (!i || block_array[i] != block_array[i - 1]) {
			struct block_internal *ri = block_array[i];
			if (ri->flags & BI_APPLIED) {
				for (int j = 0; j < ri->nlinks; j++) {
					if(ri->link[j] == bi && ri->linkamount[j]) {
						if(callback(data, 1 << j & ri->in_mask, ri->hash, ri->linkamount[j], ri->time)) {
							free(block_array);
							return 0;
						}
					}
				}
			}
		}
	}
	
	free(block_array);
	
	return n;
}
