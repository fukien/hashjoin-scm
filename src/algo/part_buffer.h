 // #pragma once


#ifndef PART_BUFFER_H
#define PART_BUFFER_H

#include "../inc/lock.h"

#include "phj.h"

#ifndef PART_BUFFER_T_CAPACITY
#define PART_BUFFER_T_CAPACITY ((L1_CACHE_SIZE)/16/TUPLE_T_SIZE)
#endif /* PART_BUFFER_T_CAPACITY */


typedef struct shr_part_buffer_t {
	/* automatically padding for AVX512 */
	tuple_t tuples[PART_BUFFER_T_CAPACITY+AVX512_UNIT_NUM];
	lock_t latch;
	size_t count;
	struct shr_part_buffer_t *next;
} __attribute__((aligned(CACHELINE_SIZE))) shr_part_buffer_t;
// } shr_part_buffer_t;


typedef struct ind_part_buffer_t {
	/* automatically padding for AVX512 */
	tuple_t tuples[PART_BUFFER_T_CAPACITY+AVX512_UNIT_NUM];
	size_t count;
	struct ind_part_buffer_t *next;
} __attribute__((aligned(CACHELINE_SIZE))) ind_part_buffer_t;
// } ind_part_buffer_t;


#define SHR_PART_BUFFER_T_SIZE sizeof(shr_part_buffer_t)
#define IND_PART_BUFFER_T_SIZE sizeof(ind_part_buffer_t)


#endif /* PART_BUFFER_H */