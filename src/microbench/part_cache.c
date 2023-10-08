#include <math.h>
#include <stdlib.h>

#include "libpmem.h"

#define __USE_GNU
#include <sched.h>
#include <pthread.h>

#include "../inc/store.h"
#include "../inc/utils.h"

#include "micro_utils.h"

#define WORK_PATH_PREFIX "/optane/huang/hashjoin-scm/work"

#ifdef USE_NVM
#define TEST_THREAD_NUM 12
#else /* USE_NVM */
#define TEST_THREAD_NUM 20
#endif /* USE_NVM */

void * test_part(void *param) {
	arg_t *arg = (arg_t *) param;

	int rv;

	size_t stride_size = XPLINE_SIZE * 8;
	size_t mem_size = PAGE_SIZE * L1_TLB_NUM;

	void *dest;
#ifdef USE_NVM
	dest = new_alloc_nvm(mem_size);
#else /* USE_NVM */
#ifdef USE_HUGE
	dest = alloc_huge_dram(mem_size);
#endif /* USE_HUGE */
	dest = alloc_aligned_dram(mem_size, PAGE_SIZE);
#endif /* USE_NVM */

	memset_localize(dest, mem_size);

	const size_t max_range_idx = (1 << 7) -1; 
	const size_t min_range_idx = 0;

	size_t foodstep = mem_size / (max_range_idx - min_range_idx + 1);

	size_t swwcb_num = max_range_idx - min_range_idx + 1;
	size_t hashmask = swwcb_num - 1;

	void *swwcb;
#ifdef USE_NVM
	swwcb = new_alloc_nvm(SWWCB_SIZE*swwcb_num);
#else /* USE_NVM */
#ifdef USE_HUGE
	swwcb = alloc_huge_dram(SWWCB_SIZE*swwcb_num);
#endif /* USE_HUGE */
	swwcb = alloc_aligned_dram(SWWCB_SIZE*swwcb_num, PAGE_SIZE);
#endif /* USE_NVM */
	memset_localize(swwcb, SWWCB_SIZE*swwcb_num);

	size_t outer_loop_num = 1 << 20;
	size_t inner_loop_num = stride_size / SWWCB_SIZE;

	/* see https://people.sc.fsu.edu/~jburkardt/c_src/uniform/uniform.c */
	const int i4_huge = 2147483647;
	int k;
	double r;
	int uni_seed = rand() % RAND_MAX;

	BARRIER_ARRIVE(arg->barrier, rv);


#ifdef USE_PAPI
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
	if (arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */

	if (arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &arg->clock_start);
	}

	int dest_idx;
	int src_idx;
	for (size_t idx = 0; idx < outer_loop_num; idx ++) {
		k = uni_seed / 127773;
		uni_seed = 16807 * ( uni_seed - k * 127773 ) - k * 2836;
		if (uni_seed < 0) {
			uni_seed = uni_seed + i4_huge;
		}
		r = (double) (uni_seed) * 4.656612875E-10;
		r = ( 1.0 - r ) * ( (double) (min_range_idx) - 0.5 ) + r * ( (double)(max_range_idx) + 0.5);
		dest_idx = (int) round(r);
		if (dest_idx < min_range_idx) {
			dest_idx = min_range_idx;
		}
		if (dest_idx > max_range_idx) {
			dest_idx = max_range_idx;
		}

		src_idx = IDHASH(dest_idx, hashmask, 0);

		for (size_t jdx = 0; jdx < inner_loop_num; jdx ++) {
			void *dest_addr = dest + foodstep * dest_idx + SWWCB_SIZE * jdx;
#ifdef USE_NVM
			nontemp_store_swwcb(dest_addr, swwcb+SWWCB_SIZE*src_idx);
			// nontemp_store_write(dest_addr);
#else /* USE_NVM */
			nontemp_store_swwcb(dest_addr, swwcb+SWWCB_SIZE*src_idx);
			// nontemp_store_write(dest_addr);
#endif /* USE_NVM */
		}
	}

	BARRIER_ARRIVE(arg->barrier, rv);

	if (arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &arg->clock_end);
	}

#ifdef USE_PMWATCH
	if (arg->_tid == 0) {
		pmwEnd(PMWatch_output);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, arg->PAPI_values);
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

	dealloc_memory(swwcb, SWWCB_SIZE*swwcb_num);
	dealloc_memory(dest, mem_size);

	return NULL;
}


double test_cache() {
	int rv;

	snprintf(work_dir, CHAR_BUFFER_LEN, "%s/%s", WORK_PATH_PREFIX, current_timestamp());
	mkdir(work_dir);

	pthread_t threadpool[TEST_THREAD_NUM];
	arg_t args[TEST_THREAD_NUM];

	pthread_barrier_t barrier;
	rv = pthread_barrier_init(&barrier, NULL, TEST_THREAD_NUM);
	if(rv){
		printf("[ERROR] Couldn't create the buildpart_barrier\n");
		exit(EXIT_FAILURE);
	}

#ifdef USE_PAPI
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
#endif /* USE_PAPI */

	cpu_set_t set;
	int cpu_idx;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	mc_cfg(local_cores, remote_cores);

	for (int idx = 0; idx < TEST_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &barrier;
#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */
		pthread_create(&threadpool[idx], &attr, test_part, args+idx);
	}

	for (int idx = 0; idx < TEST_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values[jdx] += (args+idx)->PAPI_values[jdx];
		}
#endif /* USE_PAPI */
	}

#ifdef USE_PAPI
	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		printf("[PAPI] PAPI_event_name: %s", PAPI_event_names[idx]);
		printf("\ttotal_counter: %lld", PAPI_values[idx]);
		printf("\n");
	}
	PAPI_shutdown();
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
	for (int idx = 0; idx < NVMCount; idx ++) {
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

	printf("SWWCB_SIZE: %d\ttotal elapsed time: %.9f\n", SWWCB_SIZE, diff_sec(args[0].clock_start, args[0].clock_end));
	deldir(work_dir);

	return diff_sec(args[0].clock_start, args[0].clock_end);
}


int main(int argc, char **argv) {
	double total_elapsed_time = 0.0;
	for (int t = 0; t < TRIAL_NUM; t ++) {
		total_elapsed_time += test_cache();
	}
	printf("general elapsed time: %d\t%.8f\n", SWWCB_SIZE, total_elapsed_time/TRIAL_NUM);

	return 0;
}