// #pragma once

#ifdef BW_REG

#ifndef PHJ_BW_REG_H
#define PHJ_BW_REG_H

#include <math.h>

#include "../inc/store.h"
#include "../inc/utils.h"

#include "phj.h"
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
	struct timespec sync_1st_prfx_sum[2];		// 0 - R, 1 - S
	struct timespec sync_1st_pass;
	struct timespec sync_1st_pass_queue;
	struct timespec sync_2nd_pass_mid_hist;
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
	struct timespec sync_skew_prfx_sum[2];		// 0 - R, 1 - S
	struct timespec sync_skew_part;
	struct timespec sync_skew_queue;

#ifdef USE_SWWCB_OPTIMIZED_PART
	struct timespec sync_skew_swwcb[2];			// 0 - R, 1 - S
#endif /* USE_SWWCB_OPTIMIZED_PART */

#endif /* SKEW_HANDLING */

} synctimer_phj_bw_reg_t;

#endif /* SYNCSTATS */


void phj_radix_bw_reg_bc(const datameta_t, timekeeper_t * const);
void phj_radix_bw_reg_lp(const datameta_t, timekeeper_t * const);
void phj_radix_bw_reg_sc(const datameta_t, timekeeper_t * const);
void phj_radix_bw_reg_hr(const datameta_t, timekeeper_t * const);
#if TUPLE_T_SIZE <= AVX512_SIZE
void phj_radix_bw_reg_hro(const datameta_t, timekeeper_t * const);
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */ 

#endif /* PHJ_BW_REG_H */

#endif /* BW_REG */