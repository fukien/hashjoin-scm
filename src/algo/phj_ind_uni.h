// #pragma once


#ifndef PHJ_IND_UNI_H
#define PHJ_IND_UNI_H

#include "phj.h"

#define IND_UNI_SCALING_FACTOR 1.78
#ifdef RUN_PAYLOAD
#undef IND_UNI_SCALING_FACTOR
#define IND_UNI_SCALING_FACTOR 2.1
#endif /* RUN_PAYLOAD */
#ifdef RUN_BILLION
#undef IND_UNI_SCALING_FACTOR
#define IND_UNI_SCALING_FACTOR 1.1
#endif /* RUN_BILLION */

#define IND_UNI_INTERMEDIATE_SCALING_FACTOR 3

#if NUM_PASSES != 1
typedef struct {
	size_t fanout;
	key_t_ bitskip;

	size_t *r_hist;
	size_t *s_hist;
	void *swwcb_mem;

	size_t padding_num;
	size_t r_avg_part_scaling_offset;
	size_t s_avg_part_scaling_offset;
	size_t r_avg_part_thr_scaling_offset;
	size_t s_avg_part_thr_scaling_offset;
} ind_uni_serial_part_t;
#endif /* NUM_PASSES != 1 */


void phj_ind_uni_bc(const datameta_t, timekeeper_t * const);
void phj_ind_uni_lp(const datameta_t, timekeeper_t * const);
void phj_ind_uni_sc(const datameta_t, timekeeper_t * const);
void phj_ind_uni_hr(const datameta_t, timekeeper_t * const);
#if TUPLE_T_SIZE <= AVX512_SIZE
void phj_ind_uni_hro(const datameta_t, timekeeper_t * const);
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */ 

#endif /* PHJ_IND_UNI_H */