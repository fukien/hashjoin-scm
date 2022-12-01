// #pragma once

#ifdef RUN_PAYLOAD

#ifndef PTR_NPHJ_H
#define PTR_NPHJ_H

#include "../inc/lock.h"
#include "../inc/store.h"
#include "../inc/utils.h"

#include "params.h"

typedef struct ptr_bucket_t {
	lock_t latch;
	int count;
	keyrid_t keyrids[BUCKET_CAPACITY];
	struct ptr_bucket_t *next;
#ifdef BUCKET_CACHELINE_ALIGNED
} __attribute__( ( aligned( CACHELINE_SIZE ) ) ) ptr_bucket_t;
#elif BUCKET_XPLINE_ALIGNED
} __attribute__( ( aligned( XPLINE_SIZE ) ) ) ptr_bucket_t;
#else
// } __attribute__( (packed) ) ptr_bucket_t;
} ptr_bucket_t;
#endif


typedef struct {
	ptr_bucket_t *buckets;
	size_t num_buckets;
} ptr_hashtable_t;


typedef struct ptr_bucket_buffer_t ptr_bucket_buffer_t;
struct ptr_bucket_buffer_t {
    ptr_bucket_buffer_t *next;
    int count;
    ptr_bucket_t buf[OVERFLOW_BUF_SIZE];
};


typedef struct {
	hashkey_t hashkey;
	keyrid_t keyrid;
#ifdef LOCK_IN_BUCKET
	lock_t latch;
#endif /* LOCK_IN_BUCKET */
#ifdef BUCKET_CACHELINE_ALIGNED
} __attribute__( ( aligned( CACHELINE_SIZE ) ) ) ptr_hash_entry_t;
#elif BUCKET_XPLINE_ALIGNED
} __attribute__( ( aligned( XPLINE_SIZE ) ) ) ptr_hash_entry_t;
#else
// } __attribute__( (packed) ) ptr_hash_entry_t;
} ptr_hash_entry_t;
#endif


typedef struct {
	hashkey_t hashkey;
} ptr_sc_prefetch_cache_t;


typedef struct {
	hashkey_t hashkey;
} ptr_lp_prefetch_cache_t;


#ifdef SYNCSTATS
typedef struct {
	struct timespec sync_pre_memset;
	struct timespec sync_build;
	struct timespec sync_probe;
	struct timespec sync_retrieve;
} ptr_synctimer_nphj_t;
#endif /* SYNCSTATS */


void ptr_nphj_sc(const datameta_t, timekeeper_t * const);
void ptr_nphj_lp(const datameta_t, timekeeper_t * const);


#endif /* NPHJ */

#endif /* RUN_PAYLOAD */