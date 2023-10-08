#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define __USE_GNU
#include <sched.h>
#include <pthread.h>

#include "inc/store.h"
#include "inc/tpch_utils.h"
#include "inc/tpch_task_pthread.h"

#include "../inc/utils.h"

#include "algo/tpch_nphj.h"
#include "algo/tpch_rdx.h"


#ifdef USE_NVM
#define MEM_TYPE "nvm"
#else /* USE_NVM */
#define MEM_TYPE "dram"
#endif /* USE_NVM */

#define WORK_PATH_PREFIX "/optane/huang/hashjoin-scm/work"
#define DUMP_PATH_PREFIX "/optane/huang/hashjoin-scm/dump"


char dump_dir[CHAR_BUFFER_LEN];
extern char work_dir[CHAR_BUFFER_LEN];
extern int local_cores[SINGLE_SOCKET_CORE_NUM];
extern int remote_cores[SINGLE_SOCKET_CORE_NUM];


#ifdef USE_PAPI
#include "../papi/mypapi.h"
extern char* PAPI_event_names[NUM_PAPI_EVENT];
long long agg_PAPI_values[NUM_PAPI_EVENT];
long long agg_PAPI_values_pushdown[NUM_PAPI_EVENT];
long long agg_PAPI_values_buildpart[NUM_PAPI_EVENT];
long long agg_PAPI_values_probejoin[NUM_PAPI_EVENT];
long long agg_PAPI_values_aggregate[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
#include "../pmw/mypmw.h"
extern unsigned int NVMCount;
MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output[MaxNVMNum];
MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_pushdown[MaxNVMNum];
MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_buildpart[MaxNVMNum];
MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_probejoin[MaxNVMNum];
MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_aggregate[MaxNVMNum];
MY_PMWATCH_OP_BUF_NODE PMWatch_output[MaxNVMNum];
MY_PMWATCH_OP_BUF_NODE PMWatch_output_pushdown[MaxNVMNum];
MY_PMWATCH_OP_BUF_NODE PMWatch_output_buildpart[MaxNVMNum];
MY_PMWATCH_OP_BUF_NODE PMWatch_output_probejoin[MaxNVMNum];
MY_PMWATCH_OP_BUF_NODE PMWatch_output_aggregate[MaxNVMNum];
#endif /* USE_PMWATCH */


extern int sf;

extern size_t part_num;
extern size_t lineitem_num;

extern size_t fltrq_part_num;
extern size_t fltrq_lineitem_num;

extern size_t every_thread_part_num[PROBEJOIN_THREAD_NUM];
extern size_t every_thread_lineitem_num[PROBEJOIN_THREAD_NUM];
extern fltrq_part_t *part_start_addr;
extern fltrq_lineitem_t *lineitem_start_addr;
extern size_t initial_part_tuple_num_thr;
extern size_t initial_lineitem_tuple_num_thr;
extern size_t **global_part_output_offset;
extern size_t **global_lineitem_output_offset;
#ifdef USE_SWWCB_OPTIMIZED_PART
extern tpch_fltrq_part_swwcb_t **global_tpch_fltrq_part_swwcb_s;
extern tpch_fltrq_lineitem_swwcb_t **global_tpch_fltrq_lineitem_swwcb_s;
#endif /* USE_SWWCB_OPTIMIZED_PART */


int cur_t = 0;


typedef struct algo_t {
	char name[128];
	void *(*buildpart_algo)(void*);
	void *(*probejoin_algo)(void*);
} algo_t;


algo_t algos [] = {
	{"tpch_nphj_sc", tpch_run_nphj_sc_build, tpch_run_nphj_sc_probe},
	{"tpch_rdx_bc", tpch_run_rdx_part, tpch_run_rdx_bc_join},
	{"tpch_rdx_hr", tpch_run_rdx_part, tpch_run_rdx_hr_join},
	{"tpch_rdx_bw_reg_bc", tpch_run_rdx_part_bw_reg, tpch_run_rdx_bc_join},
	{"tpch_rdx_bw_reg_hr", tpch_run_rdx_part_bw_reg, tpch_run_rdx_hr_join}
};


size_t selection_filter_part(fltrq_part_t *dst, part_t *src, size_t t_num) {
	size_t idx = 0, jdx = 0;
	for (; idx < t_num; idx ++) {
		if ( part_filter(src+idx) ) {
			dst[jdx].p_partkey = src[idx].p_partkey;
#if QUERY == Q14
			memcpy(dst[jdx].p_type, src[idx].p_type, 25);
#elif QUERY == Q19
			memcpy(dst[jdx].p_brand, src[idx].p_brand, 10);
			dst[jdx].p_size = src[idx].p_size;
			memcpy(dst[jdx].p_container, src[idx].p_container, 10);
#endif /* QUERY == Q14 */
			jdx ++;
		}
	}
	return jdx;
}


size_t selection_filter_lineitem(fltrq_lineitem_t *dst, lineitem_t *src, size_t t_num) {
	size_t idx = 0, jdx = 0;
	for (; idx < t_num; idx ++) {
		if ( lineitem_filter(src+idx) ) {
			dst[jdx].l_partkey = src[idx].l_partkey;
#if QUERY == Q14
			dst[jdx].l_extendedprice = src[idx].l_extendedprice;
			dst[jdx].l_discount = src[idx].l_discount;
			memcpy(dst[jdx].l_shipdate, src[idx].l_shipdate, 10);
#elif QUERY == Q19
			dst[jdx].l_extendedprice = src[idx].l_extendedprice;
			dst[jdx].l_discount = src[idx].l_discount;
			dst[jdx].l_quantity = src[idx].l_quantity;
			memcpy(dst[jdx].l_shipinstruct, src[idx].l_shipinstruct, 25);
			memcpy(dst[jdx].l_shipmode, src[idx].l_shipmode, 10);
#endif /* QUERY == Q14 */
			jdx ++;
		}
	}
	return jdx;
}


#ifdef USE_SWWCB_OPTIMIZED_PART
size_t selection_filter_part_optimized(fltrq_part_t *dst, part_t *src, size_t t_num) {
	size_t idx = 0, jdx = 0;
	fltrq_part_t swwcb[FLTRQ_PART_PER_SWWCB] __attribute__( ( aligned( SWWCB_SIZE ) ) );
	int slot_mod;

	for (; idx < t_num; idx ++) {
		if ( part_filter(src+idx) ) {
			slot_mod = jdx & (FLTRQ_PART_PER_SWWCB - 1);
			swwcb[slot_mod].p_partkey = src[idx].p_partkey;
#if QUERY == Q14
			memcpy(swwcb[slot_mod].p_type, src[idx].p_type, 25);
#elif QUERY == Q19
			memcpy(swwcb[slot_mod].p_brand, src[idx].p_brand, 10);
			swwcb[slot_mod].p_size = src[idx].p_size;
			memcpy(swwcb[slot_mod].p_container, src[idx].p_container, 10);
#endif /* QUERY == Q14 */
			if (slot_mod == FLTRQ_PART_PER_SWWCB - 1) {
				/* non-temporal store a SWWCB */
				nontemp_store_swwcb(
					& dst[jdx-(FLTRQ_PART_PER_SWWCB-1)],
					swwcb
				);
			}
			jdx ++;
		}
	}

	slot_mod = jdx & (FLTRQ_PART_PER_SWWCB - 1);
	for (size_t kdx = 0; kdx < slot_mod; kdx ++) {
		dst[jdx-slot_mod+kdx].p_partkey = swwcb[kdx].p_partkey;
#if QUERY == Q14
		memcpy(dst[jdx-slot_mod+kdx].p_type, swwcb[kdx].p_type, 25);
#elif QUERY == Q19
		memcpy(dst[jdx-slot_mod+kdx].p_brand, swwcb[kdx].p_brand, 10);
		dst[jdx-slot_mod+kdx].p_size = swwcb[kdx].p_size;
		memcpy(dst[jdx-slot_mod+kdx].p_container, swwcb[kdx].p_container, 10);
#endif /* QUERY == Q14 */
	}

	return jdx;
}


size_t selection_filter_lineitem_optimized(fltrq_lineitem_t *dst, lineitem_t *src, size_t t_num) {
	size_t idx = 0, jdx = 0;
	fltrq_lineitem_t swwcb[FLTRQ_LINEITEM_PER_SWWCB] __attribute__( ( aligned( SWWCB_SIZE ) ) );
	int slot_mod;

	for (; idx < t_num; idx ++) {
		if ( lineitem_filter(src+idx) ) {
			slot_mod = jdx & (FLTRQ_LINEITEM_PER_SWWCB - 1);
			swwcb[slot_mod].l_partkey = src[idx].l_partkey;
#if QUERY == Q14
			swwcb[slot_mod].l_extendedprice = src[idx].l_extendedprice;
			swwcb[slot_mod].l_discount = src[idx].l_discount;
			memcpy(swwcb[slot_mod].l_shipdate, src[idx].l_shipdate, 10);
#elif QUERY == Q19
			swwcb[slot_mod].l_extendedprice = src[idx].l_extendedprice;
			swwcb[slot_mod].l_discount = src[idx].l_discount;
			swwcb[slot_mod].l_quantity = src[idx].l_quantity;
			memcpy(swwcb[slot_mod].l_shipinstruct, src[idx].l_shipinstruct, 25);
			memcpy(swwcb[slot_mod].l_shipmode, src[idx].l_shipmode, 10);
#endif /* QUERY == Q14 */
			if (slot_mod == FLTRQ_LINEITEM_PER_SWWCB - 1) {
				/* non-temporal store a SWWCB */
				nontemp_store_swwcb(
					& dst[jdx-(FLTRQ_LINEITEM_PER_SWWCB-1)],
					swwcb
				);
			}
			jdx ++;
		}
	}

	slot_mod = jdx & (FLTRQ_LINEITEM_PER_SWWCB - 1);
	for (size_t kdx = 0; kdx < slot_mod; kdx ++) {
		dst[jdx-slot_mod+kdx].l_partkey = swwcb[kdx].l_partkey;
#if QUERY == Q14
		dst[jdx-slot_mod+kdx].l_extendedprice = swwcb[kdx].l_extendedprice;
		dst[jdx-slot_mod+kdx].l_discount = swwcb[kdx].l_discount;
		memcpy(dst[jdx-slot_mod+kdx].l_shipdate, swwcb[kdx].l_shipdate, 10);
#elif QUERY == Q19
		dst[jdx-slot_mod+kdx].l_extendedprice = swwcb[kdx].l_extendedprice;
		dst[jdx-slot_mod+kdx].l_discount = swwcb[kdx].l_discount;
		dst[jdx-slot_mod+kdx].l_quantity = swwcb[kdx].l_quantity;
		memcpy(dst[jdx-slot_mod+kdx].l_shipinstruct, swwcb[kdx].l_shipinstruct, 25);
		memcpy(dst[jdx-slot_mod+kdx].l_shipmode, swwcb[kdx].l_shipmode, 10);
#endif /* QUERY == Q14 */
	}

	return jdx;
}
#endif /* USE_SWWCB_OPTIMIZED_PART */


void *tpch_pushdown(void *arg) {
	tpch_arg_t *tpch_arg = (tpch_arg_t *) arg;
#ifdef WARMUP
	tpch_warmup_localize(tpch_arg->tpch_query_meta->use_part, 
		tpch_arg->part_tmp_addr, tpch_arg->self_part_num, PART_T_SIZE
	);

	tpch_warmup_localize(tpch_arg->tpch_query_meta->use_lineitem, 
		tpch_arg->lineitem_tmp_addr, tpch_arg->self_lineitem_num, LINEITEM_T_SIZE
	);
#endif /* WARMUP */

	if (tpch_arg->tpch_query_meta->use_part) {
		memset_localize(tpch_arg->part_org_tup, FLTRQ_PART_T_SIZE * (tpch_arg->self_part_num));
	}
	if (tpch_arg->tpch_query_meta->use_lineitem) {
		memset_localize(tpch_arg->lineitem_org_tup, FLTRQ_LINEITEM_T_SIZE * (tpch_arg->self_part_num));
	}

	int rv;

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
	if (tpch_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */


	BARRIER_ARRIVE(tpch_arg->barrier, rv);

	/**************** PARTITION PHASE ****************/
	if (tpch_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &tpch_arg->pushdown_start);
	}

#ifdef USE_SWWCB_OPTIMIZED_PART
	tpch_arg->self_part_num = selection_filter_part_optimized(tpch_arg->part_org_tup, tpch_arg->part_tmp_addr, tpch_arg->self_part_num);
	tpch_arg->self_lineitem_num = selection_filter_lineitem_optimized(tpch_arg->lineitem_org_tup, tpch_arg->lineitem_tmp_addr, tpch_arg->self_lineitem_num);
#else /* USE_SWWCB_OPTIMIZED_PART */
	tpch_arg->self_part_num = selection_filter_part(tpch_arg->part_org_tup, tpch_arg->part_tmp_addr, tpch_arg->self_part_num);
	tpch_arg->self_lineitem_num = selection_filter_lineitem(tpch_arg->lineitem_org_tup, tpch_arg->lineitem_tmp_addr, tpch_arg->self_lineitem_num);
#endif /* USE_SWWCB_OPTIMIZED_PART */

	/* wait for the first thread finishs counting the maximum size of hashtable */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_pushdown);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_pushdown);
