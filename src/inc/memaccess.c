#include "memaccess.h"

void write_nt_64(void *dest, void *src) {
	asm volatile(
		"vmovntdqa  (%[data]), %%zmm0 \n"
		WRITE_NT_64_ASM
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0"
	);
}

void write_nt_128(void *dest, void *src) {
	asm volatile(
		"vmovntdqa  (%[data]), %%zmm0 \n"
		WRITE_NT_128_ASM
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0"
	);
}

void write_nt_256(void *dest, void *src) {
	asm volatile(
		"vmovntdqa  (%[data]), %%zmm0 \n"
		WRITE_NT_256_ASM
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0"
	);
}

void write_nt_512(void *dest, void *src) {
	asm volatile(
		"vmovntdqa  (%[data]), %%zmm0 \n"
		WRITE_NT_512_ASM
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0"
	);
}

void write_nt_1024(void *dest, void *src) {
	asm volatile(
		"vmovntdqa  (%[data]), %%zmm0 \n"
		WRITE_NT_1024_ASM
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0"
	);
}

void write_nt_2048(void *dest, void *src) {
	asm volatile(
		"vmovntdqa  (%[data]), %%zmm0 \n"
		WRITE_NT_2048_ASM
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0"
	);
}

void write_nt_4096(void *dest, void *src) {
	asm volatile(
		"vmovntdqa  (%[data]), %%zmm0 \n"
		WRITE_NT_4096_ASM
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0"
	);
}

void write_nt_8192(void *dest, void *src) {
	asm volatile(
		"vmovntdqa  (%[data]), %%zmm0 \n"
		WRITE_NT_8192_ASM
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0"
	);
}

void write_nt_16384(void *dest, void *src) {
	asm volatile(
		"vmovntdqa  (%[data]), %%zmm0 \n"
		WRITE_NT_8192_ASM
		"add $8192, %[addr]\n"
		WRITE_NT_8192_ASM
		: [addr] "+r" (dest)
		: [data] "r" (src)
		: "%zmm0"
	);
}

void write_nt_32768(void *dest, void *src) {
	asm volatile(
		"vmovntdqa  (%[data]), %%zmm0 \n"
		EXEC_4_TIMES(
			WRITE_NT_8192_ASM
			"add $8192, %[addr]\n"
		)
		: [addr] "+r" (dest)
		: [data] "r" (src)
		: "%zmm0"
	);
}

void write_nt_65536(void *dest, void *src) {
	asm volatile(
		"vmovntdqa  (%[data]), %%zmm0 \n"
		EXEC_8_TIMES(
			WRITE_NT_8192_ASM
			"add $8192, %[addr]\n"
		)
		: [addr] "+r" (dest)
		: [data] "r" (src)
		: "%zmm0"
	);
}

void memcpy_nt_128(void *dest, void *src) {
	asm volatile(
		"vmovntdqa (%[data]), %%zmm0 \n"
		"vmovntdqa 1*64(%[data]), %%zmm1 \n"
		"vmovntdq %%zmm0, 0(%[addr]) \n"\
		"vmovntdq %%zmm1, 1*64(%[addr]) \n"
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0", "%zmm1"
	);
}

void memcpy_nt_256(void *dest, void *src) {
	asm volatile(
		"vmovntdqa (%[data]), %%zmm0 \n"
		"vmovntdqa 1*64(%[data]), %%zmm1 \n"
		"vmovntdqa 2*64(%[data]), %%zmm2 \n"
		"vmovntdqa 3*64(%[data]), %%zmm3 \n"
		"vmovntdq %%zmm0, 0(%[addr]) \n"
		"vmovntdq %%zmm1, 1*64(%[addr]) \n"
		"vmovntdq %%zmm2, 2*64(%[addr]) \n"
		"vmovntdq %%zmm3, 3*64(%[addr]) \n"
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0", "%zmm1", "%zmm2", "%zmm3"
	);
}

