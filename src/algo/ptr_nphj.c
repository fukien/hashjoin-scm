#ifdef RUN_PAYLOAD

#include <libpmem.h>

#include "ptr_nphj.h"
#include "../inc/memaccess.h"

#define __USE_GNU
#include <sched.h>
#include <pthread.h>

#ifdef TEST_SELECTIVITY
#include <stdbool.h>
#endif /* TEST_SELECTIVITY */

extern char dump_dir[CHAR_BUFFER_LEN];
extern int local_cores[SINGLE_SOCKET_CORE_NUM];
extern int remote_cores[SINGLE_SOCKET_CORE_NUM];

#ifdef USE_PAPI
#include "../papi/mypapi.h"
extern char* PAPI_event_names[NUM_PAPI_EVENT];
extern long long agg_PAPI_values[NUM_PAPI_EVENT];
extern long long agg_PAPI_values_buildpart[NUM_PAPI_EVENT];
extern long long agg_PAPI_values_probejoin[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
#include "../pmw/mypmw.h"
extern unsigned int NVMCount;
static MY_PMWATCH_OP_BUF_NODE PMWatch_output[MaxNVMNum];
static MY_PMWATCH_OP_BUF_NODE PMWatch_output_buildpart[MaxNVMNum];
static MY_PMWATCH_OP_BUF_NODE PMWatch_output_probejoin[MaxNVMNum];
static MY_PMWATCH_OP_BUF_NODE PMWatch_output_retrieve[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_buildpart[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_probejoin[MaxNVMNum];
#endif /* USE_PMWATCH */

extern int cur_t;

#ifdef NONTEMP_STORE_MATER
static inline void nontemp_store_swwcb(void *dst, void *src) {
#if SWWCB_SIZE == 64
	write_nt_64(dst, src);
#elif SWWCB_SIZE == 128
	memcpy_nt_128(dst, src);
#elif SWWCB_SIZE == 256
	memcpy_nt_256(dst, src);
#elif SWWCB_SIZE == 512
	memcpy_nt_512(dst, src);
#elif SWWCB_SIZE == 1024
	memcpy_nt_1024(dst, src);
#elif SWWCB_SIZE == 2048
	memcpy_nt_2048(dst, src);
#elif SWWCB_SIZE == 4096
	memcpy_nt_2048(dst, src);
	memcpy_nt_2048(dst+2048, src+2048);
#elif SWWCB_SIZE == 8192
	memcpy_nt_2048(dst, src);
	memcpy_nt_2048(dst+2048, src+2048);
	memcpy_nt_2048(dst+4096, src+4096);
	memcpy_nt_2048(dst+6144, src+6144);
#else 	/* default setting*/

#ifdef USE_NVM
	/* NVM default setting, 256B */
	memcpy_nt_256(dst, src);
#else /* USE_NVM */
	/* DRAM default setting, 64B */
	write_nt_64(dst, src);
#endif /* USE_NVM */

#endif /* SWWCB_SIZE == 64 */

#ifdef ENABLE_FENCE
	sfence();
#endif /* ENABLE_FENCE */
}
#endif /* NONTEMP_STORE_MATER */


typedef struct ptr_nphj_sc_arg_t {
	int _tid;
	pthread_barrier_t *barrier;

	relation_t r_rel;
	relation_t s_rel;

	ptr_hashtable_t *hashtable;
	ptr_bucket_buffer_t *overflowbuf;

	size_t matched_cnt;
	rowid_t checksum;

	struct timespec build_start, build_end, probe_start, probe_end, retrieve_start, retrieve_end;

#ifdef SYNCSTATS
	ptr_synctimer_nphj_t *localtimer;
	ptr_synctimer_nphj_t *globaltimer;
#endif /* SYNCSTATS */

	row_id_pair_t **mater;
	size_t *mater_cnt;

	size_t row_id_pairs_processed;

	relation_t *org_relR;
	relation_t *org_relS;

#ifdef USE_PAPI
	long long PAPI_values[NUM_PAPI_EVENT];
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
	long long PAPI_values_retrieve[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

} ptr_nphj_sc_arg_t;


typedef struct ptr_nphj_lp_arg_t {
	int _tid;
	pthread_barrier_t *barrier;

	relation_t r_rel;
	relation_t s_rel;
	
	ptr_hash_entry_t *hashtable;
	size_t num_entrys;

	size_t matched_cnt;
	rowid_t checksum;

	struct timespec build_start, build_end, probe_start, probe_end, retrieve_start, retrieve_end;

#ifdef SYNCSTATS
	ptr_synctimer_nphj_t *localtimer;
	ptr_synctimer_nphj_t *globaltimer;
#endif /* SYNCSTATS */

	row_id_pair_t **mater;
	size_t *mater_cnt;

	size_t row_id_pairs_processed;

	relation_t *org_relR;
	relation_t *org_relS;

#ifdef USE_PAPI
	long long PAPI_values[NUM_PAPI_EVENT];
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
	long long PAPI_values_retrieve[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

} ptr_nphj_lp_arg_t;


static key_t_ hashmask;
static key_t_ bitskip;

void ptr_init_bucket_buffer(ptr_bucket_buffer_t ** ppbuf) {
	ptr_bucket_buffer_t * overflowbuf;
	overflowbuf = (ptr_bucket_buffer_t *) alloc_memory(sizeof(ptr_bucket_buffer_t), CACHELINE_SIZE);
	overflowbuf->count = 0;
	overflowbuf->next = NULL;
	*ppbuf = overflowbuf;
}

void ptr_get_new_bucket(ptr_bucket_t ** result, ptr_bucket_buffer_t ** buf) {
	if( (*buf)->count < OVERFLOW_BUF_SIZE ) {
		*result = (*buf)->buf + (*buf)->count;
		(*buf)->count ++;
	} else {
		/* need to allocate new buffer */
		ptr_bucket_buffer_t * new_buf = (ptr_bucket_buffer_t*) alloc_memory(sizeof(ptr_bucket_buffer_t), CACHELINE_SIZE);
		new_buf->count = 1;
		new_buf->next  = *buf;
		*buf	= new_buf;
		*result = new_buf->buf;
	}
}


void ptr_free_bucket_buffer(ptr_bucket_buffer_t *overflowbuf) {
	size_t size = sizeof(ptr_bucket_buffer_t);
	do {
		ptr_bucket_buffer_t * tmp = overflowbuf->next;
		dealloc_memory(overflowbuf, size);
		overflowbuf = tmp;
	} while (overflowbuf);
}


void ptr_build_hashtable_nphj_sc(ptr_hashtable_t *ht, relation_t * rel, ptr_bucket_buffer_t ** overflowbuf, int _tid) {
	keyrid_t *dest;
	ptr_bucket_t *curr, *nxt;
	hashkey_t tmp_hashkey;

#ifdef PREFETCH_DISTANCE
	size_t prefetch_index = next_pow_2(PREFETCH_DISTANCE);
	hashkey_t hashkey_prefetch;

#ifdef PREFETCH_CACHE
	key_t_ cachemask = prefetch_index - 1;
	size_t cache_idx;
	ptr_sc_prefetch_cache_t *sc_prefetch_cache;
	size_t cache_jdx;
#ifdef PREFETCH_CACHE_NVM
	sc_prefetch_cache = (ptr_sc_prefetch_cache_t *) new_alloc_nvm(sizeof(ptr_sc_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	sc_prefetch_cache = (ptr_sc_prefetch_cache_t *) alloc_dram(sizeof(ptr_sc_prefetch_cache_t) * prefetch_index, CACHELINE_SIZE);
#endif /* PREFETCH_CACHE_NVM */

	for (size_t idx = 0; idx < prefetch_index; idx ++) {
		cache_idx = idx & cachemask;
		(sc_prefetch_cache + cache_idx)->hashkey = IDHASH(rel->tuples[idx].key, hashmask, bitskip);
		hashkey_prefetch = (sc_prefetch_cache + cache_idx)->hashkey;
		__builtin_prefetch(ht->buckets + hashkey_prefetch, 1, 1);
	}

	cache_jdx = prefetch_index;

	for (size_t idx = 0; idx < rel->tuple_num - prefetch_index; idx ++) {

		/** insert **/ 
		cache_idx = idx & cachemask;
		tmp_hashkey = sc_prefetch_cache[cache_idx].hashkey;
		curr = ht->buckets + tmp_hashkey;

		lock(&curr->latch);
		nxt = curr->next;	
		if(curr->count == BUCKET_CAPACITY) {
			if(!nxt || nxt->count == BUCKET_CAPACITY) {
				ptr_bucket_t * b;
				ptr_get_new_bucket(&b, overflowbuf);
				curr->next = b;
				b->next	= nxt;
				b->count = 1;
				dest = b->keyrids;
			} else {
				dest = nxt->keyrids + nxt->count;
				(nxt->count) ++;
			}
		} else {
			dest = curr->keyrids + curr->count;
			(curr->count) ++;
		}

		unlock(&curr->latch);

		store_a_keyrid(dest, &rel->tuples[idx]);

		/** prefetch **/ 
		(sc_prefetch_cache + cache_idx)->hashkey = IDHASH(rel->tuples[cache_jdx].key, hashmask, bitskip);
		hashkey_prefetch = (sc_prefetch_cache + cache_idx)->hashkey;
		__builtin_prefetch(ht->buckets + hashkey_prefetch, 1, 1);
		cache_jdx ++;
	}

	for (size_t idx = rel->tuple_num - prefetch_index; idx < rel->tuple_num; idx ++) {
		cache_idx  = idx & cachemask;
		tmp_hashkey = sc_prefetch_cache[cache_idx].hashkey;
		curr = ht->buckets + tmp_hashkey;

		lock(&curr->latch);
		nxt = curr->next;	
		if(curr->count == BUCKET_CAPACITY) {
			if(!nxt || nxt->count == BUCKET_CAPACITY) {
				ptr_bucket_t * b;
				ptr_get_new_bucket(&b, overflowbuf);
				curr->next = b;
				b->next	= nxt;
				b->count = 1;
				dest = b->keyrids;
			} else {
				dest = nxt->keyrids + nxt->count;
				(nxt->count) ++;
			}
		} else {
			dest = curr->keyrids + curr->count;
			(curr->count) ++;
		}

		unlock(&curr->latch);

		store_a_keyrid(dest, &rel->tuples[idx]);
	}

#ifdef PREFETCH_CACHE_NVM
	pmem_unmap(sc_prefetch_cache, sizeof(ptr_sc_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	dealloc_dram(sc_prefetch_cache, sizeof(ptr_sc_prefetch_cache_t) * prefetch_index);
#endif /* PREFETCH_CACHE_NVM */

#else /* PREFETCH_CACHE */

	for (size_t idx = 0; idx < rel->tuple_num; idx ++) {

		if (prefetch_index < rel->tuple_num) {
			hashkey_prefetch = IDHASH(rel->tuples[prefetch_index++].key, hashmask, bitskip);
			__builtin_prefetch(ht->buckets + hashkey_prefetch, 1, 1);
		}

		tmp_hashkey = IDHASH(rel->tuples[idx].key, hashmask, bitskip);
		curr = ht->buckets + tmp_hashkey;
		lock(&curr->latch);
		nxt = curr->next;

		if(curr->count == BUCKET_CAPACITY) {
			if(!nxt || nxt->count == BUCKET_CAPACITY) {
				ptr_bucket_t * b;
				ptr_get_new_bucket(&b, overflowbuf);
				curr->next = b;
				b->next	= nxt;
				b->count = 1;
				dest = b->keyrids;
			} else {
				dest = nxt->keyrids + nxt->count;
				(nxt->count) ++;
			}
		} else {
			dest = curr->keyrids + curr->count;
			(curr->count) ++;
		}

		unlock(&curr->latch);

		store_a_keyrid(dest, &rel->tuples[idx]);
	}

#endif /* PREFETCH_CACHE */
#else /* PREFETCH_DISTANCE */

	for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
		tmp_hashkey = IDHASH(rel->tuples[idx].key, hashmask, bitskip);
		curr = ht->buckets + tmp_hashkey;
		lock(&curr->latch);
		nxt = curr->next;

		if(curr->count == BUCKET_CAPACITY) {
			if(!nxt || nxt->count == BUCKET_CAPACITY) {
				ptr_bucket_t * b;
				ptr_get_new_bucket(&b, overflowbuf);
				curr->next = b;
				b->next	= nxt;
				b->count = 1;
				dest = b->keyrids;
			} else {
				dest = nxt->keyrids + nxt->count;
				(nxt->count) ++;
			}
		} else {
			dest = curr->keyrids + curr->count;
			(curr->count) ++;
		}

		unlock(&curr->latch);

		store_a_keyrid(dest, &rel->tuples[idx]);
	}

#endif /* PREFETCH_DISTANCE */
}


void ptr_probe_hashtable_nphj_sc(ptr_hashtable_t *ht, relation_t *rel, size_t *matched_cnt, row_id_pair_t *mater) {
	hashkey_t tmp_hashkey;
	ptr_bucket_t *b;

#ifdef NONTEMP_STORE_MATER
	size_t slot_mod;
	row_id_pair_swwcb_t *row_id_pair_swwcb;
	row_id_pair_t *tmp_row_id_pair_swwcb;
#ifdef NONTEMP_STORE_MATER_ON_NVM
	row_id_pair_swwcb = (row_id_pair_swwcb_t *)new_alloc_nvm(sizeof(row_id_pair_swwcb_t));
	pmem_memset_persist(row_id_pair_swwcb, 0, sizeof(row_id_pair_swwcb_t));
#else /* NONTEMP_STORE_MATER_ON_NVM */
	row_id_pair_swwcb = (row_id_pair_swwcb_t *)alloc_aligned_dram(sizeof(row_id_pair_swwcb_t), CACHELINE_SIZE);
	memset(row_id_pair_swwcb, 0, sizeof(row_id_pair_swwcb_t));
#endif /* NONTEMP_STORE_MATER_ON_NVM */
#else /* NONTEMP_STORE_MATER */
	row_id_pair_t tmp_row_id_pair;
#endif /* NONTEMP_STORE_MATER */

#ifdef PREFETCH_DISTANCE
	size_t prefetch_index = next_pow_2(PREFETCH_DISTANCE);
	hashkey_t hashkey_prefetch;

#ifdef PREFETCH_CACHE
	key_t_ cachemask = prefetch_index - 1;
	size_t cache_idx;
	ptr_sc_prefetch_cache_t *sc_prefetch_cache;
	size_t cache_jdx;

#ifdef PREFETCH_CACHE_NVM
	sc_prefetch_cache = (ptr_sc_prefetch_cache_t *) new_alloc_nvm(sizeof(ptr_sc_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	sc_prefetch_cache = (ptr_sc_prefetch_cache_t *) alloc_dram(sizeof(ptr_sc_prefetch_cache_t) * prefetch_index, CACHELINE_SIZE);
#endif /* PREFETCH_CACHE_NVM */

	for (size_t idx = 0; idx < prefetch_index; idx ++) {
		cache_idx = idx & cachemask;
		(sc_prefetch_cache + cache_idx)->hashkey = IDHASH(rel->tuples[idx].key, hashmask, bitskip);
		hashkey_prefetch = (sc_prefetch_cache + cache_idx)->hashkey;
		__builtin_prefetch(ht->buckets + hashkey_prefetch, 0, 1);
	}

	cache_jdx = prefetch_index;
	for (size_t idx = 0; idx < rel->tuple_num - prefetch_index; idx ++) {

		/** lookup **/ 
		cache_idx = idx & cachemask;
		tmp_hashkey = sc_prefetch_cache[cache_idx].hashkey;
		b = ht->buckets + tmp_hashkey;

		do {
			for (size_t jdx = 0; jdx < b->count; jdx++) {
				if (rel->tuples[idx].key == b->keyrids[jdx].key){
#ifdef NONTEMP_STORE_MATER
					slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
					tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
					tmp_row_id_pair_swwcb[slot_mod].r_row_id = b->keyrids[jdx].row_id;
					tmp_row_id_pair_swwcb[slot_mod].s_row_id = rel->tuples[idx].row_id;
					if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
						/* non-temporal store a SWWCB */
						nontemp_store_swwcb(mater+((*matched_cnt)-(ROW_ID_PAIR_PER_SWWCB-1)), tmp_row_id_pair_swwcb);
					}
#else /* NONTEMP_STORE_MATER */
					tmp_row_id_pair.r_row_id = b->keyrids[jdx].row_id;
					tmp_row_id_pair.s_row_id = rel->tuples[idx].row_id;
					store_a_row_id_pair(mater+(*matched_cnt), &tmp_row_id_pair);
#endif /* NONTEMP_STORE_MATER */
					(*matched_cnt) ++;
				}
			}

			b = b->next;/* follow overflow pointer */
		} while (b);

		/** prefetch **/ 
		(sc_prefetch_cache + cache_idx)->hashkey = IDHASH(rel->tuples[cache_jdx].key, hashmask, bitskip);
		hashkey_prefetch = (sc_prefetch_cache + cache_idx)->hashkey;
		__builtin_prefetch(ht->buckets + hashkey_prefetch, 0, 1);
		cache_jdx ++;
	}


	for (size_t idx = rel->tuple_num - prefetch_index; idx < rel->tuple_num; idx ++) {
		cache_idx = idx & cachemask;
		tmp_hashkey = sc_prefetch_cache[cache_idx].hashkey;
		b = ht->buckets + tmp_hashkey;
		do {
			for (size_t jdx = 0; jdx < b->count; jdx++) {
				if (rel->tuples[idx].key == b->keyrids[jdx].key){
#ifdef NONTEMP_STORE_MATER
					slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
					tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
					tmp_row_id_pair_swwcb[slot_mod].r_row_id = b->keyrids[jdx].row_id;
					tmp_row_id_pair_swwcb[slot_mod].s_row_id = rel->tuples[idx].row_id;
					if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
						/* non-temporal store a SWWCB */
						nontemp_store_swwcb(mater+((*matched_cnt)-(ROW_ID_PAIR_PER_SWWCB-1)), tmp_row_id_pair_swwcb);
					}
#else /* NONTEMP_STORE_MATER */
					tmp_row_id_pair.r_row_id = b->keyrids[jdx].row_id;
					tmp_row_id_pair.s_row_id = rel->tuples[idx].row_id;
					store_a_row_id_pair(mater+(*matched_cnt), &tmp_row_id_pair);
#endif /* NONTEMP_STORE_MATER */
					(*matched_cnt) ++;
				}
			}

			/* follow overflow pointer */
			b = b->next;
		} while (b);

	}

#ifdef NONTEMP_STORE_MATER
	slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
	for (size_t idx = 0; idx < slot_mod; idx ++) {
		store_a_row_id_pair(mater+(*matched_cnt)-slot_mod+idx, &row_id_pair_swwcb->data.row_id_pairs[idx]);
	}
#endif /* NONTEMP_STORE_MATER */

#ifdef PREFETCH_CACHE_NVM
	pmem_unmap(sc_prefetch_cache, sizeof(ptr_sc_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	dealloc_dram(sc_prefetch_cache, sizeof(ptr_sc_prefetch_cache_t) * prefetch_index);
#endif /* PREFETCH_CACHE_NVM */

#else /* PREFETCH_CACHE */
	for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
		if (prefetch_index < rel->tuple_num) {
			hashkey_prefetch = IDHASH(rel->tuples[prefetch_index++].key, hashmask, bitskip);
			__builtin_prefetch(ht->buckets + hashkey_prefetch, 0, 1);
		}

		tmp_hashkey = IDHASH(rel->tuples[idx].key, hashmask, bitskip);
		b = ht->buckets + tmp_hashkey;

		do {
			for (size_t jdx = 0; jdx < b->count; jdx++) {
				if (rel->tuples[idx].key == b->keyrids[jdx].key){
#ifdef NONTEMP_STORE_MATER
					slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
					tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
					tmp_row_id_pair_swwcb[slot_mod].r_row_id = b->keyrids[jdx].row_id;
					tmp_row_id_pair_swwcb[slot_mod].s_row_id = rel->tuples[idx].row_id;
					if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
						/* non-temporal store a SWWCB */
						nontemp_store_swwcb(mater+((*matched_cnt)-(ROW_ID_PAIR_PER_SWWCB-1)), tmp_row_id_pair_swwcb);
					}
#else /* NONTEMP_STORE_MATER */
					tmp_row_id_pair.r_row_id = b->keyrids[jdx].row_id;
					tmp_row_id_pair.s_row_id = rel->tuples[idx].row_id;
					store_a_row_id_pair(mater+(*matched_cnt), &tmp_row_id_pair);
#endif /* NONTEMP_STORE_MATER */
					(*matched_cnt) ++;
				}
			}
			
			/* follow overflow pointer */
			b = b->next;
		} while (b);
	}

#ifdef NONTEMP_STORE_MATER
	slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
	for (size_t idx = 0; idx < slot_mod; idx ++) {
		store_a_row_id_pair(mater+(*matched_cnt)-slot_mod+idx, &row_id_pair_swwcb->data.row_id_pairs[idx]);
	}
#endif /* NONTEMP_STORE_MATER */

#endif /* PREFETCH_CACHE */
#else /* PREFETCH_DISTANCE */
	for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
		tmp_hashkey = IDHASH(rel->tuples[idx].key, hashmask, bitskip);
		b = ht->buckets + tmp_hashkey;
		do {
			for (size_t jdx = 0; jdx < b->count; jdx++) {

				if (rel->tuples[idx].key == b->keyrids[jdx].key){
#ifdef NONTEMP_STORE_MATER
					slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
					tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
					tmp_row_id_pair_swwcb[slot_mod].r_row_id = b->keyrids[jdx].row_id;
					tmp_row_id_pair_swwcb[slot_mod].s_row_id = rel->tuples[idx].row_id;
					if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
						/* non-temporal store a SWWCB */
						nontemp_store_swwcb(mater+((*matched_cnt)-(ROW_ID_PAIR_PER_SWWCB-1)), tmp_row_id_pair_swwcb);
					}
#else /* NONTEMP_STORE_MATER */
					tmp_row_id_pair.r_row_id = b->keyrids[jdx].row_id;
					tmp_row_id_pair.s_row_id = rel->tuples[idx].row_id;
					store_a_row_id_pair(mater+(*matched_cnt), &tmp_row_id_pair);
#endif /* NONTEMP_STORE_MATER */
					(*matched_cnt) ++;
				}
			}

			/* follow overflow pointer */
			b = b->next;
		} while (b);
	}
#ifdef NONTEMP_STORE_MATER
	slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
	for (size_t idx = 0; idx < slot_mod; idx ++) {
		store_a_row_id_pair(mater+(*matched_cnt)-slot_mod+idx, &row_id_pair_swwcb->data.row_id_pairs[idx]);
	}
#endif /* NONTEMP_STORE_MATER */
#endif /* PREFETCH_DISTANCE */

#ifdef NONTEMP_STORE_MATER
#ifdef NONTEMP_STORE_MATER_ON_NVM
	pmem_unmap(row_id_pair_swwcb, sizeof(row_id_pair_swwcb_t));
#else /* NONTEMP_STORE_MATER_ON_NVM */
	free(row_id_pair_swwcb);
#endif /* NONTEMP_STORE_MATER_ON_NVM */
#endif /* NONTEMP_STORE_MATER */
}


void *ptr_run_nphj_sc_buildpart(void *arg) {
	ptr_nphj_sc_arg_t *nphj_sc_arg = (ptr_nphj_sc_arg_t *) arg;

#ifdef WARMUP
	warmup_localize(nphj_sc_arg->r_rel.tuples, nphj_sc_arg->r_rel.tuple_num);
#endif /* WARMUP */

	/* eliminate the page faults of the hash table */
	size_t memset_num_buckets_thr = nphj_sc_arg->hashtable->num_buckets / BUILDPART_THREAD_NUM;
	size_t memset_num_buckets_size = (nphj_sc_arg->_tid == BUILDPART_THREAD_NUM -1) ? 
		sizeof(ptr_bucket_t) * (nphj_sc_arg->hashtable->num_buckets - nphj_sc_arg->_tid * memset_num_buckets_thr) : 
		sizeof(ptr_bucket_t) * memset_num_buckets_thr;
	memset_localize(nphj_sc_arg->hashtable->buckets + nphj_sc_arg->_tid * memset_num_buckets_thr, 
		memset_num_buckets_size );

	ptr_bucket_buffer_t *overflowbuf;
	ptr_init_bucket_buffer(&overflowbuf);

	int rv;

#ifdef USE_PAPI
	int eventset = PAPI_NULL;

	rv = PAPI_create_eventset(&eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Assign it to the CPU component */
	rv = PAPI_assign_eventset_component(eventset, 0);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Convert the EventSet to a multiplexed event set */
	rv = PAPI_set_multiplex(eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		rv = PAPI_add_named_event(eventset, PAPI_event_names[idx]);
		if (rv != PAPI_OK) {
			ERROR_RETURN(rv);
			exit(1);
		}		
	}

	PAPI_reset(eventset);
	PAPI_start(eventset);
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
	if (nphj_sc_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */

	BARRIER_ARRIVE(nphj_sc_arg->barrier, rv);

	if (nphj_sc_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_sc_arg->build_start);
	}

	// /* eliminate the page faults of the hash table */
	// size_t memset_num_buckets_thr = nphj_sc_arg->hashtable->num_buckets / BUILDPART_THREAD_NUM;
	// size_t memset_num_buckets_size = (nphj_sc_arg->_tid == BUILDPART_THREAD_NUM -1) ? 
	// 	sizeof(ptr_bucket_t) * (nphj_sc_arg->hashtable->num_buckets - nphj_sc_arg->_tid * memset_num_buckets_thr) : 
	// 	sizeof(ptr_bucket_t) * memset_num_buckets_thr;
	// memset_localize(nphj_sc_arg->hashtable->buckets + nphj_sc_arg->_tid * memset_num_buckets_thr, 
	// 	memset_num_buckets_size );

	// /** intiate overflow buffers **/
	// ptr_bucket_buffer_t *overflowbuf;
	// init_bucket_buffer(&overflowbuf);


#ifdef SYNCSTATS
	SYNC_TIMER(nphj_sc_arg->localtimer->sync_pre_memset);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(nphj_sc_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(nphj_sc_arg->_tid, nphj_sc_arg->globaltimer->sync_pre_memset);
#endif /* SYNCSTATS */

	/** build the hash table **/
	ptr_build_hashtable_nphj_sc(nphj_sc_arg->hashtable, &nphj_sc_arg->r_rel, &overflowbuf, nphj_sc_arg->_tid);

#ifdef SYNCSTATS
	SYNC_TIMER(nphj_sc_arg->localtimer->sync_build);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(nphj_sc_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(nphj_sc_arg->_tid, nphj_sc_arg->globaltimer->sync_build);
#endif /* SYNCSTATS */

	if (nphj_sc_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_sc_arg->build_end);
	}

#ifdef USE_PMWATCH
	if (nphj_sc_arg->_tid == 0) {
		pmwEnd(PMWatch_output_buildpart);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, nphj_sc_arg->PAPI_values_buildpart);
	/* Free all memory and data structures, eventset must be empty. */
	if ( (rv = PAPI_cleanup_eventset(eventset))!=PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	if ( (rv = PAPI_destroy_eventset(&eventset)) != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}
#endif /* USE_PAPI */

	nphj_sc_arg->overflowbuf = overflowbuf;

	return NULL;
}


void *ptr_run_nphj_sc_probejoin(void *arg) {
	ptr_nphj_sc_arg_t *nphj_sc_arg = (ptr_nphj_sc_arg_t *) arg;

#ifdef WARMUP
	warmup_localize(nphj_sc_arg->s_rel.tuples, nphj_sc_arg->s_rel.tuple_num);
#endif /* WARMUP */

	/* currently, we only consider the uniform pkfk scenario */
	size_t tot_row_id_pair_num = nphj_sc_arg->s_rel.tuple_num;
#ifdef USE_NVM
	row_id_pair_t *mater = (row_id_pair_t *) new_alloc_nvm(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);
#else /* USE_NVM */
#ifdef USE_HUGE
	row_id_pair_t *mater = (row_id_pair_t *) alloc_huge_dram(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	row_id_pair_t *mater = (row_id_pair_t *) alloc_aligned_dram(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	memset_localize(mater, ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);

	int rv;

#ifdef USE_PAPI
	int eventset = PAPI_NULL;

	rv = PAPI_create_eventset(&eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Assign it to the CPU component */
	rv = PAPI_assign_eventset_component(eventset, 0);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Convert the EventSet to a multiplexed event set */
	rv = PAPI_set_multiplex(eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		rv = PAPI_add_named_event(eventset, PAPI_event_names[idx]);
		if (rv != PAPI_OK) {
			ERROR_RETURN(rv);
			exit(1);
		}		
	}

	PAPI_reset(eventset);
	PAPI_start(eventset);
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
	if (nphj_sc_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */

	/** probe the hash table **/
	if (nphj_sc_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_sc_arg->probe_start);
	}

// 	/* currently, we only consider the uniform pkfk scenario */
// 	size_t tot_row_id_pair_num = nphj_sc_arg->s_rel.tuple_num;
// #ifdef USE_NVM
// 	row_id_pair_t *mater = (row_id_pair_t *) new_alloc_nvm(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);
// #else /* USE_NVM */
// 	row_id_pair_t *mater = (row_id_pair_t *) alloc_aligned_dram(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE, CACHELINE_SIZE);
// #endif /* USE_NVM */
// 	memset_localize(mater, ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);

	ptr_probe_hashtable_nphj_sc(nphj_sc_arg->hashtable, &nphj_sc_arg->s_rel, &nphj_sc_arg->matched_cnt, mater);

#ifdef SYNCSTATS
	SYNC_TIMER(nphj_sc_arg->localtimer->sync_probe);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(nphj_sc_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(nphj_sc_arg->_tid, nphj_sc_arg->globaltimer->sync_probe);
#endif /* SYNCSTATS */

	if (nphj_sc_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_sc_arg->probe_end);
	}

#ifdef USE_PMWATCH
	if (nphj_sc_arg->_tid == 0) {
		pmwEnd(PMWatch_output_probejoin);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, nphj_sc_arg->PAPI_values_probejoin);
	/* Free all memory and data structures, eventset must be empty. */
	if ( (rv = PAPI_cleanup_eventset(eventset))!=PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	if ( (rv = PAPI_destroy_eventset(&eventset)) != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}
#endif /* USE_PAPI */

	nphj_sc_arg->mater[nphj_sc_arg->_tid] = mater;

	return NULL;
}


void *ptr_run_nphj_sc_retrieve(void *arg) {
	ptr_nphj_sc_arg_t *nphj_sc_arg = (ptr_nphj_sc_arg_t *) arg;

	size_t start_offset = 0;
	size_t processed_num = 0;
	size_t processed_num_thr = 0;
	rowid_t tmp_checksum = 0;

	int rv;

#ifdef USE_PAPI
	int eventset = PAPI_NULL;

	rv = PAPI_create_eventset(&eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Assign it to the CPU component */
	rv = PAPI_assign_eventset_component(eventset, 0);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Convert the EventSet to a multiplexed event set */
	rv = PAPI_set_multiplex(eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		rv = PAPI_add_named_event(eventset, PAPI_event_names[idx]);
		if (rv != PAPI_OK) {
			ERROR_RETURN(rv);
			exit(1);
		}		
	}

	PAPI_reset(eventset);
	PAPI_start(eventset);
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
	if (nphj_sc_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */

	if (nphj_sc_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_sc_arg->retrieve_start);
	}

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		processed_num_thr = nphj_sc_arg->mater_cnt[idx] / RETRIEVE_THREAD_NUM;
		start_offset = nphj_sc_arg->_tid * processed_num_thr;
		processed_num = (nphj_sc_arg->_tid == RETRIEVE_THREAD_NUM - 1) ? 
			nphj_sc_arg->mater_cnt[idx] - nphj_sc_arg->_tid * processed_num_thr : processed_num_thr;
		ptr_retrieve(&tmp_checksum, nphj_sc_arg->mater[idx]+start_offset, processed_num, *nphj_sc_arg->org_relR, *nphj_sc_arg->org_relS);
		nphj_sc_arg->checksum += tmp_checksum;
		nphj_sc_arg->row_id_pairs_processed += processed_num;
	}

#ifdef SYNCSTATS
	SYNC_TIMER(nphj_sc_arg->localtimer->sync_retrieve);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(nphj_sc_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(nphj_sc_arg->_tid, nphj_sc_arg->globaltimer->sync_retrieve);
#endif /* SYNCSTATS */

	if (nphj_sc_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_sc_arg->retrieve_end);
	}

#ifdef USE_PMWATCH
	if (nphj_sc_arg->_tid == 0) {
		pmwEnd(PMWatch_output_retrieve);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, nphj_sc_arg->PAPI_values_retrieve);
	/* Free all memory and data structures, eventset must be empty. */
	if ( (rv = PAPI_cleanup_eventset(eventset))!=PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	if ( (rv = PAPI_destroy_eventset(&eventset)) != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}
#endif /* USE_PAPI */

	return NULL;
}


void ptr_nphj_sc(const datameta_t datameta, timekeeper_t * const timekeeper) {
	int rv;

	size_t matched_cnt = 0;
	rowid_t checksum = 0;

	/**************** LOAD DATA ****************/
	char r_path[CHAR_BUFFER_LEN], s_path[CHAR_BUFFER_LEN];
	snprintf(r_path, CHAR_BUFFER_LEN, "%s/%s", DATA_PATH_PREFIX, datameta.r_path_suffix);
	snprintf(s_path, CHAR_BUFFER_LEN, "%s/%s", DATA_PATH_PREFIX, datameta.s_path_suffix);

	tuple_t *r_original_addr = (tuple_t*) create_pmem_buffer(r_path, TUPLE_T_SIZE * datameta.r_tuple_num);
	tuple_t *s_original_addr = (tuple_t*) create_pmem_buffer(s_path, TUPLE_T_SIZE * datameta.s_tuple_num);

	relation_t r_relation, s_relation;

	r_relation.tuple_num = datameta.r_tuple_num;
	s_relation.tuple_num = datameta.s_tuple_num;

#ifdef USE_NVM
	r_relation.tuples = new_alloc_nvm(TUPLE_T_SIZE * datameta.r_tuple_num);
	s_relation.tuples = new_alloc_nvm(TUPLE_T_SIZE * datameta.s_tuple_num);
#else /* USE_NVM */
#ifdef USE_HUGE
	r_relation.tuples = (tuple_t*) alloc_huge_dram(TUPLE_T_SIZE * r_relation.tuple_num);
	s_relation.tuples = (tuple_t*) alloc_huge_dram(TUPLE_T_SIZE * s_relation.tuple_num);
#else /* USE_HUGE */
	r_relation.tuples = (tuple_t*) alloc_aligned_dram(TUPLE_T_SIZE * r_relation.tuple_num, CACHELINE_SIZE);
	s_relation.tuples = (tuple_t*) alloc_aligned_dram(TUPLE_T_SIZE * s_relation.tuple_num, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memcpy(r_relation.tuples, r_original_addr, TUPLE_T_SIZE * r_relation.tuple_num);
	parallel_memcpy(s_relation.tuples, s_original_addr, TUPLE_T_SIZE * s_relation.tuple_num);
	pmem_unmap(r_original_addr, TUPLE_T_SIZE * r_relation.tuple_num);
	pmem_unmap(s_original_addr, TUPLE_T_SIZE * s_relation.tuple_num);

	row_id_pair_t **mater = (row_id_pair_t **) alloc_memory( sizeof(row_id_pair_t *) * PROBEJOIN_THREAD_NUM, CACHELINE_SIZE );
	size_t mater_cnt[PROBEJOIN_THREAD_NUM];
	memset(mater_cnt, 0, sizeof(size_t)*PROBEJOIN_THREAD_NUM);

	int max_thread_num = MAX( MAX(BUILDPART_THREAD_NUM, PROBEJOIN_THREAD_NUM), RETRIEVE_THREAD_NUM );

	pthread_t threadpool[max_thread_num];
	ptr_nphj_sc_arg_t args[max_thread_num];


	pthread_barrier_t buildpart_barrier;
	rv = pthread_barrier_init(&buildpart_barrier, NULL, BUILDPART_THREAD_NUM);
	if(rv){
		printf("[ERROR] Couldn't create the buildpart_barrier\n");
		exit(EXIT_FAILURE);
	}
	pthread_barrier_t probejoin_barrier;
	rv = pthread_barrier_init(&probejoin_barrier, NULL, PROBEJOIN_THREAD_NUM);
	if(rv){
		printf("[ERROR] Couldn't create the probejoin_barrier\n");
		exit(EXIT_FAILURE);
	}
	pthread_barrier_t retrieve_barrier;
	rv = pthread_barrier_init(&retrieve_barrier, NULL, RETRIEVE_THREAD_NUM);
	if(rv){
		printf("[ERROR] Couldn't create the retrieve_barrier\n");
		exit(EXIT_FAILURE);
	}

#ifdef SYNCSTATS
	ptr_synctimer_nphj_t *localtimer = (ptr_synctimer_nphj_t *) malloc( max_thread_num * sizeof(ptr_synctimer_nphj_t) );
	ptr_synctimer_nphj_t *globaltimer = (ptr_synctimer_nphj_t *) malloc( sizeof(ptr_synctimer_nphj_t) );
	memset(localtimer, 0, max_thread_num * sizeof(ptr_synctimer_nphj_t));
	memset(globaltimer, 0, sizeof(ptr_synctimer_nphj_t));
#endif /* SYNCSTATS */

	cpu_set_t set;
	int cpu_idx;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	/** allocate a hashtable **/
	// ptr_hashtable_t* hashtable = (ptr_hashtable_t *) alloc_memory(sizeof(ptr_hashtable_t), CACHELINE_SIZE);
	ptr_hashtable_t hashtable;
	hashtable.num_buckets = datameta.r_tuple_num / BUCKET_CAPACITY;

#ifdef USE_NVM
	hashtable.buckets = (ptr_bucket_t *) new_alloc_nvm(sizeof(ptr_bucket_t) * hashtable.num_buckets);
#else /* USE_NVM */
#ifdef USE_HUGE
	hashtable.buckets = (ptr_bucket_t *) alloc_huge_dram(sizeof(ptr_bucket_t) * hashtable.num_buckets);
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	hashtable.buckets = (ptr_bucket_t *) alloc_aligned_dram(sizeof(ptr_bucket_t) * hashtable.num_buckets, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */


	hashmask = hashtable.num_buckets - 1;
	bitskip = 0;

	size_t r_tuple_num_thr = datameta.r_tuple_num / BUILDPART_THREAD_NUM;
	size_t s_tuple_num_thr = datameta.s_tuple_num / PROBEJOIN_THREAD_NUM;


#ifdef USE_PAPI
	long long PAPI_values[NUM_PAPI_EVENT];
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
	long long PAPI_values_retrieve[NUM_PAPI_EVENT];
	memset(PAPI_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_retrieve, 0, sizeof(long long) * NUM_PAPI_EVENT );

	/* papi library initialization */
	if ( (rv = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
		printf("Library initialization error! \n");
		exit(1);
	}

	/** PAPI_thread_init initializes thread support in the PAPI library. 
	 * 	Itshould be called only once, just after PAPI_library_init (3), 
	 * 	and before any other PAPI calls. Applications that make no use 
	 * 	of threads do not need to call this routine.
	 */
	rv = PAPI_thread_init( (unsigned long(*) (void) ) (pthread_self) );
	if ( (rv = PAPI_library_init(PAPI_VER_CURRENT) ) != PAPI_VER_CURRENT) {
		printf("Library initialization error! \n");
		exit(1);
	}

	/* Enable and initialize multiplex support */
	rv = PAPI_multiplex_init();
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}
#endif /* USE_PAPI */

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &buildpart_barrier;

		(args+idx)->r_rel.tuple_num = (idx == BUILDPART_THREAD_NUM -1) ? (datameta.r_tuple_num - idx * r_tuple_num_thr) : r_tuple_num_thr;
		(args+idx)->r_rel.tuples = r_relation.tuples + r_tuple_num_thr * idx;

		(args+idx)->hashtable = & hashtable;

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, ptr_run_nphj_sc_buildpart, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_buildpart[jdx] += (args+idx)->PAPI_values_buildpart[jdx];
		}
#endif /* USE_PAPI */
	}


	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &probejoin_barrier;

		(args+idx)->s_rel.tuple_num = (idx == PROBEJOIN_THREAD_NUM -1) ? (datameta.s_tuple_num - idx * s_tuple_num_thr) : s_tuple_num_thr;
		(args+idx)->s_rel.tuples = s_relation.tuples + s_tuple_num_thr * idx;

		(args+idx)->hashtable = & hashtable;

		(args+idx)->matched_cnt = 0;

		(args+idx)->mater = mater;

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, ptr_run_nphj_sc_probejoin, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
		matched_cnt += (args+idx)->matched_cnt;
		mater_cnt[idx] = (args+idx)->matched_cnt;
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_probejoin[jdx] += (args+idx)->PAPI_values_probejoin[jdx];
		}
#endif /* USE_PAPI */
	}

	for (int idx = 0; idx < RETRIEVE_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &retrieve_barrier;

		(args+idx)->checksum = 0;

		(args+idx)->mater = mater;
		(args+idx)->mater_cnt = mater_cnt;

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

		(args+idx)->org_relR = &r_relation;
		(args+idx)->org_relS = &s_relation;

		(args+idx)->row_id_pairs_processed = 0;

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_retrieve, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, ptr_run_nphj_sc_retrieve, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < RETRIEVE_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
		checksum += (args+idx)->checksum;
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_retrieve[jdx] += (args+idx)->PAPI_values_retrieve[jdx];
		}
#endif /* USE_PAPI */
	}

	purple();
	printf("matched_cnt: %zu\tcheck_sum: %zu\n", matched_cnt, checksum);

#ifdef SYNCSTATS
	printf("[SYNCSTATS] ID\tR_TUPS\tPRE_MEMSET\tBUILD\n");
	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%zu\t%.9f\t%.9f\n",
			idx, (args+idx)->r_rel.tuple_num,
			diff_sec( (args+idx)->localtimer->sync_pre_memset,
				(args+idx)->globaltimer->sync_pre_memset ),
			diff_sec( (args+idx)->localtimer->sync_build,
				(args+idx)->globaltimer->sync_build )
		);
	}

	printf("[SYNCSTATS] TID\tS_TUPS\tPROBE\n");
	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%zu\t%.9f\n",
			idx, (args+idx)->s_rel.tuple_num,
			diff_sec( (args+idx)->localtimer->sync_probe,
				(args+idx)->globaltimer->sync_probe )
		);
	}

	printf("[SYNCSTATS] TID\tPAIRS\tRETRIEVE\n");
	for (int idx = 0; idx < RETRIEVE_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%zu\t%.9f\n", idx, (args+idx)->row_id_pairs_processed,
			diff_sec( (args+idx)->localtimer->sync_retrieve,
				(args+idx)->globaltimer->sync_retrieve )
		);
	}
#endif /* SYNCSTATS */

#ifdef USE_PMWATCH
	for (int idx = 0; idx < NVMCount; idx ++) {
		PMWatch_output[idx].total_bytes_read = PMWatch_output_buildpart[idx].total_bytes_read + 
			PMWatch_output_probejoin[idx].total_bytes_read + PMWatch_output_retrieve[idx].total_bytes_read;
		PMWatch_output[idx].total_bytes_written = PMWatch_output_buildpart[idx].total_bytes_written + 
			PMWatch_output_probejoin[idx].total_bytes_written + PMWatch_output_retrieve[idx].total_bytes_written;
		PMWatch_output[idx].media_read_ops = PMWatch_output_buildpart[idx].media_read_ops + 
			PMWatch_output_probejoin[idx].media_read_ops + PMWatch_output_retrieve[idx].media_read_ops;
		PMWatch_output[idx].media_write_ops = PMWatch_output_buildpart[idx].media_write_ops +
			PMWatch_output_probejoin[idx].media_write_ops + PMWatch_output_retrieve[idx].media_write_ops;
		PMWatch_output[idx].read_64B_ops_received = PMWatch_output_buildpart[idx].read_64B_ops_received + 
			PMWatch_output_probejoin[idx].read_64B_ops_received + PMWatch_output_retrieve[idx].read_64B_ops_received;
		PMWatch_output[idx].write_64B_ops_received = PMWatch_output_buildpart[idx].write_64B_ops_received + 
			PMWatch_output_probejoin[idx].write_64B_ops_received + PMWatch_output_retrieve[idx].write_64B_ops_received;
		PMWatch_output[idx].host_reads = PMWatch_output_buildpart[idx].host_reads + 
			PMWatch_output_probejoin[idx].host_reads + PMWatch_output_retrieve[idx].host_reads;
		PMWatch_output[idx].host_writes = PMWatch_output_buildpart[idx].host_writes +
			PMWatch_output_probejoin[idx].host_writes + PMWatch_output_retrieve[idx].host_writes;
		PMWatch_output[idx].read_hit_ratio = (PMWatch_output_buildpart[idx].read_hit_ratio + 
			PMWatch_output_probejoin[idx].read_hit_ratio + PMWatch_output_retrieve[idx].read_hit_ratio) / 3;
		PMWatch_output[idx].write_hit_ratio = (PMWatch_output_buildpart[idx].write_hit_ratio + 
			PMWatch_output_probejoin[idx].write_hit_ratio + PMWatch_output_retrieve[idx].write_hit_ratio) / 3;
		PMWatch_output[idx].read_amplification = (PMWatch_output_buildpart[idx].read_amplification + 
			PMWatch_output_probejoin[idx].read_amplification + PMWatch_output_retrieve[idx].read_amplification) / 3;
		PMWatch_output[idx].write_amplification = (PMWatch_output_buildpart[idx].write_amplification +
			PMWatch_output_probejoin[idx].write_amplification + PMWatch_output_retrieve[idx].write_amplification) / 3;
		printf("[PMWatch] DIMM: %d\n", idx);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_read: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].total_bytes_read, PMWatch_output_buildpart[idx].total_bytes_read, 
			PMWatch_output_probejoin[idx].total_bytes_read, PMWatch_output_retrieve[idx].total_bytes_read
		);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_written: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].total_bytes_written, PMWatch_output_buildpart[idx].total_bytes_written, 
			PMWatch_output_probejoin[idx].total_bytes_written, PMWatch_output_retrieve[idx].total_bytes_written
		);
		printf("[PMWatch] DIMM: %d\tmedia_read_ops: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].media_read_ops, PMWatch_output_buildpart[idx].media_read_ops, 
			PMWatch_output_probejoin[idx].media_read_ops, PMWatch_output_retrieve[idx].media_read_ops
		);
		printf("[PMWatch] DIMM: %d\tmedia_write_ops: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].media_write_ops, PMWatch_output_buildpart[idx].media_write_ops, 
			PMWatch_output_probejoin[idx].media_write_ops, PMWatch_output_retrieve[idx].media_write_ops
		);
		printf("[PMWatch] DIMM: %d\tread_64B_ops_received: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].read_64B_ops_received, PMWatch_output_buildpart[idx].read_64B_ops_received,
			PMWatch_output_probejoin[idx].read_64B_ops_received, PMWatch_output_retrieve[idx].read_64B_ops_received
		);
		printf("[PMWatch] DIMM: %d\twrite_64B_ops_received: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].write_64B_ops_received, PMWatch_output_buildpart[idx].write_64B_ops_received,
			PMWatch_output_probejoin[idx].write_64B_ops_received, PMWatch_output_retrieve[idx].write_64B_ops_received
		);
		printf("[PMWatch] DIMM: %d\thost_reads: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].host_reads, PMWatch_output_buildpart[idx].host_reads, 
			PMWatch_output_probejoin[idx].host_reads, PMWatch_output_retrieve[idx].host_reads
		);
		printf("[PMWatch] DIMM: %d\thost_writes: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].host_writes, PMWatch_output_buildpart[idx].host_writes, 
			PMWatch_output_probejoin[idx].host_writes, PMWatch_output_retrieve[idx].host_writes
		);
		printf("[PMWatch] DIMM: %d\tread_hit_ratio: %f\t%f\t%f\t%f\n", idx, 
			PMWatch_output[idx].read_hit_ratio, PMWatch_output_buildpart[idx].read_hit_ratio, 
			PMWatch_output_probejoin[idx].read_hit_ratio, PMWatch_output_retrieve[idx].read_hit_ratio
		);
		printf("[PMWatch] DIMM: %d\twrite_hit_ratio: %f\t%f\t%f\t%f\n", idx, 
			PMWatch_output[idx].write_hit_ratio, PMWatch_output_buildpart[idx].write_hit_ratio, 
			PMWatch_output_probejoin[idx].write_hit_ratio, PMWatch_output_retrieve[idx].write_hit_ratio
		);
		printf("[PMWatch] DIMM: %d\tread_amplification: %.9f\t%.9f\t %.9f\t %.9f\n", idx, 
			PMWatch_output[idx].read_amplification, PMWatch_output_buildpart[idx].read_amplification, 
			PMWatch_output_probejoin[idx].read_amplification, PMWatch_output_retrieve[idx].read_amplification
		);
		printf("[PMWatch] DIMM: %d\twrite_amplification: %.9f\t%.9f\t %.9f\t %.9f\n", idx, 
			PMWatch_output[idx].write_amplification, PMWatch_output_buildpart[idx].write_amplification, 
			PMWatch_output_probejoin[idx].write_amplification, PMWatch_output_retrieve[idx].write_amplification
		);
		printf("\n");
		if (cur_t != 0) {
			agg_PMWatch_output[idx].total_bytes_read += PMWatch_output[idx].total_bytes_read;
			agg_PMWatch_output[idx].total_bytes_written += PMWatch_output[idx].total_bytes_written;
			agg_PMWatch_output[idx].media_read_ops += PMWatch_output[idx].media_read_ops;
			agg_PMWatch_output[idx].media_write_ops += PMWatch_output[idx].media_write_ops;
			agg_PMWatch_output[idx].read_64B_ops_received += PMWatch_output[idx].read_64B_ops_received;
			agg_PMWatch_output[idx].write_64B_ops_received += PMWatch_output[idx].write_64B_ops_received;
			agg_PMWatch_output[idx].host_reads += PMWatch_output[idx].host_reads;
			agg_PMWatch_output[idx].host_writes += PMWatch_output[idx].host_writes;
			agg_PMWatch_output[idx].read_hit_ratio += PMWatch_output[idx].read_hit_ratio;
			agg_PMWatch_output[idx].write_hit_ratio += PMWatch_output[idx].write_hit_ratio;
			agg_PMWatch_output[idx].read_amplification += PMWatch_output[idx].read_amplification;
			agg_PMWatch_output[idx].write_amplification += PMWatch_output[idx].write_amplification;

			agg_PMWatch_output_buildpart[idx].total_bytes_read += PMWatch_output_buildpart[idx].total_bytes_read;
			agg_PMWatch_output_buildpart[idx].total_bytes_written += PMWatch_output_buildpart[idx].total_bytes_written;
			agg_PMWatch_output_buildpart[idx].media_read_ops += PMWatch_output_buildpart[idx].media_read_ops;
			agg_PMWatch_output_buildpart[idx].media_write_ops += PMWatch_output_buildpart[idx].media_write_ops;
			agg_PMWatch_output_buildpart[idx].read_64B_ops_received += PMWatch_output_buildpart[idx].read_64B_ops_received;
			agg_PMWatch_output_buildpart[idx].write_64B_ops_received += PMWatch_output_buildpart[idx].write_64B_ops_received;
			agg_PMWatch_output_buildpart[idx].host_reads += PMWatch_output_buildpart[idx].host_reads;
			agg_PMWatch_output_buildpart[idx].host_writes += PMWatch_output_buildpart[idx].host_writes;
			agg_PMWatch_output_buildpart[idx].read_hit_ratio += PMWatch_output_buildpart[idx].read_hit_ratio;
			agg_PMWatch_output_buildpart[idx].write_hit_ratio += PMWatch_output_buildpart[idx].write_hit_ratio;
			agg_PMWatch_output_buildpart[idx].read_amplification += PMWatch_output_buildpart[idx].read_amplification;
			agg_PMWatch_output_buildpart[idx].write_amplification += PMWatch_output_buildpart[idx].write_amplification;

			agg_PMWatch_output_probejoin[idx].total_bytes_read += PMWatch_output_probejoin[idx].total_bytes_read;
			agg_PMWatch_output_probejoin[idx].total_bytes_written += PMWatch_output_probejoin[idx].total_bytes_written;
			agg_PMWatch_output_probejoin[idx].media_read_ops += PMWatch_output_probejoin[idx].media_read_ops;
			agg_PMWatch_output_probejoin[idx].media_write_ops += PMWatch_output_probejoin[idx].media_write_ops;
			agg_PMWatch_output_probejoin[idx].read_64B_ops_received += PMWatch_output_probejoin[idx].read_64B_ops_received;
			agg_PMWatch_output_probejoin[idx].write_64B_ops_received += PMWatch_output_probejoin[idx].write_64B_ops_received;
			agg_PMWatch_output_probejoin[idx].host_reads += PMWatch_output_probejoin[idx].host_reads;
			agg_PMWatch_output_probejoin[idx].host_writes += PMWatch_output_probejoin[idx].host_writes;
			agg_PMWatch_output_probejoin[idx].read_hit_ratio += PMWatch_output_probejoin[idx].read_hit_ratio;
			agg_PMWatch_output_probejoin[idx].write_hit_ratio += PMWatch_output_probejoin[idx].write_hit_ratio;
			agg_PMWatch_output_probejoin[idx].read_amplification += PMWatch_output_probejoin[idx].read_amplification;
			agg_PMWatch_output_probejoin[idx].write_amplification += PMWatch_output_probejoin[idx].write_amplification;
		}
	}
#endif /* USE_PMWATCH */
#ifdef USE_PAPI
	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		PAPI_values[idx] += PAPI_values_buildpart[idx] + PAPI_values_probejoin[idx] + PAPI_values_retrieve[idx];
		printf("[PAPI] PAPI_event_name: %s", PAPI_event_names[idx]);
		printf("\ttotal_counter: %lld", PAPI_values[idx]);
		printf("\tbuildpart_counter: %lld", PAPI_values_buildpart[idx]);
		printf("\tprobejoin_counter: %lld", PAPI_values_probejoin[idx]);
		printf("\tretrieve_counter: %lld", PAPI_values_retrieve[idx]);
		printf("\n");
		if (cur_t != 0) {
			agg_PAPI_values[idx] += PAPI_values[idx];
			agg_PAPI_values_buildpart[idx] += PAPI_values_buildpart[idx];
			agg_PAPI_values_probejoin[idx] += PAPI_values_probejoin[idx];
		}
	}
	PAPI_shutdown();
#endif /* USE_PAPI */

#ifdef SYNCSTATS
	free(localtimer);
	free(globaltimer);
#endif /* SYNCSTATS */

	pthread_attr_destroy(&attr);
	pthread_barrier_destroy(&buildpart_barrier);
	pthread_barrier_destroy(&probejoin_barrier);
	pthread_barrier_destroy(&retrieve_barrier);
	dealloc_memory(hashtable.buckets, sizeof(ptr_bucket_t) * hashtable.num_buckets);
	// memsync(r_relation.tuples, TUPLE_T_SIZE * r_relation.tuple_num);
	// memsync(s_relation.tuples, TUPLE_T_SIZE * s_relation.tuple_num);
	dealloc_memory(r_relation.tuples, TUPLE_T_SIZE * r_relation.tuple_num);
	dealloc_memory(s_relation.tuples, TUPLE_T_SIZE * s_relation.tuple_num);

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		ptr_free_bucket_buffer((args+idx)->overflowbuf);
	}

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		size_t tot_row_id_pair_num = (args+idx)->s_rel.tuple_num;
		dealloc_memory(mater[idx], ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);
	}

	dealloc_memory(mater, sizeof(row_id_pair_t *) * PROBEJOIN_THREAD_NUM);

	timekeeper->buildpart = diff_sec(args[0].build_start, args[0].build_end);
	timekeeper->probejoin = diff_sec(args[0].probe_start, args[0].probe_end);
	timekeeper->total = timekeeper->buildpart + timekeeper->probejoin + diff_sec(args[0].retrieve_start, args[0].retrieve_end);
}