#endif /* SYNCSTATS */

	if (tpch_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &tpch_arg->pushdown_end);
	}

#ifdef USE_PMWATCH
	if (tpch_arg->_tid == 0) {
		pmwEnd(PMWatch_output_pushdown);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, tpch_arg->PAPI_values_pushdown);
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

	return NULL;
}


void tpch_spj(tpch_query_meta_t tpch_query_meta, tpch_timekeeper_t* const tpch_timekeeper, 
	algo_t *algo) {
	int rv;

	size_t matched_cnt = 0;

#if QUERY == Q14
	double numerator = 0.0;
	double denominator = 0.0;
#elif QUERY == Q19
	double revenue = 0.0;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

	char part_path[CHAR_BUFFER_LEN], lineitem_path[CHAR_BUFFER_LEN];

	snprintf(part_path, CHAR_BUFFER_LEN, "%s/sf%d/part.bin", TPCH_DATA_PREFIX_PATH, sf);
	snprintf(lineitem_path, CHAR_BUFFER_LEN, "%s/sf%d/lineitem.bin", TPCH_DATA_PREFIX_PATH, sf);

	part_t *part_org_addr = NULL, *part_tmp_addr = NULL;
	lineitem_t *lineitem_org_addr = NULL, *lineitem_tmp_addr = NULL;

	fltrq_hashtable_t fltrq_lineitem_hashtable;

	fltrq_part_t *part_org_tup = NULL, *part_tmp_tup = NULL;
	fltrq_lineitem_t *lineitem_org_tup = NULL, *lineitem_tmp_tup = NULL;

	size_t **part_hist = NULL;
	size_t **lineitem_hist = NULL;

	tpch_task_queue_t *part_queue = NULL;
	tpch_task_queue_t *join_queue = NULL;
	
	size_t intermediate_size = 0;
	void *intermediate = NULL;

	// fltrq_part_num = tpch_query_meta.ff_part * part_num;
	// fltrq_lineitem_num = tpch_query_meta.ff_lineitem * lineitem_num;

	tpch_org_map(tpch_query_meta.use_part, part_path, part_org_addr, (void **) &part_tmp_addr, 
		(void **) &part_org_tup, part_num, PART_T_SIZE, FLTRQ_PART_T_SIZE);
	tpch_org_map(tpch_query_meta.use_lineitem, lineitem_path, lineitem_org_addr, (void **) &lineitem_tmp_addr, 
		(void **) &lineitem_org_tup, lineitem_num, LINEITEM_T_SIZE, FLTRQ_LINEITEM_T_SIZE);

	part_start_addr = part_org_tup;
	lineitem_start_addr = lineitem_org_tup;

	int max_thread_num = MAX(BUILDPART_THREAD_NUM, PROBEJOIN_THREAD_NUM);

	size_t max_build_side_tup_num[max_thread_num];
	memset(max_build_side_tup_num, 0, sizeof(size_t)*max_thread_num);

	pthread_t threadpool[max_thread_num];
	tpch_arg_t args[max_thread_num];

	pthread_barrier_t pushdown_barrier;
	rv = pthread_barrier_init(&pushdown_barrier, NULL, PROBEJOIN_THREAD_NUM);
	if(rv){
		printf("[ERROR] Couldn't create the buildpart_barrier\n");
		exit(EXIT_FAILURE);
	}
	pthread_barrier_t buildpart_barrier;
	rv = pthread_barrier_init(&buildpart_barrier, NULL, BUILDPART_THREAD_NUM);
	if(rv){
		printf("[ERROR] Couldn't create the buildpart_barrier\n");
		exit(EXIT_FAILURE);
	}
	pthread_barrier_t probejoin_barrier;
	rv = pthread_barrier_init(&probejoin_barrier, NULL, PROBEJOIN_THREAD_NUM);
	if(rv){
		printf("[ERROR] Couldn't create the probejoin_barrier\n");
		exit(EXIT_FAILURE);
	}

	cpu_set_t set;
	int cpu_idx;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

#ifdef USE_PAPI
	long long PAPI_values[NUM_PAPI_EVENT];
	long long PAPI_values_pushdown[NUM_PAPI_EVENT];
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
	long long PAPI_values_aggregate[NUM_PAPI_EVENT];
	memset(PAPI_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_pushdown, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_aggregate, 0, sizeof(long long) * NUM_PAPI_EVENT );

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

#ifdef SYNCSTATS
	tpch_synctimer_t *localtimer = (tpch_synctimer_t *) malloc( max_thread_num * sizeof(tpch_synctimer_t) );
	tpch_synctimer_t *globaltimer = (tpch_synctimer_t *) malloc( sizeof(tpch_synctimer_t) );
	memset(localtimer, 0, max_thread_num * sizeof(tpch_synctimer_t));
	memset(globaltimer, 0, sizeof(tpch_synctimer_t));
#endif /* SYNCSTATS */

	/**
	 * For the ease of programming, use BULDPART_THREAD_NUM.
	 * Actually this phase (selection pushdown) is read-intensive,
	 * it's recommended to use all available physical cores.
	 */ 
	size_t part_tuple_num_thr = part_num / PROBEJOIN_THREAD_NUM;
	size_t lineitem_tuple_num_thr = lineitem_num / PROBEJOIN_THREAD_NUM;
	initial_part_tuple_num_thr = part_tuple_num_thr;
	initial_lineitem_tuple_num_thr = lineitem_tuple_num_thr;

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &pushdown_barrier;
		(args+idx)->tpch_query_meta = &tpch_query_meta;

		(args+idx)->self_part_num = (idx == PROBEJOIN_THREAD_NUM - 1) ? 
			part_num - idx * part_tuple_num_thr :
			part_tuple_num_thr;
		(args+idx)->part_tmp_addr = part_tmp_addr + part_tuple_num_thr * idx;
		(args+idx)->part_org_tup = part_org_tup + part_tuple_num_thr * idx;

		(args+idx)->self_lineitem_num = (idx == PROBEJOIN_THREAD_NUM - 1) ? 
			lineitem_num - idx * lineitem_tuple_num_thr :
			lineitem_tuple_num_thr;
		(args+idx)->lineitem_tmp_addr = lineitem_tmp_addr + lineitem_tuple_num_thr * idx;
		(args+idx)->lineitem_org_tup = lineitem_org_tup + lineitem_tuple_num_thr * idx;

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_pushdown, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, tpch_pushdown, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	fltrq_part_num = 0;
	fltrq_lineitem_num = 0;
	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
		fltrq_part_num += (args+idx)->self_part_num;
		fltrq_lineitem_num += (args+idx)->self_lineitem_num;
		every_thread_part_num[idx] = (args+idx)->self_part_num;
		every_thread_lineitem_num[idx] = (args+idx)->self_lineitem_num;
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_pushdown[jdx] += (args+idx)->PAPI_values_pushdown[jdx];
		}
