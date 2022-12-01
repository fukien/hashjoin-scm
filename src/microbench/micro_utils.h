#include <time.h>

#include "../inc/memaccess.h"

#ifdef USE_PAPI
#include "../papi/mypapi.h"
extern char* PAPI_event_names[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
#include "../pmw/mypmw.h"
extern unsigned int NVMCount;
static MY_PMWATCH_OP_BUF_NODE PMWatch_output[MaxNVMNum];
#endif /* USE_PMWATCH */


#ifndef IDHASH
#define IDHASH(X, MASK, BITSKIP) (((X) & MASK) >> BITSKIP)
#endif /* IDHASH */

extern char work_dir[CHAR_BUFFER_LEN];
extern int local_cores[SINGLE_SOCKET_CORE_NUM];
extern int remote_cores[SINGLE_SOCKET_CORE_NUM];

typedef struct {
	int _tid;
	pthread_barrier_t *barrier;
	struct timespec clock_start, clock_end;
#ifdef USE_PAPI
	long long PAPI_values[NUM_PAPI_EVENT];
#endif /* USE_PAPI */
} arg_t;

void nt_store_64(void *dst) {
	asm volatile(
		WRITE_NT_64_ASM
		:
		: [addr] "r" (dst)
		: "%zmm0"
	);
}

void nt_store_128(void *dst) {
	asm volatile(
		WRITE_NT_128_ASM
		:
		: [addr] "r" (dst)
		: "%zmm0"
	);
}

void nt_store_256(void *dst) {
	asm volatile(
		WRITE_NT_256_ASM
		:
		: [addr] "r" (dst)
		: "%zmm0"
	);
}

void nt_store_512(void *dst) {
	asm volatile(
		WRITE_NT_512_ASM
		:
		: [addr] "r" (dst)
		: "%zmm0"
	);
}

void nt_store_1024(void *dst) {
	asm volatile(
		WRITE_NT_1024_ASM
		:
		: [addr] "r" (dst)
		: "%zmm0"
	);
}

void nt_store_2048(void *dst) {
	asm volatile(
		WRITE_NT_2048_ASM
		:
		: [addr] "r" (dst)
		: "%zmm0"
	);
}

void nt_store_4096(void *dst) {
	asm volatile(
		WRITE_NT_4096_ASM
		:
		: [addr] "r" (dst)
		: "%zmm0"
	);
}

void nt_store_8192(void *dst) {
	asm volatile(
		WRITE_NT_8192_ASM
		:
		: [addr] "r" (dst)
		: "%zmm0"
	);
}

static inline void nontemp_store_write(void *dst) {
#if SWWCB_SIZE == 64
	nt_store_64(dst);
#elif SWWCB_SIZE == 128
	nt_store_128(dst);
#elif SWWCB_SIZE == 256
	nt_store_256(dst);
#elif SWWCB_SIZE == 512
	nt_store_512(dst);
#elif SWWCB_SIZE == 1024
	nt_store_1024(dst);
#elif SWWCB_SIZE == 2048
	nt_store_2048(dst);
#elif SWWCB_SIZE == 4096
	nt_store_4096(dst);
#elif SWWCB_SIZE == 8192
	nt_store_8192(dst);
#else 	/* default setting*/

#ifdef USE_NVM
	/* NVM default setting, 256B */
	nt_store_256(dst);
#else /* USE_NVM */
	/* DRAM default setting, 64B */
	nt_store_64(dst);
#endif /* USE_NVM */

#endif /* SWWCB_SIZE == 64 */

#ifdef ENABLE_FENCE
	sfence();
#endif /* ENABLE_FENCE */
}


static inline void nontemp_store_swwcb(void *dst, void *src) {
#if SWWCB_SIZE == 64
	write_nt_64(dst, src);
#elif SWWCB_SIZE == 128
	memcpy_nt_128(dst, src);
#elif SWWCB_SIZE == 256
	memcpy_nt_256(dst, src);
#elif SWWCB_SIZE == 512
	memcpy_nt_512(dst, src);
#elif SWWCB_SIZE == 1024
	memcpy_nt_1024(dst, src);
#elif SWWCB_SIZE == 2048
	memcpy_nt_2048(dst, src);
#elif SWWCB_SIZE == 4096
	memcpy_nt_2048(dst, src);
	memcpy_nt_2048(dst+2048, src+2048);
#elif SWWCB_SIZE == 8192
	memcpy_nt_2048(dst, src);
	memcpy_nt_2048(dst+2048, src+2048);
	memcpy_nt_2048(dst+4096, src+4096);
	memcpy_nt_2048(dst+6144, src+6144);
#else 	/* default setting*/

#ifdef USE_NVM
	/* NVM default setting, 256B */
	memcpy_nt_256(dst, src);
#else /* USE_NVM */
	/* DRAM default setting, 64B */
	write_nt_64(dst, src);
#endif /* USE_NVM */

#endif /* SWWCB_SIZE == 64 */

#ifdef ENABLE_FENCE
	sfence();
#endif /* ENABLE_FENCE */
}