void memcpy_nt_512(void *dest, void *src) {
	asm volatile(
		"vmovntdqa (%[data]), %%zmm0 \n"
		"vmovntdqa 1*64(%[data]), %%zmm1 \n"
		"vmovntdqa 2*64(%[data]), %%zmm2 \n"
		"vmovntdqa 3*64(%[data]), %%zmm3 \n"
		"vmovntdqa 4*64(%[data]), %%zmm4 \n"
		"vmovntdqa 5*64(%[data]), %%zmm5 \n"
		"vmovntdqa 6*64(%[data]), %%zmm6 \n"
		"vmovntdqa 7*64(%[data]), %%zmm7 \n"
		"vmovntdq %%zmm0, 0*64(%[addr]) \n"
		"vmovntdq %%zmm1, 1*64(%[addr]) \n"
		"vmovntdq %%zmm2, 2*64(%[addr]) \n"
		"vmovntdq %%zmm3, 3*64(%[addr]) \n"
		"vmovntdq %%zmm4, 4*64(%[addr]) \n"
		"vmovntdq %%zmm5, 5*64(%[addr]) \n"
		"vmovntdq %%zmm6, 6*64(%[addr]) \n"
		"vmovntdq %%zmm7, 7*64(%[addr]) \n" 
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0", "%zmm1", "%zmm2", "%zmm3",
			"%zmm4", "%zmm5", "%zmm6", "%zmm7"
	);
}

void memcpy_nt_1024(void *dest, void *src) {
	asm volatile(
		"vmovntdqa (%[data]), %%zmm0 \n"
		"vmovntdqa 1*64(%[data]), %%zmm1 \n"
		"vmovntdqa 2*64(%[data]), %%zmm2 \n"
		"vmovntdqa 3*64(%[data]), %%zmm3 \n"
		"vmovntdqa 4*64(%[data]), %%zmm4 \n"
		"vmovntdqa 5*64(%[data]), %%zmm5 \n"
		"vmovntdqa 6*64(%[data]), %%zmm6 \n"
		"vmovntdqa 7*64(%[data]), %%zmm7 \n"
		"vmovntdqa 8*64(%[data]), %%zmm8 \n"
		"vmovntdqa 9*64(%[data]), %%zmm9 \n"
		"vmovntdqa 10*64(%[data]), %%zmm10 \n"
		"vmovntdqa 11*64(%[data]), %%zmm11 \n"
		"vmovntdqa 12*64(%[data]), %%zmm12 \n"
		"vmovntdqa 13*64(%[data]), %%zmm13 \n"
		"vmovntdqa 14*64(%[data]), %%zmm14 \n"
		"vmovntdqa 15*64(%[data]), %%zmm15 \n"
		"vmovntdq %%zmm0, 0*64(%[addr]) \n"
		"vmovntdq %%zmm1, 1*64(%[addr]) \n"
		"vmovntdq %%zmm2, 2*64(%[addr]) \n"
		"vmovntdq %%zmm3, 3*64(%[addr]) \n"
		"vmovntdq %%zmm4, 4*64(%[addr]) \n"
		"vmovntdq %%zmm5, 5*64(%[addr]) \n"
		"vmovntdq %%zmm6, 6*64(%[addr]) \n"
		"vmovntdq %%zmm7, 7*64(%[addr]) \n"
		"vmovntdq %%zmm8, 8*64(%[addr]) \n"
		"vmovntdq %%zmm9, 9*64(%[addr]) \n"
		"vmovntdq %%zmm10, 10*64(%[addr]) \n"
		"vmovntdq %%zmm11, 11*64(%[addr]) \n"
		"vmovntdq %%zmm12, 12*64(%[addr]) \n"
		"vmovntdq %%zmm13, 13*64(%[addr]) \n"
		"vmovntdq %%zmm14, 14*64(%[addr]) \n"
		"vmovntdq %%zmm15, 15*64(%[addr]) \n"
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0", "%zmm1", "%zmm2", "%zmm3",
			"%zmm4", "%zmm5", "%zmm6", "%zmm7",
			"%zmm8", "%zmm9", "%zmm10", "%zmm11",
			"%zmm12", "%zmm13", "%zmm14", "%zmm15"
	);
}