#ifdef LOCK_IN_BUCKET
int acquire_lock_and_store(ptr_hash_entry_t *hash_entry, hashkey_t hashkey) {
	lock(&hash_entry->latch);
	if (hash_entry->hashkey == 0) {
		/* we make sure that the hashkey starts from 1 */
		hash_entry->hashkey = hashkey + 1;
		unlock(&hash_entry->latch);
		return 1;
	} else {
		unlock(&hash_entry->latch);
		return 0;
	}
}
#endif /* LOCK_IN_BUCKET */


void ptr_lp_insert_with_pos(ptr_hash_entry_t *hashtable, tuple_t *tuple, hashkey_t tmp_hashkey, hashkey_t pos) {
#ifdef LOCK_IN_BUCKET	
	while ( hashtable[pos].hashkey || ( !acquire_lock_and_store(&hashtable[pos], tmp_hashkey) ) ) {		
		pos = (pos + 1) & hashmask;
	}
	store_a_keyrid(&hashtable[pos].keyrid, tuple);
#else /* LOCK_IN_BUCKET */
	/* we make sure that the hashkey starts from 1 */
	while ( hashtable[pos].hashkey || ( !__sync_bool_compare_and_swap(&hashtable[pos].hashkey, 0, tmp_hashkey+1) ) ) {
		pos = (pos + 1) & hashmask;
	}
	store_a_keyrid(&hashtable[pos].keyrid, tuple);

#endif /* LOCK_IN_BUCKET */
}

