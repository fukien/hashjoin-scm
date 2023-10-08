// #pragma once

#ifndef TPCH_STORE_H
#define TPCH_STORE_H

#include <stdbool.h>

#include "q14.h"
#include "q19.h"
#include "schema.h"


#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 64
#endif /* CACHELINE_SIZE */

#ifndef XPLINE_SIZE
#define XPLINE_SIZE 256
#endif /* XPLINE_SIZE */

#ifndef SWWCB_SIZE
#ifdef USE_NVM
#define SWWCB_SIZE XPLINE_SIZE
#else /* USE_NVM */
#define SWWCB_SIZE CACHELINE_SIZE
#endif /* USE_NVM */
#endif /* SWWCB_SIZE */

#ifndef PADDING_SIZE
#define PADDING_SIZE SWWCB_SIZE
#endif /* PADDING_SIZE */

#define TPCH_DATA_PREFIX_PATH "/dcpmm/huang/hashjoin-scm/tpch-data"

#if QUERY == Q14
typedef fltrq14_part_t fltrq_part_t;
typedef fltrq14_lineitem_t fltrq_lineitem_t;
#elif QUERY == Q19
typedef fltrq19_part_t fltrq_part_t;
typedef fltrq19_lineitem_t fltrq_lineitem_t;
typedef fltrq19_match_t fltrq_match_t;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

#define FLTRQ_PART_T_SIZE sizeof(fltrq_part_t)
#define FLTRQ_LINEITEM_T_SIZE sizeof(fltrq_lineitem_t)
#if QUERY == Q19
#define FLTRQ_MATCH_T_SIZE sizeof(fltrq_match_t)
#else /* QUERY == Q19 */
#endif /* QUERY == Q19 */

#define FLTRQ_PART_PER_SWWCB SWWCB_SIZE / FLTRQ_PART_T_SIZE
#define FLTRQ_LINEITEM_PER_SWWCB SWWCB_SIZE / FLTRQ_LINEITEM_T_SIZE
#if QUERY == Q19
#define FLTRQ_MATCH_PER_SWWCB SWWCB_SIZE / FLTRQ_MATCH_T_SIZE
#else /* QUERY == Q19 */
#endif /* QUERY == Q19 */


typedef struct {
	bool use_part;
	bool use_supplier;
	bool use_partsupp;
	bool use_customer;
	bool use_order;
	bool use_lineitem;
	bool use_nation;
	bool use_region;

	double ff_part;
	double ff_supplier;
	double ff_partsupp;
	double ff_customer;
	double ff_order;
	double ff_lineitem;
	double ff_nation;
	double ff_region;
} tpch_query_meta_t;


typedef struct {
	double pushdown;
	double buildpart;
	double probejoin;
	double aggregate;
	double total;
} tpch_timekeeper_t;


#ifdef SYNCSTATS

typedef struct {
	struct timespec sync_pushdown;

	struct timespec sync_pre_memset;
	struct timespec sync_build;
	struct timespec sync_probe;

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

// #ifdef SKEW_HANDLING
// 	struct timespec sync_skew_taskfetch;
// 	struct timespec sync_skew_prepart_memset;
// 	struct timespec sync_skew_hist[2];			// 0 - R, 1 - S
// 	struct timespec sync_skew_prfx_sum[2];		// 0 - R, 1 - S
// 	struct timespec sync_skew_part;
// 	struct timespec sync_skew_queue;

// #ifdef USE_SWWCB_OPTIMIZED_PART
// 	struct timespec sync_skew_swwcb[2];			// 0 - R, 1 - S
// #endif /* USE_SWWCB_OPTIMIZED_PART */

// #endif /* SKEW_HANDLING */

} tpch_synctimer_t;

#endif /* SYNCSTATS */


#endif /* TPCH_STORE_H */