#endif /* USE_PAPI */
	}

	if (strcmp("tpch_nphj_sc", algo->name) == 0) {
		tpch_alloc_hash_bucket(tpch_query_meta.use_lineitem, fltrq_lineitem_num,
			sizeof(fltrq_lineitem_bucket_t), &fltrq_lineitem_hashtable);
	} else {
		tpch_init_part_mem(tpch_query_meta.use_part, part_org_tup, 
			(void **) &part_tmp_tup, fltrq_part_num, FLTRQ_PART_T_SIZE
		);
		tpch_init_part_mem(tpch_query_meta.use_lineitem, lineitem_org_tup, 
			(void **) &lineitem_tmp_tup, fltrq_lineitem_num, FLTRQ_LINEITEM_T_SIZE
		);

		part_hist = (size_t **) alloc_memory( sizeof(size_t *) * BUILDPART_THREAD_NUM, CACHELINE_SIZE );
		lineitem_hist = (size_t **) alloc_memory( sizeof(size_t *) * BUILDPART_THREAD_NUM, CACHELINE_SIZE );

		part_queue = tpch_task_queue_init(FANOUT_PASS1);
		join_queue = tpch_task_queue_init(1 << NUM_RADIX_BITS);

		if (strncmp("tpch_rdx_bw_reg", algo->name, 15) == 0) {
			global_part_output_offset = (size_t **) alloc_memory( sizeof(size_t *) * BUILDPART_THREAD_NUM, CACHELINE_SIZE );
			global_lineitem_output_offset = (size_t **) alloc_memory( sizeof(size_t *) * BUILDPART_THREAD_NUM, CACHELINE_SIZE );
#ifdef USE_SWWCB_OPTIMIZED_PART
			global_tpch_fltrq_part_swwcb_s = (tpch_fltrq_part_swwcb_t **) alloc_memory( sizeof(tpch_fltrq_part_swwcb_t *) * BUILDPART_THREAD_NUM, CACHELINE_SIZE );
			global_tpch_fltrq_lineitem_swwcb_s = (tpch_fltrq_lineitem_swwcb_t **) alloc_memory( sizeof(tpch_fltrq_lineitem_swwcb_t *) * BUILDPART_THREAD_NUM, CACHELINE_SIZE );
#endif /* USE_SWWCB_OPTIMIZED_PART */
		}
	}

	tpch_unmap(tpch_query_meta.use_part, part_tmp_addr, 
		part_num * PART_T_SIZE + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	tpch_unmap(tpch_query_meta.use_lineitem, lineitem_tmp_addr, 
		lineitem_num * LINEITEM_T_SIZE + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));

	// part_tuple_num_thr = part_num / BUILDPART_THREAD_NUM;

	// if (strcmp("tpch_nphj_sc", algo->name) == 0) {
	// 	lineitem_tuple_num_thr = lineitem_num / PROBEJOIN_THREAD_NUM;
	// } else {
	// 	lineitem_tuple_num_thr = lineitem_num / BUILDPART_THREAD_NUM;
	// }

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &buildpart_barrier;

		(args+idx)->tpch_query_meta = &tpch_query_meta;

		// (args+idx)->self_part_num = (idx == BUILDPART_THREAD_NUM - 1) ? 
		//  part_num - idx * part_tuple_num_thr :
		//  part_tuple_num_thr;
		// (args+idx)->part_org_tup = part_org_tup + part_tuple_num_thr * idx;

		if (strcmp("tpch_nphj_sc", algo->name) == 0) {
			(args+idx)->fltrq_lineitem_hashtable = &fltrq_lineitem_hashtable;
		} else {
			// (args+idx)->self_lineitem_num = (idx == BUILDPART_THREAD_NUM - 1) ? 
			// 	lineitem_num - idx * lineitem_tuple_num_thr :
			// 	lineitem_tuple_num_thr;
			// (args+idx)->lineitem_org_tup = lineitem_org_tup + lineitem_tuple_num_thr * idx;

			(args+idx)->part_tmp_tup = part_tmp_tup;
			(args+idx)->lineitem_tmp_tup = lineitem_tmp_tup;

			(args+idx)->part_hist = part_hist;
			(args+idx)->lineitem_hist = lineitem_hist;

			(args+idx)->max_build_side_tup_num = max_build_side_tup_num;

			(args+idx)->part_queue = part_queue;
			(args+idx)->join_queue = join_queue;
		}

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, algo->buildpart_algo, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_buildpart[jdx] += (args+idx)->PAPI_values_buildpart[jdx];
		}
#endif /* USE_PAPI */
	}

#ifdef PHJ_MBP
	double avg_memset_time = 0.0;
	double avg_build_time = 0.0;
	double avg_probe_time = 0.0;
#endif /* PHJ_MBP */

	if (strcmp("tpch_nphj_sc", algo->name)!=0) {
		/* reset the join_queue */
#if NUM_PASSES == 1
		tpch_task_queue_t *swap = join_queue;
		join_queue = part_queue;
		part_queue = swap;
#endif /* NUM_PASSES == 1 */

		intermediate_size = TPCH_KEY_T__SIZE*next_pow_2(max_build_side_tup_num[0])*INTERMEDIATE_SCALING_FACTOR;
		intermediate = alloc_memory(intermediate_size*PROBEJOIN_THREAD_NUM, CACHELINE_SIZE);
	}

#ifndef TEST_PARTITIONING
	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &probejoin_barrier;

		(args+idx)->tpch_query_meta = &tpch_query_meta;

		if (strcmp("tpch_nphj_sc", algo->name) == 0) {
			// (args+idx)->self_lineitem_num = (idx == PROBEJOIN_THREAD_NUM - 1) ? 
			// 	lineitem_num - idx * lineitem_tuple_num_thr :
			// 	lineitem_tuple_num_thr;
			// (args+idx)->lineitem_org_tup = lineitem_org_tup + lineitem_tuple_num_thr * idx;

			(args+idx)->fltrq_lineitem_hashtable = &fltrq_lineitem_hashtable;
		} else {
			(args+idx)->parts_processed = 0;
			(args+idx)->max_build_side_tup_num = max_build_side_tup_num;
			(args+idx)->join_queue = join_queue;
			(args+idx)->intermediate_size = intermediate_size;
			(args+idx)->intermediate = intermediate;

#ifdef PHJ_MBP
			(args+idx)->memset_time = 0.0;
			(args+idx)->build_time = 0.0;
			(args+idx)->probe_time = 0.0;
#endif /* PHJ_MBP */
		}

		(args+idx)->matched_cnt = 0;

#if QUERY == Q14
		(args+idx)->numerator = 0.0;
		(args+idx)->denominator = 0.0;
#elif QUERY == Q19
		(args+idx)->revenue = 0.0;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, algo->probejoin_algo, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
		matched_cnt += (args+idx)->matched_cnt;
#if QUERY == Q14
		numerator += (args+idx)->numerator;
		denominator += (args+idx)->denominator;
#elif QUERY == Q19
		revenue += (args+idx)->revenue;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_probejoin[jdx] += (args + idx) -> PAPI_values_probejoin[jdx];
		}
#endif /* USE_PAPI */
	}
#endif /* TEST_PARTITIONING */


	purple();
	printf("matched_cnt: %zu\t", matched_cnt);
#if QUERY == Q14
	printf("promo_revenue: %.2f\tnumerator: %.2f\tdenominator: %.2f\n",
		(double) (100 * numerator / denominator), numerator, denominator
	);
#elif QUERY == Q19
	printf("revenue: %.2f\n", revenue);
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

#ifdef SYNCSTATS
	printf("[SYNCSTATS] TID\tpushdown\n");
	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%.9f\n", idx,
			diff_sec( (args+idx)->localtimer->sync_pushdown,
				(args+idx)->globaltimer->sync_pushdown )
		);
	}
	if (strcmp("tpch_nphj_sc", algo->name) == 0) {
		printf("[SYNCSTATS] TID\tDATE_TUPS\tPART_TUPS\tSUPPLIER_TUPS\tCUSTOMER_TUPS\tPRE_MEMSET\tBUILD\n");
		for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
			printf("[SYNCSTATS] %d\t%zu\t%.9f\t%.9f\n",
				idx, (args+idx)->self_part_num,
				diff_sec( (args+idx)->localtimer->sync_pre_memset,
					(args+idx)->globaltimer->sync_pre_memset ),
				diff_sec( (args+idx)->localtimer->sync_build,
					(args+idx)->globaltimer->sync_build )
			);
		}

		printf("[SYNCSTATS] TID\tLINEITEM_TUPS\tPROBE\n");
		for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
			printf("[SYNCSTATS] %d\t%zu\t%.9f\n",
				idx, (args+idx)->self_lineitem_num,
				diff_sec( (args+idx)->localtimer->sync_probe,
					(args+idx)->globaltimer->sync_probe )
			);
		}
	} else {
	printf("[SYNCSTATS] TID\tPREPART_MEMSET\t1ST_HIST_R\t1ST_PRFX_SUM_R\t");
#ifdef USE_SWWCB_OPTIMIZED_PART
	printf("1ST_SWWCB_R\t");
#endif /* USE_SWWCB_OPTIMIZED_PART */
	printf("1ST_HIST_S\t1ST_PRFX_SUM_S\t");
#ifdef USE_SWWCB_OPTIMIZED_PART
	printf("1ST_SWWCB_S\t");
#endif /* USE_SWWCB_OPTIMIZED_PART */
	printf("1ST_PASS\t1ST_PASS_QUEUE\tPART\tMAX_HASHTABLE\n");


	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%.9f\t%.9f\t%.9f\t", idx, 
			diff_sec( (args+idx)->localtimer->sync_prepart_memset,
				(args+idx)->globaltimer->sync_prepart_memset ),
			diff_sec( (args+idx)->localtimer->sync_1st_hist[0],
				(args+idx)->globaltimer->sync_1st_hist[0] ),
			diff_sec( (args+idx)->localtimer->sync_1st_prfx_sum[0],
				(args+idx)->globaltimer->sync_1st_prfx_sum[0] )
		);
#ifdef USE_SWWCB_OPTIMIZED_PART
		printf("%.9f\t", diff_sec( (args+idx)->localtimer->sync_1st_swwcb[0],
			(args+idx)->globaltimer->sync_1st_swwcb[0] )
		);
#endif /* USE_SWWCB_OPTIMIZED_PART */
		printf("%.9f\t%.9f\t", diff_sec( (args+idx)->localtimer->sync_1st_hist[1],
			(args+idx)->globaltimer->sync_1st_hist[1] ),
			diff_sec( (args+idx)->localtimer->sync_1st_prfx_sum[1],
				(args+idx)->globaltimer->sync_1st_prfx_sum[1] )
		);
#ifdef USE_SWWCB_OPTIMIZED_PART
		printf("%.9f\t", diff_sec( (args+idx)->localtimer->sync_1st_swwcb[1],
			(args+idx)->globaltimer->sync_1st_swwcb[1] )
		);
