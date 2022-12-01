
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libconfig.h>
#include <libpmem.h>

#include "inc/lock.h"
#include "inc/store.h"
#include "inc/task.h"
#include "inc/utils.h"

#include "papi/mypapi.h"
#include "pmw/mypmw.h"

#ifdef IN_DEBUG
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#endif /* IN_DEBUG */

extern unsigned int NVMCount;
extern char* PAPI_event_names[NUM_PAPI_EVENT];
extern int node_mapping[CORE_NUM];
extern int local_cores[SINGLE_SOCKET_CORE_NUM];
extern int remote_cores[SINGLE_SOCKET_CORE_NUM];

void test_PMWatch_PAPI() {

	struct timespec tmp1, tmp2;

	MY_PMWATCH_OP_BUF_NODE output[MaxNVMNum];
	// long long values[NUM_PAPI_EVENT];

	pmwInit();
	// papiInit();

	clock_gettime(CLOCK_REALTIME, &tmp1);

	// papiStart();
	pmwStart();

	/**
	 *  Some test codes
	 */

	pmwEnd(output);
	// papiEnd(values);

	clock_gettime(CLOCK_REALTIME, &tmp2);

	pmwClear();
	// papiClear();

	printf("%.2f\n", 
		( tmp2.tv_sec - tmp1.tv_sec ) + ( tmp2.tv_nsec - tmp1.tv_nsec ) * 1e-9
	);


	for (int idx = 0; idx < NVMCount; idx ++) {
		printf("DIMM: %d\n", idx);
		printf("total_bytes_read: %zu\n", output[idx].total_bytes_read);
		printf("total_bytes_written: %zu\n", output[idx].total_bytes_written);
		printf("media_read_ops: %zu\n", output[idx].media_read_ops);
		printf("media_write_ops: %zu\n", output[idx].media_write_ops);
		printf("read_64B_ops_received: %zu\n", output[idx].read_64B_ops_received);
		printf("write_64B_ops_received: %zu\n", output[idx].write_64B_ops_received);
		printf("host_reads: %zu\n", output[idx].host_reads);
		printf("host_writes: %zu\n", output[idx].host_writes);
		printf("read_hit_ratio: %f\n", output[idx].read_hit_ratio);
		printf("write_hit_ratio: %f\n", output[idx].write_hit_ratio);
		printf("read_amplification: %.9f\n", output[idx].read_amplification);
		printf("write_amplification: %.9f\n", output[idx].write_amplification);
		printf("\n");
	}

	/**
	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		printf("event_name: %s\tevent_counter: %lld\n", PAPI_event_names[idx], values[idx]);
	}
	*/

	printf("TUPLE SIZE: %zu\n", (size_t) TUPLE_T_SIZE);
}


void test_data_cfg () {

		datameta_t datameta;
		char* path;
		path = "../config/data/sparsity/0.500_A.cfg";
		// path = "../config/data/uniform/pkfk_A.cfg";


		data_cfg(&datameta, path);

		printf("r_path_suffix: %s\n"
			"s_path_suffix: %s\n"
			"theta: %f\n"
			"selectivity: %f\n"
			"density: %f\n"
			"sparsity: %f\n"
			"r_tuple_num: %zu\n"
			"s_tuple_num: %zu\n"
			"r_tuple_size: %zu\n"
			"s_tuple_size: %zu\n"
			"min_key: %zu\n"
			"max_key: %zu\n"
			"workload_name: %s\n",
			datameta.r_path_suffix,
			datameta.s_path_suffix,
			datameta.theta,
			datameta.selectivity,
			datameta.density,
			datameta.sparsity,
			datameta.r_tuple_num,
			datameta.s_tuple_num,
			datameta.r_tuple_size,
			datameta.s_tuple_size,
			datameta.min_key,
			datameta.max_key,
			datameta.workload_name
		);

		free(datameta.r_path_suffix);
		free(datameta.s_path_suffix);
		free(datameta.workload_name);
}




#ifdef IN_DEBUG

