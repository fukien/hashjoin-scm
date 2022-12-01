// #pragma once


#ifndef PHJ_IND_H
#define PHJ_IND_H

#include <stdbool.h>

#include "part_buffer.h"

#if SKEW_HANDLING
typedef struct {
	key_t_ hashmask;
	key_t_ bitskip;
	ind_part_buffer_t *part_buffer;
	ind_part_buffer_t *part_rear;
	ind_part_buffer_t *in_part;
	size_t *output_offset;

	size_t link_buffer_start_idx;
	size_t link_buffer_num;

	bool *is_rear_flag;

	ind_part_buffer_t *pvt_buf;
	size_t *pvt_buf_idx;
	size_t pvt_buf_num;

	size_t *skew_skew_link_buffer_num;

	short rel_flag;		/* 0: R, 1: S */
} ind_skew_part_t;
#endif /* SKEW_HANDLING */


#if NUM_PASSES != 1
typedef struct {
	size_t fanout;
	key_t_ bitskip;
	size_t *r_hist;
	size_t *s_hist;
	void *swwcb_mem;
	size_t *max_build_side_tup_num;

	ind_part_buffer_t *r_part_buffer;
	ind_part_buffer_t *s_part_buffer;

	ind_part_buffer_t *r_pvt_buf;
	size_t *r_pvt_buf_idx;
	size_t r_pvt_buf_num;

	ind_part_buffer_t *s_pvt_buf;
	size_t *s_pvt_buf_idx;
	size_t s_pvt_buf_num;
} ind_serial_part_t;


typedef struct {
	size_t fanout;
	key_t_ bitskip;
	size_t *hist;
	ind_part_buffer_t *part_buffer;
	ind_part_buffer_t *in_part;
	size_t *output_offset;

	ind_part_buffer_t *pvt_buf;
	size_t *pvt_buf_idx;
	size_t pvt_buf_num;

	short rel_flag;		/* 0: R, 1: S */
} ind_cluster_arg_t;
#endif /* NUM_PASSES != 1 */


#ifdef SYNCSTATS

typedef struct {
	struct timespec	sync_prepart_memset;
	struct timespec sync_1st_pass;
	struct timespec sync_1st_pass_queue;
	struct timespec sync_part;
	struct timespec sync_max_hashtable;
	struct timespec sync_intermediate_memset;
	struct timespec sync_join;

#ifdef SKEW_HANDLING
	struct timespec sync_skew_taskfetch;
	struct timespec sync_skew_prepart_memset;
	struct timespec sync_skew_part;
	struct timespec sync_skew_queue;
#endif /* SKEW_HANDLING */

} synctimer_phj_ind_t;

#endif /* SYNCSTATS */


void phj_ind_lp(const datameta_t, timekeeper_t * const);
void phj_ind_sc(const datameta_t, timekeeper_t * const);
void phj_ind_hr(const datameta_t, timekeeper_t * const);
#if TUPLE_T_SIZE <= AVX512_SIZE
void phj_ind_hro(const datameta_t, timekeeper_t * const);
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */ 

#endif /* PHJ_IND_H */