#endif /* USE_SWWCB_OPTIMIZED_PART */
		printf("%.9f\t%.9f\t%.9f\t%.9f\n", diff_sec( (args+idx)->localtimer->sync_1st_pass,
			(args+idx)->globaltimer->sync_1st_pass ),
			diff_sec( (args+idx)->localtimer->sync_1st_pass_queue,
				(args+idx)->globaltimer->sync_1st_pass_queue ),
			diff_sec( (args+idx)->localtimer->sync_part,
				(args+idx)->globaltimer->sync_part ),
			diff_sec( (args+idx)->localtimer->sync_max_hashtable,
				(args+idx)->globaltimer->sync_max_hashtable )
		);
	}


#ifdef PHJ_SYNCSTATS
		struct timespec *tmp_start;
		printf("[PHJ_SYNCSTATS] PREPART_MEMSET: %.9f\t1ST_HIST_R: %.9f\t",
			diff_sec( args[0].buildpart_start,
				args[0].globaltimer->sync_prepart_memset ),
			diff_sec( args[0].globaltimer->sync_prepart_memset,
				args[0].globaltimer->sync_1st_hist[0] )
		);
		tmp_start = & args[0].globaltimer->sync_1st_hist[0];
		if (strncmp("tpch_rdx_bw_reg", algo->name, 15) == 0) {
			printf("1ST_PRFX_SUM_R: %.9f\t",
				diff_sec( args[0].globaltimer->sync_1st_hist[0],
					args[0].globaltimer->sync_1st_prfx_sum[0] ) 
			);
			tmp_start = & args[0].globaltimer->sync_1st_prfx_sum[0];
		}

#ifdef USE_SWWCB_OPTIMIZED_PART
		printf("1ST_SWWCB_R: %.9f\t", 
			diff_sec( *tmp_start,
				args[0].globaltimer->sync_1st_swwcb[0] )
		);
		tmp_start = & args[0].globaltimer->sync_1st_swwcb[0];
#endif /* USE_SWWCB_OPTIMIZED_PART */
		printf("1ST_HIST_S: %.9f\t",
			diff_sec( *tmp_start,
				args[0].globaltimer->sync_1st_hist[1] )
		);
		tmp_start = & args[0].globaltimer->sync_1st_hist[1];
		if (strncmp("tpch_rdx_bw_reg", algo->name, 15) == 0) {
			printf("1ST_PRFX_SUM_S: %.9f\t",
				diff_sec( args[0].globaltimer->sync_1st_hist[1],
					args[0].globaltimer->sync_1st_prfx_sum[1] ) 
			);
			tmp_start = & args[0].globaltimer->sync_1st_prfx_sum[1];
		}
#ifdef USE_SWWCB_OPTIMIZED_PART
		printf("1ST_SWWCB_S: %.9f\t",
			diff_sec( *tmp_start,
				args[0].globaltimer->sync_1st_swwcb[1] )
		);
		tmp_start = & args[0].globaltimer->sync_1st_swwcb[1];
#endif /* USE_SWWCB_OPTIMIZED_PART */
		printf("1ST_PASS: %.9f\t1ST_PASS_QUEUE: %.9f\tPART: %.9f\n",
			diff_sec( *tmp_start,
				args[0].globaltimer->sync_1st_pass ),
			diff_sec( args[0].globaltimer->sync_1st_pass,
				args[0].globaltimer->sync_1st_pass_queue ),
			diff_sec( args[0].globaltimer->sync_1st_pass_queue,
				args[0].globaltimer->sync_part )
		);
#endif /* PHJ_SYNCSTATS */

#ifndef TEST_PARTITIONING
		printf("[SYNCSTATS] TID\tTASKS\tINTERMEDIATE\tJOIN\n");
		for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
			printf("[SYNCSTATS] %d\t%zu\t%.9f\t%.9f\n", idx, (args+idx)->parts_processed,
				diff_sec( (args+idx)->localtimer->sync_intermediate_memset,
					(args+idx)->globaltimer->sync_intermediate_memset ),
				diff_sec( (args+idx)->localtimer->sync_join,
					(args+idx)->globaltimer->sync_join )
			);
		}
#endif /* TEST_PARTITIONING */

	}
#endif /* SYNCSTATS */


#ifdef PHJ_MBP
	if (strcmp("tpch_nphj_sc", algo->name) == 0) {
		printf("[PHJ_MBP] avg_memset_time: %.9f\tavg_build_time: %.9f\tavg_probe_time: %.9f\n",
			avg_memset_time/PROBEJOIN_THREAD_NUM, avg_build_time/PROBEJOIN_THREAD_NUM, avg_probe_time/PROBEJOIN_THREAD_NUM
		);
	}
#endif /* PHJ_MBP */


