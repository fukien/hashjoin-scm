// #pragma once


#ifndef PHJ_SHR_UNI_H
#define PHJ_SHR_UNI_H

#include "phj.h"

#define SHR_UNI_SCALING_FACTOR 1.58
#ifdef RUN_BILLION
#undef SHR_UNI_SCALING_FACTOR
#define SHR_UNI_SCALING_FACTOR 1.1
#endif /* RUN_BILLION */

#ifdef SYNCSTATS

typedef struct {
	struct timespec	sync_prepart_memset;
	struct timespec sync_1st_pass_buildside;
	struct timespec sync_1st_pass;
	struct timespec sync_1st_pass_queue;
	struct timespec sync_part;
	struct timespec sync_max_hashtable;
	struct timespec sync_intermediate_memset;
	struct timespec sync_join;

#ifdef SKEW_HANDLING
	struct timespec sync_skew_taskfetch;
	struct timespec sync_skew_prepart_memset;
	struct timespec sync_skew_buildside;
	struct timespec sync_skew_part;
	struct timespec sync_skew_queue;
#endif /* SKEW_HANDLING */

} synctimer_phj_shr_uni_t;

#endif /* SYNCSTATS */


void phj_shr_uni_bc(const datameta_t, timekeeper_t * const);
void phj_shr_uni_lp(const datameta_t, timekeeper_t * const);
void phj_shr_uni_sc(const datameta_t, timekeeper_t * const);
void phj_shr_uni_hr(const datameta_t, timekeeper_t * const);
#if TUPLE_T_SIZE <= AVX512_SIZE
void phj_shr_uni_hro(const datameta_t, timekeeper_t * const);
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */ 

#endif /* PHJ_SHR_UNI_H */