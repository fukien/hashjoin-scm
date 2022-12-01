// #pragma once

#ifndef TPCH_UTILS_H
#define TPCH_UTILS_H

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define __USE_GNU
#include <sched.h>
#include <pthread.h>

#include <libpmem.h>

#include "tpch_task_pthread.h"
#include "store.h"

#include "../../inc/memaccess.h"
#include "../../inc/utils.h"



#ifdef USE_PAPI
#include "../../papi/mypapi.h"
#endif /* USE_PAPI */


#ifndef IDHASH
#define IDHASH(X, MASK, BITSKIP) (((X) & MASK) >> BITSKIP)
#endif /* IDHASH */


typedef struct {
	int _tid;
	pthread_barrier_t *barrier;

	tpch_query_meta_t *tpch_query_meta;

	size_t self_part_num;
	size_t self_lineitem_num;

	part_t *part_tmp_addr;
	lineitem_t *lineitem_tmp_addr;

	fltrq_part_t *part_org_tup, *part_tmp_tup;
	fltrq_lineitem_t *lineitem_org_tup, *lineitem_tmp_tup;

	void *fltrq_lineitem_hashtable;
	void *fltrq_lineitem_overflowbuf;

	size_t matched_cnt;

#if QUERY == Q14
	double numerator;
	double denominator;
#elif QUERY == Q19
	double revenue;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

	size_t **part_hist;
	size_t **lineitem_hist;

	size_t *max_build_side_tup_num;

	/* stats about the thread */
	size_t parts_processed;

	tpch_task_queue_t *part_queue;
	tpch_task_queue_t *join_queue;
#if SKEW_HANDLING
	tpch_task_queue_t *skew_queue;
	tpch_task_t ** skewtask;
#endif /* SKEW_HANDLING */

	struct timespec pushdown_start, pushdown_end, \
		buildpart_start, buildpart_end, \
		probejoin_start, probejoin_end, \
		aggregate_start, aggregate_end;

#ifdef SYNCSTATS
	tpch_synctimer_t *localtimer;
	tpch_synctimer_t *globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
	long long PAPI_values_pushdown[NUM_PAPI_EVENT];
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
	long long PAPI_values_aggregate[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

#ifdef PHJ_MBP
	double memset_time, build_time, probe_time;
#endif /* PHJ_MBP */

	size_t intermediate_size;
	void *intermediate;
} tpch_arg_t;


bool part_filter(part_t *);
bool lineitem_filter(lineitem_t *);
#if QUERY == Q19
bool match_filter(fltrq_match_t *);
#else /* QUERY == Q19 */
#endif /* QUERY == Q19 */

void tpch_query_meta_cfg(tpch_query_meta_t *tpch_query_meta);

void get_cardinality(int sf);

void tpch_org_map(bool, char*, void *, void**, void**, size_t, size_t, size_t);
void tpch_unmap(bool, void *, size_t);

void tpch_warmup_localize(bool use, void *addr, const size_t tuplpe_num, const size_t tuple_size);


#ifdef USE_SWWCB_OPTIMIZED_PART
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
#endif /* USE_SWWCB_OPTIMIZED_PART */


#endif /* TPCH_UTILS_H */