#ifdef USE_PMWATCH
	for (int idx = 0; idx < NVMCount; idx ++) {
		PMWatch_output[idx].total_bytes_read = PMWatch_output_pushdown[idx].total_bytes_read + PMWatch_output_buildpart[idx].total_bytes_read + 
			PMWatch_output_probejoin[idx].total_bytes_read + PMWatch_output_aggregate[idx].total_bytes_read;
		PMWatch_output[idx].total_bytes_written = PMWatch_output_pushdown[idx].total_bytes_written + PMWatch_output_buildpart[idx].total_bytes_written + 
			PMWatch_output_probejoin[idx].total_bytes_written + PMWatch_output_aggregate[idx].total_bytes_written;
		PMWatch_output[idx].media_read_ops = PMWatch_output_pushdown[idx].media_read_ops + PMWatch_output_buildpart[idx].media_read_ops + 
			PMWatch_output_probejoin[idx].media_read_ops + PMWatch_output_aggregate[idx].media_read_ops;
		PMWatch_output[idx].media_write_ops = PMWatch_output_pushdown[idx].media_write_ops + PMWatch_output_buildpart[idx].media_write_ops +
			PMWatch_output_probejoin[idx].media_write_ops + PMWatch_output_aggregate[idx].media_write_ops;
		PMWatch_output[idx].read_64B_ops_received = PMWatch_output_pushdown[idx].read_64B_ops_received + PMWatch_output_buildpart[idx].read_64B_ops_received + 
			PMWatch_output_probejoin[idx].read_64B_ops_received + PMWatch_output_aggregate[idx].read_64B_ops_received;
		PMWatch_output[idx].write_64B_ops_received = PMWatch_output_pushdown[idx].write_64B_ops_received + PMWatch_output_buildpart[idx].write_64B_ops_received + 
			PMWatch_output_probejoin[idx].write_64B_ops_received + PMWatch_output_aggregate[idx].write_64B_ops_received;
		PMWatch_output[idx].host_reads = PMWatch_output_pushdown[idx].host_reads + PMWatch_output_buildpart[idx].host_reads + 
			PMWatch_output_probejoin[idx].host_reads + PMWatch_output_aggregate[idx].host_reads;
		PMWatch_output[idx].host_writes = PMWatch_output_pushdown[idx].host_writes + PMWatch_output_buildpart[idx].host_writes +
			PMWatch_output_probejoin[idx].host_writes + PMWatch_output_aggregate[idx].host_writes;
		PMWatch_output[idx].read_hit_ratio = (PMWatch_output_pushdown[idx].read_hit_ratio + PMWatch_output_buildpart[idx].read_hit_ratio + 
			PMWatch_output_probejoin[idx].read_hit_ratio + PMWatch_output_aggregate[idx].read_hit_ratio) / 4;
		PMWatch_output[idx].write_hit_ratio = (PMWatch_output_pushdown[idx].write_hit_ratio + PMWatch_output_buildpart[idx].write_hit_ratio + 
			PMWatch_output_probejoin[idx].write_hit_ratio + PMWatch_output_aggregate[idx].write_hit_ratio) / 4;
		PMWatch_output[idx].read_amplification = (PMWatch_output_pushdown[idx].read_amplification + PMWatch_output_buildpart[idx].read_amplification + 
			PMWatch_output_probejoin[idx].read_amplification + PMWatch_output_aggregate[idx].read_amplification) / 4;
		PMWatch_output[idx].write_amplification = (PMWatch_output_pushdown[idx].write_amplification + PMWatch_output_buildpart[idx].write_amplification +
			PMWatch_output_probejoin[idx].write_amplification + PMWatch_output_aggregate[idx].write_amplification) / 4;
		printf("[PMWatch] DIMM: %d\n", idx);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_read: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].total_bytes_read, PMWatch_output_pushdown[idx].total_bytes_read, PMWatch_output_buildpart[idx].total_bytes_read, 
			PMWatch_output_probejoin[idx].total_bytes_read, PMWatch_output_aggregate[idx].total_bytes_read
		);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_written: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].total_bytes_written, PMWatch_output_pushdown[idx].total_bytes_written, PMWatch_output_buildpart[idx].total_bytes_written, 
			PMWatch_output_probejoin[idx].total_bytes_written, PMWatch_output_aggregate[idx].total_bytes_written
		);
		printf("[PMWatch] DIMM: %d\tmedia_read_ops: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].media_read_ops, PMWatch_output_pushdown[idx].media_read_ops, PMWatch_output_buildpart[idx].media_read_ops, 
			PMWatch_output_probejoin[idx].media_read_ops, PMWatch_output_aggregate[idx].media_read_ops
		);
		printf("[PMWatch] DIMM: %d\tmedia_write_ops: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].media_write_ops, PMWatch_output_pushdown[idx].media_write_ops, PMWatch_output_buildpart[idx].media_write_ops, 
			PMWatch_output_probejoin[idx].media_write_ops, PMWatch_output_aggregate[idx].media_write_ops
		);
		printf("[PMWatch] DIMM: %d\tread_64B_ops_received: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].read_64B_ops_received, PMWatch_output_pushdown[idx].read_64B_ops_received, PMWatch_output_buildpart[idx].read_64B_ops_received,
			PMWatch_output_probejoin[idx].read_64B_ops_received, PMWatch_output_aggregate[idx].read_64B_ops_received
		);
		printf("[PMWatch] DIMM: %d\twrite_64B_ops_received: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].write_64B_ops_received, PMWatch_output_pushdown[idx].write_64B_ops_received, PMWatch_output_buildpart[idx].write_64B_ops_received,
			PMWatch_output_probejoin[idx].write_64B_ops_received, PMWatch_output_aggregate[idx].write_64B_ops_received
		);
		printf("[PMWatch] DIMM: %d\thost_reads: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].host_reads, PMWatch_output_pushdown[idx].host_reads, PMWatch_output_buildpart[idx].host_reads, 
			PMWatch_output_probejoin[idx].host_reads, PMWatch_output_aggregate[idx].host_reads
		);
		printf("[PMWatch] DIMM: %d\thost_writes: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].host_writes, PMWatch_output_pushdown[idx].host_writes, PMWatch_output_buildpart[idx].host_writes, 
			PMWatch_output_probejoin[idx].host_writes, PMWatch_output_aggregate[idx].host_writes
		);
		printf("[PMWatch] DIMM: %d\tread_hit_ratio: %f\t%f\t%f\t%f\t%f\n", idx, 
			PMWatch_output[idx].read_hit_ratio, PMWatch_output_pushdown[idx].read_hit_ratio, PMWatch_output_buildpart[idx].read_hit_ratio, 
			PMWatch_output_probejoin[idx].read_hit_ratio, PMWatch_output_aggregate[idx].read_hit_ratio
		);
		printf("[PMWatch] DIMM: %d\twrite_hit_ratio: %f\t%f\t%f\t%f\t%f\n", idx, 
			PMWatch_output[idx].write_hit_ratio, PMWatch_output_pushdown[idx].write_hit_ratio, PMWatch_output_buildpart[idx].write_hit_ratio, 
			PMWatch_output_probejoin[idx].write_hit_ratio, PMWatch_output_aggregate[idx].write_hit_ratio
		);
		printf("[PMWatch] DIMM: %d\tread_amplification: %.9f\t%.9f\t%.9f\t %.9f\t %.9f\n", idx, 
			PMWatch_output[idx].read_amplification, PMWatch_output_pushdown[idx].read_amplification, PMWatch_output_buildpart[idx].read_amplification, 
			PMWatch_output_probejoin[idx].read_amplification, PMWatch_output_aggregate[idx].read_amplification
		);
		printf("[PMWatch] DIMM: %d\twrite_amplification: %.9f\t%.9f\t%.9f\t %.9f\t %.9f\n", idx, 
			PMWatch_output[idx].write_amplification, PMWatch_output_pushdown[idx].write_amplification, PMWatch_output_buildpart[idx].write_amplification, 
			PMWatch_output_probejoin[idx].write_amplification, PMWatch_output_aggregate[idx].write_amplification
		);
		printf("\n");
		if (cur_t != 0) {
			agg_PMWatch_output[idx].total_bytes_read += PMWatch_output[idx].total_bytes_read;
			agg_PMWatch_output[idx].total_bytes_written += PMWatch_output[idx].total_bytes_written;
			agg_PMWatch_output[idx].media_read_ops += PMWatch_output[idx].media_read_ops;
			agg_PMWatch_output[idx].media_write_ops += PMWatch_output[idx].media_write_ops;
			agg_PMWatch_output[idx].read_64B_ops_received += PMWatch_output[idx].read_64B_ops_received;
			agg_PMWatch_output[idx].write_64B_ops_received += PMWatch_output[idx].write_64B_ops_received;
			agg_PMWatch_output[idx].host_reads += PMWatch_output[idx].host_reads;
			agg_PMWatch_output[idx].host_writes += PMWatch_output[idx].host_writes;
			agg_PMWatch_output[idx].read_hit_ratio += PMWatch_output[idx].read_hit_ratio;
			agg_PMWatch_output[idx].write_hit_ratio += PMWatch_output[idx].write_hit_ratio;
			agg_PMWatch_output[idx].read_amplification += PMWatch_output[idx].read_amplification;
			agg_PMWatch_output[idx].write_amplification += PMWatch_output[idx].write_amplification;

			agg_PMWatch_output_pushdown[idx].total_bytes_read += PMWatch_output_pushdown[idx].total_bytes_read;
			agg_PMWatch_output_pushdown[idx].total_bytes_written += PMWatch_output_pushdown[idx].total_bytes_written;
			agg_PMWatch_output_pushdown[idx].media_read_ops += PMWatch_output_pushdown[idx].media_read_ops;
			agg_PMWatch_output_pushdown[idx].media_write_ops += PMWatch_output_pushdown[idx].media_write_ops;
			agg_PMWatch_output_pushdown[idx].read_64B_ops_received += PMWatch_output_pushdown[idx].read_64B_ops_received;
			agg_PMWatch_output_pushdown[idx].write_64B_ops_received += PMWatch_output_pushdown[idx].write_64B_ops_received;
			agg_PMWatch_output_pushdown[idx].host_reads += PMWatch_output_pushdown[idx].host_reads;
			agg_PMWatch_output_pushdown[idx].host_writes += PMWatch_output_pushdown[idx].host_writes;
			agg_PMWatch_output_pushdown[idx].read_hit_ratio += PMWatch_output_pushdown[idx].read_hit_ratio;
			agg_PMWatch_output_pushdown[idx].write_hit_ratio += PMWatch_output_pushdown[idx].write_hit_ratio;
			agg_PMWatch_output_pushdown[idx].read_amplification += PMWatch_output_pushdown[idx].read_amplification;
			agg_PMWatch_output_pushdown[idx].write_amplification += PMWatch_output_pushdown[idx].write_amplification;

			agg_PMWatch_output_buildpart[idx].total_bytes_read += PMWatch_output_buildpart[idx].total_bytes_read;
			agg_PMWatch_output_buildpart[idx].total_bytes_written += PMWatch_output_buildpart[idx].total_bytes_written;
			agg_PMWatch_output_buildpart[idx].media_read_ops += PMWatch_output_buildpart[idx].media_read_ops;
			agg_PMWatch_output_buildpart[idx].media_write_ops += PMWatch_output_buildpart[idx].media_write_ops;
			agg_PMWatch_output_buildpart[idx].read_64B_ops_received += PMWatch_output_buildpart[idx].read_64B_ops_received;
			agg_PMWatch_output_buildpart[idx].write_64B_ops_received += PMWatch_output_buildpart[idx].write_64B_ops_received;
			agg_PMWatch_output_buildpart[idx].host_reads += PMWatch_output_buildpart[idx].host_reads;
			agg_PMWatch_output_buildpart[idx].host_writes += PMWatch_output_buildpart[idx].host_writes;
			agg_PMWatch_output_buildpart[idx].read_hit_ratio += PMWatch_output_buildpart[idx].read_hit_ratio;
			agg_PMWatch_output_buildpart[idx].write_hit_ratio += PMWatch_output_buildpart[idx].write_hit_ratio;
			agg_PMWatch_output_buildpart[idx].read_amplification += PMWatch_output_buildpart[idx].read_amplification;
			agg_PMWatch_output_buildpart[idx].write_amplification += PMWatch_output_buildpart[idx].write_amplification;

			agg_PMWatch_output_probejoin[idx].total_bytes_read += PMWatch_output_probejoin[idx].total_bytes_read;
			agg_PMWatch_output_probejoin[idx].total_bytes_written += PMWatch_output_probejoin[idx].total_bytes_written;
			agg_PMWatch_output_probejoin[idx].media_read_ops += PMWatch_output_probejoin[idx].media_read_ops;
			agg_PMWatch_output_probejoin[idx].media_write_ops += PMWatch_output_probejoin[idx].media_write_ops;
			agg_PMWatch_output_probejoin[idx].read_64B_ops_received += PMWatch_output_probejoin[idx].read_64B_ops_received;
			agg_PMWatch_output_probejoin[idx].write_64B_ops_received += PMWatch_output_probejoin[idx].write_64B_ops_received;
			agg_PMWatch_output_probejoin[idx].host_reads += PMWatch_output_probejoin[idx].host_reads;
			agg_PMWatch_output_probejoin[idx].host_writes += PMWatch_output_probejoin[idx].host_writes;
			agg_PMWatch_output_probejoin[idx].read_hit_ratio += PMWatch_output_probejoin[idx].read_hit_ratio;
			agg_PMWatch_output_probejoin[idx].write_hit_ratio += PMWatch_output_probejoin[idx].write_hit_ratio;
			agg_PMWatch_output_probejoin[idx].read_amplification += PMWatch_output_probejoin[idx].read_amplification;
			agg_PMWatch_output_probejoin[idx].write_amplification += PMWatch_output_probejoin[idx].write_amplification;

			agg_PMWatch_output_aggregate[idx].total_bytes_read += PMWatch_output_aggregate[idx].total_bytes_read;
			agg_PMWatch_output_aggregate[idx].total_bytes_written += PMWatch_output_aggregate[idx].total_bytes_written;
			agg_PMWatch_output_aggregate[idx].media_read_ops += PMWatch_output_aggregate[idx].media_read_ops;
			agg_PMWatch_output_aggregate[idx].media_write_ops += PMWatch_output_aggregate[idx].media_write_ops;
			agg_PMWatch_output_aggregate[idx].read_64B_ops_received += PMWatch_output_aggregate[idx].read_64B_ops_received;
			agg_PMWatch_output_aggregate[idx].write_64B_ops_received += PMWatch_output_aggregate[idx].write_64B_ops_received;
			agg_PMWatch_output_aggregate[idx].host_reads += PMWatch_output_aggregate[idx].host_reads;
			agg_PMWatch_output_aggregate[idx].host_writes += PMWatch_output_aggregate[idx].host_writes;
			agg_PMWatch_output_aggregate[idx].read_hit_ratio += PMWatch_output_aggregate[idx].read_hit_ratio;
			agg_PMWatch_output_aggregate[idx].write_hit_ratio += PMWatch_output_aggregate[idx].write_hit_ratio;
			agg_PMWatch_output_aggregate[idx].read_amplification += PMWatch_output_aggregate[idx].read_amplification;
			agg_PMWatch_output_aggregate[idx].write_amplification += PMWatch_output_aggregate[idx].write_amplification;
		}
	}