void ptr_lp_insert(ptr_hash_entry_t *hashtable, tuple_t *tuple) {
	hashkey_t tmp_hashkey = IDHASH(tuple->key, hashmask, bitskip);
	ptr_lp_insert_with_pos(hashtable, tuple, tmp_hashkey, tmp_hashkey & hashmask);
}


void ptr_build_hashtable_nphj_lp(ptr_hash_entry_t *hashtable, relation_t *rel, int _tid) {
#ifdef PREFETCH_DISTANCE
	size_t prefetch_index = next_pow_2(PREFETCH_DISTANCE);
	hashkey_t hashkey_prefetch;

#ifdef PREFETCH_CACHE
	key_t_ cachemask = prefetch_index - 1;
	size_t cache_idx;
	ptr_lp_prefetch_cache_t *lp_prefetch_cache;
	size_t cache_jdx;

#ifdef PREFETCH_CACHE_NVM
	lp_prefetch_cache = (ptr_lp_prefetch_cache_t *) new_alloc_nvm(sizeof(ptr_lp_prefetch_cache_t) * prefetch_index);

#else /* PREFETCH_CACHE_NVM */
	lp_prefetch_cache = (ptr_lp_prefetch_cache_t *) alloc_dram(sizeof(ptr_lp_prefetch_cache_t) * prefetch_index, CACHELINE_SIZE);
#endif /* PREFETCH_CACHE_NVM */

	for (size_t idx = 0; idx < prefetch_index; idx ++) {
		cache_idx = idx & cachemask;
		(lp_prefetch_cache + cache_idx)->hashkey = IDHASH(rel->tuples[idx].key, hashmask, bitskip);
		hashkey_prefetch = (lp_prefetch_cache + cache_idx)->hashkey;
		__builtin_prefetch(hashtable + hashkey_prefetch, 1, 1);
	}

	cache_jdx = prefetch_index;
	for (size_t idx = 0; idx < rel->tuple_num - prefetch_index; idx ++ ) {

		/** insert **/ 
		cache_idx = idx & cachemask;
		ptr_lp_insert_with_pos(hashtable, &rel->tuples[idx], lp_prefetch_cache[cache_idx].hashkey, lp_prefetch_cache[cache_idx].hashkey & hashmask);
		
		/** prefetch **/ 
		(lp_prefetch_cache + cache_idx)->hashkey = IDHASH(rel->tuples[cache_jdx].key, hashmask, bitskip);
		hashkey_prefetch = (lp_prefetch_cache + cache_idx)->hashkey;
		__builtin_prefetch(hashtable + hashkey_prefetch, 1, 1);
		cache_jdx ++;
	}

	for (size_t idx = rel->tuple_num - prefetch_index; idx < rel->tuple_num; idx ++) {
		cache_idx  = idx & cachemask;
		ptr_lp_insert_with_pos(hashtable, &rel->tuples[idx], lp_prefetch_cache[cache_idx].hashkey, lp_prefetch_cache[cache_idx].hashkey & hashmask);
	}

#ifdef PREFETCH_CACHE_NVM
	pmem_unmap(lp_prefetch_cache, sizeof(ptr_lp_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	dealloc_dram(lp_prefetch_cache, sizeof(ptr_lp_prefetch_cache_t) * prefetch_index);
#endif /* PREFETCH_CACHE_NVM */

#else /* PREFETCH_CACHE */
		for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
			if (prefetch_index < rel->tuple_num) {
				hashkey_prefetch = IDHASH(rel->tuples[prefetch_index++].key, hashmask, bitskip);
				__builtin_prefetch(hashtable + hashkey_prefetch, 1, 1);
			}
			ptr_lp_insert(hashtable, &rel->tuples[idx]);
		}

#endif /* PREFETCH_CACHE */
#else /* PREFETCH_DISTANCE */
		for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
			ptr_lp_insert(hashtable, &rel->tuples[idx]);
		}
#endif /* PREFETCH_DISTANCE */
}


