// #pragma once


#ifndef TPCH_NPHJ_H
#define TPCH_NPHJ_H

#include "../inc/store.h"
#include "../inc/tpch_utils.h"

#include "../../inc/lock.h"


typedef struct fltrq_part_bucket_t {
	struct fltrq_part_bucket_t *next;
	int count;
	lock_t latch;
	fltrq_part_t tuples[BUCKET_CAPACITY];
#ifdef BUCKET_CACHELINE_ALIGNED
} __attribute__( ( aligned( CACHELINE_SIZE ) ) ) fltrq_part_bucket_t;
#elif BUCKET_XPLINE_ALIGNED
} __attribute__( ( aligned( XPLINE_SIZE ) ) ) fltrq_part_bucket_t;
#else
// } __attribute__( (packed) ) fltrq_part_bucket_t;
} fltrq_part_bucket_t;
#endif


typedef struct fltrq_part_bucket_buffer_t fltrq_part_bucket_buffer_t;
struct fltrq_part_bucket_buffer_t {
	fltrq_part_bucket_buffer_t *next;
	int count;
	fltrq_part_bucket_t buf[OVERFLOW_BUF_SIZE];
};


typedef struct fltrq_lineitem_bucket_t {
	struct fltrq_lineitem_bucket_t *next;
	int count;
	lock_t latch;
	fltrq_lineitem_t tuples[BUCKET_CAPACITY];
#ifdef BUCKET_CACHELINE_ALIGNED
} __attribute__( ( aligned( CACHELINE_SIZE ) ) ) fltrq_lineitem_bucket_t;
#elif BUCKET_XPLINE_ALIGNED
} __attribute__( ( aligned( XPLINE_SIZE ) ) ) fltrq_lineitem_bucket_t;
#else
// } __attribute__( (packed) ) fltrq_lineitem_bucket_t;
} fltrq_lineitem_bucket_t;
#endif


typedef struct fltrq_lineitem_bucket_buffer_t fltrq_lineitem_bucket_buffer_t;
struct fltrq_lineitem_bucket_buffer_t {
	fltrq_lineitem_bucket_buffer_t *next;
	int count;
	fltrq_lineitem_bucket_t buf[OVERFLOW_BUF_SIZE];
};


typedef struct {
	size_t num_buckets;
	tpch_key_t hashmask;
	size_t bkt_size;
	void *buckets;
} fltrq_hashtable_t;


void tpch_alloc_hash_bucket(bool, size_t, size_t, fltrq_hashtable_t*);
void tpch_dealloc_hash_bucket(bool, fltrq_hashtable_t*);
void free_fltrq_part_bucket_buffer(bool, fltrq_part_bucket_buffer_t *);
void free_fltrq_lineitem_bucket_buffer(bool, fltrq_lineitem_bucket_buffer_t *);


void *tpch_run_nphj_sc_build(void *);
void *tpch_run_nphj_sc_probe(void *);


#endif /* TPCH_NPHJ_H */








