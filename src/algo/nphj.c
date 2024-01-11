#include <libpmem.h>

#include "nphj.h"
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
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_buildpart[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_probejoin[MaxNVMNum];
#endif /* USE_PMWATCH */

extern int cur_t;

typedef struct nphj_sc_arg_t {
	int _tid;
	pthread_barrier_t *barrier;

	relation_t r_rel;
	relation_t s_rel;

	hashtable_t *hashtable;
	bucket_buffer_t *overflowbuf;

	size_t matched_cnt;
	rowid_t checksum;

	struct timespec build_start, build_end, probe_start, probe_end;

#ifdef SYNCSTATS
	synctimer_nphj_t *localtimer;
	synctimer_nphj_t *globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

} nphj_sc_arg_t;


typedef struct nphj_lp_arg_t {
	int _tid;
	pthread_barrier_t *barrier;

	relation_t r_rel;
	relation_t s_rel;
	
	hash_entry_t *hashtable;
	size_t num_entrys;

	size_t matched_cnt;
	rowid_t checksum;

	struct timespec build_start, build_end, probe_start, probe_end;

#ifdef SYNCSTATS
	synctimer_nphj_t *localtimer;
	synctimer_nphj_t *globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

} nphj_lp_arg_t;


static key_t_ hashmask;
static key_t_ bitskip;


void init_bucket_buffer(bucket_buffer_t ** ppbuf) {
	bucket_buffer_t * overflowbuf;
	overflowbuf = (bucket_buffer_t *) alloc_memory(sizeof(bucket_buffer_t), CACHELINE_SIZE);
	overflowbuf->count = 0;
	overflowbuf->next = NULL;
	*ppbuf = overflowbuf;
}

void get_new_bucket(bucket_t ** result, bucket_buffer_t ** buf) {
	if( (*buf)->count < OVERFLOW_BUF_SIZE ) {
		*result = (*buf)->buf + (*buf)->count;
		(*buf)->count ++;
	} else {
		/* need to allocate new buffer */
		bucket_buffer_t * new_buf = (bucket_buffer_t*) alloc_memory(sizeof(bucket_buffer_t), CACHELINE_SIZE);
		new_buf->count = 1;
		new_buf->next  = *buf;
		*buf	= new_buf;
		*result = new_buf->buf;
	}
}


void free_bucket_buffer(bucket_buffer_t *overflowbuf) {
	size_t size = sizeof(bucket_buffer_t);
	do {
		bucket_buffer_t * tmp = overflowbuf->next;
		dealloc_memory(overflowbuf, size);
		overflowbuf = tmp;
	} while (overflowbuf);
}


void build_hashtable_nphj_sc(hashtable_t *ht, relation_t * rel, bucket_buffer_t ** overflowbuf, int _tid) {
	tuple_t *dest;
	bucket_t *curr, *nxt;
	hashkey_t tmp_hashkey;

#ifdef PREFETCH_DISTANCE
	size_t prefetch_index = next_pow_2(PREFETCH_DISTANCE);
	hashkey_t hashkey_prefetch;

#ifdef PREFETCH_CACHE
	key_t_ cachemask = prefetch_index - 1;
	size_t cache_idx;
	sc_prefetch_cache_t *sc_prefetch_cache;
	size_t cache_jdx;
#ifdef PREFETCH_CACHE_NVM
	sc_prefetch_cache = (sc_prefetch_cache_t *) new_alloc_nvm(sizeof(sc_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	sc_prefetch_cache = (sc_prefetch_cache_t *) alloc_dram(sizeof(sc_prefetch_cache_t) * prefetch_index, CACHELINE_SIZE);
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
				bucket_t * b;
				get_new_bucket(&b, overflowbuf);
				curr->next = b;
				b->next	= nxt;
				b->count = 1;
				dest = b->tuples;
			} else {
				dest = nxt->tuples + nxt->count;
				(nxt->count) ++;
			}
		} else {
			dest = curr->tuples + curr->count;
			(curr->count) ++;
		}

		unlock(&curr->latch);
		
		store_a_tuple(dest, &rel->tuples[idx]);

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
				bucket_t * b;
				get_new_bucket(&b, overflowbuf);
				curr->next = b;
				b->next	= nxt;
				b->count = 1;
				dest = b->tuples;
			} else {
				dest = nxt->tuples + nxt->count;
				(nxt->count) ++;
			}
		} else {
			dest = curr->tuples + curr->count;
			(curr->count) ++;
		}

		unlock(&curr->latch);

		store_a_tuple(dest, &rel->tuples[idx]);
	}

#ifdef PREFETCH_CACHE_NVM
	pmem_unmap(sc_prefetch_cache, sizeof(sc_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	dealloc_dram(sc_prefetch_cache, sizeof(sc_prefetch_cache_t) * prefetch_index);
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
				bucket_t * b;
				get_new_bucket(&b, overflowbuf);
				curr->next = b;
				b->next	= nxt;
				b->count = 1;
				dest = b->tuples;
			} else {
				dest = nxt->tuples + nxt->count;
				(nxt->count) ++;
			}
		} else {
			dest = curr->tuples + curr->count;
			(curr->count) ++;
		}

		unlock(&curr->latch);

		store_a_tuple(dest, &rel->tuples[idx]);
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
				bucket_t * b;
				get_new_bucket(&b, overflowbuf);
				curr->next = b;
				b->next	= nxt;
				b->count = 1;
				dest = b->tuples;
			} else {
				dest = nxt->tuples + nxt->count;
				(nxt->count) ++;
			}
		} else {
			dest = curr->tuples + curr->count;
			(curr->count) ++;
		}

		unlock(&curr->latch);

		store_a_tuple(dest, &rel->tuples[idx]);
	}