keyrid_t * ptr_lp_lookup_with_pos(ptr_hash_entry_t *hashtable, tuple_t *tuple, hashkey_t tmp_hashkey, hashkey_t pos) {
#ifdef TEST_SELECTIVITY
	bool flag = true;
	hashkey_t pos_0 = pos;

	while (  hashtable[pos].hashkey && 
		(hashtable[pos].keyrid.key != tuple->key) && 
		( pos != pos_0 || flag) ) {
		
		flag = false;
		pos = (pos + 1) & hashmask;
	}

	return ( hashtable[pos].hashkey && ( pos != pos_0 || flag ) ) ? & hashtable[pos].keyrid : NULL;

#else /* TEST_SELECTIVITY */
	while (  hashtable[pos].hashkey && (hashtable[pos].keyrid.key != tuple->key) ) {
		pos = (pos + 1) & hashmask;
	}
	return hashtable[pos].hashkey ? & hashtable[pos].keyrid : NULL;
#endif /* TEST_SELECTIVITY */
}


keyrid_t * ptr_lp_lookup(ptr_hash_entry_t *hashtable, tuple_t *tuple) {
	hashkey_t tmp_hashkey = IDHASH(tuple->key, hashmask, bitskip);
	return ptr_lp_lookup_with_pos(hashtable, tuple, tmp_hashkey, tmp_hashkey & hashmask);
}


