#include "libpmem.h"

#include "../inc/memaccess.h"
#include "../inc/store.h"
#include "../inc/utils.h"

#define WORK_PATH_PREFIX "/optane/huang/pm-join/work"

#ifdef USE_PMWATCH
#include "../pmw/mypmw.h"
extern unsigned int NVMCount;
// static MY_PMWATCH_OP_BUF_NODE PMWatch_output[MaxNVMNum];
#endif /* USE_PMWATCH */

extern char work_dir[CHAR_BUFFER_LEN];

const size_t buffer_size = 1 << 14; 					// 16KB
const size_t tuple_num = buffer_size / TUPLE_T_SIZE;
const size_t iter_num = 10 * 44 * ( (1 << 20) / buffer_size );		// 100 * L3-cache size

struct timespec mgmt_start, mgmt_end, build_start, build_end, probe_start, probe_end, total_start, total_end;
double mgmt_time = 0.0;
double build_time = 0.0;
double probe_time = 0.0;
double total_time = 0.0;

int main(int argc, char **argv) {
	int seed = 32768;
	srand(seed);

	size_t kdx = 0;
	size_t key_sum = 0;
	size_t row_id_sum = 0;

	snprintf(work_dir, CHAR_BUFFER_LEN, "%s/%s", WORK_PATH_PREFIX, current_timestamp());
	mkdir(work_dir);

	clock_gettime(CLOCK_REALTIME, &total_start);


	tuple_t* addr;
	clock_gettime(CLOCK_REALTIME, &mgmt_start);
// #ifdef USE_NVM
// 	addr = (tuple_t*) new_alloc_nvm(buffer_size);
// #else /* USE_NVM */
// 	addr = (tuple_t*) alloc_aligned_dram(buffer_size, CACHELINE_SIZE);
// #endif /* USE_NVM */
	addr = (tuple_t*) alloc_memory(buffer_size, CACHELINE_SIZE);
	memset(addr, 0, buffer_size);
	clock_gettime(CLOCK_REALTIME, &mgmt_end);
	mgmt_time += diff_sec(mgmt_start, mgmt_end);

	for (size_t idx = 0; idx < iter_num; idx ++) {
		clock_gettime(CLOCK_REALTIME, &mgmt_start);
		memset(addr, 0, buffer_size);
		clock_gettime(CLOCK_REALTIME, &mgmt_end);

		clock_gettime(CLOCK_REALTIME, &build_start);
		for (size_t jdx = 0; jdx < tuple_num; jdx ++) {
			kdx = rand()%(tuple_num-1);
			(addr+kdx)->key = tuple_num-jdx;
			(addr+kdx)->row_id = jdx;
		}
		clock_gettime(CLOCK_REALTIME, &build_end);

		clock_gettime(CLOCK_REALTIME, &probe_start);
		for (size_t jdx = 0; jdx < tuple_num; jdx ++) {
			kdx = rand()%(tuple_num-1);
			key_sum += (addr+kdx)->key;
			row_id_sum += (addr+kdx)->row_id;
		}
		clock_gettime(CLOCK_REALTIME, &probe_end);

		mgmt_time += diff_sec(mgmt_start, mgmt_end);
		build_time += diff_sec(build_start, build_end);
		probe_time += diff_sec(probe_start, probe_end);
	}

	clock_gettime(CLOCK_REALTIME, &mgmt_start);
// #ifdef USE_NVM
// 		pmem_unmap(addr, buffer_size);
// #else /* USE_NVMs */
// 		free(addr);
// #endif /* USE_NVM */
	dealloc_memory(addr, buffer_size);
	clock_gettime(CLOCK_REALTIME, &mgmt_end);

	mgmt_time += diff_sec(mgmt_start, mgmt_end);

	clock_gettime(CLOCK_REALTIME, &total_end);
	total_time = diff_sec(total_start, total_end);

	deldir(work_dir);

	printf("key_sum: %zu\trow_id_sum: %zu\n", key_sum, row_id_sum);
	printf("total_time: %.8f\tmgmt_time: %.8f\tbuild_time: %.8f\tprobe_time: %.8f\n",
		total_time, mgmt_time, build_time, probe_time
	);

	return 0;
}



// #ifdef USE_PMWATCH	
// 	pmwEnd(PMWatch_output);
// 	pmwClear();
// #endif /* USE_PMWATCH */


// #ifdef USE_PMWATCH
// 	pmwInit();
// 	pmwStart();
// #endif /* USE_PMWATCH */


// #ifdef USE_PMWATCH
// 	for (int idx = 0; idx < NVMCount; idx ++) {
// 		printf("[PMWatch] DIMM: %d\n", idx);
// 		printf("[PMWatch] DIMM: %d\ttotal_bytes_read: %zu\n", idx, PMWatch_output[idx].total_bytes_read);
// 		printf("[PMWatch] DIMM: %d\ttotal_bytes_written: %zu\n", idx, PMWatch_output[idx].total_bytes_written);
// 		printf("[PMWatch] DIMM: %d\tmedia_read_ops: %zu\n", idx, PMWatch_output[idx].media_read_ops);
// 		printf("[PMWatch] DIMM: %d\tmedia_write_ops: %zu\n", idx, PMWatch_output[idx].media_write_ops);
// 		printf("[PMWatch] DIMM: %d\tread_64B_ops_received: %zu\n", idx, PMWatch_output[idx].read_64B_ops_received);
// 		printf("[PMWatch] DIMM: %d\twrite_64B_ops_received: %zu\n", idx, PMWatch_output[idx].write_64B_ops_received);
// 		printf("[PMWatch] DIMM: %d\thost_reads: %zu\n", idx, PMWatch_output[idx].host_reads);
// 		printf("[PMWatch] DIMM: %d\thost_writes: %zu\n", idx, PMWatch_output[idx].host_writes);
// 		printf("[PMWatch] DIMM: %d\tread_hit_ratio: %f\n", idx, PMWatch_output[idx].read_hit_ratio);
// 		printf("[PMWatch] DIMM: %d\twrite_hit_ratio: %f\n", idx, PMWatch_output[idx].write_hit_ratio);
// 		printf("[PMWatch] DIMM: %d\tread_amplification: %.9f\n", idx, PMWatch_output[idx].read_amplification);
// 		printf("[PMWatch] DIMM: %d\twrite_amplification: %.9f\n", idx, PMWatch_output[idx].write_amplification);
// 		printf("\n");
// 	}
// #endif /* USE_PMWATCH */