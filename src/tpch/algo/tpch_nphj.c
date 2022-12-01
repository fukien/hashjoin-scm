#include "tpch_nphj.h"


#ifdef USE_PAPI
#include "../../papi/mypapi.h"
extern char* PAPI_event_names[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
#include "../../pmw/mypmw.h"
extern unsigned int NVMCount;
extern MY_PMWATCH_OP_BUF_NODE PMWatch_output_buildpart[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE PMWatch_output_probejoin[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE PMWatch_output_aggregate[MaxNVMNum];
#endif /* USE_PMWATCH */

extern size_t part_num;
extern size_t lineitem_num;

extern size_t every_thread_part_num[PROBEJOIN_THREAD_NUM];
extern size_t every_thread_lineitem_num[PROBEJOIN_THREAD_NUM];
extern fltrq_part_t *part_start_addr;
extern fltrq_lineitem_t *lineitem_start_addr;
extern size_t initial_part_tuple_num_thr;
extern size_t initial_lineitem_tuple_num_thr;


void tpch_alloc_hash_bucket(bool use, size_t t_num, size_t bkt_size,
	 fltrq_hashtable_t *fltrq_hashtable) {
	if (use) {
		fltrq_hashtable->num_buckets = next_pow_2(t_num / BUCKET_CAPACITY);
		fltrq_hashtable->hashmask = fltrq_hashtable->num_buckets - 1;
		fltrq_hashtable->bkt_size = bkt_size;
#ifdef USE_NVM
		fltrq_hashtable->buckets = new_alloc_nvm(fltrq_hashtable->num_buckets * bkt_size);
#else /* USE_NVM */
#ifdef USE_HUGE
		fltrq_hashtable->buckets = alloc_huge_dram(fltrq_hashtable->num_buckets * bkt_size);
#else /* USE_HUGE */
		/* aligned on a 64-byte boundary */
		fltrq_hashtable->buckets = alloc_aligned_dram(fltrq_hashtable->num_buckets * bkt_size, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	}
}


void tpch_dealloc_hash_bucket(bool use, fltrq_hashtable_t *fltrq_hashtable) {
	if (use) {
		dealloc_memory(fltrq_hashtable->buckets, fltrq_hashtable->num_buckets * fltrq_hashtable->bkt_size);
	}
}


void init_fltrq_part_bucket_buffer(bool use, fltrq_part_bucket_buffer_t ** ppbuf) {
	if (use) {
		fltrq_part_bucket_buffer_t * overflowbuf;
		overflowbuf = (fltrq_part_bucket_buffer_t *) alloc_memory(sizeof(fltrq_part_bucket_buffer_t), CACHELINE_SIZE);
		overflowbuf->count = 0;
		overflowbuf->next = NULL;
		*ppbuf = overflowbuf;
	}
}


void free_fltrq_part_bucket_buffer(bool use, fltrq_part_bucket_buffer_t *overflowbuf) {
	if (use) {
		size_t size = sizeof(fltrq_part_bucket_buffer_t);
		do {
			fltrq_part_bucket_buffer_t * tmp = overflowbuf->next;
			dealloc_memory(overflowbuf, size);
			overflowbuf = tmp;
		} while (overflowbuf);
	}
}


void get_new_fltrq_part_bucket(fltrq_part_bucket_t ** result, fltrq_part_bucket_buffer_t ** buf) {
	if( (*buf)->count < OVERFLOW_BUF_SIZE ) {
		*result = (*buf)->buf + (*buf)->count;
		(*buf)->count ++;
	} else {
		/* need to allocate new buffer */
		fltrq_part_bucket_buffer_t * new_buf = (fltrq_part_bucket_buffer_t*) alloc_memory(sizeof(fltrq_part_bucket_buffer_t), CACHELINE_SIZE);
		new_buf->count = 1;
		new_buf->next  = *buf;
		*buf	= new_buf;
		*result = new_buf->buf;
	}
}


void init_fltrq_lineitem_bucket_buffer(bool use, fltrq_lineitem_bucket_buffer_t ** ppbuf) {
	if (use) {
		fltrq_lineitem_bucket_buffer_t * overflowbuf;
		overflowbuf = (fltrq_lineitem_bucket_buffer_t *) alloc_memory(sizeof(fltrq_lineitem_bucket_buffer_t), CACHELINE_SIZE);
		overflowbuf->count = 0;
		overflowbuf->next = NULL;
		*ppbuf = overflowbuf;
	}
}


void free_fltrq_lineitem_bucket_buffer(bool use, fltrq_lineitem_bucket_buffer_t *overflowbuf) {
	if (use) {
		size_t size = sizeof(fltrq_lineitem_bucket_buffer_t);
		do {
			fltrq_lineitem_bucket_buffer_t * tmp = overflowbuf->next;
			dealloc_memory(overflowbuf, size);
			overflowbuf = tmp;
		} while (overflowbuf);
	}
}


void get_new_fltrq_lineitem_bucket(fltrq_lineitem_bucket_t ** result, fltrq_lineitem_bucket_buffer_t ** buf) {
	if( (*buf)->count < OVERFLOW_BUF_SIZE ) {
		*result = (*buf)->buf + (*buf)->count;
		(*buf)->count ++;
	} else {
		/* need to allocate new buffer */
		fltrq_lineitem_bucket_buffer_t * new_buf = (fltrq_lineitem_bucket_buffer_t*) alloc_memory(sizeof(fltrq_lineitem_bucket_buffer_t), CACHELINE_SIZE);
		new_buf->count = 1;
		new_buf->next  = *buf;
		*buf	= new_buf;
		*result = new_buf->buf;
	}
}



void tpch_build_lineitem_hashtable_nphj_sc(bool use, fltrq_lineitem_t *lineitem_org_tup, 
	size_t self_lineitem_num, fltrq_hashtable_t *fltrq_hashtable, 
	fltrq_lineitem_bucket_buffer_t **fltrq_lineitem_overflowbuf, int _tid
	) {

	if (use) {
		fltrq_lineitem_t *dest;
		fltrq_lineitem_bucket_t *org_bkt = (fltrq_lineitem_bucket_t *) fltrq_hashtable->buckets;
		fltrq_lineitem_bucket_t *curr, *nxt;
		hashkey_t tmp_hashkey;

#ifdef PREFETCH_DISTANCE
		size_t prefetch_index = next_pow_2(PREFETCH_DISTANCE);
		hashkey_t hashkey_prefetch;

		for (size_t idx = 0; idx < self_lineitem_num; idx ++) {
			if (prefetch_index < self_lineitem_num) {
				hashkey_prefetch = IDHASH(
					lineitem_org_tup[prefetch_index ++].l_partkey, 
					fltrq_hashtable->hashmask, 0
				);
				__builtin_prefetch(
					org_bkt + hashkey_prefetch, 1, 1
				);
			}

			tmp_hashkey = IDHASH(
				(lineitem_org_tup+idx)->l_partkey,
				fltrq_hashtable->hashmask, 0
			);

			curr = org_bkt + tmp_hashkey;

			lock(&curr->latch);
			nxt = curr->next;

			if(curr->count == BUCKET_CAPACITY) {
				if(!nxt || nxt->count == BUCKET_CAPACITY) {
					fltrq_lineitem_bucket_t *b;

					get_new_fltrq_lineitem_bucket( 
						&b, fltrq_lineitem_overflowbuf
					);

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
			memcpy(dest, lineitem_org_tup+idx, FLTRQ_LINEITEM_T_SIZE);
		}

#else /* PREFETCH_DISTANCE */

		for (size_t idx = 0; idx < self_lineitem_num; idx ++) {
			tmp_hashkey = IDHASH(
				(lineitem_org_tup+idx)->l_partkey,
				fltrq_hashtable->hashmask, 0
			);
			curr = org_bkt + tmp_hashkey;
			lock(&curr->latch);
			nxt = curr->next;

			if(curr->count == BUCKET_CAPACITY) {
				if(!nxt || nxt->count == BUCKET_CAPACITY) {
					fltrq_lineitem_bucket_t *b;

					get_new_fltrq_lineitem_bucket( 
						&b, fltrq_lineitem_overflowbuf
					);

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
			memcpy(dest, lineitem_org_tup+idx, FLTRQ_LINEITEM_T_SIZE);
		}
#endif /* PREFETCH_DISTANCE */
	}
}




void tpch_probe_hashtable_nphj_sc(tpch_arg_t *tpch_arg) {
	fltrq_hashtable_t *fltrq_lineitem_hashtable = tpch_arg->fltrq_lineitem_hashtable;

	fltrq_lineitem_bucket_t *fltrq_lineitem_buckets = (fltrq_lineitem_bucket_t *) fltrq_lineitem_hashtable->buckets;

	fltrq_part_t *part_org_tup = tpch_arg->part_org_tup;
	size_t self_part_num = tpch_arg->self_part_num;

	hashkey_t tmp_hashkey;

	fltrq_lineitem_bucket_t *lineitem_b;

#if QUERY == Q19
	fltrq_match_t fltrq_match;
#else /* QUERY == Q19 */
#endif /* QUERY == Q19 */


#ifdef PREFETCH_DISTANCE
	size_t prefetch_index = next_pow_2(PREFETCH_DISTANCE);
	hashkey_t hashkey_prefetch;

	for (size_t idx = 0; idx < self_part_num; idx ++) {
		if (prefetch_index < self_part_num) {
			hashkey_prefetch = IDHASH(
				part_org_tup[prefetch_index ++].p_partkey, 
				fltrq_lineitem_hashtable->hashmask, 0
			);
			__builtin_prefetch(fltrq_lineitem_buckets + hashkey_prefetch, 0, 1);
		}

		tmp_hashkey = IDHASH(
			(part_org_tup+idx)->p_partkey, 
			fltrq_lineitem_hashtable->hashmask, 0
		);
		lineitem_b = fltrq_lineitem_buckets + tmp_hashkey;

		do {
			for (size_t jdx = 0; jdx < lineitem_b->count; jdx ++) {

//				if ((part_org_tup+idx)->p_partkey == lineitem_b->tuples[jdx].l_partkey){
// #if QUERY == Q14
// 					(tpch_arg->matched_cnt) ++;
// 					if ( strncmp(part_org_tup[idx].p_type, "PROMO", 5) == 0 ) {
// 						(tpch_arg->numerator) += 
// 							lineitem_b->tuples[jdx].l_extendedprice *
// 							( 1 - lineitem_b->tuples[jdx].l_discount );
// 					}
// 					(tpch_arg->denominator) += 
// 						lineitem_b->tuples[jdx].l_extendedprice *
// 						( 1 - lineitem_b->tuples[jdx].l_discount );
// #elif QUERY == Q19
// 					memcpy(fltrq_match.p_brand, part_org_tup[idx].p_brand, 10);
// 					fltrq_match.p_size = part_org_tup[idx].p_size;
// 					memcpy(fltrq_match.p_container, part_org_tup[idx].p_container, 10);
// 					fltrq_match.l_quantity = lineitem_b->tuples[jdx].l_quantity;
// 					memcpy(fltrq_match.l_shipinstruct, lineitem_b->tuples[jdx].l_shipinstruct, 25);
// 					memcpy(fltrq_match.l_shipmode, lineitem_b->tuples[jdx].l_shipmode, 10);
// 					if ( match_filter(&fltrq_match) ) {
// 						(tpch_arg->matched_cnt) ++;
// 						(tpch_arg->revenue) += lineitem_b->tuples[jdx].l_extendedprice *
// 							( 1 - lineitem_b->tuples[jdx].l_discount );
// 					}
// #else /* QUERY == Q14 */
// #endif /* QUERY == Q14 */
//				}

				short matched_flag = ((part_org_tup+idx)->p_partkey == lineitem_b->tuples[jdx].l_partkey);
#if QUERY == Q14
				(tpch_arg->matched_cnt) += matched_flag;
				(tpch_arg->numerator) += lineitem_b->tuples[jdx].l_extendedprice *
					( 1 - lineitem_b->tuples[jdx].l_discount ) * 
					( matched_flag & ( strncmp(part_org_tup[idx].p_type, "PROMO", 5) == 0 ) );
				(tpch_arg->denominator) += lineitem_b->tuples[jdx].l_extendedprice *
					( 1 - lineitem_b->tuples[jdx].l_discount ) * matched_flag;
#elif QUERY == Q19
				memcpy(fltrq_match.p_brand, part_org_tup[idx].p_brand, 10);
				fltrq_match.p_size = part_org_tup[idx].p_size;
				memcpy(fltrq_match.p_container, part_org_tup[idx].p_container, 10);
				fltrq_match.l_quantity = lineitem_b->tuples[jdx].l_quantity;
				memcpy(fltrq_match.l_shipinstruct, lineitem_b->tuples[jdx].l_shipinstruct, 25);
				memcpy(fltrq_match.l_shipmode, lineitem_b->tuples[jdx].l_shipmode, 10);
				matched_flag = matched_flag & match_filter(&fltrq_match);
				tpch_arg->matched_cnt += matched_flag;
				(tpch_arg->revenue) += lineitem_b->tuples[jdx].l_extendedprice *
					( 1 - lineitem_b->tuples[jdx].l_discount ) * matched_flag;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */
			}

			/* follow overflow pointer */
			lineitem_b = lineitem_b->next;
		} while (lineitem_b);
	}
#else /* PREFETCH_DISTANCE */
	for (size_t idx = 0; idx < self_part_num; idx ++) {
		tmp_hashkey = IDHASH(
			(part_org_tup+idx)->p_partkey, 
			fltrq_lineitem_hashtable->hashmask, 0
		);
		lineitem_b = fltrq_lineitem_buckets + tmp_hashkey;

		do {
			for (size_t jdx = 0; jdx < lineitem_b->count; jdx ++) {
// 				if ((part_org_tup+idx)->p_partkey == lineitem_b->tuples[jdx].l_partkey){
// #if QUERY == Q14
// 					(tpch_arg->matched_cnt) ++;
// 					if ( strncmp(part_org_tup[idx].p_type, "PROMO", 5) == 0 ) {
// 						(tpch_arg->numerator) += 
// 							lineitem_b->tuples[jdx].l_extendedprice *
// 							( 1 - lineitem_b->tuples[jdx].l_discount );
// 					}
// 					(tpch_arg->denominator) += 
// 						lineitem_b->tuples[jdx].l_extendedprice *
// 						( 1 - lineitem_b->tuples[jdx].l_discount );
// #elif QUERY == Q19
// 					memcpy(fltrq_match.p_brand, part_org_tup[idx].p_brand, 10);
// 					fltrq_match.p_size = part_org_tup[idx].p_size;
// 					memcpy(fltrq_match.p_container, part_org_tup[idx].p_container, 10);
// 					fltrq_match.l_quantity = lineitem_b->tuples[jdx].l_quantity;
// 					memcpy(fltrq_match.l_shipinstruct, lineitem_b->tuples[jdx].l_shipinstruct, 25);
// 					memcpy(fltrq_match.l_shipmode, lineitem_b->tuples[jdx].l_shipmode, 10);
// 					if ( match_filter(&fltrq_match) ) {
// 						(tpch_arg->matched_cnt) ++;
// 						(tpch_arg->revenue) += lineitem_b->tuples[jdx].l_extendedprice *
// 							( 1 - lineitem_b->tuples[jdx]._discount );
// 					}
// #else /* QUERY == Q14 */
// #endif /* QUERY == Q14 */
// 				}

				short matched_flag = ((part_org_tup+idx)->p_partkey == lineitem_b->tuples[jdx].l_partkey);
#if QUERY == Q14
				(tpch_arg->matched_cnt) += matched_flag;
				(tpch_arg->numerator) += lineitem_b->tuples[jdx].l_extendedprice *
					( 1 - lineitem_b->tuples[jdx].l_discount ) * 
					( matched_flag & (strncmp(part_org_tup[idx].p_type, "PROMO", 5) == 0) );
				(tpch_arg->denominator) += lineitem_b->tuples[jdx].l_extendedprice *
					( 1 - lineitem_b->tuples[jdx].l_discount ) * matched_flag;
#elif QUERY == Q19
				memcpy(fltrq_match.p_brand, part_org_tup[idx].p_brand, 10);
				fltrq_match.p_size = part_org_tup[idx].p_size;
				memcpy(fltrq_match.p_container, part_org_tup[idx].p_container, 10);
				fltrq_match.l_quantity = lineitem_b->tuples[jdx].l_quantity;
				memcpy(fltrq_match.l_shipinstruct, lineitem_b->tuples[jdx].l_shipinstruct, 25);
				memcpy(fltrq_match.l_shipmode, lineitem_b->tuples[jdx].l_shipmode, 10);
				matched_flag = matched_flag & match_filter(&fltrq_match);
				(tpch_arg->matched_cnt) += matched_flag;
				(tpch_arg->revenue) += lineitem_b->tuples[jdx].l_extendedprice *
					( 1 - lineitem_b->tuples[jdx]._discount ) * matched_flag;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

			}

			/* follow overflow pointer */
			lineitem_b = lineitem_b->next;
		} while (lineitem_b);

	}
#endif /* PREFETCH_DISTANCE */
}



void tpch_memset_bucket_thr(bool use, int _tid, fltrq_hashtable_t *fltrq_hashtable) {
	if (use) {
		size_t memset_num_buckets_thr = fltrq_hashtable->num_buckets / BUILDPART_THREAD_NUM;
		size_t memset_num_buckets_size = _tid == (BUILDPART_THREAD_NUM - 1) ?
			fltrq_hashtable->bkt_size * (fltrq_hashtable->num_buckets - _tid * memset_num_buckets_thr) :
			fltrq_hashtable->bkt_size * memset_num_buckets_thr;
		memset_localize(
			fltrq_hashtable->buckets + _tid * memset_num_buckets_thr * fltrq_hashtable->bkt_size,
			memset_num_buckets_size
		);
	}
}



void *tpch_run_nphj_sc_build(void *arg) {
	tpch_arg_t *tpch_arg = (tpch_arg_t *) arg;

#ifdef WARMUP
	tpch_warmup_localize(tpch_arg->tpch_query_meta->use_part, 
		tpch_arg->part_org_tup, tpch_arg->self_part_num, PART_T_SIZE
	);
#endif /* WARMUP */

	fltrq_lineitem_bucket_buffer_t *fltrq_lineitem_overflowbuf;

	tpch_memset_bucket_thr(tpch_arg->tpch_query_meta->use_lineitem, 
		tpch_arg->_tid, tpch_arg->fltrq_lineitem_hashtable
	);
	init_fltrq_lineitem_bucket_buffer(
		tpch_arg->tpch_query_meta->use_lineitem, 
		&fltrq_lineitem_overflowbuf
	);

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
	if (tpch_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */

	BARRIER_ARRIVE(tpch_arg->barrier, rv);

	if (tpch_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &tpch_arg->buildpart_start);
	}

#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_pre_memset);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_pre_memset);
#endif /* SYNCSTATS */

	tpch_build_lineitem_hashtable_nphj_sc(
		tpch_arg->tpch_query_meta->use_lineitem, 
		tpch_arg->lineitem_org_tup, tpch_arg->self_lineitem_num, 
		tpch_arg->fltrq_lineitem_hashtable, 
		&fltrq_lineitem_overflowbuf, tpch_arg->_tid
	);

	if (BUILDPART_THREAD_NUM < PROBEJOIN_THREAD_NUM) {
		fltrq_lineitem_t *remaining_lineitem_org_tup;
		size_t remaining_lineitem_tup_num_thr;
		size_t per_thread_tup_num;
		for (size_t idx = BUILDPART_THREAD_NUM; idx < PROBEJOIN_THREAD_NUM; idx ++) {
			remaining_lineitem_org_tup = lineitem_start_addr + idx * initial_lineitem_tuple_num_thr;
			per_thread_tup_num = every_thread_lineitem_num[idx] / BUILDPART_THREAD_NUM;
			remaining_lineitem_org_tup = remaining_lineitem_org_tup + (tpch_arg->_tid) * per_thread_tup_num;
			remaining_lineitem_tup_num_thr = (tpch_arg->_tid == BUILDPART_THREAD_NUM-1) ?
				every_thread_lineitem_num[idx] - (tpch_arg->_tid) * per_thread_tup_num : per_thread_tup_num;

			tpch_build_lineitem_hashtable_nphj_sc(
				tpch_arg->tpch_query_meta->use_lineitem, 
				remaining_lineitem_org_tup, remaining_lineitem_tup_num_thr, 
				tpch_arg->fltrq_lineitem_hashtable, 
				&fltrq_lineitem_overflowbuf, tpch_arg->_tid
			);
		}
	}



#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_build);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_build);
#endif /* SYNCSTATS */

	if (tpch_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &tpch_arg->buildpart_end);
	}

#ifdef USE_PMWATCH
	if (tpch_arg->_tid == 0) {
		pmwEnd(PMWatch_output_buildpart);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, tpch_arg->PAPI_values_buildpart);
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

	tpch_arg->fltrq_lineitem_overflowbuf = fltrq_lineitem_overflowbuf;

	return NULL;
}


void *tpch_run_nphj_sc_probe(void *arg) {
	tpch_arg_t *tpch_arg = (tpch_arg_t *) arg;

#ifdef WARMUP
	tpch_warmup_localize(true, tpch_arg->lineitem_org_tup, 
		tpch_arg->self_lineitem_num, LINEITEM_T_SIZE
	);
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
	if (tpch_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */

	/** probe the hash table **/
	if (tpch_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &tpch_arg->probejoin_start);
	}

	tpch_probe_hashtable_nphj_sc(tpch_arg);

#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_probe);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_probe);
#endif /* SYNCSTATS */

	if (tpch_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &tpch_arg->probejoin_end);
	}

#ifdef USE_PMWATCH
	if (tpch_arg->_tid == 0) {
		pmwEnd(PMWatch_output_probejoin);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, tpch_arg->PAPI_values_probejoin);
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