void ptr_probe_hashtable_nphj_lp(ptr_hash_entry_t *hashtable, relation_t *rel, size_t *matched_cnt, row_id_pair_t *mater) {
	keyrid_t *found_keyrid;

#ifdef NONTEMP_STORE_MATER
	size_t slot_mod;
	row_id_pair_swwcb_t *row_id_pair_swwcb;
	row_id_pair_t *tmp_row_id_pair_swwcb;
#ifdef NONTEMP_STORE_MATER_ON_NVM
	row_id_pair_swwcb = (row_id_pair_swwcb_t *) new_alloc_nvm(sizeof(row_id_pair_swwcb_t));
	pmem_memset_persist(row_id_pair_swwcb, 0, sizeof(row_id_pair_swwcb_t));
#else /* NONTEMP_STORE_MATER_ON_NVM */
	row_id_pair_swwcb = (row_id_pair_swwcb_t *) alloc_aligned_dram(sizeof(row_id_pair_swwcb_t), CACHELINE_SIZE);
	memset(row_id_pair_swwcb, 0, sizeof(row_id_pair_swwcb_t));
#endif /* NONTEMP_STORE_MATER_ON_NVM */
#else /* NONTEMP_STORE_MATER */
	row_id_pair_t tmp_row_id_pair;
#endif /* NONTEMP_STORE_MATER */

#ifdef PREFETCH_DISTANCE
	size_t prefetch_index = next_pow_2(PREFETCH_DISTANCE);
	hashkey_t hashkey_prefetch;

#ifdef PREFETCH_CACHE
	key_t_ cachemask = prefetch_index - 1;
	size_t cache_idx;
	ptr_lp_prefetch_cache_t *lp_prefetch_cache;
	size_t cache_jdx;

#ifdef PREFETCH_CACHE_NVM
	lp_prefetch_cache = (ptr_lp_prefetch_cache_t *) new_alloc_nvm(sizeof(ptr_lp_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	lp_prefetch_cache = (ptr_lp_prefetch_cache_t *) alloc_dram(sizeof(ptr_lp_prefetch_cache_t) * prefetch_index, CACHELINE_SIZE);
#endif /* PREFETCH_CACHE_NVM */

	for (size_t idx = 0; idx < prefetch_index; idx ++) {
		cache_idx = idx & cachemask;
		(lp_prefetch_cache + cache_idx)->hashkey = IDHASH(rel->tuples[idx].key, hashmask, bitskip);
		hashkey_prefetch = (lp_prefetch_cache + cache_idx)->hashkey;
		__builtin_prefetch(hashtable + hashkey_prefetch, 0, 1);
	}

	cache_jdx = prefetch_index;
	for (size_t idx = 0; idx < rel->tuple_num - prefetch_index; idx ++ ) {

		/** lookup **/ 
		cache_idx = idx & cachemask;
		found_keyrid = ptr_lp_lookup_with_pos( hashtable, &rel->tuples[idx], lp_prefetch_cache[cache_idx].hashkey, lp_prefetch_cache[cache_idx].hashkey & hashmask );
#ifdef NONTEMP_STORE_MATER
		slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
		tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
		// tmp_row_id_pair_swwcb[slot_mod].r_row_id = (found_keyrid == NULL) ? 0 : found_keyrid->row_id;
		// tmp_row_id_pair_swwcb[slot_mod].s_row_id = (found_keyrid == NULL) ? 0 : rel->tuples[idx].row_id;

		tmp_row_id_pair_swwcb[slot_mod].r_row_id = found_keyrid->row_id * (found_keyrid != NULL);
		tmp_row_id_pair_swwcb[slot_mod].s_row_id = rel->tuples[idx].row_id * (found_keyrid != NULL);
		if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
			/* non-temporal store a SWWCB */
			nontemp_store_swwcb( mater+((*matched_cnt)-(ROW_ID_PAIR_PER_SWWCB-1)), tmp_row_id_pair_swwcb);
		}
#else /* NONTEMP_STORE_MATER */
		// tmp_row_id_pair.r_row_id = (found_keyrid == NULL) ? 0 : found_keyrid->row_id;
		// tmp_row_id_pair.s_row_id = (found_keyrid == NULL) ? 0 : rel->tuples[idx].row_id;

		tmp_row_id_pair.r_row_id = found_keyrid->row_id * (found_keyrid != NULL);
		tmp_row_id_pair.s_row_id = rel->tuples[idx].row_id * (found_keyrid != NULL);
		if ( found_keyrid ) {
			store_a_row_id_pair(mater+(*matched_cnt), &tmp_row_id_pair);
		}
#endif /* NONTEMP_STORE_MATER */
		*matched_cnt += (found_keyrid != NULL);

		/** prefetch **/ 
		(lp_prefetch_cache + cache_idx)->hashkey = IDHASH(rel->tuples[cache_jdx].key, hashmask, bitskip);
		hashkey_prefetch = (lp_prefetch_cache + cache_idx)->hashkey;
		__builtin_prefetch(hashtable + hashkey_prefetch, 0, 1);
		cache_jdx ++;
	}

	for (size_t idx = rel->tuple_num - prefetch_index; idx < rel->tuple_num; idx ++) {
		cache_idx  = idx & cachemask;
		found_keyrid = ptr_lp_lookup_with_pos( hashtable, &rel->tuples[idx], lp_prefetch_cache[cache_idx].hashkey, lp_prefetch_cache[cache_idx].hashkey & hashmask );
#ifdef NONTEMP_STORE_MATER
		slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
		tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
		// tmp_row_id_pair_swwcb[slot_mod].r_row_id = (found_keyrid == NULL) ? 0 : found_keyrid->row_id;
		// tmp_row_id_pair_swwcb[slot_mod].s_row_id = (found_keyrid == NULL) ? 0 : rel->tuples[idx].row_id;

		tmp_row_id_pair_swwcb[slot_mod].r_row_id = found_keyrid->row_id * (found_keyrid != NULL);
		tmp_row_id_pair_swwcb[slot_mod].s_row_id = rel->tuples[idx].row_id * (found_keyrid != NULL);

		if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
			/* non-temporal store a SWWCB */
			nontemp_store_swwcb( mater+((*matched_cnt)-(ROW_ID_PAIR_PER_SWWCB-1)), tmp_row_id_pair_swwcb);
		}
#else /* NONTEMP_STORE_MATER */
		// tmp_row_id_pair.r_row_id = (found_keyrid == NULL) ? 0 : found_keyrid->row_id;
		// tmp_row_id_pair.s_row_id = (found_keyrid == NULL) ? 0 : rel->tuples[idx].row_id;

		tmp_row_id_pair.r_row_id = found_keyrid->row_id * (found_keyrid != NULL);
		tmp_row_id_pair.s_row_id = rel->tuples[idx].row_id * (found_keyrid != NULL);
		if ( found_keyrid ) {
			store_a_row_id_pair(mater+(*matched_cnt), &tmp_row_id_pair);
		}
#endif /* NONTEMP_STORE_MATER */
		*matched_cnt += (found_keyrid != NULL);

	}

#ifdef NONTEMP_STORE_MATER
	slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
	for (size_t idx = 0; idx < slot_mod; idx ++) {
		store_a_row_id_pair(mater+(*matched_cnt)-slot_mod+idx, &row_id_pair_swwcb->data.row_id_pairs[idx]);
	}
#endif /* NONTEMP_STORE_MATER */


#ifdef PREFETCH_CACHE_NVM
	pmem_unmap(lp_prefetch_cache, sizeof(ptr_lp_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	dealloc_dram(lp_prefetch_cache, sizeof(ptr_lp_prefetch_cache_t) * prefetch_index);
#endif /* PREFETCH_CACHE_NVM */

#else /* PREFETCH_CACHE */
		for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
			if (prefetch_index < rel->tuple_num) {
				hashkey_prefetch = IDHASH(rel->tuples[prefetch_index++].key, hashmask, bitskip);
				__builtin_prefetch(hashtable + hashkey_prefetch, 0, 1);
			}
			found_keyrid = ptr_lp_lookup(hashtable, &rel->tuples[idx]);
#ifdef NONTEMP_STORE_MATER
			slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
			tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
			// tmp_row_id_pair_swwcb[slot_mod].r_row_id = (found_keyrid == NULL) ? 0 : found_keyrid->row_id;
			// tmp_row_id_pair_swwcb[slot_mod].s_row_id = (found_keyrid == NULL) ? 0 : rel->tuples[idx].row_id;

			tmp_row_id_pair_swwcb[slot_mod].r_row_id = found_keyrid->row_id * (found_keyrid != NULL);
			tmp_row_id_pair_swwcb[slot_mod].s_row_id = rel->tuples[idx].row_id * (found_keyrid != NULL);
			if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
				/* non-temporal store a SWWCB */
				nontemp_store_swwcb( mater+((*matched_cnt)-(ROW_ID_PAIR_PER_SWWCB-1)), tmp_row_id_pair_swwcb);
			}
#else /* NONTEMP_STORE_MATER */
			// tmp_row_id_pair.r_row_id = (found_keyrid == NULL) ? 0 : found_keyrid->row_id;
			// tmp_row_id_pair.s_row_id = (found_keyrid == NULL) ? 0 : rel->tuples[idx].row_id;

			tmp_row_id_pair.r_row_id = found_keyrid->row_id * (found_keyrid != NULL);
			tmp_row_id_pair.s_row_id = rel->tuples[idx].row_id * (found_keyrid != NULL);
			if ( found_keyrid != NULL) {
				store_a_row_id_pair(mater+(*matched_cnt), &tmp_row_id_pair);
			}
#endif /* NONTEMP_STORE_MATER */
			*matched_cnt += (found_keyrid != NULL);
		}

#ifdef NONTEMP_STORE_MATER
	slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
	for (size_t idx = 0; idx < slot_mod; idx ++) {
		store_a_row_id_pair(mater+(*matched_cnt)-slot_mod+idx, &row_id_pair_swwcb->data.row_id_pairs[idx]);
	}
#endif /* NONTEMP_STORE_MATER */

#endif /* PREFETCH_CACHE */
#else /* PREFETCH_DISTANCE */
		for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
			found_keyrid = ptr_lp_lookup(hashtable, &rel->tuples[idx]);
#ifdef NONTEMP_STORE_MATER
			slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
			tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
			// tmp_row_id_pair_swwcb[slot_mod].r_row_id = (found_keyrid == NULL) ? 0 : found_keyrid->row_id;
			// tmp_row_id_pair_swwcb[slot_mod].s_row_id = (found_keyrid == NULL) ? 0 : rel->tuples[idx].row_id;

			tmp_row_id_pair_swwcb[slot_mod].r_row_id = found_keyrid->row_id * (found_keyrid != NULL);
			tmp_row_id_pair_swwcb[slot_mod].s_row_id = rel->tuples[idx].row_id * (found_keyrid != NULL);
			if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
				/* non-temporal store a SWWCB */
				nontemp_store_swwcb( mater+((*matched_cnt)-(ROW_ID_PAIR_PER_SWWCB-1)), tmp_row_id_pair_swwcb);
			}