#endif /* USE_PMWATCH */
#ifdef USE_PAPI
	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		PAPI_values[idx] += PAPI_values_pushdown[idx] + PAPI_values_buildpart[idx] + PAPI_values_probejoin[idx] + PAPI_values_aggregate[idx];
		printf("[PAPI] PAPI_event_name: %s", PAPI_event_names[idx]);
		printf("\ttotal_counter: %lld", PAPI_values[idx]);
		printf("\tpushdown_counter: %lld", PAPI_values_pushdown[idx]);
		printf("\tbuildpart_counter: %lld", PAPI_values_buildpart[idx]);
		printf("\tprobejoin_counter: %lld", PAPI_values_probejoin[idx]);
		printf("\taggregate_counter: %lld", PAPI_values_aggregate[idx]);
		printf("\n");
		if (cur_t != 0) {
			agg_PAPI_values[idx] += PAPI_values[idx];
			agg_PAPI_values_pushdown[idx] += PAPI_values_pushdown[idx];
			agg_PAPI_values_buildpart[idx] += PAPI_values_buildpart[idx];
			agg_PAPI_values_probejoin[idx] += PAPI_values_probejoin[idx];
			agg_PAPI_values_aggregate[idx] += PAPI_values_aggregate[idx];
		}
	}
	PAPI_shutdown();
#endif /* USE_PAPI */

#ifdef SYNCSTATS
	free(localtimer);
	free(globaltimer);
#endif /* SYNCSTATS */

	pthread_attr_destroy(&attr);
	pthread_barrier_destroy(&buildpart_barrier);
	pthread_barrier_destroy(&probejoin_barrier);

	tpch_unmap(tpch_query_meta.use_part, part_org_tup, 
		fltrq_part_num * FLTRQ_PART_T_SIZE + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	tpch_unmap(tpch_query_meta.use_lineitem, lineitem_org_tup, 
		fltrq_lineitem_num * FLTRQ_LINEITEM_T_SIZE + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));

	if (strcmp("tpch_nphj_sc", algo->name) == 0) {
		tpch_dealloc_hash_bucket(tpch_query_meta.use_lineitem, &fltrq_lineitem_hashtable);
		for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
			free_fltrq_lineitem_bucket_buffer(
				tpch_query_meta.use_lineitem,
				(args+idx)->fltrq_lineitem_overflowbuf
			);
		}
		

	} else {
		tpch_unmap(tpch_query_meta.use_part, part_tmp_tup, 
			fltrq_part_num * FLTRQ_PART_T_SIZE + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
		tpch_unmap(tpch_query_meta.use_lineitem, lineitem_tmp_tup, 
			fltrq_lineitem_num * FLTRQ_LINEITEM_T_SIZE + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));

		dealloc_memory(intermediate, intermediate_size*PROBEJOIN_THREAD_NUM);
		tpch_task_queue_free(part_queue);
		tpch_task_queue_free(join_queue);

		dealloc_memory(part_hist, sizeof(size_t *) * BUILDPART_THREAD_NUM);
		dealloc_memory(lineitem_hist, sizeof(size_t *) * BUILDPART_THREAD_NUM);

		if (strncmp("tpch_rdx_bw_reg", algo->name, 15) == 0) {
			dealloc_memory(global_part_output_offset, sizeof(size_t *) * BUILDPART_THREAD_NUM);
			dealloc_memory(global_lineitem_output_offset, sizeof(size_t *) * BUILDPART_THREAD_NUM);
#ifdef USE_SWWCB_OPTIMIZED_PART
			dealloc_memory(global_tpch_fltrq_part_swwcb_s, sizeof(tpch_fltrq_part_swwcb_t *) * BUILDPART_THREAD_NUM);
			dealloc_memory(global_tpch_fltrq_lineitem_swwcb_s, sizeof(tpch_fltrq_lineitem_swwcb_t *) * BUILDPART_THREAD_NUM);
#endif /* USE_SWWCB_OPTIMIZED_PART */
		}
	}

	tpch_timekeeper->pushdown = diff_sec(args[0].pushdown_start, args[0].pushdown_end);
	tpch_timekeeper->buildpart = diff_sec(args[0].buildpart_start, args[0].buildpart_end);
	tpch_timekeeper->probejoin = diff_sec(args[0].probejoin_start, args[0].probejoin_end);
#if QUERY == Q14
#elif QUERY == Q19
#else /* QUERY == Q14 */
	// tpch_timekeeper->aggregate = diff_sec(args[0].aggregate_start, args[0].aggregate_end);
#endif /* QUERY == Q14 */
	tpch_timekeeper->total = tpch_timekeeper->pushdown + tpch_timekeeper->buildpart + \
		tpch_timekeeper->probejoin + tpch_timekeeper->aggregate;
}



int main(int argc, char** argv) {
	assert (argc == 3);

	algo_t *algo = &algos[0];
	sf = 1;

	int command_arg_char;
	int i = 0;
	int found = 0;
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"algo",			required_argument,	NULL,	0},
			{"sf",				required_argument,	NULL,	0},
			{NULL,				0,					NULL,	0}
		};
		command_arg_char = getopt_long(argc, argv, "-:", long_options, &option_index);
		if (command_arg_char == -1) {
			break;
		}
		switch (command_arg_char) {
			case 0:
					/**
					printf("long option %s", long_options[option_index].name);
					if (optarg) {
						printf(" with arg %s, index %d \n", optarg, option_index);
					}
					*/
				switch (option_index) {
					case 0:
						i = 0;
						found = 0;
						while (algos[i].buildpart_algo) {
							if (strcmp(optarg, algos[i].name) == 0) {
								algo = &algos[i];
								found = 1;
								break;
							}
							i ++;
						}
						if (found == 0) {
							printf("[ERROR] Join algorithm named `%s' does not exist!\n", optarg);
							exit(EXIT_FAILURE);
						}

						break;

					case 1:
						sf = atoi(optarg);
						break;
					default:
						perror("command line arguments parsing error\n");
						printf("unknow arg %s\tof index %d\nerrono: %d", optarg, option_index, errno);
						exit(EXIT_FAILURE);
						break;
				}
				break;
			case '?':
				printf("unknown option %c\n", optopt);
				break;
		}
	}

	if (strncmp("tpch_rdx_bw_reg", algo->name, 15) == 0) {
		assert ( BUILDPART_THREAD_NUM == PROBEJOIN_THREAD_NUM );
		assert ( BUILDPART_WRITE_THREAD_NUM <= BUILDPART_THREAD_NUM );
	}

	get_cardinality(sf);

	snprintf(work_dir, CHAR_BUFFER_LEN, "%s/q%d_sf%d_%s", WORK_PATH_PREFIX, QUERY, sf, current_timestamp());
	snprintf(dump_dir, CHAR_BUFFER_LEN, "%s/q%d_sf%d_%s", DUMP_PATH_PREFIX, QUERY, sf, current_timestamp());

	tpch_query_meta_t tpch_query_meta;
	tpch_query_meta_cfg(&tpch_query_meta);

	printf("CONFIGURATION:");
#ifdef CLFLUSH
	printf("\tCLFLUSH");
#endif /* CLFLUSH */
#ifdef CLFLUSHOPT
	printf("\tCLFLUSHOPT");
#endif /* CLFLUSHOPT */
#ifdef NT_STORE
	printf("\tNT_STORE");
#endif /* NT_STORE */
#ifdef ENABLE_FENCE
	printf("\tENABLE_FENCE");
#endif /* ENABLE_FENCE */
#ifdef WARMUP
	printf("\tWARMUP");
#endif /* WARMUP */
#ifdef USE_HUGE
	printf("\tUSE_HUGE");
#endif /* USE_HUGE */
#ifdef USE_ALIGNMENT
	printf("\tUSE_ALIGNMENT");
#endif /* USE_ALIGNMENT */
#ifdef USE_HYPERTHREADING
	printf("\tUSE_HYPERTHREADING");
#endif /* USE_HYPERTHREADING */
#ifdef SYNCSTATS
	printf("\tSYNCSTATS");
#endif /* SYNCSTATS */
#ifdef USE_NUMA
	printf("\tUSE_NUMA");
#endif /* USE_NUMA */
#ifdef TUPLE_T_SIZE
	printf("\tTUPLE_T_SIZE: %d", TUPLE_T_SIZE);
#endif /* TUPLE_T_SIZE */
#ifdef THREAD_NUM
	printf("\tTHREAD_NUM: %d", THREAD_NUM);
#endif /* THREAD_NUM */
#ifdef BUILDPART_THREAD_NUM
	printf("\tBUILDPART_THREAD_NUM: %d", BUILDPART_THREAD_NUM);
#endif /* BUILDPART_THREAD_NUM */
#ifdef BUILDPART_WRITE_THREAD_NUM
	printf("\tBUILDPART_WRITE_THREAD_NUM: %d", BUILDPART_WRITE_THREAD_NUM);
#endif /* BUILDPART_WRITE_THREAD_NUM */
#ifdef INTERMEDIATEMEMSET_THREAD_NUM
	printf("\tINTERMEDIATEMEMSET_THREAD_NUM: %d", INTERMEDIATEMEMSET_THREAD_NUM);
#endif /* INTERMEDIATEMEMSET_THREAD_NUM */
#ifdef PROBEJOIN_THREAD_NUM
	printf("\tPROBEJOIN_THREAD_NUM: %d", PROBEJOIN_THREAD_NUM);
#endif /* PROBEJOIN_THREAD_NUM */
#ifdef RETRIEVE_THREAD_NUM
	printf("\tRETRIEVE_THREAD_NUM: %d", RETRIEVE_THREAD_NUM);
#endif /* RETRIEVE_THREAD_NUM */
#ifdef XSOCKET_THREAD_NUM_DEFAULT
	printf("\tXSOCKET_THREAD_NUM_DEFAULT: %d", XSOCKET_THREAD_NUM_DEFAULT);
#endif /* XSOCKET_THREAD_NUM_DEFAULT */
#ifdef TEST_SELECTIVITY
	printf("\tTEST_SELECTIVITY";)
#endif /* TEST_SELECTIVITY */
#ifdef RUN_BILLION
	printf("\tRUN_BILLION");
#endif /* RUN_BILLION */
#ifdef RUN_FNDLY
	printf("\tRUN_FNDLY");
#endif /* RUN_FNDLY */
#ifdef RUN_MINI
	printf("\tRUN_MINI");
#endif /* RUN_MINI */
#ifdef RUN_PAYLOAD
	printf("\tRUN_PAYLOAD");
#endif /* RUN_PAYLOAD */
	// ptr
#ifdef NONTEMP_STORE_MATER
	printf("\tNONTEMP_STORE_MATER");
#endif /* NONTEMP_STORE_MATER */
#ifdef NONTEMP_STORE_MATER_ON_NVM
	printf("\tNONTEMP_STORE_MATER_ON_NVM");
#endif /* NONTEMP_STORE_MATER_ON_NVM */
	// NPHJ
