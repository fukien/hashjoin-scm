// #pragma once


#ifndef PHJ_SHR_H
#define PHJ_SHR_H

#include "part_buffer.h"
#include "phj_shr_uni.h"


#if SKEW_HANDLING
typedef struct {
	key_t_ hashmask;
	key_t_ bitskip;
	shr_part_buffer_t *part_buffer;
	shr_part_buffer_t *in_part;
	size_t *output_offset;

	size_t link_buffer_start_idx;
	size_t link_buffer_num;

	short rel_flag;		/* 0: R, 1: S */
} shr_skew_part_t;
#endif /* SKEW_HANDLING */


#if NUM_PASSES != 1
typedef struct {
	size_t fanout;
	key_t_ bitskip;
	size_t *r_hist;
	size_t *s_hist;
	void *swwcb_mem;
	size_t *max_build_side_tup_num;

	shr_part_buffer_t *r_part_buffer;
	shr_part_buffer_t *s_part_buffer;
} shr_serial_part_t;


typedef struct {
	size_t fanout;
	key_t_ bitskip;
	size_t *hist;
	shr_part_buffer_t *part_buffer;
	shr_part_buffer_t *in_part;
	size_t *output_offset;
	short rel_flag;		/* 0: R, 1: S */
} shr_cluster_arg_t;
#endif /* NUM_PASSES != 1 */


void phj_shr_lp(const datameta_t, timekeeper_t * const);
void phj_shr_sc(const datameta_t, timekeeper_t * const);
void phj_shr_hr(const datameta_t, timekeeper_t * const);
#if TUPLE_T_SIZE <= AVX512_SIZE
void phj_shr_hro(const datameta_t, timekeeper_t * const);
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */ 

#endif /* PHJ_SHR_H */