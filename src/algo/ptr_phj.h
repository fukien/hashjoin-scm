// #pragma once

#ifdef RUN_PAYLOAD

#ifndef PTR_PHJ_H
#define PTR_PHJ_H

#include <math.h>

#include "../inc/store.h"
#include "../inc/utils.h"

#include "params.h"

#ifndef PADDING_SIZE
#define PADDING_SIZE SWWCB_SIZE
#endif /* PADDING_SIZE */

#ifndef PTR_PADDING_UNIT_NUM
#define PTR_PADDING_UNIT_NUM SWWCB_SIZE / KEYRID_T_SIZE
#endif /* PTR_PADDING_UNIT_NUM */

#ifndef PTR_HR_HRO_FACTOR
#define PTR_HR_HRO_FACTOR ((int)(log(AVX512_SIZE/KEYRID_T_SIZE)/log(2)))
#endif /* PTR_HR_HRO_FACTOR */

#ifndef PTR_AVX512_UNIT_NUM
#define PTR_AVX512_UNIT_NUM AVX512_SIZE / KEYRID_T_SIZE
#endif /* PTR_AVX512_UNIT_NUM */


#ifdef SYNCSTATS

typedef struct {
	struct timespec	sync_prepart_memset;
	struct timespec sync_1st_hist[2];			// 0 - R, 1 - S
	struct timespec sync_1st_pass;
	struct timespec sync_1st_pass_queue;
	struct timespec sync_part;
	struct timespec sync_max_hashtable;
	struct timespec sync_intermediate_memset;
	struct timespec sync_join;
	struct timespec sync_retrieve;

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	struct timespec sync_1st_swwcb[2];			// 0 - R, 1 - S
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

#ifdef SKEW_HANDLING
	struct timespec sync_skew_taskfetch;
	struct timespec sync_skew_prepart_memset;
	struct timespec sync_skew_hist[2];			// 0 - R, 1 - S
	struct timespec sync_skew_part;
	struct timespec sync_skew_queue;

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	struct timespec sync_skew_swwcb[2];	// 0 - R, 1 - S
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

#endif /* SKEW_HANDLING */

} ptr_synctimer_phj_t;

#endif /* SYNCSTATS */


static inline void ptr_prefetch(void *addr) {
	/* #ifdef __x86_64__ */
	__asm__ __volatile__ ("prefetcht0 %0" :: "m" (*(uint32_t*)addr));
	/* _mm_prefetch(addr, _MM_HINT_T0); */
	/* #endif */
}


#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
#ifndef PTR_SWWCB_T
#define PTR_SWWCB_T 1
typedef union {

	struct {
		keyrid_t keyrids[KEYRID_PER_SWWCB];
	} keyrids;

	struct {
		keyrid_t keyrids[KEYRID_PER_SWWCB - 1];
		size_t slot;
	} data;

} ptr_swwcb_t;
#endif /* PTR_SWWCB_T */
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */


#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
#ifndef PTR_SWWCB_T
#define PTR_SWWCB_T 1
typedef union {

	struct {
		keyrid_t keyrids[KEYRID_PER_SWWCB];
	} keyrids;

	struct {
		keyrid_t keyrids[KEYRID_PER_SWWCB - 1];
		size_t slot;
	} data;

} ptr_swwcb_t;
#endif /* PTR_SWWCB_T */
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */


typedef struct {
	size_t matched_cnt;
	rowid_t checksum;

	void *intermediate;

	row_id_pair_t *mater;

#ifdef NONTEMP_STORE_MATER
	row_id_pair_swwcb_t *row_id_pair_swwcb;
#endif /* NONTEMP_STORE_MATER */

#ifdef PHJ_MBP
	double memset_time, build_time, probe_time;
#endif /* PHJ_MBP */

} ptr_join_arg_t;


typedef struct ptr_phj_sc_bkt_t {
	int count;
	struct ptr_phj_sc_bkt_t *next;
	keyrid_t keyrids[PHJ_SC_BUCKET_CAPACITY];
// } __attribute__( (packed) ) ptr_phj_sc_bkt_t;
} ptr_phj_sc_bkt_t;

#ifndef PTR_PHJ_SC_BKT_T_SIZE
#define PTR_PHJ_SC_BKT_T_SIZE sizeof(ptr_phj_sc_bkt_t)
#endif /* PTR_PHJ_SC_BKT_T_SIZE */


#ifndef AVG_MATCH_NUM_PER_THR_SCALING_FACTOR
#define AVG_MATCH_NUM_PER_THR_SCALING_FACTOR 1.2
#endif /* AVG_MATCH_NUM_PER_THR_SCALING_FACTOR */


void ptr_phj_radix_bc(const datameta_t, timekeeper_t * const);
void ptr_phj_radix_lp(const datameta_t, timekeeper_t * const);
void ptr_phj_radix_sc(const datameta_t, timekeeper_t * const);
void ptr_phj_radix_hr(const datameta_t, timekeeper_t * const);
// void ptr_phj_radix_hro(const datameta_t, timekeeper_t * const);

#endif /* PTR_PHJ_H */

#endif /* RUN_PAYLOAD */