#else /* NONTEMP_STORE_MATER */
			// tmp_row_id_pair.r_row_id = (found_keyrid == NULL) ? 0 : found_keyrid->row_id;
			// tmp_row_id_pair.s_row_id = (found_keyrid == NULL) ? 0 : rel->tuples[idx].row_id;

			tmp_row_id_pair.r_row_id = found_keyrid->row_id * (found_keyrid != NULL);
			tmp_row_id_pair.s_row_id = rel->tuples[idx].row_id * (found_keyrid != NULL);
			if ( found_keyrid != NULL ) {
				store_a_row_id_pair(mater+(*matched_cnt), &tmp_row_id_pair);
			}
#endif /* NONTEMP_STORE_MATER */
			*matched_cnt += (found_keyrid != NULL);
		}

#ifdef NONTEMP_STORE_MATER
	slot_mod = (*matched_cnt) & (ROW_ID_PAIR_PER_SWWCB - 1);
	for (size_t idx = 0; idx < slot_mod; idx ++) {
		store_a_row_id_pair(mater+(*matched_cnt)-slot_mod+idx, &row_id_pair_swwcb->data.row_id_pairs[idx]);
	}
#endif /* NONTEMP_STORE_MATER */

#endif /* PREFETCH_DISTANCE */

#ifdef NONTEMP_STORE_MATER
#ifdef NONTEMP_STORE_MATER_ON_NVM
	pmem_unmap(row_id_pair_swwcb, sizeof(row_id_pair_swwcb_t));
#else /* NONTEMP_STORE_MATER_ON_NVM */
	free(row_id_pair_swwcb);
#endif /* NONTEMP_STORE_MATER_ON_NVM */
#endif /* NONTEMP_STORE_MATER */
}


