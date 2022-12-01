// #pragma once


#ifndef MEMACCESS_H
#define MEMACCESS_H

#include "utils.h"


#define EXEC_2_TIMES(x) x x
#define EXEC_4_TIMES(x) EXEC_2_TIMES(EXEC_2_TIMES(x))
#define EXEC_8_TIMES(x) EXEC_2_TIMES(EXEC_4_TIMES(x))
#define EXEC_16_TIMES(x) EXEC_2_TIMES(EXEC_8_TIMES(x))
#define EXEC_32_TIMES(x) EXEC_2_TIMES(EXEC_16_TIMES(x))
#define EXEC_64_TIMES(x) EXEC_2_TIMES(EXEC_32_TIMES(x))
#define EXEC_128_TIMES(x) EXEC_2_TIMES(EXEC_64_TIMES(x))
#define EXEC_256_TIMES(x) EXEC_2_TIMES(EXEC_128_TIMES(x))

#define WRITE_NT_64_ASM \
	"vmovntdq %%zmm0, 0(%[addr]) \n"\

#define WRITE_NT_128_ASM \
	"vmovntdq %%zmm0, 0(%[addr]) \n"\
	"vmovntdq %%zmm0, 1*64(%[addr]) \n"

#define WRITE_NT_256_ASM \
	"vmovntdq %%zmm0, 0(%[addr]) \n" \
	"vmovntdq %%zmm0, 1*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 2*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 3*64(%[addr]) \n"

#define WRITE_NT_512_ASM \
	"vmovntdq %%zmm0, 0*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 1*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 2*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 3*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 4*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 5*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 6*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 7*64(%[addr]) \n" 

#define WRITE_NT_1024_ASM \
	"vmovntdq %%zmm0, 0*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 1*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 2*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 3*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 4*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 5*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 6*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 7*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 8*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 9*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 10*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 11*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 12*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 13*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 14*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 15*64(%[addr]) \n" \

#define WRITE_NT_2048_ASM \
	"vmovntdq %%zmm0, 0*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 1*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 2*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 3*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 4*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 5*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 6*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 7*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 8*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 9*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 10*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 11*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 12*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 13*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 14*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 15*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 16*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 17*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 18*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 19*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 20*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 21*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 22*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 23*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 24*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 25*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 26*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 27*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 28*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 29*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 30*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 31*64(%[addr]) \n"

#define WRITE_NT_4096_ASM \
	"vmovntdq %%zmm0, 0*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 1*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 2*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 3*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 4*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 5*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 6*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 7*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 8*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 9*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 10*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 11*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 12*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 13*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 14*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 15*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 16*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 17*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 18*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 19*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 20*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 21*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 22*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 23*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 24*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 25*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 26*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 27*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 28*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 29*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 30*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 31*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 32*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 33*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 34*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 35*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 36*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 37*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 38*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 39*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 40*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 41*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 42*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 43*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 44*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 45*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 46*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 47*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 48*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 49*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 50*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 51*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 52*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 53*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 54*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 55*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 56*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 57*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 58*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 59*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 60*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 61*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 62*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 63*64(%[addr]) \n"

#define WRITE_NT_8192_ASM \
	"vmovntdq %%zmm0, 0*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 1*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 2*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 3*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 4*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 5*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 6*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 7*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 8*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 9*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 10*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 11*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 12*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 13*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 14*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 15*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 16*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 17*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 18*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 19*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 20*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 21*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 22*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 23*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 24*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 25*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 26*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 27*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 28*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 29*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 30*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 31*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 32*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 33*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 34*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 35*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 36*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 37*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 38*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 39*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 40*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 41*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 42*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 43*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 44*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 45*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 46*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 47*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 48*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 49*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 50*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 51*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 52*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 53*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 54*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 55*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 56*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 57*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 58*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 59*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 60*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 61*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 62*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 63*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 64*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 65*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 66*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 67*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 68*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 69*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 70*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 71*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 72*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 73*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 74*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 75*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 76*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 77*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 78*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 79*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 80*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 81*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 82*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 83*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 84*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 85*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 86*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 87*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 88*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 89*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 90*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 91*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 92*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 93*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 94*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 95*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 96*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 97*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 98*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 99*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 100*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 101*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 102*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 103*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 104*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 105*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 106*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 107*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 108*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 109*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 110*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 111*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 112*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 113*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 114*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 115*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 116*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 117*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 118*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 119*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 120*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 121*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 122*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 123*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 124*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 125*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 126*64(%[addr]) \n" \
	"vmovntdq %%zmm0, 127*64(%[addr]) \n"

