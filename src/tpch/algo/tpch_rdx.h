// #pragma once


#ifndef TPCH_PHJ_H
#define TPCH_PHJ_H

#include "../inc/store.h"
#include "../inc/tpch_utils.h"
#include "../inc/tpch_task_pthread.h"

#define FANOUT_PASS1 ( 1 << (NUM_RADIX_BITS / NUM_PASSES) )
#define FANOUT_PASS2 ( 1 << (NUM_RADIX_BITS - (NUM_RADIX_BITS / NUM_PASSES)) )

#ifndef FLTRQ_PART_PADDING_UNIT_NUM
#define FLTRQ_PART_PADDING_UNIT_NUM SWWCB_SIZE / FLTRQ_PART_T_SIZE
#endif /* FLTRQ_PART_PADDING_UNIT_NUM */

#ifndef FLTRQ_LINEITEM_PADDING_UNIT_NUM
#define FLTRQ_LINEITEM_PADDING_UNIT_NUM SWWCB_SIZE / FLTRQ_LINEITEM_T_SIZE
#endif /* FLTRQ_PART_PADDING_UNIT_NUM */

typedef struct {
	int _tid;
	pthread_barrier_t *barrier;

	tpch_key_t hashmask;
	tpch_key_t bitskip;

	size_t org_tup_num;
	void *org_tup;
	void *tmp_tup;

	size_t **hist;
	size_t *output_offset;

	size_t padding_num;

#ifdef SYNCSTATS
	struct timespec *sync_local_hist;
	struct timespec *sync_global_hist;
#ifdef USE_SWWCB_OPTIMIZED_PART
	struct timespec *sync_local_swwcb;
	struct timespec *sync_global_swwcb;
#endif /* USE_SWWCB_OPTIMIZED_PART */
	struct timespec *sync_local_prfx_sum;
	struct timespec *sync_global_prfx_sum;
#endif /* SYNCSTATS */

} part_arg_t;


typedef struct {
	size_t matched_cnt;
	void *intermediate;

#if QUERY == Q14
	double numerator;
	double denominator;
#elif QUERY == Q19
	double revenue;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

#ifdef PHJ_MBP
	double memset_time, build_time, probe_time;
#endif /* PHJ_MBP */

} join_arg_t;



#if defined(USE_SWWCB_OPTIMIZED_PART) || defined(USE_SWWCB_OPTIMIZED_RADIX_CLUSTER)
typedef union {
	struct {
		fltrq_part_t tuples[FLTRQ_PART_PER_SWWCB];
	} tuples;

	struct {
		fltrq_part_t tuples[FLTRQ_PART_PER_SWWCB-1];
		size_t slot;
	} data;
} tpch_fltrq_part_swwcb_t;
typedef union {
	struct {
		fltrq_lineitem_t tuples[FLTRQ_LINEITEM_PER_SWWCB];
	} tuples;

	struct {
		fltrq_lineitem_t tuples[FLTRQ_LINEITEM_PER_SWWCB-1];
		size_t slot;
	} data;
} tpch_fltrq_lineitem_swwcb_t;
#endif /* defined(USE_SWWCB_OPTIMIZED_PART) || defined(USE_SWWCB_OPTIMIZED_RADIX_CLUSTER) */



static inline void prefetch(void *addr) {
	/* #ifdef __x86_64__ */
	__asm__ __volatile__ ("prefetcht0 %0" :: "m" (*(uint32_t*)addr));
	/* _mm_prefetch(addr, _MM_HINT_T0); */
	/* #endif */
}

void tpch_init_part_mem(bool, void*, void **, size_t, size_t);

void *tpch_run_rdx_part(void *);
void *tpch_run_rdx_part_bw_reg(void *arg);
void *tpch_run_rdx_bc_join(void *);
void *tpch_run_rdx_hr_join(void *);

#endif /* TPCH_PHJ_H */