void *ptr_run_nphj_lp_buildpart(void *arg) {
	ptr_nphj_lp_arg_t *nphj_lp_arg = (ptr_nphj_lp_arg_t *) arg;

#ifdef WARMUP
	warmup_localize(nphj_lp_arg->r_rel.tuples, nphj_lp_arg->r_rel.tuple_num);
#endif /* WARMUP */

	/* eliminate the page faults of the hash table */
	size_t memset_num_entrys_thr = nphj_lp_arg->num_entrys / BUILDPART_THREAD_NUM;
	size_t memset_num_entrys_size = (nphj_lp_arg->_tid == BUILDPART_THREAD_NUM -1) ? 
		sizeof(ptr_hash_entry_t) * (nphj_lp_arg->num_entrys - nphj_lp_arg->_tid * memset_num_entrys_thr) : 
		sizeof(ptr_hash_entry_t) * memset_num_entrys_thr;
	memset_localize(nphj_lp_arg->hashtable + nphj_lp_arg->_tid * memset_num_entrys_thr, memset_num_entrys_size );

// 	/* currently, we only consider the uniform pkfk scenario */
// 	size_t tot_row_id_pair_num = nphj_lp_arg->s_rel.tuple_num;
// #ifdef USE_NVM
// 	row_id_pair_t *mater = (row_id_pair_t *) new_alloc_nvm(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);
// #else /* USE_NVM */
// 	row_id_pair_t *mater = (row_id_pair_t *) alloc_aligned_dram(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE, CACHELINE_SIZE);
// #endif /* USE_NVM */
// 	memset_localize(mater, ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);

	int rv;

#ifdef USE_PAPI
	int eventset = PAPI_NULL;

	rv = PAPI_create_eventset(&eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Assign it to the CPU component */
	rv = PAPI_assign_eventset_component(eventset, 0);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Convert the EventSet to a multiplexed event set */
	rv = PAPI_set_multiplex(eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		rv = PAPI_add_named_event(eventset, PAPI_event_names[idx]);
		if (rv != PAPI_OK) {
			ERROR_RETURN(rv);
			exit(1);
		}		
	}

	PAPI_reset(eventset);
	PAPI_start(eventset);
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
	if (nphj_lp_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */

	BARRIER_ARRIVE(nphj_lp_arg->barrier, rv);

	if (nphj_lp_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_lp_arg->build_start);
	}

	/** build the hash table **/

	// /* eliminate the page faults of the hash table */
	// size_t memset_num_entrys_thr = nphj_lp_arg->num_entrys / BUILDPART_THREAD_NUM;
	// size_t memset_num_entrys_size = (nphj_lp_arg->_tid == BUILDPART_THREAD_NUM -1) ? 
	// 	sizeof(ptr_hash_entry_t) * (nphj_lp_arg->num_entrys - nphj_lp_arg->_tid * memset_num_entrys_thr) : 
	// 	sizeof(ptr_hash_entry_t) * memset_num_entrys_thr;
	// memset_localize(nphj_lp_arg->hashtable + nphj_lp_arg->_tid * memset_num_entrys_thr, memset_num_entrys_size );

#ifdef SYNCSTATS
	SYNC_TIMER(nphj_lp_arg->localtimer->sync_pre_memset);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(nphj_lp_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(nphj_lp_arg->_tid, nphj_lp_arg->globaltimer->sync_pre_memset);
#endif /* SYNCSTATS */

	ptr_build_hashtable_nphj_lp(nphj_lp_arg->hashtable, &nphj_lp_arg->r_rel, nphj_lp_arg->_tid);

#ifdef SYNCSTATS
	SYNC_TIMER(nphj_lp_arg->localtimer->sync_build);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(nphj_lp_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(nphj_lp_arg->_tid, nphj_lp_arg->globaltimer->sync_build);
#endif /* SYNCSTATS */

	if (nphj_lp_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_lp_arg->build_end);
	}

#ifdef USE_PMWATCH
	if (nphj_lp_arg->_tid == 0) {
		pmwEnd(PMWatch_output_buildpart);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, nphj_lp_arg->PAPI_values_buildpart);
	/* Free all memory and data structures, eventset must be empty. */
	if ( (rv = PAPI_cleanup_eventset(eventset))!=PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	if ( (rv = PAPI_destroy_eventset(&eventset)) != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}
#endif /* USE_PAPI */

	return NULL;
}


void *ptr_run_nphj_lp_probejoin(void *arg) {
	ptr_nphj_lp_arg_t *nphj_lp_arg = (ptr_nphj_lp_arg_t *) arg;

#ifdef WARMUP
	warmup_localize(nphj_lp_arg->s_rel.tuples, nphj_lp_arg->s_rel.tuple_num);
#endif /* WARMUP */

	/* currently, we only consider the uniform pkfk scenario */
	size_t tot_row_id_pair_num = nphj_lp_arg->s_rel.tuple_num;
#ifdef USE_NVM
	row_id_pair_t *mater = (row_id_pair_t *) new_alloc_nvm(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);
#else /* USE_NVM */
#ifdef USE_HUGE
	row_id_pair_t *mater = (row_id_pair_t *) alloc_huge_dram(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	row_id_pair_t *mater = (row_id_pair_t *) alloc_aligned_dram(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	memset_localize(mater, ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);

	int rv;

#ifdef USE_PAPI
	int eventset = PAPI_NULL;

	rv = PAPI_create_eventset(&eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Assign it to the CPU component */
	rv = PAPI_assign_eventset_component(eventset, 0);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Convert the EventSet to a multiplexed event set */
	rv = PAPI_set_multiplex(eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		rv = PAPI_add_named_event(eventset, PAPI_event_names[idx]);
		if (rv != PAPI_OK) {
			ERROR_RETURN(rv);
			exit(1);
		}		
	}

	PAPI_reset(eventset);
	PAPI_start(eventset);
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
	if (nphj_lp_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */

	/** probe the hash table **/
	if (nphj_lp_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_lp_arg->probe_start);
	}

	// /* currently, we only consider the uniform pkfk scenario */
	// size_t tot_row_id_pair_num = nphj_sc_arg->s_rel.tuple_num;
	// row_id_pair_t *mater = (row_id_pair_t *) alloc_memory(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE, CACHELINE_SIZE);
	// memset_localize(mater, ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);

	ptr_probe_hashtable_nphj_lp(nphj_lp_arg->hashtable, &nphj_lp_arg->s_rel, &nphj_lp_arg->matched_cnt, mater);

#ifdef SYNCSTATS
	SYNC_TIMER(nphj_lp_arg->localtimer->sync_probe);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(nphj_lp_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(nphj_lp_arg->_tid, nphj_lp_arg->globaltimer->sync_probe);
#endif /* SYNCSTATS */

	if (nphj_lp_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_lp_arg->probe_end);
	}

#ifdef USE_PMWATCH
	if (nphj_lp_arg->_tid == 0) {
		pmwEnd(PMWatch_output_probejoin);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, nphj_lp_arg->PAPI_values_probejoin);
	/* Free all memory and data structures, eventset must be empty. */
	if ( (rv = PAPI_cleanup_eventset(eventset))!=PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	if ( (rv = PAPI_destroy_eventset(&eventset)) != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}
#endif /* USE_PAPI */

	nphj_lp_arg->mater[nphj_lp_arg->_tid] = mater;

	return NULL;
}


void *ptr_run_nphj_lp_retrieve(void *arg) {
	ptr_nphj_lp_arg_t *nphj_lp_arg = (ptr_nphj_lp_arg_t *) arg;

	size_t start_offset = 0;
	size_t processed_num = 0;
	size_t processed_num_thr = 0;
	rowid_t tmp_checksum = 0;

	int rv;

#ifdef USE_PAPI
	int eventset = PAPI_NULL;

	rv = PAPI_create_eventset(&eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Assign it to the CPU component */
	rv = PAPI_assign_eventset_component(eventset, 0);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Convert the EventSet to a multiplexed event set */
	rv = PAPI_set_multiplex(eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		rv = PAPI_add_named_event(eventset, PAPI_event_names[idx]);
		if (rv != PAPI_OK) {
			ERROR_RETURN(rv);
			exit(1);
		}		
	}

	PAPI_reset(eventset);
	PAPI_start(eventset);
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
	if (nphj_lp_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */

	if (nphj_lp_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_lp_arg->retrieve_start);
	}

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		processed_num_thr = nphj_lp_arg->mater_cnt[idx] / RETRIEVE_THREAD_NUM;
		start_offset = nphj_lp_arg->_tid * processed_num_thr;
		processed_num = (nphj_lp_arg->_tid == RETRIEVE_THREAD_NUM - 1) ? 
			nphj_lp_arg->mater_cnt[idx] - nphj_lp_arg->_tid * processed_num_thr : processed_num_thr;
		ptr_retrieve(&tmp_checksum, nphj_lp_arg->mater[idx]+start_offset, processed_num, *nphj_lp_arg->org_relR, *nphj_lp_arg->org_relS);
		nphj_lp_arg->checksum += tmp_checksum;
		nphj_lp_arg->row_id_pairs_processed += processed_num;
	}

#ifdef SYNCSTATS
	SYNC_TIMER(nphj_lp_arg->localtimer->sync_retrieve);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(nphj_lp_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(nphj_lp_arg->_tid, nphj_lp_arg->globaltimer->sync_retrieve);
#endif /* SYNCSTATS */

	if (nphj_lp_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &nphj_lp_arg->retrieve_end);
	}

#ifdef USE_PMWATCH
	if (nphj_lp_arg->_tid == 0) {
		pmwEnd(PMWatch_output_retrieve);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, nphj_lp_arg->PAPI_values_retrieve);
	/* Free all memory and data structures, eventset must be empty. */
	if ( (rv = PAPI_cleanup_eventset(eventset))!=PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	if ( (rv = PAPI_destroy_eventset(&eventset)) != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}
#endif /* USE_PAPI */

	return NULL;
}


void ptr_nphj_lp(const datameta_t datameta, timekeeper_t * const timekeeper) {
	int rv;

	size_t matched_cnt = 0;
	rowid_t checksum = 0;

	/**************** LOAD DATA ****************/
	char r_path[CHAR_BUFFER_LEN], s_path[CHAR_BUFFER_LEN];
	snprintf(r_path, CHAR_BUFFER_LEN, "%s/%s", DATA_PATH_PREFIX, datameta.r_path_suffix);
	snprintf(s_path, CHAR_BUFFER_LEN, "%s/%s", DATA_PATH_PREFIX, datameta.s_path_suffix);

	tuple_t *r_original_addr = (tuple_t*) create_pmem_buffer(r_path, TUPLE_T_SIZE * datameta.r_tuple_num);
	tuple_t *s_original_addr = (tuple_t*) create_pmem_buffer(s_path, TUPLE_T_SIZE * datameta.s_tuple_num);

	relation_t r_relation, s_relation;

	r_relation.tuple_num = datameta.r_tuple_num;
	s_relation.tuple_num = datameta.s_tuple_num;

#ifdef USE_NVM
	r_relation.tuples = new_alloc_nvm(TUPLE_T_SIZE * datameta.r_tuple_num);
	s_relation.tuples = new_alloc_nvm(TUPLE_T_SIZE * datameta.s_tuple_num);
#else /* USE_NVM */
#ifdef USE_HUGE
	r_relation.tuples = (tuple_t*) alloc_huge_dram(TUPLE_T_SIZE * r_relation.tuple_num);
	s_relation.tuples = (tuple_t*) alloc_huge_dram(TUPLE_T_SIZE * s_relation.tuple_num);
#else /* USE_HUGE */
	r_relation.tuples = (tuple_t*) alloc_aligned_dram(TUPLE_T_SIZE * r_relation.tuple_num, CACHELINE_SIZE);
	s_relation.tuples = (tuple_t*) alloc_aligned_dram(TUPLE_T_SIZE * s_relation.tuple_num, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memcpy(r_relation.tuples, r_original_addr, TUPLE_T_SIZE * r_relation.tuple_num);
	parallel_memcpy(s_relation.tuples, s_original_addr, TUPLE_T_SIZE * s_relation.tuple_num);
	pmem_unmap(r_original_addr, TUPLE_T_SIZE * r_relation.tuple_num);
	pmem_unmap(s_original_addr, TUPLE_T_SIZE * s_relation.tuple_num);

	row_id_pair_t **mater = (row_id_pair_t **) alloc_memory( sizeof(row_id_pair_t *) * PROBEJOIN_THREAD_NUM, CACHELINE_SIZE );
	size_t mater_cnt[PROBEJOIN_THREAD_NUM];
	memset(mater_cnt, 0, sizeof(size_t)*PROBEJOIN_THREAD_NUM);

	int max_thread_num = MAX( MAX(BUILDPART_THREAD_NUM, PROBEJOIN_THREAD_NUM), RETRIEVE_THREAD_NUM );

	pthread_t threadpool[max_thread_num];
	ptr_nphj_lp_arg_t args[max_thread_num];

	pthread_barrier_t buildpart_barrier;
	rv = pthread_barrier_init(&buildpart_barrier, NULL, BUILDPART_THREAD_NUM);
	if(rv){
		printf("[ERROR] Couldn't create the buildpart_barrier\n");
		exit(EXIT_FAILURE);
	}
	pthread_barrier_t probejoin_barrier;
	rv = pthread_barrier_init(&probejoin_barrier, NULL, PROBEJOIN_THREAD_NUM);
	if(rv){
		printf("[ERROR] Couldn't create the probejoin_barrier\n");
		exit(EXIT_FAILURE);
	}
	pthread_barrier_t retrieve_barrier;
	rv = pthread_barrier_init(&retrieve_barrier, NULL, RETRIEVE_THREAD_NUM);
	if(rv){
		printf("[ERROR] Couldn't create the retrieve_barrier\n");
		exit(EXIT_FAILURE);
	}


#ifdef SYNCSTATS
	ptr_synctimer_nphj_t *localtimer = (ptr_synctimer_nphj_t *) malloc( max_thread_num * sizeof(ptr_synctimer_nphj_t) );
	ptr_synctimer_nphj_t *globaltimer = (ptr_synctimer_nphj_t *) malloc( sizeof(ptr_synctimer_nphj_t) );
	memset(localtimer, 0, max_thread_num * sizeof(ptr_synctimer_nphj_t));
	memset(globaltimer, 0, sizeof(ptr_synctimer_nphj_t));
#endif /* SYNCSTATS */

	cpu_set_t set;
	int cpu_idx;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	/** allocate a hashtable **/

	size_t num_entrys = next_pow_2(datameta.r_tuple_num);
	hashmask = num_entrys - 1;
	bitskip = 0;

#ifdef USE_NVM
	ptr_hash_entry_t *hashtable = (ptr_hash_entry_t *) new_alloc_nvm(sizeof(ptr_hash_entry_t) * num_entrys);
#else /* USE_NVM */
#ifdef USE_HUGE
	ptr_hash_entry_t *hashtable = (ptr_hash_entry_t *) alloc_huge_dram(sizeof(ptr_hash_entry_t) * num_entrys);
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	ptr_hash_entry_t *hashtable = (ptr_hash_entry_t *) alloc_aligned_dram(sizeof(ptr_hash_entry_t) * num_entrys, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */

	size_t r_tuple_num_thr = datameta.r_tuple_num / BUILDPART_THREAD_NUM;
	size_t s_tuple_num_thr = datameta.s_tuple_num / PROBEJOIN_THREAD_NUM;

#ifdef USE_PAPI
	long long PAPI_values[NUM_PAPI_EVENT];
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
	long long PAPI_values_retrieve[NUM_PAPI_EVENT];
	memset(PAPI_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_retrieve, 0, sizeof(long long) * NUM_PAPI_EVENT );

	/* papi library initialization */
	if ( (rv = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
		printf("Library initialization error! \n");
		exit(1);
	}

	/** PAPI_thread_init initializes thread support in the PAPI library. 
	 * 	Itshould be called only once, just after PAPI_library_init (3), 
	 * 	and before any other PAPI calls. Applications that make no use 
	 * 	of threads do not need to call this routine.
	 */
	rv = PAPI_thread_init( (unsigned long(*) (void) ) (pthread_self) );
	if ( (rv = PAPI_library_init(PAPI_VER_CURRENT) ) != PAPI_VER_CURRENT) {
		printf("Library initialization error! \n");
		exit(1);
	}

	/* Enable and initialize multiplex support */
	rv = PAPI_multiplex_init();
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}
#endif /* USE_PAPI */


	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &buildpart_barrier;

		(args+idx)->r_rel.tuple_num = (idx == BUILDPART_THREAD_NUM -1) ? (datameta.r_tuple_num - idx * r_tuple_num_thr) : r_tuple_num_thr;
		(args+idx)->r_rel.tuples = r_relation.tuples + r_tuple_num_thr * idx;
		
		(args+idx)->hashtable = hashtable;
		(args+idx)->num_entrys = num_entrys;

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, ptr_run_nphj_lp_buildpart, args + idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}

	}

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_buildpart[jdx] += (args+idx)->PAPI_values_buildpart[jdx];
		}
#endif /* USE_PAPI */
	}


	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &probejoin_barrier;

		(args+idx)->s_rel.tuple_num = (idx == PROBEJOIN_THREAD_NUM -1) ? (datameta.s_tuple_num - idx * s_tuple_num_thr) : s_tuple_num_thr;
		(args+idx)->s_rel.tuples = s_relation.tuples + s_tuple_num_thr * idx;
		
		(args+idx)->hashtable = hashtable;

		(args+idx)->matched_cnt = 0;

		(args+idx)->mater = mater;

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, ptr_run_nphj_lp_probejoin, args + idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}

	}

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
		matched_cnt += (args+idx)->matched_cnt;
		mater_cnt[idx] = (args+idx)->matched_cnt;
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_probejoin[jdx] += (args+idx)->PAPI_values_probejoin[jdx];
		}
#endif /* USE_PAPI */
	}


	for (int idx = 0; idx < RETRIEVE_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &retrieve_barrier;
		
		(args+idx)->checksum = 0;

		(args+idx)->mater = mater;
		(args+idx)->mater_cnt = mater_cnt;

		(args+idx)->row_id_pairs_processed = 0;

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

		(args+idx)->org_relR = &r_relation;
		(args+idx)->org_relS = &s_relation;

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_retrieve, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, ptr_run_nphj_lp_retrieve, args + idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}

	}

	for (int idx = 0; idx < RETRIEVE_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
		checksum += (args+idx)->checksum;
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_retrieve[jdx] += (args+idx)->PAPI_values_retrieve[jdx];
		}
#endif /* USE_PAPI */
	}


	purple();
	printf("matched_cnt: %zu\tcheck_sum: %zu\n", matched_cnt, checksum);

#ifdef SYNCSTATS
	printf("[SYNCSTATS] TID\tR_TUPS\tPRE_MEMSET\tBUILD\n");
	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%zu\t%.9f\t%.9f\n",
			idx, (args+idx)->r_rel.tuple_num,
			diff_sec( (args+idx)->localtimer->sync_pre_memset,
				(args+idx)->globaltimer->sync_pre_memset ),
			diff_sec( (args+idx)->localtimer->sync_build,
				(args+idx)->globaltimer->sync_build )
		);
	}

	printf("[SYNCSTATS] TID\tS_TUPS\tPROBE\n");
	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%zu\t%.9f\n",
			idx, (args+idx)->s_rel.tuple_num,
			diff_sec( (args+idx)->localtimer->sync_probe,
				(args+idx)->globaltimer->sync_probe )
		);
	}

	printf("[SYNCSTATS] TID\tPAIRS\tRETRIEVE\n");
	for (int idx = 0; idx < RETRIEVE_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%zu\t%.9f\n", idx, (args+idx)->row_id_pairs_processed,
			diff_sec( (args+idx)->localtimer->sync_retrieve,
				(args+idx)->globaltimer->sync_retrieve )
		);
	}
