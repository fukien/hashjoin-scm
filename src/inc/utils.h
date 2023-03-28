// #pragma once

#ifndef UTILS_H
#define UTILS_H

#include <emmintrin.h>  // _mm_clflush _mm_stream_si32 _mm_stream_si64 _mm_stream_si128
#include <immintrin.h>  // _mm_clflushopt _mm512_stream_si512 _mm512_load_epi32
#include <stdint.h>	 	// uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "store.h"

#define MAX_FILE_NUM 102306 // maximum file number for a specific directory, can change via ulimit

#ifdef SYNCSTATS
#define SYNC_TIMER(T) { clock_gettime(CLOCK_REALTIME, &T); }
#define SYNC_GLOBAL(TID, T) if(TID==0) { clock_gettime(CLOCK_REALTIME, &T); }
#endif /* SYNCSTATS */


#ifndef ERROR_RETURN
#define ERROR_RETURN(retval) { fprintf(stderr, "Error %d %s:line %d: \n", retval,__FILE__,__LINE__);  exit(retval); }
#endif /* ERROR_RETURN */

#ifndef MAX
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif /* MAX */

#ifndef NEXT_POW_2
/** 
 *  compute the next number, greater than or equal to 32-bit unsigned v.
 *  taken from "bit twiddling hacks":
 *  http://graphics.stanford.edu/~seander/bithacks.html
 */
#define NEXT_POW_2(V)							\
	do {										\
		V--;									\
		V |= V >> 1;							\
		V |= V >> 2;							\
		V |= V >> 4;							\
		V |= V >> 8;							\
		V |= V >> 16;						 	\
		V++;									\
	} while(0)
#endif /* NEXT_POW_2 */

#ifndef BARRIER_ARRIVE
/** barrier wait macro */
#define BARRIER_ARRIVE(B, RV)							\
	RV = pthread_barrier_wait(B);					   	\
	if(RV !=0 && RV != PTHREAD_BARRIER_SERIAL_THREAD){  \
		printf("Couldn't wait on barrier\n");		   	\
		exit(EXIT_FAILURE);							 	\
	}
#endif /* BARRIER_ARRIVE */


typedef struct {
	int _tid;
	void *dst;
	void *src;
	size_t size;
} par_mem_arg_t;


void red();
void yellow();
void blue();
void green();
void cyan();
void purple();
void white();
void black();
void reset();

void mkdir(const char* dirpath);
void deldir(const char* dirpath);
void delfile(const char* filepath);
void touchfile(const char* filename);
void renamefile(const char* oldname, const char* newname);
int file_exists(const char* filepath);
int dir_exists(const char* dirpath);
size_t count_lines_num(const char* filepath);
char * current_timestamp();
char * concat_str_by(const char* concat, const int count, ...);
char * substr(const char *src, int m, int n);
char * get_command_output_short(const char* command);
void data_cfg(datameta_t* const datameta, const char* cfg_path);
void mc_cfg(int* local_cores, int* remote_cores);
int get_cpu_id(int thread_id);
size_t next_pow_2(size_t x);
size_t cnt_power(size_t N);
double diff_sec(const struct timespec start, const struct timespec end);

size_t time_index_descending_qsort_partition(double *time, size_t lower, size_t upper, size_t* index);
void time_index_descending_qsort(double* time, size_t lower, size_t upper, size_t* index);

size_t size_index_descending_qsort_partition(size_t *size, size_t lower, size_t upper, size_t* index);
void size_index_descending_qsort(size_t* size, size_t lower, size_t upper, size_t* index);

void * alloc_aligned_dram(const size_t size, const size_t alignment);
void * alloc_huge_dram(const size_t size);
void dealloc_huge_dram(void * addr, const size_t size);
void * create_pmem_buffer(const char * filepath, const size_t buffer_size);
char * new_nvm_filepath(const char* const directory, const size_t file_idx, const size_t thread_id);
void * new_alloc_nvm(const size_t buffer_size);
void * alloc_dram(const size_t buffer_size, const size_t alignment);
void dealloc_dram(void * addr, const size_t size);
void * alloc_memory(const size_t buffer_size, const size_t alignment);
void dealloc_memory(void * addr, const size_t size);
void memset_localize(void * addr, const size_t size);
void warmup_localize(const tuple_t * const addr, const size_t tuplpe_num);
void memsync(const void * const mem_addr, const size_t size);
void parallel_memcpy(void * const dst, void * const src, const size_t size);
void parallel_memset_localize(void * const dst, const size_t size);
void ptr_retrieve(rowid_t *checksum, row_id_pair_t * const mater, const size_t tot_row_id_pair_num, 
	const relation_t org_relR, const relation_t org_relS);

