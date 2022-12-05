// #pragma once


#ifndef NPHJ_H
#define NPHJ_H

#include "../inc/lock.h"
#include "../inc/store.h"
#include "../inc/utils.h"

#include "params.h"

typedef struct bucket_t {
	lock_t latch;
	int count;
	tuple_t tuples[BUCKET_CAPACITY];
	struct bucket_t *next;
#ifdef BUCKET_CACHELINE_ALIGNED
} __attribute__( ( aligned( CACHELINE_SIZE ) ) ) bucket_t;
#elif BUCKET_XPLINE_ALIGNED
} __attribute__( ( aligned( XPLINE_SIZE ) ) ) bucket_t;
#else
} __attribute__( (packed) ) bucket_t;
// } bucket_t;
#endif


typedef struct {
	bucket_t *buckets;
	size_t num_buckets;
} hashtable_t;


typedef struct bucket_buffer_t bucket_buffer_t;
struct bucket_buffer_t {
    bucket_buffer_t *next;
    int count;
    bucket_t buf[OVERFLOW_BUF_SIZE];
};


typedef struct {
	hashkey_t hashkey;
	tuple_t tuple;
#ifdef LOCK_IN_BUCKET
	lock_t latch;
#endif /* LOCK_IN_BUCKET */
#ifdef BUCKET_CACHELINE_ALIGNED
} __attribute__( ( aligned( CACHELINE_SIZE ) ) ) hash_entry_t;
#elif BUCKET_XPLINE_ALIGNED
} __attribute__( ( aligned( XPLINE_SIZE ) ) ) hash_entry_t;
#else
// } __attribute__( (packed) ) hash_entry_t;
} hash_entry_t;
#endif




typedef struct {
	hashkey_t hashkey;
} sc_prefetch_cache_t;


typedef struct {
	hashkey_t hashkey;
} lp_prefetch_cache_t;


#ifdef SYNCSTATS
typedef struct {
	struct timespec sync_pre_memset;
	struct timespec sync_build;
	struct timespec sync_probe;
} synctimer_nphj_t;
#endif /* SYNCSTATS */


void nphj_sc(const datameta_t, timekeeper_t * const);
void nphj_lp(const datameta_t, timekeeper_t * const);


#endif /* NPHJ_H */