#endif /* SYNCSTATS */


#ifdef USE_PMWATCH
	for (int idx = 0; idx < NVMCount; idx ++) {
		PMWatch_output[idx].total_bytes_read = PMWatch_output_buildpart[idx].total_bytes_read + 
			PMWatch_output_probejoin[idx].total_bytes_read + PMWatch_output_retrieve[idx].total_bytes_read;
		PMWatch_output[idx].total_bytes_written = PMWatch_output_buildpart[idx].total_bytes_written + 
			PMWatch_output_probejoin[idx].total_bytes_written + PMWatch_output_retrieve[idx].total_bytes_written;
		PMWatch_output[idx].media_read_ops = PMWatch_output_buildpart[idx].media_read_ops + 
			PMWatch_output_probejoin[idx].media_read_ops + PMWatch_output_retrieve[idx].media_read_ops;
		PMWatch_output[idx].media_write_ops = PMWatch_output_buildpart[idx].media_write_ops +
			PMWatch_output_probejoin[idx].media_write_ops + PMWatch_output_retrieve[idx].media_write_ops;
		PMWatch_output[idx].read_64B_ops_received = PMWatch_output_buildpart[idx].read_64B_ops_received + 
			PMWatch_output_probejoin[idx].read_64B_ops_received + PMWatch_output_retrieve[idx].read_64B_ops_received;
		PMWatch_output[idx].write_64B_ops_received = PMWatch_output_buildpart[idx].write_64B_ops_received + 
			PMWatch_output_probejoin[idx].write_64B_ops_received + PMWatch_output_retrieve[idx].write_64B_ops_received;
		PMWatch_output[idx].host_reads = PMWatch_output_buildpart[idx].host_reads + 
			PMWatch_output_probejoin[idx].host_reads + PMWatch_output_retrieve[idx].host_reads;
		PMWatch_output[idx].host_writes = PMWatch_output_buildpart[idx].host_writes +
			PMWatch_output_probejoin[idx].host_writes + PMWatch_output_retrieve[idx].host_writes;
		PMWatch_output[idx].read_hit_ratio = (PMWatch_output_buildpart[idx].read_hit_ratio + 
			PMWatch_output_probejoin[idx].read_hit_ratio + PMWatch_output_retrieve[idx].read_hit_ratio) / 3;
		PMWatch_output[idx].write_hit_ratio = (PMWatch_output_buildpart[idx].write_hit_ratio + 
			PMWatch_output_probejoin[idx].write_hit_ratio + PMWatch_output_retrieve[idx].write_hit_ratio) / 3;
		PMWatch_output[idx].read_amplification = (PMWatch_output_buildpart[idx].read_amplification + 
			PMWatch_output_probejoin[idx].read_amplification + PMWatch_output_retrieve[idx].read_amplification) / 3;
		PMWatch_output[idx].write_amplification = (PMWatch_output_buildpart[idx].write_amplification +
			PMWatch_output_probejoin[idx].write_amplification + PMWatch_output_retrieve[idx].write_amplification) / 3;
		printf("[PMWatch] DIMM: %d\n", idx);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_read: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].total_bytes_read, PMWatch_output_buildpart[idx].total_bytes_read, 
			PMWatch_output_probejoin[idx].total_bytes_read, PMWatch_output_retrieve[idx].total_bytes_read
		);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_written: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].total_bytes_written, PMWatch_output_buildpart[idx].total_bytes_written, 
			PMWatch_output_probejoin[idx].total_bytes_written, PMWatch_output_retrieve[idx].total_bytes_written
		);
		printf("[PMWatch] DIMM: %d\tmedia_read_ops: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].media_read_ops, PMWatch_output_buildpart[idx].media_read_ops, 
			PMWatch_output_probejoin[idx].media_read_ops, PMWatch_output_retrieve[idx].media_read_ops
		);
		printf("[PMWatch] DIMM: %d\tmedia_write_ops: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].media_write_ops, PMWatch_output_buildpart[idx].media_write_ops, 
			PMWatch_output_probejoin[idx].media_write_ops, PMWatch_output_retrieve[idx].media_write_ops
		);
		printf("[PMWatch] DIMM: %d\tread_64B_ops_received: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].read_64B_ops_received, PMWatch_output_buildpart[idx].read_64B_ops_received,
			PMWatch_output_probejoin[idx].read_64B_ops_received, PMWatch_output_retrieve[idx].read_64B_ops_received
		);
		printf("[PMWatch] DIMM: %d\twrite_64B_ops_received: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].write_64B_ops_received, PMWatch_output_buildpart[idx].write_64B_ops_received,
			PMWatch_output_probejoin[idx].write_64B_ops_received, PMWatch_output_retrieve[idx].write_64B_ops_received
		);
		printf("[PMWatch] DIMM: %d\thost_reads: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].host_reads, PMWatch_output_buildpart[idx].host_reads, 
			PMWatch_output_probejoin[idx].host_reads, PMWatch_output_retrieve[idx].host_reads
		);
		printf("[PMWatch] DIMM: %d\thost_writes: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].host_writes, PMWatch_output_buildpart[idx].host_writes, 
			PMWatch_output_probejoin[idx].host_writes, PMWatch_output_retrieve[idx].host_writes
		);
		printf("[PMWatch] DIMM: %d\tread_hit_ratio: %f\t%f\t%f\t%f\n", idx, 
			PMWatch_output[idx].read_hit_ratio, PMWatch_output_buildpart[idx].read_hit_ratio, 
			PMWatch_output_probejoin[idx].read_hit_ratio, PMWatch_output_retrieve[idx].read_hit_ratio
		);
		printf("[PMWatch] DIMM: %d\twrite_hit_ratio: %f\t%f\t%f\t%f\n", idx, 
			PMWatch_output[idx].write_hit_ratio, PMWatch_output_buildpart[idx].write_hit_ratio, 
			PMWatch_output_probejoin[idx].write_hit_ratio, PMWatch_output_retrieve[idx].write_hit_ratio
		);
		printf("[PMWatch] DIMM: %d\tread_amplification: %.9f\t%.9f\t %.9f\t %.9f\n", idx, 
			PMWatch_output[idx].read_amplification, PMWatch_output_buildpart[idx].read_amplification, 
			PMWatch_output_probejoin[idx].read_amplification, PMWatch_output_retrieve[idx].read_amplification
		);
		printf("[PMWatch] DIMM: %d\twrite_amplification: %.9f\t%.9f\t %.9f\t %.9f\n", idx, 
			PMWatch_output[idx].write_amplification, PMWatch_output_buildpart[idx].write_amplification, 
			PMWatch_output_probejoin[idx].write_amplification, PMWatch_output_retrieve[idx].write_amplification
		);
		printf("\n");
		if (cur_t != 0) {
			agg_PMWatch_output[idx].total_bytes_read += PMWatch_output[idx].total_bytes_read;
			agg_PMWatch_output[idx].total_bytes_written += PMWatch_output[idx].total_bytes_written;
			agg_PMWatch_output[idx].media_read_ops += PMWatch_output[idx].media_read_ops;
			agg_PMWatch_output[idx].media_write_ops += PMWatch_output[idx].media_write_ops;
			agg_PMWatch_output[idx].read_64B_ops_received += PMWatch_output[idx].read_64B_ops_received;
			agg_PMWatch_output[idx].write_64B_ops_received += PMWatch_output[idx].write_64B_ops_received;
			agg_PMWatch_output[idx].host_reads += PMWatch_output[idx].host_reads;
			agg_PMWatch_output[idx].host_writes += PMWatch_output[idx].host_writes;
			agg_PMWatch_output[idx].read_hit_ratio += PMWatch_output[idx].read_hit_ratio;
			agg_PMWatch_output[idx].write_hit_ratio += PMWatch_output[idx].write_hit_ratio;
			agg_PMWatch_output[idx].read_amplification += PMWatch_output[idx].read_amplification;
			agg_PMWatch_output[idx].write_amplification += PMWatch_output[idx].write_amplification;

			agg_PMWatch_output_buildpart[idx].total_bytes_read += PMWatch_output_buildpart[idx].total_bytes_read;
			agg_PMWatch_output_buildpart[idx].total_bytes_written += PMWatch_output_buildpart[idx].total_bytes_written;
			agg_PMWatch_output_buildpart[idx].media_read_ops += PMWatch_output_buildpart[idx].media_read_ops;
			agg_PMWatch_output_buildpart[idx].media_write_ops += PMWatch_output_buildpart[idx].media_write_ops;
			agg_PMWatch_output_buildpart[idx].read_64B_ops_received += PMWatch_output_buildpart[idx].read_64B_ops_received;
			agg_PMWatch_output_buildpart[idx].write_64B_ops_received += PMWatch_output_buildpart[idx].write_64B_ops_received;
			agg_PMWatch_output_buildpart[idx].host_reads += PMWatch_output_buildpart[idx].host_reads;
			agg_PMWatch_output_buildpart[idx].host_writes += PMWatch_output_buildpart[idx].host_writes;
			agg_PMWatch_output_buildpart[idx].read_hit_ratio += PMWatch_output_buildpart[idx].read_hit_ratio;
			agg_PMWatch_output_buildpart[idx].write_hit_ratio += PMWatch_output_buildpart[idx].write_hit_ratio;
			agg_PMWatch_output_buildpart[idx].read_amplification += PMWatch_output_buildpart[idx].read_amplification;
			agg_PMWatch_output_buildpart[idx].write_amplification += PMWatch_output_buildpart[idx].write_amplification;

			agg_PMWatch_output_probejoin[idx].total_bytes_read += PMWatch_output_probejoin[idx].total_bytes_read;
			agg_PMWatch_output_probejoin[idx].total_bytes_written += PMWatch_output_probejoin[idx].total_bytes_written;
			agg_PMWatch_output_probejoin[idx].media_read_ops += PMWatch_output_probejoin[idx].media_read_ops;
			agg_PMWatch_output_probejoin[idx].media_write_ops += PMWatch_output_probejoin[idx].media_write_ops;
			agg_PMWatch_output_probejoin[idx].read_64B_ops_received += PMWatch_output_probejoin[idx].read_64B_ops_received;
			agg_PMWatch_output_probejoin[idx].write_64B_ops_received += PMWatch_output_probejoin[idx].write_64B_ops_received;
			agg_PMWatch_output_probejoin[idx].host_reads += PMWatch_output_probejoin[idx].host_reads;
			agg_PMWatch_output_probejoin[idx].host_writes += PMWatch_output_probejoin[idx].host_writes;
			agg_PMWatch_output_probejoin[idx].read_hit_ratio += PMWatch_output_probejoin[idx].read_hit_ratio;
			agg_PMWatch_output_probejoin[idx].write_hit_ratio += PMWatch_output_probejoin[idx].write_hit_ratio;
			agg_PMWatch_output_probejoin[idx].read_amplification += PMWatch_output_probejoin[idx].read_amplification;
			agg_PMWatch_output_probejoin[idx].write_amplification += PMWatch_output_probejoin[idx].write_amplification;
		}
	}
#endif /* USE_PMWATCH */
#ifdef USE_PAPI
	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		PAPI_values[idx] += PAPI_values_buildpart[idx] + PAPI_values_probejoin[idx] + PAPI_values_retrieve[idx];
		printf("[PAPI] PAPI_event_name: %s", PAPI_event_names[idx]);
		printf("\ttotal_counter: %lld", PAPI_values[idx]);
		printf("\tbuildpart_counter: %lld", PAPI_values_buildpart[idx]);
		printf("\tprobejoin_counter: %lld", PAPI_values_probejoin[idx]);
		printf("\tretrieve_counter: %lld", PAPI_values_retrieve[idx]);
		printf("\n");
		if (cur_t != 0) {
			agg_PAPI_values[idx] += PAPI_values[idx];
			agg_PAPI_values_buildpart[idx] += PAPI_values_buildpart[idx];
			agg_PAPI_values_probejoin[idx] += PAPI_values_probejoin[idx];
		}
	}
	PAPI_shutdown();
#endif /* USE_PAPI */

#ifdef SYNCSTATS
	free(localtimer);
	free(globaltimer);
#endif /* SYNCSTATS */

	pthread_attr_destroy(&attr);
	pthread_barrier_destroy(&buildpart_barrier);
	pthread_barrier_destroy(&probejoin_barrier);
	pthread_barrier_destroy(&retrieve_barrier);
	dealloc_memory(hashtable, sizeof(ptr_hash_entry_t) * num_entrys );
	dealloc_memory(r_relation.tuples, TUPLE_T_SIZE * r_relation.tuple_num);
	dealloc_memory(s_relation.tuples, TUPLE_T_SIZE * s_relation.tuple_num);

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		size_t tot_row_id_pair_num = (args+idx)->s_rel.tuple_num;
		dealloc_memory(mater[idx], ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);
	}

	dealloc_memory(mater, sizeof(row_id_pair_t *) * PROBEJOIN_THREAD_NUM);

	timekeeper->buildpart = diff_sec(args[0].build_start, args[0].build_end);
	timekeeper->probejoin = diff_sec(args[0].probe_start, args[0].probe_end);
	timekeeper->total = timekeeper->buildpart + timekeeper->probejoin + diff_sec(args[0].retrieve_start, args[0].retrieve_end);

}


#endif /* RUN_PAYLOAD */