#endif /* PREFETCH_DISTANCE */
}


void probe_hashtable_nphj_sc(hashtable_t *ht, relation_t *rel, size_t *matched_cnt, rowid_t *checksum) {
	hashkey_t tmp_hashkey;
	bucket_t *b;

	short matched_flag;

#ifdef PREFETCH_DISTANCE
	size_t prefetch_index = next_pow_2(PREFETCH_DISTANCE);
	hashkey_t hashkey_prefetch;

#ifdef PREFETCH_CACHE
	key_t_ cachemask = prefetch_index - 1;
	size_t cache_idx;
	sc_prefetch_cache_t *sc_prefetch_cache;
	size_t cache_jdx;

#ifdef PREFETCH_CACHE_NVM
	sc_prefetch_cache = (sc_prefetch_cache_t *) new_alloc_nvm(sizeof(sc_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	sc_prefetch_cache = (sc_prefetch_cache_t *) alloc_dram(sizeof(sc_prefetch_cache_t) * prefetch_index, CACHELINE_SIZE);
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
				// if (rel->tuples[idx].key == b->tuples[jdx].key) {
				// 	(*matched_cnt) ++;
				// 	*checksum += rel->tuples[idx].row_id + b->tuples[jdx].row_id;
				// }

				matched_flag = (rel->tuples[idx].key == b->tuples[jdx].key);
				*matched_cnt += matched_flag;
				*checksum += (rel->tuples[idx].row_id + b->tuples[jdx].row_id)*matched_flag;
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
				// if (rel->tuples[idx].key == b->tuples[jdx].key) {
				// 	(*matched_cnt) ++;
				// 	*checksum += rel->tuples[idx].row_id + b->tuples[jdx].row_id;
				// }

				matched_flag = (rel->tuples[idx].key == b->tuples[jdx].key);
				*matched_cnt += matched_flag;
				*checksum += (rel->tuples[idx].row_id + b->tuples[jdx].row_id)*matched_flag;
			}

			/* follow overflow pointer */
			b = b->next;
		} while (b);

	}

#ifdef PREFETCH_CACHE_NVM
	pmem_unmap(sc_prefetch_cache, sizeof(sc_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	dealloc_dram(sc_prefetch_cache, sizeof(sc_prefetch_cache_t) * prefetch_index);
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
				// if (rel->tuples[idx].key == b->tuples[jdx].key){
				// 	(*matched_cnt) ++;
				// 	*checksum += rel->tuples[idx].row_id + b->tuples[jdx].row_id;
				// }

				matched_flag = (rel->tuples[idx].key == b->tuples[jdx].key);
				*matched_cnt += matched_flag;
				*checksum += (rel->tuples[idx].row_id + b->tuples[jdx].row_id)*matched_flag;
			}
			
			/* follow overflow pointer */
			b = b->next;
		} while (b);
	}
#endif /* PREFETCH_CACHE */
#else /* PREFETCH_DISTANCE */
	for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
		tmp_hashkey = IDHASH(rel->tuples[idx].key, hashmask, bitskip);
		b = ht->buckets + tmp_hashkey;

		do {
			for (size_t jdx = 0; jdx < b->count; jdx++) {
				// if (rel->tuples[idx].key == b->tuples[jdx].key){
				// 	(*matched_cnt) ++;
				// 	*checksum += rel->tuples[idx].row_id + b->tuples[jdx].row_id;
				// }

				matched_flag = (rel->tuples[idx].key == b->tuples[jdx].key);
				*matched_cnt += matched_flag;
				*checksum += (rel->tuples[idx].row_id + b->tuples[jdx].row_id)*matched_flag;
			}

			/* follow overflow pointer */
			b = b->next;
		} while (b);
	}

#endif /* PREFETCH_DISTANCE */
}


