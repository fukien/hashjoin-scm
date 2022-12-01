// #pragma once

#ifdef BW_REG
#ifdef RUN_PAYLOAD

#ifndef PTR_PHJ_BW_REG_H
#define PTR_PHJ_BW_REG_H

#include <math.h>

#include "../inc/store.h"
#include "../inc/utils.h"

#include "ptr_phj.h"
#include "params.h"

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
	struct timespec sync_retrieve;

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	struct timespec sync_1st_swwcb[2];			// 0 - R, 1 - S
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

#ifdef SKEW_HANDLING
	struct timespec sync_skew_taskfetch;
	struct timespec sync_skew_prepart_memset;
	struct timespec sync_skew_hist[2];			// 0 - R, 1 - S
	struct timespec sync_skew_prfx_sum[2];		// 0 - R, 1 - S
	struct timespec sync_skew_part;
	struct timespec sync_skew_queue;

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	struct timespec sync_skew_swwcb[2];			// 0 - R, 1 - S
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

#endif /* SKEW_HANDLING */

} ptr_synctimer_phj_bw_reg_t;

#endif /* SYNCSTATS */


void ptr_phj_radix_bw_reg_bc(const datameta_t, timekeeper_t * const);
void ptr_phj_radix_bw_reg_lp(const datameta_t, timekeeper_t * const);
void ptr_phj_radix_bw_reg_sc(const datameta_t, timekeeper_t * const);
void ptr_phj_radix_bw_reg_hr(const datameta_t, timekeeper_t * const);
// void ptr_phj_radix_bw_reg_hro(const datameta_t, timekeeper_t * const);

#endif /* PTR_PHJ_BW_REG_H */

#endif /* RUN_PAYLOAD */
#endif /* BW_REG */