#ifdef PREFETCH_DISTANCE
	printf("\tPREFETCH_DISTANCE: %d", PREFETCH_DISTANCE);
#endif /* PREFETCH_DISTANCE */
#ifdef PREFETCH_CACHE
	printf("\tPREFETCH_CACHE");
#endif /* PREFETCH_CACHE */
#ifdef PREFETCH_CACHE_NVM
	printf("\tPREFETCH_CACHE_NVM");
#endif /* PREFETCH_CACHE_NVM */
#ifdef BUCKET_CAPACITY
	printf("\tBUCKET_CAPACITY: %d", BUCKET_CAPACITY);
#endif /* BUCKET_CAPACITY */
#ifdef BUCKET_CACHELINE_ALIGNED
	printf("\tBUCKET_CACHELINE_ALIGNED");
#endif /* BUCKET_CACHELINE_ALIGNED */
#ifdef BUCKET_XPLINE_ALIGNED
	printf("\tBUCKET_XPLINE_ALIGNED");
#endif /* BUCKET_XPLINE_ALIGNED */
#ifdef OVERFLOW_BUF_SIZE
	printf("\tOVERFLOW_BUF_SIZE: %d", OVERFLOW_BUF_SIZE);
#endif /* OVERFLOW_BUF_SIZE */
#ifdef LOCK_IN_BUCKET
	printf("\tLOCK_IN_BUCKET");
#endif /* LOCK_IN_BUCKET */
	// PHJ
#ifdef BW_REG
	printf("\tBW_REG");
#endif /* BW_REG */
#ifdef NUM_RADIX_BITS
	printf("\tNUM_RADIX_BITS: %d", NUM_RADIX_BITS);
#endif /* NUM_RADIX_BITS */
#ifdef NUM_RADIX_BITS_PASS1
	printf("\tNUM_RADIX_BITS_PASS1: %d", NUM_RADIX_BITS_PASS1);
#endif /* NUM_RADIX_BITS_PASS1 */
#ifdef NUM_PASSES
	printf("\tNUM_PASSES: %d", NUM_PASSES);
#endif /* NUM_PASSES */
#ifdef SKEW_HANDLING
	printf("\tSKEW_HANDLING");
#endif /* SKEW_HANDLING */
#ifdef PHJ_SYNCSTATS
	printf("\tPHJ_SYNCSTATS");
#endif /* PHJ_SYNCSTATS */
#ifdef OUTPUT_OFFSET_NVM
	printf("\tOUTPUT_OFFSET_NVM");
#endif /* OUTPUT_OFFSET_NVM */
#ifdef SWWCB_SIZE
	printf("\tSWWCB_SIZE: %d", SWWCB_SIZE);
#endif /* SWWCB_SIZE */
#ifdef USE_SWWCB_OPTIMIZED_PART
	printf("\tUSE_SWWCB_OPTIMIZED_PART");
#endif /* USE_SWWCB_OPTIMIZED_PART */
#ifdef USE_SWWCB_OPTIMIZED_PART_ON_NVM
	printf("\tUSE_SWWCB_OPTIMIZED_PART_ON_NVM");
#endif /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
	printf("\tUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER");
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */
#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM
	printf("\tUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM");
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
#ifdef PHJ_MBP
	printf("\tPHJ_MBP");
#endif /* PHJ_MBP */
#ifdef INTERMEDIATE_SCALING_FACTOR
	printf("\tINTERMEDIATE_SCALING_FACTOR: %d", INTERMEDIATE_SCALING_FACTOR);
#endif /* INTERMEDIATE_SCALING_FACTOR */
#ifdef PHJ_SC_BUCKET_CAPACITY
	printf("\tPHJ_SC_BUCKET_CAPACITY: %d", PHJ_SC_BUCKET_CAPACITY);
#endif /* PHJ_SC_BUCKET_CAPACITY */
	// PHJ_SHR
#ifdef SHARE_LATCH_ON_NVM
	printf("\tSHARE_LATCH_ON_NVM");
#endif /* SHARE_LATCH_ON_NVM */
#ifdef PART_BUFFER_T_CAPACITY
	printf("\tPART_BUFFER_T_CAPACITY: %d", PART_BUFFER_T_CAPACITY);
#endif /* PART_BUFFER_T_CAPACITY */
	printf("\n");