void *run_nphj_sc_buildpart(void *arg) {
	nphj_sc_arg_t *nphj_sc_arg = (nphj_sc_arg_t *) arg;

#ifdef WARMUP
	warmup_localize(nphj_sc_arg->r_rel.tuples, nphj_sc_arg->r_rel.tuple_num);
#endif /* WARMUP */

#ifndef UN_PREPOPULATE
	/* eliminate the page faults of the hash table */
	size_t memset_num_buckets_thr = nphj_sc_arg->hashtable->num_buckets / BUILDPART_THREAD_NUM;
	size_t memset_num_buckets_size = (nphj_sc_arg->_tid == BUILDPART_THREAD_NUM -1) ? 
		sizeof(bucket_t) * (nphj_sc_arg->hashtable->num_buckets - nphj_sc_arg->_tid * memset_num_buckets_thr) : 
		sizeof(bucket_t) * memset_num_buckets_thr;
	memset_localize(nphj_sc_arg->hashtable->buckets + nphj_sc_arg->_tid * memset_num_buckets_thr, 
		memset_num_buckets_size );

	bucket_buffer_t *overflowbuf;
	init_bucket_buffer(&overflowbuf);
#endif /* UN_PREPOPULATE */

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

#ifdef UN_PREPOPULATE
	/* eliminate the page faults of the hash table */
	// size_t memset_num_buckets_thr = nphj_sc_arg->hashtable->num_buckets / BUILDPART_THREAD_NUM;
	// size_t memset_num_buckets_size = (nphj_sc_arg->_tid == BUILDPART_THREAD_NUM -1) ? 
	// 	sizeof(bucket_t) * (nphj_sc_arg->hashtable->num_buckets - nphj_sc_arg->_tid * memset_num_buckets_thr) : 
	// 	sizeof(bucket_t) * memset_num_buckets_thr;
	// memset_localize(nphj_sc_arg->hashtable->buckets + nphj_sc_arg->_tid * memset_num_buckets_thr, 
	// 	memset_num_buckets_size );

	/** intiate overflow buffers **/
	bucket_buffer_t *overflowbuf;
	init_bucket_buffer(&overflowbuf);
#endif /* UN_PREPOPULATE */

#ifdef SYNCSTATS
	SYNC_TIMER(nphj_sc_arg->localtimer->sync_pre_memset);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(nphj_sc_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(nphj_sc_arg->_tid, nphj_sc_arg->globaltimer->sync_pre_memset);
#endif /* SYNCSTATS */

	/** build the hash table **/
	build_hashtable_nphj_sc(nphj_sc_arg->hashtable, &nphj_sc_arg->r_rel, &overflowbuf, nphj_sc_arg->_tid);

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

	// /** clean-up the overflow buffers **/
	// free_bucket_buffer(overflowbuf);

	nphj_sc_arg->overflowbuf = overflowbuf;

	return NULL;
}


void *run_nphj_sc_probejoin(void *arg) {
	nphj_sc_arg_t *nphj_sc_arg = (nphj_sc_arg_t *) arg;

#ifdef WARMUP
	warmup_localize(nphj_sc_arg->s_rel.tuples, nphj_sc_arg->s_rel.tuple_num);
#endif /* WARMUP */

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

	probe_hashtable_nphj_sc(nphj_sc_arg->hashtable, &nphj_sc_arg->s_rel, &nphj_sc_arg->matched_cnt, &nphj_sc_arg->checksum);

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

	return NULL;
}


void nphj_sc(const datameta_t datameta, timekeeper_t * const timekeeper) {
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

	int max_thread_num = MAX(BUILDPART_THREAD_NUM, PROBEJOIN_THREAD_NUM);

	pthread_t threadpool[max_thread_num];
	nphj_sc_arg_t args[max_thread_num];

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

#ifdef SYNCSTATS
	synctimer_nphj_t *localtimer = (synctimer_nphj_t *) malloc( max_thread_num * sizeof(synctimer_nphj_t) );
	synctimer_nphj_t *globaltimer = (synctimer_nphj_t *) malloc( sizeof(synctimer_nphj_t) );
	memset(localtimer, 0, max_thread_num * sizeof(synctimer_nphj_t));
	memset(globaltimer, 0, sizeof(synctimer_nphj_t));
#endif /* SYNCSTATS */

	cpu_set_t set;
	int cpu_idx;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	/** allocate a hashtable **/
	// hashtable_t* hashtable = (hashtable_t *) alloc_memory(sizeof(hashtable_t), CACHELINE_SIZE);
	hashtable_t hashtable;
	hashtable.num_buckets = datameta.r_tuple_num / BUCKET_CAPACITY;

#ifdef USE_NVM
	hashtable.buckets = (bucket_t *) new_alloc_nvm(sizeof(bucket_t) * hashtable.num_buckets);
#else /* USE_NVM */
#ifdef USE_HUGE
	hashtable.buckets = (bucket_t *) alloc_huge_dram(sizeof(bucket_t) * hashtable.num_buckets);
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	hashtable.buckets = (bucket_t *) alloc_aligned_dram(sizeof(bucket_t) * hashtable.num_buckets, CACHELINE_SIZE);
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
	memset(PAPI_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );

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

		rv = pthread_create(&threadpool[idx], &attr, run_nphj_sc_buildpart, args + idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_buildpart[jdx] += (args + idx) -> PAPI_values_buildpart[jdx];
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
		(args+idx)->checksum = 0;

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, run_nphj_sc_probejoin, args + idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
		matched_cnt += (args+idx)->matched_cnt;
		checksum += (args+idx)->checksum;
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_probejoin[jdx] += (args + idx) -> PAPI_values_probejoin[jdx];
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
#endif /* SYNCSTATS */

#ifdef USE_PMWATCH
	for (int idx = 0; idx < NVMCount; idx ++) {
		PMWatch_output[idx].total_bytes_read = PMWatch_output_buildpart[idx].total_bytes_read + 
			PMWatch_output_probejoin[idx].total_bytes_read;
		PMWatch_output[idx].total_bytes_written = PMWatch_output_buildpart[idx].total_bytes_written + 
			PMWatch_output_probejoin[idx].total_bytes_written;
		PMWatch_output[idx].media_read_ops = PMWatch_output_buildpart[idx].media_read_ops + 
			PMWatch_output_probejoin[idx].media_read_ops;
		PMWatch_output[idx].media_write_ops = PMWatch_output_buildpart[idx].media_write_ops +
			PMWatch_output_probejoin[idx].media_write_ops;
		PMWatch_output[idx].read_64B_ops_received = PMWatch_output_buildpart[idx].read_64B_ops_received + 
			PMWatch_output_probejoin[idx].read_64B_ops_received;
		PMWatch_output[idx].write_64B_ops_received = PMWatch_output_buildpart[idx].write_64B_ops_received + 
			PMWatch_output_probejoin[idx].write_64B_ops_received;
		PMWatch_output[idx].host_reads = PMWatch_output_buildpart[idx].host_reads + 
			PMWatch_output_probejoin[idx].host_reads;
		PMWatch_output[idx].host_writes = PMWatch_output_buildpart[idx].host_writes +
			PMWatch_output_probejoin[idx].host_writes;
		PMWatch_output[idx].read_hit_ratio = (PMWatch_output_buildpart[idx].read_hit_ratio + 
			PMWatch_output_probejoin[idx].read_hit_ratio) / 2;
		PMWatch_output[idx].write_hit_ratio = (PMWatch_output_buildpart[idx].write_hit_ratio + 
			PMWatch_output_probejoin[idx].write_hit_ratio) / 2;
		PMWatch_output[idx].read_amplification = (PMWatch_output_buildpart[idx].read_amplification + 
			PMWatch_output_probejoin[idx].read_amplification) / 2;
		PMWatch_output[idx].write_amplification = (PMWatch_output_buildpart[idx].write_amplification +
			PMWatch_output_probejoin[idx].write_amplification) / 2;
		printf("[PMWatch] DIMM: %d\n", idx);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_read: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].total_bytes_read, 
			PMWatch_output_buildpart[idx].total_bytes_read, PMWatch_output_probejoin[idx].total_bytes_read);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_written: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].total_bytes_written,
			PMWatch_output_buildpart[idx].total_bytes_written, PMWatch_output_probejoin[idx].total_bytes_written);
		printf("[PMWatch] DIMM: %d\tmedia_read_ops: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].media_read_ops,
			PMWatch_output_buildpart[idx].media_read_ops, PMWatch_output_probejoin[idx].media_read_ops);
		printf("[PMWatch] DIMM: %d\tmedia_write_ops: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].media_write_ops,
			PMWatch_output_buildpart[idx].media_write_ops, PMWatch_output_probejoin[idx].media_write_ops);
		printf("[PMWatch] DIMM: %d\tread_64B_ops_received: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].read_64B_ops_received,
			PMWatch_output_buildpart[idx].read_64B_ops_received, PMWatch_output_probejoin[idx].read_64B_ops_received);
		printf("[PMWatch] DIMM: %d\twrite_64B_ops_received: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].write_64B_ops_received,
			PMWatch_output_buildpart[idx].write_64B_ops_received, PMWatch_output_probejoin[idx].write_64B_ops_received);
		printf("[PMWatch] DIMM: %d\thost_reads: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].host_reads,
			PMWatch_output_buildpart[idx].host_reads, PMWatch_output_probejoin[idx].host_reads);
		printf("[PMWatch] DIMM: %d\thost_writes: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].host_writes,
			PMWatch_output_buildpart[idx].host_writes, PMWatch_output_probejoin[idx].host_writes);
		printf("[PMWatch] DIMM: %d\tread_hit_ratio: %f\t%f\t%f\n", idx, PMWatch_output[idx].read_hit_ratio,
			PMWatch_output_buildpart[idx].read_hit_ratio, PMWatch_output_probejoin[idx].read_hit_ratio);
		printf("[PMWatch] DIMM: %d\twrite_hit_ratio: %f\t%f\t%f\n", idx, PMWatch_output[idx].write_hit_ratio,
			PMWatch_output_buildpart[idx].write_hit_ratio, PMWatch_output_probejoin[idx].write_hit_ratio);
		printf("[PMWatch] DIMM: %d\tread_amplification: %.9f\t%.9f\t %.9f\n", idx, PMWatch_output[idx].read_amplification,
			PMWatch_output_buildpart[idx].read_amplification, PMWatch_output_probejoin[idx].read_amplification);
		printf("[PMWatch] DIMM: %d\twrite_amplification: %.9f\t%.9f\t %.9f\n", idx, PMWatch_output[idx].write_amplification,
			PMWatch_output_buildpart[idx].write_amplification, PMWatch_output_probejoin[idx].write_amplification);
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
		PAPI_values[idx] += PAPI_values_buildpart[idx] + PAPI_values_probejoin[idx];
		printf("[PAPI] PAPI_event_name: %s", PAPI_event_names[idx]);
		printf("\ttotal_counter: %lld", PAPI_values[idx]);
		printf("\tbuildpart_counter: %lld", PAPI_values_buildpart[idx]);
		printf("\tprobejoin_counter: %lld", PAPI_values_probejoin[idx]);
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
	dealloc_memory(hashtable.buckets, sizeof(bucket_t) * hashtable.num_buckets);
	// memsync(r_relation.tuples, TUPLE_T_SIZE * r_relation.tuple_num);
	// memsync(s_relation.tuples, TUPLE_T_SIZE * s_relation.tuple_num);
	dealloc_memory(r_relation.tuples, TUPLE_T_SIZE * r_relation.tuple_num);
	dealloc_memory(s_relation.tuples, TUPLE_T_SIZE * s_relation.tuple_num);
	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		free_bucket_buffer((args+idx)->overflowbuf);
	}

	timekeeper->buildpart = diff_sec(args[0].build_start, args[0].build_end);
	timekeeper->probejoin = diff_sec(args[0].probe_start, args[0].probe_end);
	timekeeper->total = timekeeper->buildpart + timekeeper->probejoin;

}


