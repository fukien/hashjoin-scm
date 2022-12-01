// #pragma once


#ifndef PARAMS_H
#define PARAMS_H

#include "../inc/store.h"

#ifndef IDHASH
#define IDHASH(X, MASK, BITSKIP) (((X) & MASK) >> BITSKIP)
#endif /* IDHASH */


/** Parameters for NPHJ */
#ifndef BUCKET_CAPACITY
#define BUCKET_CAPACITY 2
#endif /* BUCKET_CAPACITY */

/** Pre-allocation size for overflow buffers */
#ifndef OVERFLOW_BUF_SIZE
#define OVERFLOW_BUF_SIZE 1024 
#endif /* OVERFLOW_BUF_SIZE */

/** Parameters for PHJ */
/** number of total radix bits used for partitioning. */
#ifndef NUM_RADIX_BITS
#define NUM_RADIX_BITS 10	/* 4-18 */
#endif /* NUM_RADIX_BITS */

#ifndef NUM_PASSES
#define NUM_PASSES 1
#endif /* NUM_PASSES */

#define FANOUT_PASS1 ( 1 << (NUM_RADIX_BITS / NUM_PASSES) )
#define FANOUT_PASS2 ( 1 << (NUM_RADIX_BITS - (NUM_RADIX_BITS / NUM_PASSES)) )

#ifdef SKEW_HANDLING
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define THRESHOLD_0 MAX(FANOUT_PASS1, FANOUT_PASS2) * THREAD_NUM * L1_CACHE_SIZE / TUPLE_T_SIZE
#define THRESHOLD_1 THREAD_NUM * L1_CACHE_SIZE / TUPLE_T_SIZE
#define THRESHOLD_2 THREAD_NUM * THREAD_NUM * L1_CACHE_SIZE / TUPLE_T_SIZE
#endif /* SKEW_HANDLING */

#define SMALL_PADDING_TUPLES  3 * CACHELINE_SIZE / TUPLE_T_SIZE
#define PADDING_TUPLES  SMALL_PADDING_TUPLES * (FANOUT_PASS2 + 1)
#define RELATION_PADDING PADDING_TUPLES * FANOUT_PASS1 * TUPLE_T_SIZE


#ifdef NONTEMP_STORE_MATER
#define ROW_ID_PAIR_PER_SWWCB SWWCB_SIZE / ROW_ID_PAIR_T_SIZE
typedef union {

	struct {
		row_id_pair_t row_id_pairs[ROW_ID_PAIR_PER_SWWCB];
	} row_id_pairs;

	struct {
		row_id_pair_t row_id_pairs[ROW_ID_PAIR_PER_SWWCB - 1];
		size_t slot;
	} data;

} row_id_pair_swwcb_t;
#endif /* NONTEMP_STORE_MATER */


#endif	/* PARAMS_H */