void test_size_of_task() {
	printf("size of lock_t: %zu\n", sizeof(lock_t));				// 1B
	printf("size of task_t: %zu\n", sizeof(task_t));				// 72B
	printf("size of task_list_t: %zu\n", sizeof(task_list_t));		// 24B
	printf("size of task_queue_t: %zu\n", sizeof(task_queue_t));	// 40B
}


void test_simd_output() {
	__m512i aaa = _mm512_set_epi64(91, 3, 4, 312, 14, 535, 12, 9);
	print_512_64(aaa);
	__m256i bbb = _mm256_set_epi64x(34, 424, 51356, 734);
	print_256_64(bbb);

	printf("sizeof(aaa): %zu\tsizeof(aaa): %zu\n", sizeof(aaa), sizeof(bbb));

	typedef struct {
		char characters[CACHELINE_SIZE];
	} __attribute__((aligned(CACHELINE_SIZE))) CCC;

	CCC *ccc = malloc(sizeof(CCC));
	print_512_64( _mm512_loadu_si512(ccc) ); 									// don't use _mm512_load_si512 for it has to be aligned on a 64-byte boundary


	void *ddd = alloc_aligned_dram(AVX512_SIZE, CACHELINE_SIZE);
	_mm512_stream_si512( (__m512i *) ddd, _mm512_loadu_si512(ccc)); 			// destination must be aligned on a 64-byte boundary

	print_512_64( _mm512_loadu_si512(ddd) );
	
	free(ccc);
	free(ddd);

	printf("\n\n");

	size_t num = 10;

	void *eee = alloc_aligned_dram(AVX512_SIZE * num, CACHELINE_SIZE);
	printf("eee\n");
	for (size_t idx = 0; idx < num; idx ++) {
		printf("%zu:\t%p\n", idx, eee+AVX512_SIZE*idx);
		_mm512_load_si512(eee+AVX512_SIZE*idx);
		assert( (((uintptr_t)(eee+AVX512_SIZE*idx))&0x3f) == 0 );
	}
	free(eee);

	printf("\n");

	void *fff = alloc_aligned_dram(XPLINE_SIZE * num, CACHELINE_SIZE);
	printf("fff\n");
	for (size_t idx = 0; idx < num; idx ++) {
		printf("%zu:\t%p\n", idx, fff+XPLINE_SIZE*idx);
		_mm512_load_si512(fff+XPLINE_SIZE*idx);
		assert( (((uintptr_t)(fff+XPLINE_SIZE*idx))&0x3f) == 0 );
	}
	free(fff);
}

#define DEBUGGING_PATH "/optane/huang/pm-join/work/debug"
#define T_NUM 200
#define LOOP_BUN 32

size_t debugging_global_index = 0;
size_t buffer_size = 1 << 25;

typedef struct {
	size_t _tid;
} __attribute__( (packed) ) debugging_struct;

void *run_debugging(void *arg) {
	char filepath[CHAR_BUFFER_LEN*2];
	debugging_struct * stu = (debugging_struct*) arg;
	for (size_t idx = 0; idx < LOOP_BUN; idx ++ ) {
		snprintf(filepath, CHAR_BUFFER_LEN*2, "%s/%zu-%zu.bin", DEBUGGING_PATH, stu->_tid, debugging_global_index);
		debugging_global_index ++;
		void *aaa = create_pmem_buffer(filepath, buffer_size);
		pmem_unmap(aaa, buffer_size);
	}
	return NULL;
}

void debugging_errno_12() {
	pthread_t threadpool[T_NUM];
	debugging_struct args[T_NUM];
	mkdir(DEBUGGING_PATH);
	for (size_t idx = 0; idx < T_NUM; idx ++ ) {
		args[idx]._tid = idx;

		pthread_create(&threadpool[idx], NULL, run_debugging, args + idx);
	}
	for (size_t idx = 0; idx < T_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
	}
	deldir(DEBUGGING_PATH);
}

#endif /* IN_DEBUG */


int main(int argc, char** argv) {
	return 0;
}