#ifdef LOCK_IN_BUCKET
int acquire_lock_and_store(hash_entry_t *hash_entry, hashkey_t hashkey) {
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


void lp_insert_with_pos(hash_entry_t *hashtable, tuple_t *tuple, hashkey_t tmp_hashkey, hashkey_t pos) {
#ifdef LOCK_IN_BUCKET	
	while ( hashtable[pos].hashkey || ( !acquire_lock_and_store(&hashtable[pos], tmp_hashkey) ) ) {		
		pos = (pos + 1) & hashmask;
	}
	store_a_tuple(&hashtable[pos].tuple, tuple);
#else /* LOCK_IN_BUCKET */
	/* we make sure that the hashkey starts from 1 */
	while ( hashtable[pos].hashkey || ( !__sync_bool_compare_and_swap(&hashtable[pos].hashkey, 0, tmp_hashkey+1) ) ) {
		pos = (pos + 1) & hashmask;
	}
	store_a_tuple(&hashtable[pos].tuple, tuple);

#endif /* LOCK_IN_BUCKET */
}

void lp_insert(hash_entry_t *hashtable, tuple_t *tuple) {
	hashkey_t tmp_hashkey = IDHASH(tuple->key, hashmask, bitskip);
	lp_insert_with_pos(hashtable, tuple, tmp_hashkey, tmp_hashkey & hashmask);
}


void build_hashtable_nphj_lp(hash_entry_t *hashtable, relation_t *rel, int _tid) {
#ifdef PREFETCH_DISTANCE
	size_t prefetch_index = next_pow_2(PREFETCH_DISTANCE);
	hashkey_t hashkey_prefetch;

#ifdef PREFETCH_CACHE
	key_t_ cachemask = prefetch_index - 1;
	size_t cache_idx;
	lp_prefetch_cache_t *lp_prefetch_cache;
	size_t cache_jdx;

#ifdef PREFETCH_CACHE_NVM
	lp_prefetch_cache = (lp_prefetch_cache_t *) new_alloc_nvm(sizeof(lp_prefetch_cache_t) * prefetch_index);

#else /* PREFETCH_CACHE_NVM */
	lp_prefetch_cache = (lp_prefetch_cache_t *) alloc_dram(sizeof(lp_prefetch_cache_t) * prefetch_index, CACHELINE_SIZE);
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
		lp_insert_with_pos(hashtable, &rel->tuples[idx], lp_prefetch_cache[cache_idx].hashkey, lp_prefetch_cache[cache_idx].hashkey & hashmask);
		
		/** prefetch **/ 
		(lp_prefetch_cache + cache_idx)->hashkey = IDHASH(rel->tuples[cache_jdx].key, hashmask, bitskip);
		hashkey_prefetch = (lp_prefetch_cache + cache_idx)->hashkey;
		__builtin_prefetch(hashtable + hashkey_prefetch, 1, 1);
		cache_jdx ++;
	}

	for (size_t idx = rel->tuple_num - prefetch_index; idx < rel->tuple_num; idx ++) {
		cache_idx  = idx & cachemask;
		lp_insert_with_pos(hashtable, &rel->tuples[idx], lp_prefetch_cache[cache_idx].hashkey, lp_prefetch_cache[cache_idx].hashkey & hashmask);
	}

#ifdef PREFETCH_CACHE_NVM
	pmem_unmap(lp_prefetch_cache, sizeof(lp_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	dealloc_dram(lp_prefetch_cache, sizeof(lp_prefetch_cache_t) * prefetch_index);
#endif /* PREFETCH_CACHE_NVM */

#else /* PREFETCH_CACHE */
		for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
			if (prefetch_index < rel->tuple_num) {
				hashkey_prefetch = IDHASH(rel->tuples[prefetch_index++].key, hashmask, bitskip);
				__builtin_prefetch(hashtable + hashkey_prefetch, 1, 1);
			}
			lp_insert(hashtable, &rel->tuples[idx]);
		}

#endif /* PREFETCH_CACHE */
#else /* PREFETCH_DISTANCE */
		for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
			lp_insert(hashtable, &rel->tuples[idx]);
		}
#endif /* PREFETCH_DISTANCE */
}