void memcpy_nt_2048(void *dest, void *src) {
	asm volatile(
		"vmovntdqa (%[data]), %%zmm0 \n"
		"vmovntdqa 1*64(%[data]), %%zmm1 \n"
		"vmovntdqa 2*64(%[data]), %%zmm2 \n"
		"vmovntdqa 3*64(%[data]), %%zmm3 \n"
		"vmovntdqa 4*64(%[data]), %%zmm4 \n"
		"vmovntdqa 5*64(%[data]), %%zmm5 \n"
		"vmovntdqa 6*64(%[data]), %%zmm6 \n"
		"vmovntdqa 7*64(%[data]), %%zmm7 \n"
		"vmovntdqa 8*64(%[data]), %%zmm8 \n"
		"vmovntdqa 9*64(%[data]), %%zmm9 \n"
		"vmovntdqa 10*64(%[data]), %%zmm10 \n"
		"vmovntdqa 11*64(%[data]), %%zmm11 \n"
		"vmovntdqa 12*64(%[data]), %%zmm12 \n"
		"vmovntdqa 13*64(%[data]), %%zmm13 \n"
		"vmovntdqa 14*64(%[data]), %%zmm14 \n"
		"vmovntdqa 15*64(%[data]), %%zmm15 \n"
		"vmovntdqa 16*64(%[data]), %%zmm16 \n"
		"vmovntdqa 17*64(%[data]), %%zmm17 \n"
		"vmovntdqa 18*64(%[data]), %%zmm18 \n"
		"vmovntdqa 19*64(%[data]), %%zmm19 \n"
		"vmovntdqa 20*64(%[data]), %%zmm20 \n"
		"vmovntdqa 21*64(%[data]), %%zmm21 \n"
		"vmovntdqa 22*64(%[data]), %%zmm22 \n"
		"vmovntdqa 23*64(%[data]), %%zmm23 \n"
		"vmovntdqa 24*64(%[data]), %%zmm24 \n"
		"vmovntdqa 25*64(%[data]), %%zmm25 \n"
		"vmovntdqa 26*64(%[data]), %%zmm26 \n"
		"vmovntdqa 27*64(%[data]), %%zmm27 \n"
		"vmovntdqa 28*64(%[data]), %%zmm28 \n"
		"vmovntdqa 29*64(%[data]), %%zmm29 \n"
		"vmovntdqa 30*64(%[data]), %%zmm30 \n"
		"vmovntdqa 31*64(%[data]), %%zmm31 \n"
		"vmovntdq %%zmm0, 0*64(%[addr]) \n"
		"vmovntdq %%zmm1, 1*64(%[addr]) \n"
		"vmovntdq %%zmm2, 2*64(%[addr]) \n"
		"vmovntdq %%zmm3, 3*64(%[addr]) \n"
		"vmovntdq %%zmm4, 4*64(%[addr]) \n"
		"vmovntdq %%zmm5, 5*64(%[addr]) \n"
		"vmovntdq %%zmm6, 6*64(%[addr]) \n"
		"vmovntdq %%zmm7, 7*64(%[addr]) \n"
		"vmovntdq %%zmm8, 8*64(%[addr]) \n"
		"vmovntdq %%zmm9, 9*64(%[addr]) \n"
		"vmovntdq %%zmm10, 10*64(%[addr]) \n"
		"vmovntdq %%zmm11, 11*64(%[addr]) \n"
		"vmovntdq %%zmm12, 12*64(%[addr]) \n"
		"vmovntdq %%zmm13, 13*64(%[addr]) \n"
		"vmovntdq %%zmm14, 14*64(%[addr]) \n"
		"vmovntdq %%zmm15, 15*64(%[addr]) \n"
		"vmovntdq %%zmm16, 16*64(%[addr]) \n"
		"vmovntdq %%zmm17, 17*64(%[addr]) \n"
		"vmovntdq %%zmm18, 18*64(%[addr]) \n"
		"vmovntdq %%zmm19, 19*64(%[addr]) \n"
		"vmovntdq %%zmm20, 20*64(%[addr]) \n"
		"vmovntdq %%zmm21, 21*64(%[addr]) \n"
		"vmovntdq %%zmm22, 22*64(%[addr]) \n"
		"vmovntdq %%zmm23, 23*64(%[addr]) \n"
		"vmovntdq %%zmm24, 24*64(%[addr]) \n"
		"vmovntdq %%zmm25, 25*64(%[addr]) \n"
		"vmovntdq %%zmm26, 26*64(%[addr]) \n"
		"vmovntdq %%zmm27, 27*64(%[addr]) \n"
		"vmovntdq %%zmm28, 28*64(%[addr]) \n"
		"vmovntdq %%zmm29, 29*64(%[addr]) \n"
		"vmovntdq %%zmm30, 30*64(%[addr]) \n"
		"vmovntdq %%zmm31, 31*64(%[addr]) \n"
		:
		: [addr] "r" (dest), [data] "r" (src)
		: "%zmm0", "%zmm1", "%zmm2", "%zmm3",
			"%zmm4", "%zmm5", "%zmm6", "%zmm7",
			"%zmm8", "%zmm9", "%zmm10", "%zmm11",
			"%zmm12", "%zmm13", "%zmm14", "%zmm15",
			"%zmm16", "%zmm17", "%zmm18", "%zmm19",
			"%zmm20", "%zmm21", "%zmm22", "%zmm23",
			"%zmm24", "%zmm25", "%zmm26", "%zmm27",
			"%zmm28", "%zmm29", "%zmm30", "%zmm31"
	);
}