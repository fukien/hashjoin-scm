// #pragma once


#ifndef PHJ_H
#define PHJ_H

#include <math.h>

#include "../inc/store.h"
#include "../inc/utils.h"

#include "params.h"

#ifndef PADDING_SIZE
#define PADDING_SIZE SWWCB_SIZE
#endif /* PADDING_SIZE */

#ifndef PADDING_UNIT_NUM
#define PADDING_UNIT_NUM SWWCB_SIZE / TUPLE_T_SIZE
#endif /* PADDING_UNIT_NUM */

#ifndef HR_HRO_FACTOR
#define HR_HRO_FACTOR ((int)(log(AVX512_SIZE/TUPLE_T_SIZE)/log(2)))
#endif /* HR_HRO_FACTOR */

#ifndef AVX512_UNIT_NUM
#define AVX512_UNIT_NUM AVX512_SIZE / TUPLE_T_SIZE
#endif /* AVX512_UNIT_NUM */


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

#ifdef USE_SWWCB_OPTIMIZED_PART
	struct timespec sync_1st_swwcb[2];			// 0 - R, 1 - S
#endif /* USE_SWWCB_OPTIMIZED_PART */

#ifdef SKEW_HANDLING
	struct timespec sync_skew_taskfetch;
	struct timespec sync_skew_prepart_memset;
	struct timespec sync_skew_hist[2];			// 0 - R, 1 - S
	struct timespec sync_skew_part;
	struct timespec sync_skew_queue;

#ifdef USE_SWWCB_OPTIMIZED_PART
	struct timespec sync_skew_swwcb[2];	// 0 - R, 1 - S
#endif /* USE_SWWCB_OPTIMIZED_PART */

#endif /* SKEW_HANDLING */

} synctimer_phj_t;

#endif /* SYNCSTATS */


typedef struct {
	size_t matched_cnt;
	rowid_t checksum;

	void *intermediate;

#ifdef PHJ_MBP
	double memset_time, build_time, probe_time;
#endif /* PHJ_MBP */

} join_arg_t;


static inline void prefetch(void *addr) {
	/* #ifdef __x86_64__ */
	__asm__ __volatile__ ("prefetcht0 %0" :: "m" (*(uint32_t*)addr));
	/* _mm_prefetch(addr, _MM_HINT_T0); */
	/* #endif */
}


#ifdef USE_SWWCB_OPTIMIZED_PART
#ifndef SWWCB_T
#define SWWCB_T 1
typedef union {

	struct {
		tuple_t tuples[TUPLE_PER_SWWCB];
	} tuples;

	struct {
		tuple_t tuples[TUPLE_PER_SWWCB-1];
		size_t slot;
	} data;

} swwcb_t;
#endif /* SWWCB_T */
#endif /* USE_SWWCB_OPTIMIZED_PART */


#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
#ifndef SWWCB_T
#define SWWCB_T 1
typedef union {

	struct {
		tuple_t tuples[TUPLE_PER_SWWCB];
	} tuples;

	struct {
		tuple_t tuples[TUPLE_PER_SWWCB-1];
		size_t slot;
	} data;

} swwcb_t;
#endif /* SWWCB_T */
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */


typedef struct phj_sc_bkt_t {
	int count;
	struct phj_sc_bkt_t *next;
	tuple_t tuples[PHJ_SC_BUCKET_CAPACITY];
// } __attribute__( (packed) ) phj_sc_bkt_t;
} phj_sc_bkt_t;

#ifndef PHJ_SC_BKT_T_SIZE
#define PHJ_SC_BKT_T_SIZE sizeof(phj_sc_bkt_t)
#endif /* PHJ_SC_BKT_T_SIZE */


void phj_radix_bc(const datameta_t, timekeeper_t * const);
void phj_radix_lp(const datameta_t, timekeeper_t * const);
void phj_radix_sc(const datameta_t, timekeeper_t * const);
void phj_radix_hr(const datameta_t, timekeeper_t * const);
#if TUPLE_T_SIZE <= AVX512_SIZE
void phj_radix_hro(const datameta_t, timekeeper_t * const);
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */ 

#endif /* PHJ_H */