tuple_t * lp_lookup_with_pos(hash_entry_t *hashtable, tuple_t *tuple, hashkey_t tmp_hashkey, hashkey_t pos) {
#ifdef TEST_SELECTIVITY
	bool flag = true;
	hashkey_t pos_0 = pos;

	while (  hashtable[pos].hashkey && 
		(hashtable[pos].tuple.key != tuple->key) && 
		( pos != pos_0 || flag) ) {
		
		flag = false;
		pos = (pos + 1) & hashmask;
	}

	return ( hashtable[pos].hashkey && ( pos != pos_0 || flag ) ) ? & hashtable[pos].tuple : NULL;

#else /* TEST_SELECTIVITY */
	while (  hashtable[pos].hashkey && (hashtable[pos].tuple.key != tuple->key) ) {
		pos = (pos + 1) & hashmask;
	}
	return hashtable[pos].hashkey ? & hashtable[pos].tuple : NULL;
#endif /* TEST_SELECTIVITY */
}


tuple_t * lp_lookup(hash_entry_t *hashtable, tuple_t *tuple) {
	hashkey_t tmp_hashkey = IDHASH(tuple->key, hashmask, bitskip);
	return lp_lookup_with_pos(hashtable, tuple, tmp_hashkey, tmp_hashkey & hashmask);
}