static inline void sfence(void) {asm volatile("sfence");}
static inline void lfence(void) {asm volatile("lfence");}

inline void nt_store(void * dest, const void * src, size_t len) {
	if ( len <= 4) {
		_mm_stream_si32((int*)dest, *(int*)src);
	} else if (len <= 8) {
		_mm_stream_si64((long long*)dest, *(long long*)src);
	} else if (len <= 16) {
		_mm_stream_si128((__m128i*)dest, *(__m128i*)src);
	} else if (len <= 32) {
		_mm256_stream_si256((__m256i*)dest, *(__m256i*) src);
	} else if (len <= 64) {
		_mm512_stream_si512((__m512i*)dest, *(__m512i*)src);
	} else {
		size_t interval = 64;
		for (int idx = 0; idx < (float)(len)/interval; idx ++) {
			_mm512_stream_si512((__m512i*)(dest+idx*interval), *(__m512i*)(src+idx*interval));
		}
	}
}

inline void clflush(void* addr, size_t size) {
	void *p = addr;
	uintptr_t endline = ((uintptr_t)addr + size - 1) | (CACHELINE_SIZE-1);
	// set the offset-within-line address bits to get the last byte 
	// of the cacheline containing the end of the struct.

	do {
		// flush while p is in a cache line that contains any of the struct
		_mm_clflush(p);
		p += CACHELINE_SIZE;
	} while(p <= (const void*)endline);
}

inline void clflushopt(void* addr, size_t size) {
	void *p = addr;
	uintptr_t endline = ((uintptr_t)addr + size - 1) | (CACHELINE_SIZE-1);
	// set the offset-within-line address bits to get the last byte 
	// of the cacheline containing the end of the struct.

	do {
		// flush while p is in a cache line that contains any of the struct
		_mm_clflushopt(p);
		p += CACHELINE_SIZE;
	} while(p <= (const void*)endline);
}

#define pmem_clwb(addr) asm volatile(".byte 0x66; xsaveopt %0" : "+m"(*(volatile char *)(addr)));
inline void clwb(void* addr, size_t size) {
	void* p = addr;
	uintptr_t endline = ((uintptr_t)addr + size - 1) | (CACHELINE_SIZE-1);
	// set the offset-within-line address bits to get the last byte 
	// of the cacheline containing the end of the struct.

	do {
		// flush while p is in a cache line that contains any of the struct
		// _mm_clwb(p);
		pmem_clwb(p);
		p += CACHELINE_SIZE;
	} while(p <= (const void*)endline);
}

static inline int fast_atoi(const char * str){
	int val = 0;
	while( *str >= '0' && *str <= '9' ) {
		val = val*10 + (*str++ - '0');
	}
	return val;
}


#ifdef IN_DEBUG
/**
 * determine if a memory address if 64byte-aligned
 * assert ( ( (unsigned long) addr & 0x3f) == 0 );
 * determine if a memory address if 8byte-aligned
 * assert ( ( (unsigned long) addr & 0x7) == 0 );
 */

static inline void print_512_64(__m512i avx512) {
	size_t output[AVX512_SIZE/sizeof(size_t)];
	memcpy(output, &avx512, AVX512_SIZE);
	for (size_t idx = 0; idx < AVX512_SIZE/sizeof(size_t); idx ++) {
		// printf("output[%zu]: %zu\n", idx, output[idx]);
		printf("%zu\t", output[idx]);
	}
	printf("\n");
}

static inline void print_256_64(__m256i m256i) {
	size_t output[M256I_SIZE/sizeof(size_t)];
	memcpy(output, &m256i, M256I_SIZE);
	for (size_t idx = 0; idx < M256I_SIZE/sizeof(size_t); idx ++) {
		// printf("output[%zu]: %zu\n", idx, output[idx]);
		printf("%zu\t", output[idx]);
	}
	printf("\n");
}
#endif /* IN_DEBUG */



#endif	/* UTILS_H */