void write_nt_64(void *dest, void *src);
void write_nt_128(void *dest, void *src);
void write_nt_256(void *dest, void *src);
void write_nt_512(void *dest, void *src);
void write_nt_1024(void *dest, void *src);
void write_nt_2048(void *dest, void *src);
void write_nt_4096(void *dest, void *src);
void write_nt_8192(void *dest, void *src);
void write_nt_16384(void *dest, void *src);
void write_nt_32768(void *dest, void *src);
void write_nt_65536(void *dest, void *src);

void memcpy_nt_128(void *dest, void *src);
void memcpy_nt_256(void *dest, void *src);
void memcpy_nt_512(void *dest, void *src);
void memcpy_nt_1024(void *dest, void *src);
void memcpy_nt_2048(void *dest, void *src);


static inline void store_a_tuple(void* dest, void* src) {
#ifdef USE_NVM

#ifdef CLFLUSH
	memcpy(dest, src, TUPLE_T_SIZE);
	clflush(dest, TUPLE_T_SIZE);
#elif CLFLUSHOPT
	memcpy(dest, src, TUPLE_T_SIZE);
	clflushopt(dest, TUPLE_T_SIZE);

#elif NT_STORE

#if TUPLE_T_SIZE == 16
	_mm_stream_si128 ((__m128i*) dest, *(__m128i*) src); 	// 16B tuple only
#elif TUPLE_T_SIZE == 32
	_mm256_stream_si256 ((__m256i*) dest, *(__m256i*) src); // 32B tuple only
#elif TUPLE_T_SIZE == 64
	write_nt_64(dest, src);						 			// 64B tuple only
#elif TUPLE_T_SIZE == 128
	memcpy_nt_128(dest, src);
#elif TUPLE_T_SIZE == 256
	memcpy_nt_256(dest, src);
#elif TUPLE_T_SIZE == 512
	memcpy_nt_512(dest, src);
#elif TUPLE_T_SIZE == 1024
	memcpy_nt_1024(dest, src);
#elif TUPLE_T_SIZE == 2048
	memcpy_nt_2048(dest, src);
#else /* TUPLE_T_SIZE == 16 */
	write_nt_64(dest, src);									// first 64B
	for (int idx = 1; idx < TUPLE_T_SIZE/64; idx ++) {
		write_nt_64(dest+idx*64, src+idx*64);
	}
#endif /* TUPLE_T_SIZE == 16 */

#else /* CLFLUSH */
	memcpy(dest, src, TUPLE_T_SIZE);
#endif /* CLFLUSH */

#else /* USE_NVM */
	memcpy(dest, src, TUPLE_T_SIZE);
#endif /* USE_NVM */

#ifdef ENABLE_FENCE
	sfence();
#endif /* ENABLE_FENCE */
}


static inline void store_a_keyrid(void* dest, void* src) {
#ifdef USE_NVM

#ifdef CLFLUSH
	memcpy(dest, src, KEYRID_T_SIZE);
	clflush(dest, KEYRID_T_SIZE);
#elif CLFLUSHOPT
	memcpy(dest, src, KEYRID_T_SIZE);
	clflushopt(dest, KEYRID_T_SIZE);

#elif NT_STORE
	/* keyrid_t is 16B */
	_mm_stream_si128 ((__m128i*) dest, *(__m128i*) src);
#else /* CLFLUSH */
	memcpy(dest, src, KEYRID_T_SIZE);
#endif /* CLFLUSH */

#else /* USE_NVM */
	memcpy(dest, src, KEYRID_T_SIZE);
#endif /* USE_NVM */

#ifdef ENABLE_FENCE
	sfence();
#endif /* ENABLE_FENCE */
}


static inline void store_a_row_id_pair(void* dest, void* src) {
#ifdef USE_NVM

#ifdef CLFLUSH
	memcpy(dest, src, ROW_ID_PAIR_T_SIZE);
	clflush(dest, ROW_ID_PAIR_T_SIZE);
#elif CLFLUSHOPT
	memcpy(dest, src, ROW_ID_PAIR_T_SIZE);
	clflushopt(dest, ROW_ID_PAIR_T_SIZE);

#elif NT_STORE
	/* keyrid_t is 16B */
	_mm_stream_si128 ((__m128i*) dest, *(__m128i*) src);
#else /* CLFLUSH */
	memcpy(dest, src, ROW_ID_PAIR_T_SIZE);
#endif /* CLFLUSH */

#else /* USE_NVM */
	memcpy(dest, src, ROW_ID_PAIR_T_SIZE);
#endif /* USE_NVM */

#ifdef ENABLE_FENCE
	sfence();
#endif /* ENABLE_FENCE */
}


#endif /* MEMACCESS_H */