void probe_hashtable_nphj_lp(hash_entry_t *hashtable, relation_t *rel, size_t *matched_cnt, rowid_t *checksum) {
	tuple_t *found_tuple;

	short matched_flag;

#ifdef PREFETCH_DISTANCE
	size_t prefetch_index = next_pow_2(PREFETCH_DISTANCE);
	hashkey_t hashkey_prefetch;

#ifdef PREFETCH_CACHE
	key_t_ cachemask = prefetch_index - 1;
	size_t cache_idx;
	lp_prefetch_cache_t *lp_prefetch_cache;
	size_t cache_jdx;

#ifdef PREFETCH_CACHE_NVM
	lp_prefetch_cache = (lp_prefetch_cache_t *) new_alloc_nvm(sizeof(lp_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	lp_prefetch_cache = (lp_prefetch_cache_t *) alloc_dram(sizeof(lp_prefetch_cache_t) * prefetch_index, CACHELINE_SIZE);
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
		found_tuple = lp_lookup_with_pos( hashtable, &rel->tuples[idx], lp_prefetch_cache[cache_idx].hashkey, lp_prefetch_cache[cache_idx].hashkey & hashmask );
		// *matched_cnt +=  (found_tuple == NULL) ? 0 : 1;
		// *checksum += (found_tuple == NULL) ? 0 : rel->tuples[idx].row_id + found_tuple->row_id;

		matched_flag = (found_tuple != NULL);
		*matched_cnt += matched_flag;
		*checksum += (rel->tuples[idx].row_id + found_tuple->row_id)*matched_flag;

		/** prefetch **/ 
		(lp_prefetch_cache + cache_idx)->hashkey = IDHASH(rel->tuples[cache_jdx].key, hashmask, bitskip);
		hashkey_prefetch = (lp_prefetch_cache + cache_idx)->hashkey;
		__builtin_prefetch(hashtable + hashkey_prefetch, 0, 1);
		cache_jdx ++;
	}

	for (size_t idx = rel->tuple_num - prefetch_index; idx < rel->tuple_num; idx ++) {
		cache_idx  = idx & cachemask;
		found_tuple = lp_lookup_with_pos( hashtable, &rel->tuples[idx], lp_prefetch_cache[cache_idx].hashkey, lp_prefetch_cache[cache_idx].hashkey & hashmask );
		// *matched_cnt +=  (found_tuple == NULL) ? 0 : 1;
		// *checksum += (found_tuple == NULL) ? 0 : rel->tuples[idx].row_id + found_tuple->row_id;

		matched_flag = (found_tuple != NULL);
		*matched_cnt += matched_flag;
		*checksum += (rel->tuples[idx].row_id + found_tuple->row_id)*matched_flag;
	}

#ifdef PREFETCH_CACHE_NVM
	pmem_unmap(lp_prefetch_cache, sizeof(lp_prefetch_cache_t) * prefetch_index);
#else /* PREFETCH_CACHE_NVM */
	dealloc_dram(lp_prefetch_cache, sizeof(lp_prefetch_cache_t) * prefetch_index);
#endif /* PREFETCH_CACHE_NVM */

#else /* PREFETCH_CACHE */
		for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
			if (prefetch_index < rel->tuple_num) {
				hashkey_prefetch = IDHASH(rel->tuples[prefetch_index++].key, hashmask, bitskip);
				__builtin_prefetch(hashtable + hashkey_prefetch, 0, 1);
			}
			found_tuple = lp_lookup(hashtable, &rel->tuples[idx]);
			// *matched_cnt +=  (found_tuple == NULL) ? 0 : 1;
			// *checksum += (found_tuple == NULL) ? 0 : rel->tuples[idx].row_id + found_tuple->row_id;

			matched_flag = (found_tuple != NULL);
			*matched_cnt += matched_flag;
			*checksum += (rel->tuples[idx].row_id + found_tuple->row_id)*matched_flag;
		}

#endif /* PREFETCH_CACHE */
#else /* PREFETCH_DISTANCE */
		for (size_t idx = 0; idx < rel->tuple_num; idx ++) {
			found_tuple = lp_lookup(hashtable, &rel->tuples[idx]);
			// *matched_cnt +=  (found_tuple == NULL) ? 0 : 1;
			// *checksum += (found_tuple == NULL) ? 0 : rel->tuples[idx].row_id + found_tuple->row_id;

			matched_flag = (found_tuple != NULL);
			*matched_cnt += matched_flag;
			*checksum += (rel->tuples[idx].row_id + found_tuple->row_id)*matched_flag;
		}

#endif /* PREFETCH_DISTANCE */
}


