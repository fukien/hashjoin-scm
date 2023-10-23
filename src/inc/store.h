// #pragma once


#ifndef STORE_H
#define STORE_H

#include <stdint.h>					// uint32_t
#include <stdlib.h>					// size_t

#define L1_CACHE_SIZE 32 * 1 << 10
#define L2_CACHE_SIZE 1 << 20
#define L3_CACHE_SIZE (size_t) (27.5 * 1 << 20) / 20	// actually, it's 27.5M

#define L1_ASSOCIATIVITY 8
#define L2_ASSOCIATIVITY 16
#define L3_ASSOCIATIVITY 11


#ifdef USE_NVM
#define L1_TLB_NUM 32
#else /* USE_NVM */
#ifdef USE_HUGE
#define L1_TLB_NUM 32
#else /* USE_HUGE */
#define L1_TLB_NUM 64
#endif /* USE_HUGE */
#endif /* USE_NVM */

#define L2_TLB_NUM 1536

#define L1_TLB_ASSOCIATIVITY 4
#define L2_TLB_ASSOCIATIVITY 6


#define AVX512_SIZE (512/8)
#define M256I_SIZE (256/8)

#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 64
#endif /* CACHELINE_SIZE */

#ifndef XPLINE_SIZE
#define XPLINE_SIZE 256
#endif /* XPLINE_SIZE */

#ifdef USE_NVM
#define PAGE_SIZE 2097152	// 2MB
#else /* USE_NVM */
#ifdef USE_HUGE
#define PAGE_SIZE 2097152	// 2MB
#else /* USE_HUGE */
#define PAGE_SIZE 4096			// 4KB
#endif /* USE_HUGE */
#endif /* USE_NVM */


typedef size_t key_t_;
typedef size_t rowid_t;
typedef size_t hashkey_t;

#ifndef KEY_T__SIZE
#define KEY_T__SIZE sizeof(key_t_)
#endif /* KEY_T__SIZE */

#ifndef ROWID_T_SIZE
#define ROWID_T_SIZE sizeof(rowid_t)
#endif /* ROWID_T_SIZE */

#ifndef HASHKEY_T_SIZE
#define HASHKEY_T_SIZE sizeof(hashkey_t)
#endif /* HASHKEY_T_SIZE */


typedef struct {
	double buildpart;
	double probejoin;
	double total;
} timekeeper_t;


typedef struct {
	key_t_ key;
	rowid_t row_id;
} __attribute__( (packed, aligned( KEY_T__SIZE + ROWID_T_SIZE ) ) ) keyrid_t;


typedef struct {
	key_t_ key;
	rowid_t row_id;

#ifdef TUPLE_T_SIZE
	char values[TUPLE_T_SIZE-KEY_T__SIZE-ROWID_T_SIZE];
} __attribute__( (packed, aligned( TUPLE_T_SIZE ) ) ) tuple_t;
#else /* TUPLE_T_SIZE */
} __attribute__( (packed, aligned( KEY_T__SIZE + ROWID_T_SIZE ) ) ) tuple_t;
#endif /* TUPLE_T_SIZE */


typedef struct {
	rowid_t r_row_id;
	rowid_t s_row_id;
} row_id_pair_t;


#ifdef TUPLE_T_SIZE
#define TUPLE_PER_CACHELINE CACHELINE_SIZE/TUPLE_T_SIZE
#endif /* TUPLE_T_SIZE */

#ifndef TUPLE_T_SIZE
/* amendment for #if directive */
#define TUPLE_T_SIZE 16
#define TUPLE_PER_CACHELINE CACHELINE_SIZE/TUPLE_T_SIZE
#endif /* TUPLE_T_SIZE */


#ifndef KEYRID_T_SIZE
#define KEYRID_T_SIZE sizeof(keyrid_t)
#endif /* KEYRID_T_SIZE */


#ifndef ROW_ID_PAIR_T_SIZE
#define ROW_ID_PAIR_T_SIZE sizeof(row_id_pair_t)
#endif /* ROW_ID_PAIR_T_SIZE */


typedef struct {
	char *r_path_suffix;
	char *s_path_suffix;

	double theta;
	double selectivity;
	double density;
	double sparsity;

	size_t r_tuple_num;
	size_t s_tuple_num;
	size_t r_tuple_size;
	size_t s_tuple_size;

	key_t_ min_key;
	key_t_ max_key;

	char *workload_name;
} datameta_t;


typedef struct {
	tuple_t *tuples;
	size_t tuple_num;
} relation_t;


typedef struct {
	keyrid_t *keyrids;
	size_t keyrid_num;
} ptr_relation_t;


#ifndef SWWCB_SIZE
#ifdef USE_NVM
#define SWWCB_SIZE XPLINE_SIZE
#else /* USE_NVM */
#define SWWCB_SIZE CACHELINE_SIZE
#endif /* USE_NVM */
#endif /* SWWCB_SIZE */

#define TUPLE_PER_SWWCB SWWCB_SIZE / TUPLE_T_SIZE
#define KEYRID_PER_SWWCB SWWCB_SIZE / KEYRID_T_SIZE

#if SWWCB_SIZE < TUPLE_T_SIZE
#undef USE_SWWCB_OPTIMIZED_PART
#undef USE_SWWCB_OPTIMIZED_PART_ON_NVM
#undef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
#undef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM
#endif /* SWWCB_SIZE < TUPLE_T_SIZE */

#endif /* STORE_H */