#ifdef USE_PAPI
	memset(agg_PAPI_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(agg_PAPI_values_pushdown, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(agg_PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(agg_PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(agg_PAPI_values_aggregate, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
	memset(agg_PMWatch_output, 0, sizeof(MY_PMWATCH_OP_BUF_NODE) * MaxNVMNum );
	memset(agg_PMWatch_output_pushdown, 0, sizeof(MY_PMWATCH_OP_BUF_NODE) * MaxNVMNum );
	memset(agg_PMWatch_output_buildpart, 0, sizeof(MY_PMWATCH_OP_BUF_NODE) * MaxNVMNum );
	memset(agg_PMWatch_output_probejoin, 0, sizeof(MY_PMWATCH_OP_BUF_NODE) * MaxNVMNum );
	memset(agg_PMWatch_output_aggregate, 0, sizeof(MY_PMWATCH_OP_BUF_NODE) * MaxNVMNum );
#endif /* USE_PMWATCH */

	srand(time(NULL));
	// int seed = 32768;
	// srand(seed);

	/**************** CPU MAPPING INITIALIZATION ****************/
	mc_cfg(local_cores, remote_cores);

	tpch_timekeeper_t global_time_keeper = {0.0, 0.0, 0.0, 0.0, 0.0};


	for (cur_t = 0; cur_t < TRIAL_NUM; cur_t ++) {
		mkdir(work_dir);
		mkdir(dump_dir);
		tpch_timekeeper_t tmp_time_keeper = {0.0, 0.0, 0.0, 0.0, 0.0};
		tpch_spj(tpch_query_meta, &tmp_time_keeper, algo);
		if (cur_t != 0) {
			global_time_keeper.total += tmp_time_keeper.total;
			global_time_keeper.pushdown += tmp_time_keeper.pushdown;
			global_time_keeper.buildpart += tmp_time_keeper.buildpart;
			global_time_keeper.probejoin += tmp_time_keeper.probejoin;
			global_time_keeper.aggregate += tmp_time_keeper.aggregate;
		}
		cyan();
		printf("memory_type: %s\t", MEM_TYPE);
		printf("algo: %s\t",  algo->name);
		printf("query: %d\t", QUERY);
		printf("scaling_factor: %d\t", sf);
		printf("pushdown_time: %.9f\t", tmp_time_keeper.pushdown);
		printf("buildpart_time: %.9f\t", tmp_time_keeper.buildpart);
		printf("probejoin_time: %.9f\t", tmp_time_keeper.probejoin);
		printf("aggregate_time: %.9f\t", tmp_time_keeper.aggregate);
		printf("total_time: %.9f\n", tmp_time_keeper.total);
		deldir(work_dir);
		deldir(dump_dir);
	}

#if TRIAL_NUM != 1
	green();
	printf("\n");
	printf("num_trials: %d\t", TRIAL_NUM-1);
	printf("memory_type: %s\t", MEM_TYPE);
	printf("algo: %s\t",  algo->name);
	printf("query: %d\t", QUERY);
	printf("scaling_factor: %d\t", sf);
	printf("pushdown_time: %.9f\t", global_time_keeper.pushdown/(TRIAL_NUM-1));
	printf("buildpart_time: %.9f\t", global_time_keeper.buildpart/(TRIAL_NUM-1));
	printf("probejoin_time: %.9f\t", global_time_keeper.probejoin/(TRIAL_NUM-1));
	printf("aggregate_time: %.9f\t", global_time_keeper.aggregate/(TRIAL_NUM-1));	
	printf("total_time: %.9f\n\n", global_time_keeper.total/(TRIAL_NUM-1));

#ifdef USE_PAPI
	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		agg_PAPI_values[idx] /= (TRIAL_NUM-1);
		agg_PAPI_values_pushdown[idx] /= (TRIAL_NUM-1);
		agg_PAPI_values_buildpart[idx] /= (TRIAL_NUM-1);
		agg_PAPI_values_probejoin[idx] /= (TRIAL_NUM-1);
		agg_PAPI_values_aggregate[idx] /= (TRIAL_NUM-1);
		printf("[agg_PAPI] PAPI_event_name: %s", PAPI_event_names[idx]);
		printf("\ttotal_counter: %lld", agg_PAPI_values[idx]);
		printf("\tpushdown_counter: %lld", agg_PAPI_values_pushdown[idx]);
		printf("\tbuildpart_counter: %lld", agg_PAPI_values_buildpart[idx]);
		printf("\tprobejoin_counter: %lld", agg_PAPI_values_probejoin[idx]);
		printf("\taggregate_counter: %lld", agg_PAPI_values_aggregate[idx]);
		printf("\n");
	}
	printf("\n");
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
	for (int idx = 0; idx < NVMCount; idx ++) {
		agg_PMWatch_output[idx].total_bytes_read /= (TRIAL_NUM-1);
		agg_PMWatch_output[idx].total_bytes_written /= (TRIAL_NUM-1);
		agg_PMWatch_output[idx].media_read_ops /= (TRIAL_NUM-1);
		agg_PMWatch_output[idx].media_write_ops /= (TRIAL_NUM-1);
		agg_PMWatch_output[idx].read_64B_ops_received /= (TRIAL_NUM-1);
		agg_PMWatch_output[idx].write_64B_ops_received /= (TRIAL_NUM-1);
		agg_PMWatch_output[idx].host_reads /= (TRIAL_NUM-1);
		agg_PMWatch_output[idx].host_writes /= (TRIAL_NUM-1);
		agg_PMWatch_output[idx].read_hit_ratio /= (TRIAL_NUM-1);
		agg_PMWatch_output[idx].write_hit_ratio /= (TRIAL_NUM-1);
		agg_PMWatch_output[idx].read_amplification /= (TRIAL_NUM-1);
		agg_PMWatch_output[idx].write_amplification /= (TRIAL_NUM-1);

		agg_PMWatch_output_pushdown[idx].total_bytes_read /= (TRIAL_NUM-1);
		agg_PMWatch_output_pushdown[idx].total_bytes_written /= (TRIAL_NUM-1);
		agg_PMWatch_output_pushdown[idx].media_read_ops /= (TRIAL_NUM-1);
		agg_PMWatch_output_pushdown[idx].media_write_ops /= (TRIAL_NUM-1);
		agg_PMWatch_output_pushdown[idx].read_64B_ops_received /= (TRIAL_NUM-1);
		agg_PMWatch_output_pushdown[idx].write_64B_ops_received /= (TRIAL_NUM-1);
		agg_PMWatch_output_pushdown[idx].host_reads /= (TRIAL_NUM-1);
		agg_PMWatch_output_pushdown[idx].host_writes /= (TRIAL_NUM-1);
		agg_PMWatch_output_pushdown[idx].read_hit_ratio /= (TRIAL_NUM-1);
		agg_PMWatch_output_pushdown[idx].write_hit_ratio /= (TRIAL_NUM-1);
		agg_PMWatch_output_pushdown[idx].read_amplification /= (TRIAL_NUM-1);
		agg_PMWatch_output_pushdown[idx].write_amplification /= (TRIAL_NUM-1);

		agg_PMWatch_output_buildpart[idx].total_bytes_read /= (TRIAL_NUM-1);
		agg_PMWatch_output_buildpart[idx].total_bytes_written /= (TRIAL_NUM-1);
		agg_PMWatch_output_buildpart[idx].media_read_ops /= (TRIAL_NUM-1);
		agg_PMWatch_output_buildpart[idx].media_write_ops /= (TRIAL_NUM-1);
		agg_PMWatch_output_buildpart[idx].read_64B_ops_received /= (TRIAL_NUM-1);
		agg_PMWatch_output_buildpart[idx].write_64B_ops_received /= (TRIAL_NUM-1);
		agg_PMWatch_output_buildpart[idx].host_reads /= (TRIAL_NUM-1);
		agg_PMWatch_output_buildpart[idx].host_writes /= (TRIAL_NUM-1);
		agg_PMWatch_output_buildpart[idx].read_hit_ratio /= (TRIAL_NUM-1);
		agg_PMWatch_output_buildpart[idx].write_hit_ratio /= (TRIAL_NUM-1);
		agg_PMWatch_output_buildpart[idx].read_amplification /= (TRIAL_NUM-1);
		agg_PMWatch_output_buildpart[idx].write_amplification /= (TRIAL_NUM-1);

		agg_PMWatch_output_probejoin[idx].total_bytes_read /= (TRIAL_NUM-1);
		agg_PMWatch_output_probejoin[idx].total_bytes_written /= (TRIAL_NUM-1);
		agg_PMWatch_output_probejoin[idx].media_read_ops /= (TRIAL_NUM-1);
		agg_PMWatch_output_probejoin[idx].media_write_ops /= (TRIAL_NUM-1);
		agg_PMWatch_output_probejoin[idx].read_64B_ops_received /= (TRIAL_NUM-1);
		agg_PMWatch_output_probejoin[idx].write_64B_ops_received /= (TRIAL_NUM-1);
		agg_PMWatch_output_probejoin[idx].host_reads /= (TRIAL_NUM-1);
		agg_PMWatch_output_probejoin[idx].host_writes /= (TRIAL_NUM-1);
		agg_PMWatch_output_probejoin[idx].read_hit_ratio /= (TRIAL_NUM-1);
		agg_PMWatch_output_probejoin[idx].write_hit_ratio /= (TRIAL_NUM-1);
		agg_PMWatch_output_probejoin[idx].read_amplification /= (TRIAL_NUM-1);
		agg_PMWatch_output_probejoin[idx].write_amplification /= (TRIAL_NUM-1);

		agg_PMWatch_output_aggregate[idx].total_bytes_read /= (TRIAL_NUM-1);
		agg_PMWatch_output_aggregate[idx].total_bytes_written /= (TRIAL_NUM-1);
		agg_PMWatch_output_aggregate[idx].media_read_ops /= (TRIAL_NUM-1);
		agg_PMWatch_output_aggregate[idx].media_write_ops /= (TRIAL_NUM-1);
		agg_PMWatch_output_aggregate[idx].read_64B_ops_received /= (TRIAL_NUM-1);
		agg_PMWatch_output_aggregate[idx].write_64B_ops_received /= (TRIAL_NUM-1);
		agg_PMWatch_output_aggregate[idx].host_reads /= (TRIAL_NUM-1);
		agg_PMWatch_output_aggregate[idx].host_writes /= (TRIAL_NUM-1);
		agg_PMWatch_output_aggregate[idx].read_hit_ratio /= (TRIAL_NUM-1);
		agg_PMWatch_output_aggregate[idx].write_hit_ratio /= (TRIAL_NUM-1);
		agg_PMWatch_output_aggregate[idx].read_amplification /= (TRIAL_NUM-1);
		agg_PMWatch_output_aggregate[idx].write_amplification /= (TRIAL_NUM-1);

		printf("[agg_PMWatch] DIMM: %d\n", idx);
		printf("[agg_PMWatch] DIMM: %d\ttotal_bytes_read: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			agg_PMWatch_output[idx].total_bytes_read, 
			agg_PMWatch_output_pushdown[idx].total_bytes_read, 
			agg_PMWatch_output_buildpart[idx].total_bytes_read, 
			agg_PMWatch_output_probejoin[idx].total_bytes_read,
			agg_PMWatch_output_aggregate[idx].total_bytes_read
		);
		printf("[agg_PMWatch] DIMM: %d\ttotal_bytes_written: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			agg_PMWatch_output[idx].total_bytes_written,
			agg_PMWatch_output_pushdown[idx].total_bytes_written,
			agg_PMWatch_output_buildpart[idx].total_bytes_written, 
			agg_PMWatch_output_probejoin[idx].total_bytes_written,
			agg_PMWatch_output_aggregate[idx].total_bytes_written
		);
		printf("[agg_PMWatch] DIMM: %d\tmedia_read_ops: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			agg_PMWatch_output[idx].media_read_ops,
			agg_PMWatch_output_pushdown[idx].media_read_ops, 
			agg_PMWatch_output_buildpart[idx].media_read_ops,
			agg_PMWatch_output_probejoin[idx].media_read_ops,
			agg_PMWatch_output_aggregate[idx].media_read_ops
		);
		printf("[agg_PMWatch] DIMM: %d\tmedia_write_ops: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			agg_PMWatch_output[idx].media_write_ops,
			agg_PMWatch_output_pushdown[idx].media_write_ops,
			agg_PMWatch_output_buildpart[idx].media_write_ops, 
			agg_PMWatch_output_probejoin[idx].media_write_ops,
			agg_PMWatch_output_aggregate[idx].media_write_ops
		);
		printf("[agg_PMWatch] DIMM: %d\tread_64B_ops_received: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			agg_PMWatch_output[idx].read_64B_ops_received,
			agg_PMWatch_output_pushdown[idx].read_64B_ops_received,
			agg_PMWatch_output_buildpart[idx].read_64B_ops_received,
			agg_PMWatch_output_probejoin[idx].read_64B_ops_received,
			agg_PMWatch_output_aggregate[idx].read_64B_ops_received
		);
		printf("[agg_PMWatch] DIMM: %d\twrite_64B_ops_received: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			agg_PMWatch_output[idx].write_64B_ops_received,
			agg_PMWatch_output_pushdown[idx].write_64B_ops_received,
			agg_PMWatch_output_buildpart[idx].write_64B_ops_received, 
			agg_PMWatch_output_probejoin[idx].write_64B_ops_received,
			agg_PMWatch_output_aggregate[idx].write_64B_ops_received
		);
		printf("[agg_PMWatch] DIMM: %d\thost_reads: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			agg_PMWatch_output[idx].host_reads,
			agg_PMWatch_output_pushdown[idx].host_reads,
			agg_PMWatch_output_buildpart[idx].host_reads, 
			agg_PMWatch_output_probejoin[idx].host_reads,
			agg_PMWatch_output_aggregate[idx].host_reads
		);
		printf("[agg_PMWatch] DIMM: %d\thost_writes: %zu\t%zu\t%zu\t%zu\t%zu\n", idx, 
			agg_PMWatch_output[idx].host_writes,
			agg_PMWatch_output_pushdown[idx].host_writes,
			agg_PMWatch_output_buildpart[idx].host_writes, 
			agg_PMWatch_output_probejoin[idx].host_writes,
			agg_PMWatch_output_aggregate[idx].host_writes
		);
		printf("[agg_PMWatch] DIMM: %d\tread_hit_ratio: %f\t%f\t%f\t%f\t%f\n", idx, 
			agg_PMWatch_output[idx].read_hit_ratio,
			agg_PMWatch_output_pushdown[idx].read_hit_ratio,
			agg_PMWatch_output_buildpart[idx].read_hit_ratio, 
			agg_PMWatch_output_probejoin[idx].read_hit_ratio,
			agg_PMWatch_output_aggregate[idx].read_hit_ratio
		);
		printf("[agg_PMWatch] DIMM: %d\twrite_hit_ratio: %f\t%f\t%f\t%f\t%f\n", idx, 
			agg_PMWatch_output[idx].write_hit_ratio,
			agg_PMWatch_output_pushdown[idx].write_hit_ratio,
			agg_PMWatch_output_buildpart[idx].write_hit_ratio, 
			agg_PMWatch_output_probejoin[idx].write_hit_ratio,
			agg_PMWatch_output_aggregate[idx].write_hit_ratio
		);
		printf("[agg_PMWatch] DIMM: %d\tread_amplification: %.9f\t%.9f\t%.9f\t%.9f\t%.9f\n", idx, 
			agg_PMWatch_output[idx].read_amplification,
			agg_PMWatch_output_pushdown[idx].read_amplification, 
			agg_PMWatch_output_buildpart[idx].read_amplification, 
			agg_PMWatch_output_probejoin[idx].read_amplification,
			agg_PMWatch_output_aggregate[idx].read_amplification
		);
		printf("[agg_PMWatch] DIMM: %d\twrite_amplification: %.9f\t%.9f\t%.9f\t%.9f\t%.9f\n", idx, 
			agg_PMWatch_output[idx].write_amplification,
			agg_PMWatch_output_pushdown[idx].write_amplification,
			agg_PMWatch_output_buildpart[idx].write_amplification, 
			agg_PMWatch_output_probejoin[idx].write_amplification,
			agg_PMWatch_output_aggregate[idx].write_amplification
		);
		printf("\n");
	}
#endif /* USE_PMWATCH */

#endif /* TRIAL_NUM != 1 */
	reset();
	printf("\n\n\n\n");

	return 0;
}