void *run_nphj_lp_buildpart(void *arg) {
	nphj_lp_arg_t *nphj_lp_arg = (nphj_lp_arg_t *) arg;

#ifdef WARMUP
	warmup_localize(nphj_lp_arg->r_rel.tuples, nphj_lp_arg->r_rel.tuple_num);
#endif /* WARMUP */

	/* eliminate the page faults of the hash table */
	size_t memset_num_entrys_thr = nphj_lp_arg->num_entrys / BUILDPART_THREAD_NUM;
	size_t memset_num_entrys_size = (nphj_lp_arg->_tid == BUILDPART_THREAD_NUM -1) ? 
		sizeof(hash_entry_t) * (nphj_lp_arg->num_entrys - nphj_lp_arg->_tid * memset_num_entrys_thr) : 
		sizeof(hash_entry_t) * memset_num_entrys_thr;
	memset_localize(nphj_lp_arg->hashtable + nphj_lp_arg->_tid * memset_num_entrys_thr, memset_num_entrys_size );

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
	// 	sizeof(hash_entry_t) * (nphj_lp_arg->num_entrys - nphj_lp_arg->_tid * memset_num_entrys_thr) : 
	// 	sizeof(hash_entry_t) * memset_num_entrys_thr;
	// memset_localize(nphj_lp_arg->hashtable + nphj_lp_arg->_tid * memset_num_entrys_thr, memset_num_entrys_size );

#ifdef SYNCSTATS
	SYNC_TIMER(nphj_lp_arg->localtimer->sync_pre_memset);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(nphj_lp_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(nphj_lp_arg->_tid, nphj_lp_arg->globaltimer->sync_pre_memset);
#endif /* SYNCSTATS */

	build_hashtable_nphj_lp(nphj_lp_arg->hashtable, &nphj_lp_arg->r_rel, nphj_lp_arg->_tid);

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

void *run_nphj_lp_probejoin(void *arg) {
	nphj_lp_arg_t *nphj_lp_arg = (nphj_lp_arg_t *) arg;

#ifdef WARMUP
	warmup_localize(nphj_lp_arg->s_rel.tuples, nphj_lp_arg->s_rel.tuple_num);
#endif /* WARMUP */

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

	probe_hashtable_nphj_lp(nphj_lp_arg->hashtable, &nphj_lp_arg->s_rel, &nphj_lp_arg->matched_cnt, &nphj_lp_arg->checksum);

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

	return NULL;
}


void nphj_lp(const datameta_t datameta, timekeeper_t * const timekeeper) {
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

	int max_thread_num = MAX(BUILDPART_THREAD_NUM, PROBEJOIN_THREAD_NUM);

	pthread_t threadpool[max_thread_num];
	nphj_lp_arg_t args[max_thread_num];

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

#ifdef SYNCSTATS
	synctimer_nphj_t *localtimer = (synctimer_nphj_t *) malloc( max_thread_num * sizeof(synctimer_nphj_t) );
	synctimer_nphj_t *globaltimer = (synctimer_nphj_t *) malloc( sizeof(synctimer_nphj_t) );
	memset(localtimer, 0, max_thread_num * sizeof(synctimer_nphj_t));
	memset(globaltimer, 0, sizeof(synctimer_nphj_t));
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
	hash_entry_t *hashtable = (hash_entry_t *) new_alloc_nvm(sizeof(hash_entry_t) * num_entrys);
#else /* USE_NVM */
#ifdef USE_HUGE
	hash_entry_t *hashtable = (hash_entry_t *) alloc_huge_dram(sizeof(hash_entry_t) * num_entrys);
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	hash_entry_t *hashtable = (hash_entry_t *) alloc_aligned_dram(sizeof(hash_entry_t) * num_entrys, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */


	size_t r_tuple_num_thr = datameta.r_tuple_num / BUILDPART_THREAD_NUM;
	size_t s_tuple_num_thr = datameta.s_tuple_num / PROBEJOIN_THREAD_NUM;


#ifdef USE_PAPI
	long long PAPI_values[NUM_PAPI_EVENT];
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
	memset(PAPI_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );

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

		rv = pthread_create(&threadpool[idx], &attr, run_nphj_lp_buildpart, args + idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}

	}

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_buildpart[jdx] += (args + idx) -> PAPI_values_buildpart[jdx];
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
		(args+idx)->checksum = 0;

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, run_nphj_lp_probejoin, args + idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}

	}

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
		matched_cnt += (args+idx)->matched_cnt;
		checksum += (args+idx)->checksum;
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_probejoin[jdx] += (args+idx)->PAPI_values_probejoin[jdx];
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
#endif /* SYNCSTATS */


#ifdef USE_PMWATCH
	for (int idx = 0; idx < NVMCount; idx ++) {
		PMWatch_output[idx].total_bytes_read = PMWatch_output_buildpart[idx].total_bytes_read + 
			PMWatch_output_probejoin[idx].total_bytes_read;
		PMWatch_output[idx].total_bytes_written = PMWatch_output_buildpart[idx].total_bytes_written + 
			PMWatch_output_probejoin[idx].total_bytes_written;
		PMWatch_output[idx].media_read_ops = PMWatch_output_buildpart[idx].media_read_ops + 
			PMWatch_output_probejoin[idx].media_read_ops;
		PMWatch_output[idx].media_write_ops = PMWatch_output_buildpart[idx].media_write_ops +
			PMWatch_output_probejoin[idx].media_write_ops;
		PMWatch_output[idx].read_64B_ops_received = PMWatch_output_buildpart[idx].read_64B_ops_received + 
			PMWatch_output_probejoin[idx].read_64B_ops_received;
		PMWatch_output[idx].write_64B_ops_received = PMWatch_output_buildpart[idx].write_64B_ops_received + 
			PMWatch_output_probejoin[idx].write_64B_ops_received;
		PMWatch_output[idx].host_reads = PMWatch_output_buildpart[idx].host_reads + 
			PMWatch_output_probejoin[idx].host_reads;
		PMWatch_output[idx].host_writes = PMWatch_output_buildpart[idx].host_writes +
			PMWatch_output_probejoin[idx].host_writes;
		PMWatch_output[idx].read_hit_ratio = (PMWatch_output_buildpart[idx].read_hit_ratio + 
			PMWatch_output_probejoin[idx].read_hit_ratio) / 2;
		PMWatch_output[idx].write_hit_ratio = (PMWatch_output_buildpart[idx].write_hit_ratio + 
			PMWatch_output_probejoin[idx].write_hit_ratio) / 2;
		PMWatch_output[idx].read_amplification = (PMWatch_output_buildpart[idx].read_amplification + 
			PMWatch_output_probejoin[idx].read_amplification) / 2;
		PMWatch_output[idx].write_amplification = (PMWatch_output_buildpart[idx].write_amplification +
			PMWatch_output_probejoin[idx].write_amplification) / 2;
		printf("[PMWatch] DIMM: %d\n", idx);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_read: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].total_bytes_read, 
			PMWatch_output_buildpart[idx].total_bytes_read, PMWatch_output_probejoin[idx].total_bytes_read);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_written: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].total_bytes_written,
			PMWatch_output_buildpart[idx].total_bytes_written, PMWatch_output_probejoin[idx].total_bytes_written);
		printf("[PMWatch] DIMM: %d\tmedia_read_ops: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].media_read_ops,
			PMWatch_output_buildpart[idx].media_read_ops, PMWatch_output_probejoin[idx].media_read_ops);
		printf("[PMWatch] DIMM: %d\tmedia_write_ops: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].media_write_ops,
			PMWatch_output_buildpart[idx].media_write_ops, PMWatch_output_probejoin[idx].media_write_ops);
		printf("[PMWatch] DIMM: %d\tread_64B_ops_received: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].read_64B_ops_received,
			PMWatch_output_buildpart[idx].read_64B_ops_received, PMWatch_output_probejoin[idx].read_64B_ops_received);
		printf("[PMWatch] DIMM: %d\twrite_64B_ops_received: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].write_64B_ops_received,
			PMWatch_output_buildpart[idx].write_64B_ops_received, PMWatch_output_probejoin[idx].write_64B_ops_received);
		printf("[PMWatch] DIMM: %d\thost_reads: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].host_reads,
			PMWatch_output_buildpart[idx].host_reads, PMWatch_output_probejoin[idx].host_reads);
		printf("[PMWatch] DIMM: %d\thost_writes: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].host_writes,
			PMWatch_output_buildpart[idx].host_writes, PMWatch_output_probejoin[idx].host_writes);
		printf("[PMWatch] DIMM: %d\tread_hit_ratio: %f\t%f\t%f\n", idx, PMWatch_output[idx].read_hit_ratio,
			PMWatch_output_buildpart[idx].read_hit_ratio, PMWatch_output_probejoin[idx].read_hit_ratio);
		printf("[PMWatch] DIMM: %d\twrite_hit_ratio: %f\t%f\t%f\n", idx, PMWatch_output[idx].write_hit_ratio,
			PMWatch_output_buildpart[idx].write_hit_ratio, PMWatch_output_probejoin[idx].write_hit_ratio);
		printf("[PMWatch] DIMM: %d\tread_amplification: %.9f\t%.9f\t %.9f\n", idx, PMWatch_output[idx].read_amplification,
			PMWatch_output_buildpart[idx].read_amplification, PMWatch_output_probejoin[idx].read_amplification);
		printf("[PMWatch] DIMM: %d\twrite_amplification: %.9f\t%.9f\t %.9f\n", idx, PMWatch_output[idx].write_amplification,
			PMWatch_output_buildpart[idx].write_amplification, PMWatch_output_probejoin[idx].write_amplification);
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
		PAPI_values[idx] += PAPI_values_buildpart[idx] + PAPI_values_probejoin[idx];
		printf("[PAPI] PAPI_event_name: %s", PAPI_event_names[idx]);
		printf("\ttotal_counter: %lld", PAPI_values[idx]);
		printf("\tbuildpart_counter: %lld", PAPI_values_buildpart[idx]);
		printf("\tprobejoin_counter: %lld", PAPI_values_probejoin[idx]);
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
	dealloc_memory(hashtable, sizeof(hash_entry_t) * num_entrys );
	dealloc_memory(r_relation.tuples, TUPLE_T_SIZE * r_relation.tuple_num);
	dealloc_memory(s_relation.tuples, TUPLE_T_SIZE * s_relation.tuple_num);

	timekeeper->buildpart = diff_sec(args[0].build_start, args[0].build_end);
	timekeeper->probejoin = diff_sec(args[0].probe_start, args[0].probe_end);
	timekeeper->total = timekeeper->buildpart + timekeeper->probejoin;

}