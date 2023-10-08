#include <math.h>
#include <stdlib.h>

#include "libpmem.h"

#define __USE_GNU
#include <sched.h>
#include <pthread.h>

#include "../inc/memaccess.h"
#include "../inc/store.h"
#include "../inc/utils.h"

#define WORK_PATH_PREFIX "/optane/huang/hashjoin-scm/work"

#ifdef USE_PAPI
#include "../papi/mypapi.h"
extern char* PAPI_event_names[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
#include "../pmw/mypmw.h"
extern unsigned int NVMCount;
static MY_PMWATCH_OP_BUF_NODE PMWatch_output[MaxNVMNum];
#endif /* USE_PMWATCH */

extern char work_dir[CHAR_BUFFER_LEN];


struct timespec total_start, total_end;


typedef struct bkt {
	int count;
	tuple_t padd_0[4];
	char latch;
	tuple_t padd_1[4];
	struct bkt *next;
	tuple_t padd_2[2];
	tuple_t tuples[2];
// } __attribute__( ( aligned( CACHELINE_SIZE ) ) ) bkt;
} __attribute__( ( aligned( XPLINE_SIZE ) ) ) bkt;
// } __attribute__( (packed) ) bkt;
// } bkt;
// 


void read__256(void *src) {
	asm volatile(
		"vmovdqa64 (%[data]), %%zmm0 \n"
		"vmovdqa64 1*64(%[data]), %%zmm1 \n"
		"vmovdqa64 2*64(%[data]), %%zmm2 \n"
		"vmovdqa64 3*64(%[data]), %%zmm3 \n"
		// "vmovntdqa (%[data]), %%zmm0 \n"
		// "vmovntdqa 1*64(%[data]), %%zmm1 \n"
		// "vmovntdqa 2*64(%[data]), %%zmm2 \n"
		// "vmovntdqa 3*64(%[data]), %%zmm3 \n"
		:
		: [data] "r" (src)
		: "%zmm0", "%zmm1", "%zmm2", "%zmm3"
	);
}





int main(int argc, char **argv) {
	int seed = 32768;
	srand(seed);
	size_t outer_loop_num = 1 << 18;
	size_t inner_loop_num = 1 << 7;

	char* filepath = "/optane/huang/hashjoin-scm/work/debug.bin";
	double runtime = 0.0;

	size_t sum = 0;
	bkt *tmp;

	size_t read_buf_size = 6 * 16 *( 1 << 10);	// 16 KB per DIMM

	size_t bkt_num = read_buf_size * ( 1 << 16 ) / XPLINE_SIZE;	// fixed bkt_num

	size_t buffer_size = read_buf_size * (1 << 16);

	bkt *bkt_addr = (bkt*) create_pmem_buffer(filepath, buffer_size);
	pmem_memset_persist(bkt_addr, 0, buffer_size);


#ifdef USE_PAPI
	int rv;

	long long PAPI_values[NUM_PAPI_EVENT];
	memset(PAPI_values, 0, sizeof(long long) * NUM_PAPI_EVENT );

	/* papi library initialization */
	if ( (rv = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
		printf("Library initialization error! \n");
		exit(1);
	}

	/** PAPI_thread_init initializes thread support in the PAPI library. 
	 * 	Itshould be called only once, just after PAPI_library_init (3), 
	 * 	and before any other PAPI calls. Applications that make no use 
	 * 	of threads do not need to call this routine.
	 */
	rv = PAPI_thread_init( (unsigned long(*) (void) ) (pthread_self) );
	if ( (rv = PAPI_library_init(PAPI_VER_CURRENT) ) != PAPI_VER_CURRENT) {
		printf("Library initialization error! \n");
		exit(1);
	}

	/* Enable and initialize multiplex support */
	rv = PAPI_multiplex_init();
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}


	int eventset = PAPI_NULL;

	rv = PAPI_create_eventset(&eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Assign it to the CPU component */
	rv = PAPI_assign_eventset_component(eventset, 0);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Convert the EventSet to a multiplexed event set */
	rv = PAPI_set_multiplex(eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		rv = PAPI_add_named_event(eventset, PAPI_event_names[idx]);
		if (rv != PAPI_OK) {
			ERROR_RETURN(rv);
			exit(1);
		}		
	}

	PAPI_reset(eventset);
	PAPI_start(eventset);
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
		pmwInit();
		pmwStart();
#endif /* USE_PMWATCH */

	// bkt dram_bkt;

	clock_gettime(CLOCK_REALTIME, &total_start);

	for (size_t jdx = 0; jdx < outer_loop_num; jdx ++) {
		for (size_t idx = 0; idx < inner_loop_num; idx ++) {
			tmp = bkt_addr + ( rand() % bkt_num );

			read__256(tmp);		// can not applied to CACHELINE_SIZE-aligned

			sum += tmp->count;
			sum += tmp->latch;
			// sum += tmp->next->count;
			sum += tmp->tuples[0].key;
			sum += tmp->tuples[1].key;

			// lfence();

			// // memcpy(&dram_bkt, tmp, sizeof(bkt));
			// dram_bkt = *tmp;
			// sum += dram_bkt.count;
			// sum += dram_bkt.latch;
			// // sum += dram_bkt.next->count;
			// sum += dram_bkt.tuples[0].key;
			// sum += dram_bkt.tuples[1].key;
		}	
	}

	clock_gettime(CLOCK_REALTIME, &total_end);



#ifdef USE_PMWATCH
	pmwEnd(PMWatch_output);
	pmwClear();
#endif /* USE_PMWATCH */


#ifdef USE_PAPI
	PAPI_stop(eventset, PAPI_values);
	/* Free all memory and data structures, eventset must be empty. */
	if ( (rv = PAPI_cleanup_eventset(eventset))!=PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	if ( (rv = PAPI_destroy_eventset(&eventset)) != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}
#endif /* USE_PAPI */

	runtime = diff_sec(total_start, total_end);


#ifdef USE_PAPI
	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		printf("[PAPI] PAPI_event_name: %s", PAPI_event_names[idx]);
		printf("\ttotal_counter: %lld", PAPI_values[idx]);
		printf("\n");
	}
	PAPI_shutdown();
#endif /* USE_PAPI */



#ifdef USE_PMWATCH
	// for (int idx = 0; idx < NVMCount; idx ++) {
	for (int idx = 0; idx < 1; idx ++) {
		printf("[PMWatch] DIMM: %d\n", idx);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_read: %zu\n", idx, PMWatch_output[idx].total_bytes_read);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_written: %zu\n", idx, PMWatch_output[idx].total_bytes_written);
		printf("[PMWatch] DIMM: %d\tmedia_read_ops: %zu\n", idx, PMWatch_output[idx].media_read_ops);
		printf("[PMWatch] DIMM: %d\tmedia_write_ops: %zu\n", idx, PMWatch_output[idx].media_write_ops);
		printf("[PMWatch] DIMM: %d\tread_64B_ops_received: %zu\n", idx, PMWatch_output[idx].read_64B_ops_received);
		printf("[PMWatch] DIMM: %d\twrite_64B_ops_received: %zu\n", idx, PMWatch_output[idx].write_64B_ops_received);
		printf("[PMWatch] DIMM: %d\thost_reads: %zu\n", idx, PMWatch_output[idx].host_reads);
		printf("[PMWatch] DIMM: %d\thost_writes: %zu\n", idx, PMWatch_output[idx].host_writes);
		printf("[PMWatch] DIMM: %d\tread_hit_ratio: %f\n", idx, PMWatch_output[idx].read_hit_ratio);
		printf("[PMWatch] DIMM: %d\twrite_hit_ratio: %f\n", idx, PMWatch_output[idx].write_hit_ratio);
		printf("[PMWatch] DIMM: %d\tread_amplification: %.9f\n", idx, PMWatch_output[idx].read_amplification);
		printf("[PMWatch] DIMM: %d\twrite_amplification: %.9f\n", idx, PMWatch_output[idx].write_amplification);
		printf("\n");
	}
#endif /* USE_PMWATCH */


	printf("bkt_size: %zu\truntime: %.8f\tsum: %zu\n", sizeof(bkt), runtime, sum);

	dealloc_memory(bkt_addr, buffer_size);

	return 0;
}