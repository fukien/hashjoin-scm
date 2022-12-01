#ifdef BW_REG
#ifdef RUN_PAYLOAD

#include "libpmem.h"

#include "ptr_phj_bw_reg.h"
#include "../inc/memaccess.h"

#define __USE_GNU
#include <sched.h>
#include <pthread.h>

// #include "../inc/task_ptr.h"
// #include "../inc/task_cas_ptr.h"
#include "../inc/task_pthread_ptr.h"


#ifdef TEST_SELECTIVITY
#include <stdbool.h>
#endif /* TEST_SELECTIVITY */


extern char work_dir[CHAR_BUFFER_LEN];
extern char dump_dir[CHAR_BUFFER_LEN];
extern size_t file_idx;

#ifdef USE_PAPI
#include "../papi/mypapi.h"
extern char* PAPI_event_names[NUM_PAPI_EVENT];
extern long long agg_PAPI_values[NUM_PAPI_EVENT];
extern long long agg_PAPI_values_buildpart[NUM_PAPI_EVENT];
extern long long agg_PAPI_values_probejoin[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
#include "../pmw/mypmw.h"
extern unsigned int NVMCount;
static MY_PMWATCH_OP_BUF_NODE PMWatch_output[MaxNVMNum];
static MY_PMWATCH_OP_BUF_NODE PMWatch_output_buildpart[MaxNVMNum];
static MY_PMWATCH_OP_BUF_NODE PMWatch_output_probejoin[MaxNVMNum];
static MY_PMWATCH_OP_BUF_NODE PMWatch_output_retrieve[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_buildpart[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_probejoin[MaxNVMNum];
#endif /* USE_PMWATCH */


#if NUM_PASSES != 1
static size_t *r_hist_2nd_pass;
static size_t *s_hist_2nd_pass;

#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
static ptr_swwcb_t *r_swwcb_2nd_pass_mem;
static ptr_swwcb_t *s_swwcb_2nd_pass_mem;
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */

static size_t idx_2nd_pass;
#endif /*  NUM_PASSES != 1 */

extern int cur_t;
typedef void (*ptr_joinfunction_bw_reg_t)(const ptr_relation_t * const, const ptr_relation_t * const, ptr_relation_t * const, ptr_join_arg_t * const);


/** holds the arguments passed to each thread */
typedef struct ptr_phj_arg_bw_reg_t {
	int _tid;
	pthread_barrier_t *barrier;

	const datameta_t * datameta;

	relation_t r_rel;
	ptr_relation_t r_tmp;
	ptr_relation_t r_ext;
	size_t **r_hist;

	relation_t s_rel;
	ptr_relation_t s_tmp;
	ptr_relation_t s_ext;
	size_t **s_hist;

	ptr_task_queue_t *part_queue;
	ptr_task_queue_t *join_queue;
#if NUM_PASSES != 1
	ptr_task_queue_t *part_queue_2nd_pass;
#endif /* NUM_PASSES */

#if SKEW_HANDLING
	ptr_task_queue_t *skew_queue;
	ptr_task_t ** skewtask;
#endif /* SKEW_HANDLING */

	ptr_joinfunction_bw_reg_t joinfunction;
	size_t *max_build_side_tup_num;

	size_t matched_cnt;
	rowid_t checksum;

	/* stats about the thread */
	size_t parts_processed;

	row_id_pair_t **mater;
	size_t *mater_cnt;

	size_t row_id_pairs_processed;

	struct timespec part_start, part_end, join_start, join_end, retrieve_start, retrieve_end;

#ifdef SYNCSTATS
	ptr_synctimer_phj_bw_reg_t *localtimer;
	ptr_synctimer_phj_bw_reg_t *globaltimer;
#ifdef SKEW_HANDLING
	double sync_idle_skew_taskfetch_acc;
	double sync_idle_skew_prepart_memset_acc;
	double sync_idle_skew_hist_acc[2];				// 0 - R, 1 - S
	double sync_idle_skew_prfx_sum_acc[2];			// 0 - R, 1 - S
	double sync_idle_skew_part_acc;
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	double sync_idle_skew_swwcb_acc[2];				// 0 - R, 1 - S
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
#ifdef PHJ_SYNCSTATS
	double global_acc_skew_taskfetch;
	double global_acc_skew_prepart_memset;
	double global_acc_skew_hist[2];					// 0 - R, 1 - S
	double global_acc_skew_prfx_sum[2];				// 0 - R, 1 - S
	double global_acc_skew_part;
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	double global_acc_skew_swwcb[2];				// 0 - R, 1 - S
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
#endif /* PHJ_SYNCSTATS */
#endif /* SKEW_HANDLING */
#endif /* SYNCSTATS */

	relation_t *org_relR;
	relation_t *org_relS;

#ifdef PHJ_MBP
	double memset_time, build_time, probe_time, retrieve_time;
#endif /* PHJ_MBP */

#ifdef USE_PAPI
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
	long long PAPI_values_retrieve[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

	size_t intermediate_size;
	void *intermediate;

	size_t **r_output_offset;
	size_t **s_output_offset;

	size_t r_thr_tup_num;
	size_t r_last_thr_tup_num;

	size_t s_thr_tup_num;
	size_t s_last_thr_tup_num;

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	ptr_swwcb_t **swwcb_s;
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
} ptr_phj_arg_bw_reg_t;


typedef struct ptr_part_bw_reg_t {
	int _tid;
	pthread_barrier_t *barrier;

	key_t_ hashmask;
	key_t_ bitskip;

	relation_t rel;
	ptr_relation_t tmp;
	ptr_relation_t ext;

	size_t **hist;
	size_t **output_offset;
	tuple_t *tuple_org;
	keyrid_t *keyrid_org;
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	ptr_swwcb_t **swwcb_s;
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

	size_t thr_tup_num;
	size_t last_thr_tup_num;

	short rel_flag;		/* 0: R, 1: S */

#ifdef SYNCSTATS
	struct timespec *sync_local_hist;
	struct timespec *sync_global_hist;
	struct timespec *sync_local_prfx_sum;
	struct timespec *sync_global_prfx_sum;
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	struct timespec *sync_local_swwcb;
	struct timespec *sync_global_swwcb;
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

#endif /* SYNCSTATS */

} ptr_part_bw_reg_t;


void ptr_parallel_radix_partition_first_bw_reg(ptr_part_bw_reg_t * const part, const size_t padding_num) {
	int rv;

	key_t_ hashmask = (part->hashmask) << part->bitskip;
	key_t_ bitskip = part->bitskip;

	size_t fanout = part->hashmask + 1;
	size_t **hist = part->hist;
	size_t *myhist =  part->hist[part->_tid];
	size_t *myoutput_offset = part->output_offset[part->_tid];

	relation_t rel = part->rel;
	ptr_relation_t tmp = part->tmp;

	/* compute the local histogram */
	hashkey_t tmp_hashkey;
	for (size_t idx = 0; idx < rel.tuple_num; idx ++) {
		tmp_hashkey = IDHASH(rel.tuples[idx].key, hashmask, bitskip);
		/* tuple count of current partition of current thread */
		myhist[tmp_hashkey*2] ++;
	}

	/* compute the local prefix sum on histogram */
	size_t accum_num = 0;
	for (size_t idx = 0; idx < fanout; idx ++) {
		accum_num += myhist[idx*2];
		/* tuple count prefix sum of current partition of current thread */
		myhist[idx*2+1] = accum_num;
	}

	/* local prefix sum */

#ifdef SYNCSTATS
	SYNC_TIMER(*part->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part->_tid, *part->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each cluster */
	for (size_t idx = 0; idx < part->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
		for (size_t jdx = 1; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][(jdx-1)*2+1] ;
		}
	}

#ifdef SYNCSTATS
	SYNC_TIMER(*part->sync_local_prfx_sum);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part->_tid, *part->sync_global_prfx_sum);
#endif /* SYNCSTATS */

	/* copy tuples to their corresponding clusters */
	if (part->_tid < BUILDPART_WRITE_THREAD_NUM) {
		int tmp_tid = part->_tid;

		size_t tmp_tuple_num;
		tuple_t *tmp_tuples;

		while (tmp_tid < BUILDPART_THREAD_NUM) {
			tmp_tuple_num = (tmp_tid == BUILDPART_THREAD_NUM-1) ?
				part->last_thr_tup_num : part->thr_tup_num;
			tmp_tuples = part->tuple_org + part->thr_tup_num * tmp_tid;

			for (size_t idx = 0; idx < tmp_tuple_num; idx ++) {	
				tmp_hashkey = IDHASH(tmp_tuples[idx].key, hashmask, bitskip);
				store_a_keyrid(& tmp.keyrids[ part->output_offset[tmp_tid][tmp_hashkey] + tmp_hashkey * padding_num] , 
					& tmp_tuples[idx]);
				part->output_offset[tmp_tid][tmp_hashkey] ++;
			}

			/* move back the output_offset pointer to the starting address */
			/* this code segments could be moved to main and only be executed for the 1st thread */
			for (size_t idx = 0; idx < fanout; idx ++) {
				part->output_offset[tmp_tid][idx] -= hist[tmp_tid][idx*2];
				part->output_offset[tmp_tid][idx] += idx * padding_num;
			}

			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}

	}
}


#ifdef SKEW_HANDLING
void ptr_parallel_radix_partition_following_bw_reg(ptr_part_bw_reg_t * const part, const size_t padding_num) {
	int rv;

	key_t_ hashmask = (part->hashmask) << part->bitskip;
	key_t_ bitskip = part->bitskip;

	size_t fanout = part->hashmask + 1;
	size_t **hist = part->hist;
	size_t *myhist =  part->hist[part->_tid];
	size_t *myoutput_offset = part->output_offset[part->_tid];

	ptr_relation_t rel = part->ext;
	ptr_relation_t tmp = part->tmp;

	/* compute the local histogram */
	hashkey_t tmp_hashkey;
	for (size_t idx = 0; idx < rel.keyrid_num; idx ++) {
		tmp_hashkey = IDHASH(rel.keyrids[idx].key, hashmask, bitskip);
		/* tuple count of current partition of current thread */
		myhist[tmp_hashkey*2] ++;
	}

	/* compute the local prefix sum on histogram */
	size_t accum_num = 0;
	for (size_t idx = 0; idx < fanout; idx ++) {
		accum_num += myhist[idx*2];
		/* tuple count prefix sum of current partition of current thread */
		myhist[idx*2+1] = accum_num;
	}

	/* local prefix sum */

#ifdef SYNCSTATS
	SYNC_TIMER(*part->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part->_tid, *part->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each cluster */
	for (size_t idx = 0; idx < part->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
		for (size_t jdx = 1; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][(jdx-1)*2+1] ;
		}
	}

#ifdef SYNCSTATS
	SYNC_TIMER(*part->sync_local_prfx_sum);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part->_tid, *part->sync_global_prfx_sum);
#endif /* SYNCSTATS */

	/* copy tuples to their corresponding clusters */
	if (part->_tid < BUILDPART_WRITE_THREAD_NUM) {
		int tmp_tid = part->_tid;

		size_t tmp_keyrid_num;
		keyrid_t *tmp_keyrids;

		while (tmp_tid < BUILDPART_THREAD_NUM) {
			tmp_keyrid_num = (tmp_tid == BUILDPART_THREAD_NUM-1) ?
				part->last_thr_tup_num : part->thr_tup_num;
			tmp_keyrids = part->keyrid_org + part->thr_tup_num * tmp_tid;

			for (size_t idx = 0; idx < tmp_keyrid_num; idx ++) {	
				tmp_hashkey = IDHASH(tmp_keyrids[idx].key, hashmask, bitskip);
				store_a_keyrid(& tmp.keyrids[ part->output_offset[tmp_tid][tmp_hashkey] + tmp_hashkey * padding_num] , 
					& tmp_keyrids[idx]);
				part->output_offset[tmp_tid][tmp_hashkey] ++;
			}

			/* move back the output_offset pointer to the starting address */
			/* this code segments could be moved to main and only be executed for the 1st thread */
			for (size_t idx = 0; idx < fanout; idx ++) {
				part->output_offset[tmp_tid][idx] -= hist[tmp_tid][idx*2];
				part->output_offset[tmp_tid][idx] += idx * padding_num;
			}

			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}

	}
}
#endif /* SKEW_HANDLING */


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


#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
void ptr_parallel_radix_partition_optimized_first_bw_reg(ptr_part_bw_reg_t * const part, 
	const size_t padding_num, const size_t global_offset) {
	int rv;

	key_t_ hashmask = (part->hashmask) << part->bitskip;
	key_t_ bitskip = part->bitskip;

	size_t fanout = part->hashmask + 1;
	size_t **hist = part->hist;
	size_t *myhist =  part->hist[part->_tid];
	size_t *myoutput_offset = part->output_offset[part->_tid];

	ptr_swwcb_t *myswwcb = part->swwcb_s[part->_tid];

	relation_t rel = part->rel;
	ptr_relation_t tmp = part->tmp;

	/* compute the local histogram */
	hashkey_t tmp_hashkey;
	for (size_t idx = 0; idx < rel.tuple_num; idx ++) {
		tmp_hashkey = IDHASH(rel.tuples[idx].key, hashmask, bitskip);
		/* tuple count of current partition of current thread */
		myhist[tmp_hashkey*2] ++;
	}

	/* compute the local prefix sum on histogram */
	size_t accum_num = 0;
	for (size_t idx = 0; idx < fanout; idx ++) {
		accum_num += myhist[idx*2];
		/* tuple count prefix sum of current partition of current thread */
		myhist[idx*2+1] = accum_num;
	}

	/* local prefix sum */
#ifdef SYNCSTATS
	SYNC_TIMER(*part->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part->_tid, *part->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each SWWCB */
	for (size_t idx = 0; idx < part->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
		for (size_t jdx = 1; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][(jdx-1)*2+1];
		}
	}

	/* initial updates on SWWCB slots */
	for (size_t idx = 0; idx < fanout; idx ++) {
		myoutput_offset[idx] += idx * padding_num;
		myswwcb[idx].data.slot = myoutput_offset[idx] + global_offset;
	}

#ifdef SYNCSTATS
	SYNC_TIMER(*part->sync_local_prfx_sum);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part->_tid, *part->sync_global_prfx_sum);
#endif /* SYNCSTATS */

	/* copy tuples to their corresponding SWWCBs */

	if (part->_tid < BUILDPART_WRITE_THREAD_NUM) {
		int tmp_tid = part->_tid;

		size_t slot;
		size_t slot_mod;
		size_t remainder_start_pos;
		keyrid_t *tmp_swwcb;	
		
		size_t tmp_tuple_num;
		tuple_t *tmp_tuples;

		while (tmp_tid < BUILDPART_THREAD_NUM) {
			tmp_tuple_num = (tmp_tid == BUILDPART_THREAD_NUM-1) ?
				part->last_thr_tup_num : part->thr_tup_num;
			tmp_tuples = part->tuple_org + part->thr_tup_num * tmp_tid;

			for (size_t idx = 0; idx < tmp_tuple_num; idx ++ ) {
				tmp_hashkey = IDHASH(tmp_tuples[idx].key, hashmask, bitskip);
				slot = part->swwcb_s[tmp_tid][tmp_hashkey].data.slot;
				tmp_swwcb = (keyrid_t *) ( part->swwcb_s[tmp_tid] + tmp_hashkey );
				slot_mod = slot & (KEYRID_PER_SWWCB - 1 );
				memcpy(&tmp_swwcb[slot_mod], &tmp_tuples[idx], KEYRID_T_SIZE);
				
				if (slot_mod == KEYRID_PER_SWWCB - 1) {
					nontemp_store_swwcb( & tmp.keyrids[slot-(KEYRID_PER_SWWCB-1)-global_offset], 	
						tmp_swwcb);
				}
				part->swwcb_s[tmp_tid][tmp_hashkey].data.slot = slot + 1;
			}

			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}

		/* nontemp store */
#ifdef SYNCSTATS
		SYNC_TIMER(*part->sync_local_swwcb);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(part->_tid, *part->sync_global_swwcb);
#endif /* SYNCSTATS */

		tmp_tid = part->_tid;	
		while (tmp_tid < BUILDPART_THREAD_NUM) {

			for (size_t idx = 0; idx < fanout; idx ++) {
				slot = part->swwcb_s[tmp_tid][idx].data.slot;
				slot_mod = slot & (KEYRID_PER_SWWCB - 1);
				slot -= slot_mod;

				remainder_start_pos = (slot < part->output_offset[tmp_tid][idx] + global_offset) ?
					(part->output_offset[tmp_tid][idx] + global_offset - slot) : 0;

				for (size_t jdx = remainder_start_pos; jdx < slot_mod; jdx ++) {
					store_a_keyrid( & tmp.keyrids[slot+jdx-global_offset], 
						& part->swwcb_s[tmp_tid][idx].data.keyrids[jdx] );
				}
			}

			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}

	} else {

		/* for barrier balancing among threads */
#ifdef SYNCSTATS
		SYNC_TIMER(*part->sync_local_swwcb);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(part->_tid, *part->sync_global_swwcb);
#endif /* SYNCSTATS */
	}
}


#ifdef SKEW_HANDLING
void ptr_parallel_radix_partition_optimized_following_bw_reg(ptr_part_bw_reg_t * const part,
	const size_t padding_num, const size_t global_offset) {
	int rv;

	key_t_ hashmask = (part->hashmask) << part->bitskip;
	key_t_ bitskip = part->bitskip;

	size_t fanout = part->hashmask + 1;
	size_t **hist = part->hist;
	size_t *myhist =  part->hist[part->_tid];
	size_t *myoutput_offset = part->output_offset[part->_tid];

	ptr_swwcb_t *myswwcb = part->swwcb_s[part->_tid];

	ptr_relation_t rel = part->ext;
	ptr_relation_t tmp = part->tmp;

	/* compute the local histogram */
	hashkey_t tmp_hashkey;
	for (size_t idx = 0; idx < rel.keyrid_num; idx ++) {
		tmp_hashkey = IDHASH(rel.keyrids[idx].key, hashmask, bitskip);
		/* tuple count of current partition of current thread */
		myhist[tmp_hashkey*2] ++;
	}

	/* compute the local prefix sum on histogram */
	size_t accum_num = 0;
	for (size_t idx = 0; idx < fanout; idx ++) {
		accum_num += myhist[idx*2];
		/* tuple count prefix sum of current partition of current thread */
		myhist[idx*2+1] = accum_num;
	}

	/* local prefix sum */
#ifdef SYNCSTATS
	SYNC_TIMER(*part->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part->_tid, *part->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each SWWCB */
	for (size_t idx = 0; idx < part->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
		for (size_t jdx = 1; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][(jdx-1)*2+1];
		}
	}

	/* initial updates on SWWCB slots */
	for (size_t idx = 0; idx < fanout; idx ++) {
		myoutput_offset[idx] += idx * padding_num;
		myswwcb[idx].data.slot = myoutput_offset[idx] + global_offset;
	}

#ifdef SYNCSTATS
	SYNC_TIMER(*part->sync_local_prfx_sum);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part->_tid, *part->sync_global_prfx_sum);
#endif /* SYNCSTATS */

	/* copy tuples to their corresponding SWWCBs */

	if (part->_tid < BUILDPART_WRITE_THREAD_NUM) {
		int tmp_tid = part->_tid;

		size_t slot;
		size_t slot_mod;
		size_t remainder_start_pos;
		keyrid_t *tmp_swwcb;	
		
		size_t tmp_keyrid_num;
		keyrid_t *tmp_keyrids;

		while (tmp_tid < BUILDPART_THREAD_NUM) {
			tmp_keyrid_num = (tmp_tid == BUILDPART_THREAD_NUM-1) ?
				part->last_thr_tup_num : part->thr_tup_num;
			tmp_keyrids = part->keyrid_org + part->thr_tup_num * tmp_tid;

			for (size_t idx = 0; idx < tmp_keyrid_num; idx ++ ) {
				tmp_hashkey = IDHASH(tmp_keyrids[idx].key, hashmask, bitskip);
				slot = part->swwcb_s[tmp_tid][tmp_hashkey].data.slot;
				tmp_swwcb = (keyrid_t *) ( part->swwcb_s[tmp_tid] + tmp_hashkey );
				slot_mod = slot & (KEYRID_PER_SWWCB - 1 );
				memcpy(&tmp_swwcb[slot_mod], &rel.keyrids[idx], KEYRID_T_SIZE);

				if (slot_mod == KEYRID_PER_SWWCB - 1) {
					nontemp_store_swwcb( & tmp.keyrids[slot-(KEYRID_PER_SWWCB-1)-global_offset], 	
						tmp_swwcb);
				}
				part->swwcb_s[tmp_tid][tmp_hashkey].data.slot = slot + 1;
			}

			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}

		/* nontemp store */
#ifdef SYNCSTATS
		SYNC_TIMER(*part->sync_local_swwcb);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(part->_tid, *part->sync_global_swwcb);
#endif /* SYNCSTATS */

		tmp_tid = part->_tid;	
		while (tmp_tid < BUILDPART_THREAD_NUM) {

			for (size_t idx = 0; idx < fanout; idx ++) {
				slot = part->swwcb_s[tmp_tid][idx].data.slot;
				slot_mod = slot & (KEYRID_PER_SWWCB - 1);
				slot -= slot_mod;

				remainder_start_pos = (slot < part->output_offset[tmp_tid][idx] + global_offset) ?
					(part->output_offset[tmp_tid][idx] + global_offset - slot) : 0;

				for (size_t jdx = remainder_start_pos; jdx < slot_mod; jdx ++) {
					store_a_keyrid( & tmp.keyrids[slot+jdx-global_offset], 
						& part->swwcb_s[tmp_tid][idx].data.keyrids[jdx] );
				}
			}

			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}

	} else {

		/* for barrier balancing among threads */
#ifdef SYNCSTATS
		SYNC_TIMER(*part->sync_local_swwcb);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(part->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(part->_tid, *part->sync_global_swwcb);
#endif /* SYNCSTATS */
	}
}
#endif /* SKEW_HANDLING */
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */


#if NUM_PASSES != 1


void ptr_radix_cluster_bw_reg(ptr_relation_t out_rel, ptr_relation_t in_rel, const size_t fanout, 
	const key_t_ bitskip, const size_t padding_num, size_t *hist, 
	const int _tid, const int before_mid_hist_flag) {

	key_t_ hashmask = (fanout - 1) << bitskip;
	hashkey_t tmp_hashkey;

	if (before_mid_hist_flag) {
		for (size_t idx = 0; idx < in_rel.keyrid_num; idx ++) {
			tmp_hashkey = IDHASH(in_rel.keyrids[idx].key, hashmask, bitskip);
			/* tuple count of current partition of current thread */
			hist[tmp_hashkey*2] ++;
		}

		size_t accum_num = 0;
		for (size_t idx = 0; idx < fanout; idx ++) {
			accum_num += hist[idx*2];
			/* tuple count prefix sum of current partition of current thread */
			hist[idx*2+1] = accum_num - hist[idx*2];
		}

		if (_tid < BUILDPART_WRITE_THREAD_NUM) {
			for (size_t idx = 0; idx < in_rel.keyrid_num; idx ++) {
				tmp_hashkey = IDHASH(in_rel.keyrids[idx].key, hashmask, bitskip);
				store_a_keyrid(& out_rel.keyrids[hist[tmp_hashkey*2+1] + tmp_hashkey * padding_num] , & in_rel.keyrids[idx]);
				hist[tmp_hashkey*2+1] ++;
			}
		}

	} else {
		for (size_t idx = 0; idx < in_rel.keyrid_num; idx ++) {
			tmp_hashkey = IDHASH(in_rel.keyrids[idx].key, hashmask, bitskip);
			store_a_keyrid(& out_rel.keyrids[hist[tmp_hashkey*2+1] + tmp_hashkey * padding_num] , & in_rel.keyrids[idx]);
			hist[tmp_hashkey*2+1] ++;
		}
	}
}


#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
void ptr_radix_cluster_optimized_bw_reg(ptr_relation_t out_rel, ptr_relation_t in_rel, 
	const size_t fanout, const key_t_ bitskip, const size_t padding_num, 
	const size_t global_offset, size_t *hist, ptr_swwcb_t * swwcb, 
	const int _tid, const int before_mid_hist_flag) {

	key_t_ hashmask = (fanout - 1) << bitskip;
	hashkey_t tmp_hashkey;

	if (before_mid_hist_flag) {
		for (size_t idx = 0; idx < in_rel.keyrid_num; idx ++) {
			tmp_hashkey = IDHASH(in_rel.keyrids[idx].key, hashmask, bitskip);
			/* tuple count of current partition of current thread */
			hist[tmp_hashkey*2] ++;
		}

		size_t accum_num = 0;
		for (size_t idx = 0; idx < fanout; idx ++) {
			accum_num += hist[idx*2];
			/* tuple count prefix sum of current partition of current thread */
			hist[idx*2+1] = accum_num - hist[idx*2] + idx * padding_num;
			swwcb[idx].data.slot = hist[idx*2+1] + global_offset;
		}

		if (_tid < BUILDPART_WRITE_THREAD_NUM) {
			/* copy keyrids to their corresponding SWWCBs */
			size_t slot;
			size_t slot_mod;
			size_t remainder_start_pos;

			keyrid_t * tmp_swwcb;
			for (size_t idx = 0; idx < in_rel.keyrid_num; idx ++) {
				tmp_hashkey = IDHASH(in_rel.keyrids[idx].key, hashmask, bitskip);
				slot = swwcb[tmp_hashkey].data.slot;
				tmp_swwcb = (keyrid_t *) (swwcb + tmp_hashkey);
				slot_mod = slot & (KEYRID_PER_SWWCB - 1);
				memcpy(& tmp_swwcb[slot_mod], & in_rel.keyrids[idx], KEYRID_T_SIZE);

				if (slot_mod == KEYRID_PER_SWWCB - 1) {
					/* non-temporal store a SWWCB */
					nontemp_store_swwcb( & out_rel.keyrids[slot-(KEYRID_PER_SWWCB-1)-global_offset] , tmp_swwcb);
				}

				swwcb[tmp_hashkey].data.slot = slot + 1;
			}

			/* write out the remainders in the swwcbs */
			for (size_t idx = 0; idx < fanout; idx ++) {
				slot = swwcb[idx].data.slot;
				slot_mod = slot & (KEYRID_PER_SWWCB - 1);
				slot -= slot_mod;
				
				remainder_start_pos = (slot <  hist[idx*2+1] + global_offset ) ? ( hist[idx*2+1] + global_offset - slot) : 0;
				for (size_t jdx = remainder_start_pos; jdx < slot_mod; jdx ++) {
					store_a_keyrid(& out_rel.keyrids[slot+jdx-global_offset] , & swwcb[idx].data.keyrids[jdx]);
				}
			}

		}

	} else {
		/* copy keyrids to their corresponding SWWCBs */
		size_t slot;
		size_t slot_mod;
		size_t remainder_start_pos;

		keyrid_t * tmp_swwcb;
		for (size_t idx = 0; idx < in_rel.keyrid_num; idx ++) {
			tmp_hashkey = IDHASH(in_rel.keyrids[idx].key, hashmask, bitskip);
			slot = swwcb[tmp_hashkey].data.slot;
			tmp_swwcb = (keyrid_t *) (swwcb + tmp_hashkey);
			slot_mod = slot & (KEYRID_PER_SWWCB - 1);
			// tmp_swwcb[slot_mod] = in_rel.keyrids[idx];
			memcpy(& tmp_swwcb[slot_mod], & in_rel.keyrids[idx], KEYRID_T_SIZE);

			if (slot_mod == KEYRID_PER_SWWCB - 1) {
				/* non-temporal store a SWWCB */
				nontemp_store_swwcb( & out_rel.keyrids[slot-(KEYRID_PER_SWWCB-1)-global_offset] , tmp_swwcb);
			}

			swwcb[tmp_hashkey].data.slot = slot + 1;
		}

		/* write out the remainders in the swwcbs */
		for (size_t idx = 0; idx < fanout; idx ++) {
			slot = swwcb[idx].data.slot;
			slot_mod = slot & (KEYRID_PER_SWWCB - 1);
			slot -= slot_mod;
			
			remainder_start_pos = (slot <  hist[idx*2+1] + global_offset ) ? ( hist[idx*2+1] + global_offset - slot) : 0;
			for (size_t jdx = remainder_start_pos; jdx < slot_mod; jdx ++) {
				store_a_keyrid(& out_rel.keyrids[slot+jdx-global_offset] , & swwcb[idx].data.keyrids[jdx]);
			}
		}
	}
}
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */


void ptr_serial_radix_partition_bw_reg(ptr_task_t * const task, ptr_task_queue_t * join_queue, ptr_task_queue_t * part_queue_2nd_pass, 
	const size_t fanout, const key_t_ bitskip, size_t *max_build_side_tup_num, 
	const size_t padding_num, const int _tid, const int before_mid_hist_flag) {

	size_t *tmp_r_hist_2nd_pass;
	size_t *tmp_s_hist_2nd_pass;
#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
	ptr_swwcb_t *tmp_r_swwcb_2nd_pass_mem;
	ptr_swwcb_t *tmp_s_swwcb_2nd_pass_mem;
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */

	if (before_mid_hist_flag) {
		size_t tmp_idx_2nd_pass = __sync_fetch_and_add(&idx_2nd_pass, 1);
		tmp_r_hist_2nd_pass = r_hist_2nd_pass + tmp_idx_2nd_pass * 2 * FANOUT_PASS2;
		tmp_s_hist_2nd_pass = s_hist_2nd_pass + tmp_idx_2nd_pass * 2 * FANOUT_PASS2;
#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
		tmp_r_swwcb_2nd_pass_mem = r_swwcb_2nd_pass_mem + tmp_idx_2nd_pass * FANOUT_PASS2;
		tmp_s_swwcb_2nd_pass_mem = s_swwcb_2nd_pass_mem + tmp_idx_2nd_pass * FANOUT_PASS2;
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */
	} else {
		tmp_r_hist_2nd_pass = task->r_hist_2nd_pass;
		tmp_s_hist_2nd_pass = task->s_hist_2nd_pass;
#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
		tmp_r_swwcb_2nd_pass_mem = (ptr_swwcb_t *) ( task->r_swwcb_2nd_pass_mem );
		tmp_s_swwcb_2nd_pass_mem = (ptr_swwcb_t *) ( task->s_swwcb_2nd_pass_mem );
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */
	}

#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
	ptr_radix_cluster_optimized_bw_reg(task->r_tmp, task->r_rel, fanout, bitskip, padding_num, 
		task->r_global_offset, tmp_r_hist_2nd_pass, tmp_r_swwcb_2nd_pass_mem, 
		_tid, before_mid_hist_flag);
	ptr_radix_cluster_optimized_bw_reg(task->s_tmp, task->s_rel, fanout, bitskip, padding_num, 
		task->s_global_offset, tmp_s_hist_2nd_pass, tmp_s_swwcb_2nd_pass_mem, 
		_tid, before_mid_hist_flag);
#else /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
	ptr_radix_cluster_bw_reg(task->r_tmp, task->r_rel, fanout, bitskip, padding_num, 
		tmp_r_hist_2nd_pass, _tid, before_mid_hist_flag);
	ptr_radix_cluster_bw_reg(task->s_tmp, task->s_rel, fanout, bitskip, padding_num, 
		tmp_s_hist_2nd_pass, _tid, before_mid_hist_flag);
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */

	ptr_task_t *tmp_task;

	if ( _tid < BUILDPART_WRITE_THREAD_NUM ) {
		size_t r_accum_num = 0;
		size_t s_accum_num = 0;
		for (size_t idx = 0; idx < fanout; idx ++) {
			if (tmp_r_hist_2nd_pass[idx*2] > 0 && tmp_s_hist_2nd_pass[idx*2] > 0) {
				tmp_task = task_queue_get_slot_atomic(join_queue);

				tmp_task->r_rel.keyrid_num = tmp_r_hist_2nd_pass[idx*2];
				tmp_task->r_rel.keyrids = task->r_tmp.keyrids + r_accum_num + idx * padding_num;
				tmp_task->r_tmp.keyrids = task->r_rel.keyrids + r_accum_num + idx * padding_num;

				tmp_task->s_rel.keyrid_num = tmp_s_hist_2nd_pass[idx*2];
				tmp_task->s_rel.keyrids = task->s_tmp.keyrids + s_accum_num + idx * padding_num;
				tmp_task->s_tmp.keyrids = task->s_rel.keyrids + s_accum_num + idx * padding_num;

				r_accum_num += tmp_r_hist_2nd_pass[idx*2];
				s_accum_num += tmp_s_hist_2nd_pass[idx*2];

				task_queue_add_atomic(join_queue, tmp_task);

				*max_build_side_tup_num = *max_build_side_tup_num > tmp_r_hist_2nd_pass[idx*2] ? 
					*max_build_side_tup_num : tmp_r_hist_2nd_pass[idx*2];
			} else {
				r_accum_num += tmp_r_hist_2nd_pass[idx*2];
				s_accum_num += tmp_s_hist_2nd_pass[idx*2];
			}
		}
	} else {
		tmp_task = task_queue_get_slot_atomic(part_queue_2nd_pass);

		tmp_task->r_rel.keyrid_num = task->r_rel.keyrid_num;
		tmp_task->r_rel.keyrids = task->r_rel.keyrids;
		tmp_task->r_tmp.keyrids = task->r_tmp.keyrids;

		tmp_task->s_rel.keyrid_num = task->s_rel.keyrid_num;
		tmp_task->s_rel.keyrids = task->s_rel.keyrids;
		tmp_task->s_tmp.keyrids = task->s_tmp.keyrids;

		tmp_task->r_hist_2nd_pass = tmp_r_hist_2nd_pass;
		tmp_task->s_hist_2nd_pass = tmp_s_hist_2nd_pass;

#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
		tmp_task->r_global_offset = task->r_global_offset;
		tmp_task->s_global_offset = task->s_global_offset;
		tmp_task->r_swwcb_2nd_pass_mem = (void *) tmp_r_swwcb_2nd_pass_mem;
		tmp_task->s_swwcb_2nd_pass_mem = (void *) tmp_s_swwcb_2nd_pass_mem;
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */

		task_queue_add_atomic(part_queue_2nd_pass, tmp_task);
	}
}

#endif /* NUM_PASSES != 1 */


void ptr_bc_bw_reg_join(const ptr_relation_t * const r_rel, const ptr_relation_t * const s_rel, ptr_relation_t * const r_tmp, ptr_join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
#ifdef NONTEMP_STORE_MATER
	size_t slot;
	size_t slot_mod;
	row_id_pair_swwcb_t *row_id_pair_swwcb = join_arg->row_id_pair_swwcb;
	row_id_pair_t *tmp_row_id_pair_swwcb;
#else /* NONTEMP_STORE_MATER */
	row_id_pair_t tmp_row_id_pair;
#endif /* NONTEMP_STORE_MATER */

	hashkey_t max_hashkey = next_pow_2(r_rel->keyrid_num);
	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	hashkey_t tmp_hashkey;

	rowid_t *bucket, *next;

#ifdef PHJ_MBP
	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
#endif /* PHJ_MBP */

	/* memset the intermediate */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_start);
#endif /* PHJ_MBP */
	memset_localize(join_arg->intermediate, HASHKEY_T_SIZE * max_hashkey + KEY_T__SIZE * r_rel->keyrid_num);
#ifdef PHJ_MBP	
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

	/* build the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */

	bucket = (rowid_t *) (join_arg -> intermediate);
	next = (rowid_t *) (join_arg -> intermediate + HASHKEY_T_SIZE * max_hashkey);

	for (size_t idx = 0; idx < r_rel->keyrid_num;) {
		tmp_hashkey = IDHASH( r_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);
		next[idx] = bucket[tmp_hashkey];

		/* we start pos's from 1 instead of 0 */
		bucket[tmp_hashkey] = ++ idx;
	}

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_end);
#endif /* PHJ_MBP */

	/* probe the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_start);
#endif /* PHJ_MBP */

	for (size_t idx = 0; idx < s_rel->keyrid_num; idx ++ ) {
		tmp_hashkey = IDHASH(s_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);

		for (rowid_t hit = bucket[tmp_hashkey]; hit > 0; hit = next[hit-1]) {
			if (s_rel->keyrids[idx].key == r_rel->keyrids[hit-1].key) {
#ifdef NONTEMP_STORE_MATER
				slot = row_id_pair_swwcb->data.slot;
				slot_mod = slot & (ROW_ID_PAIR_PER_SWWCB - 1);
				tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
				tmp_row_id_pair_swwcb[slot_mod].r_row_id = r_rel->keyrids[hit-1].row_id;
				tmp_row_id_pair_swwcb[slot_mod].s_row_id = s_rel->keyrids[idx].row_id;
				if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
					/* non-temporal store a SWWCB */
					nontemp_store_swwcb(join_arg->mater+slot-(ROW_ID_PAIR_PER_SWWCB-1), 
						tmp_row_id_pair_swwcb);
				}
				row_id_pair_swwcb->data.slot = slot + 1;
#else /* NONTEMP_STORE_MATER */
				tmp_row_id_pair.r_row_id = r_rel->keyrids[hit-1].row_id;
				tmp_row_id_pair.s_row_id = s_rel->keyrids[idx].row_id;
				store_a_row_id_pair(join_arg->mater+(join_arg->matched_cnt), &tmp_row_id_pair);
#endif /* NONTEMP_STORE_MATER */
				(join_arg->matched_cnt) ++;
			}
		}
	}

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_end);
#endif /* PHJ_MBP */

#ifdef PHJ_MBP
	join_arg->memset_time = diff_sec(memset_start, memset_end);
	join_arg->build_time = diff_sec(build_start, build_end);
	join_arg->probe_time = diff_sec(probe_start, probe_end);
#endif /* PHJ_MBP */

}


void ptr_lp_bw_reg_join(const ptr_relation_t * const r_rel, const ptr_relation_t * const s_rel, ptr_relation_t * const r_tmp, ptr_join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
#ifdef NONTEMP_STORE_MATER
	size_t slot;
	size_t slot_mod;
	row_id_pair_swwcb_t *row_id_pair_swwcb = join_arg->row_id_pair_swwcb;
	row_id_pair_t *tmp_row_id_pair_swwcb;
#else /* NONTEMP_STORE_MATER */
	row_id_pair_t tmp_row_id_pair;
#endif /* NONTEMP_STORE_MATER */

	hashkey_t max_hashkey = next_pow_2(r_rel->keyrid_num);

	/* the & mask for hashing, for we use some leftmost bits for radix partitioning */
	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	/* the & mask for modulo to max_hashkey buckets */
	key_t_ hashmask_0 = max_hashkey - 1;

	hashkey_t tmp_hashkey;

	keyrid_t* lp_table;

#ifdef PHJ_MBP
	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
#endif /* PHJ_MBP */

	/* memset the intermediate */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_start);
#endif /* PHJ_MBP */

	memset_localize(join_arg->intermediate, KEYRID_T_SIZE * max_hashkey);

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

	/* build the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */

	lp_table = (keyrid_t *) (join_arg -> intermediate);

	for (size_t idx = 0; idx < r_rel->keyrid_num; idx ++ ) {
		tmp_hashkey = IDHASH(r_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);

		while (  (lp_table + tmp_hashkey)->key ) {									// the min_key is at least 1
			tmp_hashkey = (tmp_hashkey + 1) & hashmask_0;
		}

		store_a_keyrid(lp_table + tmp_hashkey, & r_rel->keyrids[idx]);
	}

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_end);
#endif /* PHJ_MBP */

	/* probe the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_start);
#endif /* PHJ_MBP */

	for (size_t idx = 0; idx < s_rel->keyrid_num; idx ++) {
		tmp_hashkey = IDHASH(s_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);
#ifdef TEST_SELECTIVITY
		bool flag = true;
		hashkey_t pos_0 = tmp_hashkey;


		while ( ( (lp_table + tmp_hashkey)->key ) 										// the min_key is at least 1
			&& ( (lp_table + tmp_hashkey)->key != s_rel->keyrids[idx].key)
			&& ( tmp_hashkey != pos_0 || flag) ) {
			flag = false;
			tmp_hashkey = (tmp_hashkey + 1) & hashmask_0;

		}
#ifdef NONTEMP_STORE_MATER
		slot = row_id_pair_swwcb->data.slot;
		slot_mod = slot & (ROW_ID_PAIR_PER_SWWCB - 1);
		tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;

		// tmp_row_id_pair_swwcb[slot_mod].r_row_id = ( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) ) ? 
		// 	(lp_table + tmp_hashkey)->row_id : 0;
		// tmp_row_id_pair_swwcb[slot_mod].s_row_id = ( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) ) ? 
		// 	s_rel->keyrids[idx].row_id : 0;

		tmp_row_id_pair_swwcb[slot_mod].r_row_id = (lp_table + tmp_hashkey)->row_id *
			( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) ); 

		tmp_row_id_pair_swwcb[slot_mod].s_row_id = s_rel->keyrids[idx].row_id *
			( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) );

		if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
			/* non-temporal store a SWWCB */
			nontemp_store_swwcb(join_arg->mater+slot-(ROW_ID_PAIR_PER_SWWCB-1), 
				tmp_row_id_pair_swwcb);
		}
		row_id_pair_swwcb->data.slot = slot + 1;
#else /* NONTEMP_STORE_MATER */
		// tmp_row_id_pair.r_row_id = ( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) ) ? 
		// 	(lp_table + tmp_hashkey)->row_id : 0;
		// tmp_row_id_pair.s_row_id = ( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) ) ? 
		// 	s_rel->keyrids[idx].row_id : 0;

		tmp_row_id_pair.r_row_id = (lp_table + tmp_hashkey)->row_id *
			( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) );
		tmp_row_id_pair.s_row_id = s_rel->keyrids[idx].row_id * 
			( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) );

		if ( (lp_table + tmp_hashkey)->key ) {
			store_a_row_id_pair(join_arg->mater + (join_arg->matched_cnt), &tmp_row_id_pair);
		}
#endif /* NONTEMP_STORE_MATER */
		join_arg->matched_cnt += ( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) ) ? 1: 0;

#else /* TEST_SELECTIVITY */
		while ( ( (lp_table + tmp_hashkey)->key ) 									// the min_key is at least 1
			&& ( (lp_table + tmp_hashkey)->key != s_rel->keyrids[idx].key) ) {
			tmp_hashkey = (tmp_hashkey + 1) & hashmask_0;
		}

#ifdef NONTEMP_STORE_MATER
		slot = row_id_pair_swwcb->data.slot;
		slot_mod = slot & (ROW_ID_PAIR_PER_SWWCB - 1);
		tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
		// tmp_row_id_pair_swwcb[slot_mod].r_row_id = ( (lp_table + tmp_hashkey)->key ) ? (lp_table + tmp_hashkey)->row_id : 0;
		// tmp_row_id_pair_swwcb[slot_mod].s_row_id = ( (lp_table + tmp_hashkey)->key ) ? s_rel->keyrids[idx].row_id : 0;

		tmp_row_id_pair_swwcb[slot_mod].r_row_id = (lp_table + tmp_hashkey)->row_id * ( (lp_table + tmp_hashkey)->key != 0) ;
		tmp_row_id_pair_swwcb[slot_mod].s_row_id = s_rel->keyrids[idx].row_id * ( (lp_table + tmp_hashkey)->key != 0);

		if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
			/* non-temporal store a SWWCB */
			nontemp_store_swwcb(join_arg->mater+slot-(ROW_ID_PAIR_PER_SWWCB-1), 
				tmp_row_id_pair_swwcb);
		}
		row_id_pair_swwcb->data.slot = slot + 1;
#else /* NONTEMP_STORE_MATER */
		// tmp_row_id_pair.r_row_id = ( (lp_table + tmp_hashkey)->key ) ? (lp_table + tmp_hashkey)->row_id : 0;
		// tmp_row_id_pair.s_row_id = ( (lp_table + tmp_hashkey)->key ) ? s_rel->keyrids[idx].row_id : 0;

		tmp_row_id_pair.r_row_id = (lp_table + tmp_hashkey)->row_id * ( (lp_table + tmp_hashkey)->key != 0) ;
		tmp_row_id_pair.s_row_id = s_rel->keyrids[idx].row_id * ( (lp_table + tmp_hashkey)->key != 0);

		if ( (lp_table + tmp_hashkey)->key ) {
			store_a_row_id_pair(join_arg->mater + (join_arg->matched_cnt), &tmp_row_id_pair);
		}
#endif /* NONTEMP_STORE_MATER */
		join_arg->matched_cnt += ( (lp_table + tmp_hashkey)->key ) ? 1 : 0;

#endif/* TEST_SELECTIVITY */
	}

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_end);
#endif /* PHJ_MBP */

#ifdef PHJ_MBP
	join_arg->memset_time = diff_sec(memset_start, memset_end);
	join_arg->build_time = diff_sec(build_start, build_end);
	join_arg->probe_time = diff_sec(probe_start, probe_end);
#endif /* PHJ_MBP */

}


void ptr_sc_bw_reg_join(const ptr_relation_t * const r_rel, const ptr_relation_t * const s_rel, ptr_relation_t * const r_tmp, ptr_join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
#ifdef NONTEMP_STORE_MATER
	size_t slot;
	size_t slot_mod;
	row_id_pair_swwcb_t *row_id_pair_swwcb = join_arg->row_id_pair_swwcb;
	row_id_pair_t *tmp_row_id_pair_swwcb;
#else /* NONTEMP_STORE_MATER */
	row_id_pair_t tmp_row_id_pair;
#endif /* NONTEMP_STORE_MATER */

	hashkey_t max_hashkey = next_pow_2(r_rel->keyrid_num)  / PHJ_SC_BUCKET_CAPACITY;
	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	hashkey_t tmp_hashkey;

	ptr_phj_sc_bkt_t *buckets, *free, *curr, *nxt, *tmp;
	keyrid_t *dest;
	size_t free_idx = 0;

#ifdef PHJ_MBP
	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
#endif /* PHJ_MBP */

	/* memset the intermediate */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_start);
#endif /* PHJ_MBP */
	memset_localize(join_arg->intermediate, PTR_PHJ_SC_BKT_T_SIZE * max_hashkey * 2);
#ifdef PHJ_MBP	
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

	/* build the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */

	buckets = (ptr_phj_sc_bkt_t *) (join_arg -> intermediate);
	free = (ptr_phj_sc_bkt_t *) (join_arg -> intermediate + PTR_PHJ_SC_BKT_T_SIZE * max_hashkey);

	for (size_t idx = 0; idx < r_rel->keyrid_num; idx ++) {
		tmp_hashkey = IDHASH( r_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);

		curr = buckets + tmp_hashkey;
		nxt = curr->next;

		if (curr->count == PHJ_SC_BUCKET_CAPACITY) {
			if (!nxt || nxt->count == PHJ_SC_BUCKET_CAPACITY) {
				tmp = free + free_idx;
				free_idx ++;
				curr->next = tmp;
				tmp->next = nxt;
				tmp->count = 1;
				dest = tmp->keyrids;
			} else {
				dest = nxt->keyrids + nxt->count;
				(nxt->count) ++;
			}
		} else {
			dest = curr->keyrids + curr->count;
			(curr->count) ++;
		}

		store_a_keyrid(dest, &r_rel->keyrids[idx]);
	}

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_end);
#endif /* PHJ_MBP */

	/* probe the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_start);
#endif /* PHJ_MBP */

	for (size_t idx = 0; idx < s_rel->keyrid_num; idx ++ ) {
		tmp_hashkey = IDHASH(s_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);

		tmp = buckets + tmp_hashkey;

		do {
			for (size_t jdx = 0; jdx < tmp->count; jdx ++) {
				if (s_rel->keyrids[idx].key == tmp->keyrids[jdx].key) {
#ifdef NONTEMP_STORE_MATER
					slot = row_id_pair_swwcb->data.slot;
					slot_mod = slot & (ROW_ID_PAIR_PER_SWWCB - 1);
					tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
					tmp_row_id_pair_swwcb[slot_mod].r_row_id = tmp->keyrids[jdx].row_id;
					tmp_row_id_pair_swwcb[slot_mod].s_row_id = s_rel->keyrids[idx].row_id;
					if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
						/* non-temporal store a SWWCB */
						nontemp_store_swwcb(join_arg->mater+slot-(ROW_ID_PAIR_PER_SWWCB-1), 
							tmp_row_id_pair_swwcb);
					}
					row_id_pair_swwcb->data.slot = slot + 1;

#else /* NONTEMP_STORE_MATER */

					tmp_row_id_pair.r_row_id = tmp->keyrids[jdx].row_id;
					tmp_row_id_pair.s_row_id = s_rel->keyrids[idx].row_id;
					store_a_row_id_pair(join_arg->mater+(join_arg->matched_cnt), &tmp_row_id_pair);
#endif /* NONTEMP_STORE_MATER */
					(join_arg -> matched_cnt) ++;
				}
			}

			tmp = tmp->next;
		} while (tmp);

	}

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_end);
#endif /* PHJ_MBP */

#ifdef PHJ_MBP
	join_arg->memset_time = diff_sec(memset_start, memset_end);
	join_arg->build_time = diff_sec(build_start, build_end);
	join_arg->probe_time = diff_sec(probe_start, probe_end);
#endif /* PHJ_MBP */

}


void ptr_hr_bw_reg_join(const ptr_relation_t * const r_rel, const ptr_relation_t * const s_rel, ptr_relation_t * const r_tmp, ptr_join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
#ifdef NONTEMP_STORE_MATER
	size_t slot;
	size_t slot_mod;
	row_id_pair_swwcb_t *row_id_pair_swwcb = join_arg->row_id_pair_swwcb;
	row_id_pair_t *tmp_row_id_pair_swwcb;
#else /* NONTEMP_STORE_MATER */
	row_id_pair_t tmp_row_id_pair;
#endif /* NONTEMP_STORE_MATER */

	hashkey_t max_hashkey = next_pow_2(r_rel->keyrid_num);

	/* 4 times smaller hash table index, thus 4 time more comparison than bc_join * lp_join */
	// max_hashkey >>= PTR_HR_HRO_FACTOR;
	// if (max_hashkey < (1<<PTR_HR_HRO_FACTOR)) {
	// 	max_hashkey = (1<<PTR_HR_HRO_FACTOR);
	// }

	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	hashkey_t tmp_hashkey;

	size_t *hist;
	size_t accum_num = 0;
#ifdef PHJ_MBP
	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
#endif /* PHJ_MBP */

	/* memset the intermediate */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_start);
#endif /* PHJ_MBP */
	memset_localize(join_arg->intermediate, HASHKEY_T_SIZE * max_hashkey);
#ifdef PHJ_MBP	
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

	/* build the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */

	hist = (hashkey_t *) (join_arg -> intermediate);

	for (size_t idx = 0; idx < r_rel->keyrid_num; idx ++) {
		tmp_hashkey = IDHASH(r_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);
		hist[tmp_hashkey] ++;
	}

	for (size_t idx = 0; idx < max_hashkey; idx ++) {
		accum_num += hist[idx];
		hist[idx] = accum_num - hist[idx];
	}

	for (size_t idx = 0; idx < r_rel->keyrid_num; idx ++) {
		tmp_hashkey = IDHASH(r_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);
		store_a_keyrid(r_tmp->keyrids + hist[tmp_hashkey], & r_rel->keyrids[idx]);
		hist[tmp_hashkey] ++;
	}

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_end);
#endif /* PHJ_MBP */

	/* probe the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_start);
#endif /* PHJ_MBP */
	size_t start_idx;
	size_t end_idx;

	for (size_t idx = 0; idx < s_rel->keyrid_num; idx ++) {
		tmp_hashkey = IDHASH(s_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);
		start_idx = hist[ tmp_hashkey-(tmp_hashkey!=0) ]*(tmp_hashkey!=0);
		end_idx = hist[tmp_hashkey];
	
		for (size_t jdx = start_idx; jdx < end_idx; jdx ++) {
			if (s_rel->keyrids[idx].key == r_tmp->keyrids[jdx].key) {
#ifdef NONTEMP_STORE_MATER
				slot = row_id_pair_swwcb->data.slot;
				slot_mod = slot & (ROW_ID_PAIR_PER_SWWCB - 1);
				tmp_row_id_pair_swwcb = (row_id_pair_t *) row_id_pair_swwcb;
				tmp_row_id_pair_swwcb[slot_mod].r_row_id = r_tmp->keyrids[jdx].row_id;
				tmp_row_id_pair_swwcb[slot_mod].s_row_id = s_rel->keyrids[idx].row_id;
				if (slot_mod == ROW_ID_PAIR_PER_SWWCB - 1) {
					/* non-temporal store a SWWCB */
					nontemp_store_swwcb(join_arg->mater+slot-(ROW_ID_PAIR_PER_SWWCB-1), 
						tmp_row_id_pair_swwcb);
				}
				row_id_pair_swwcb->data.slot = slot + 1;
#else /* NONTEMP_STORE_MATER */
				tmp_row_id_pair.r_row_id = r_tmp->keyrids[jdx].row_id;
				tmp_row_id_pair.s_row_id = s_rel->keyrids[idx].row_id;
				store_a_row_id_pair(join_arg->mater+(join_arg->matched_cnt), &tmp_row_id_pair);
#endif /* NONTEMP_STORE_MATER */
				(join_arg->matched_cnt) ++;
			}
		}
	}

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_end);
#endif /* PHJ_MBP */

#ifdef PHJ_MBP
	join_arg->memset_time = diff_sec(memset_start, memset_end);
	join_arg->build_time = diff_sec(build_start, build_end);
	join_arg->probe_time = diff_sec(probe_start, probe_end);
#endif /* PHJ_MBP */

}


// void ptr_hro_bw_reg_join(const ptr_relation_t * const r_rel, const ptr_relation_t * const s_rel, ptr_relation_t * const r_tmp, ptr_join_arg_t * const join_arg) {
// 	join_arg -> matched_cnt = 0;
// 	join_arg -> checksum = 0;
// 	// row_id_pair_t tmp_row_id_pair;

// 	hashkey_t max_hashkey = next_pow_2(r_rel->keyrid_num);

// 	/* shrink the hashtable size according to AVX512_SIZE/TUPLE_T_SIZE */
// 	max_hashkey >>= PTR_HR_HRO_FACTOR;
// 	if (max_hashkey < (1<<PTR_HR_HRO_FACTOR)) {
// 		max_hashkey = (1<<PTR_HR_HRO_FACTOR);
// 	}
// 	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
// 	hashkey_t tmp_hashkey;

// 	size_t *hist;
// 	size_t accum_num = 0;
// #ifdef PHJ_MBP
// 	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
// #endif /* PHJ_MBP */

// #ifdef PHJ_MBP
// 	clock_gettime(CLOCK_REALTIME, &memset_start);
// #endif /* PHJ_MBP */

// 	memset_localize(join_arg->intermediate, HASHKEY_T_SIZE * max_hashkey + AVX512_SIZE * 3);				// 3 buffers for hashkeys, keys, and row_ids
// 	/* memset the padding region, in case the padding region happens to hold some matching keys */
// 	memset_localize(r_tmp->keyrids + r_rel->keyrid_num, AVX512_SIZE);

// #ifdef PHJ_MBP	
// 	clock_gettime(CLOCK_REALTIME, &memset_end);
// #endif /* PHJ_MBP */

// #ifdef PHJ_MBP
// 	clock_gettime(CLOCK_REALTIME, &build_start);
// #endif /* PHJ_MBP */
// 	hist = (size_t *) (join_arg -> intermediate);

// 	for (size_t idx = 0; idx < r_rel->keyrid_num; idx ++) {
// 		tmp_hashkey = IDHASH(r_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);
// 		hist[tmp_hashkey] ++;
// 	}

// 	for (size_t idx = 0; idx < max_hashkey; idx ++) {
// 		accum_num += hist[idx];
// 		hist[idx] = accum_num - hist[idx];
// 	}

// 	for (size_t idx = 0; idx < r_rel->keyrid_num; idx ++) {
// 		tmp_hashkey = IDHASH(r_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);
// 		store_a_keyrid(r_tmp->keyrids + hist[tmp_hashkey], & r_rel->keyrids[idx]);
// 		hist[tmp_hashkey] ++;
// 	}
// #ifdef PHJ_MBP
// 	clock_gettime(CLOCK_REALTIME, &build_end);
// #endif /* PHJ_MBP */


// #ifdef PHJ_MBP
// 	clock_gettime(CLOCK_REALTIME, &probe_start);
// #endif /* PHJ_MBP */
// 	hashkey_t *hashkey_buffer = (hashkey_t *) (join_arg -> intermediate + HASHKEY_T_SIZE * max_hashkey);
// 	key_t_ *key_buffer = (key_t_ *) (hashkey_buffer + AVX512_SIZE/HASHKEY_T_SIZE);							// AVX512 can hold 8 hashkeys
// 	rowid_t *rowid_buffer = (rowid_t *) (key_buffer + AVX512_SIZE/KEY_T__SIZE);								// AVX512 can hold 8 keys

// 	__m512i query_key;
// 	__m512i query_rowid;
// 	__m512i tup_array;

// 	__m512i tmp_avx_checksum;
// 	__m512i avx_checksum = _mm512_set1_epi64(0);
// 	__m512i avx_matched_cnt = _mm512_set1_epi64(0);

// 	__mmask8 res_mask;
// 	__m512i all_one = _mm512_set1_epi64(1);

// 	size_t start_idx;
// 	size_t end_idx;

// 	for (size_t idx = 0; idx < s_rel->keyrid_num / (PTR_AVX512_UNIT_NUM); idx ++) {

// 		for (int jdx = 0; jdx < PTR_AVX512_UNIT_NUM; jdx ++) {
// 			key_buffer[jdx] = s_rel->keyrids[idx*PTR_AVX512_UNIT_NUM+jdx].key;
// 			rowid_buffer[jdx] = s_rel->keyrids[idx*PTR_AVX512_UNIT_NUM+jdx].row_id;
// 			tmp_hashkey = IDHASH(key_buffer[jdx], hashmask, NUM_RADIX_BITS);
// 			hashkey_buffer[jdx] = tmp_hashkey;
//			// ptr_prefetch(r_tmp->keyrids + hist[tmp_hashkey]);
//			ptr_prefetch(r_tmp->keyrids + hist[ tmp_hashkey-(tmp_hashkey!=0) ]*(tmp_hashkey!=0) );
// 		}

// 		for (int jdx = 0; jdx < PTR_AVX512_UNIT_NUM; jdx ++) {
//			start_idx = hist[ hashkey_buffer[jdx] - (hashkey_buffer[jdx]!=0) ] * (hashkey_buffer[jdx]!=0);
// 			end_idx = hist[ hashkey_buffer[jdx] ];
// 			query_key = _mm512_set1_epi64(key_buffer[jdx]);
// 			query_rowid = _mm512_set1_epi64(rowid_buffer[jdx]);

// 			while (start_idx < end_idx) {
// 				tup_array = _mm512_loadu_si512(r_tmp->keyrids + start_idx);

// 				res_mask = _mm512_cmpeq_epi64_mask(query_key, tup_array);

// 				avx_matched_cnt = _mm512_mask_add_epi64(avx_matched_cnt, res_mask, avx_matched_cnt, all_one);

// 				tmp_avx_checksum = _mm512_maskz_add_epi64(res_mask << 1, tup_array, query_rowid);
// 				avx_checksum = _mm512_mask_add_epi64(avx_checksum, res_mask << 1, avx_checksum, tmp_avx_checksum);
	
// 				start_idx += PTR_AVX512_UNIT_NUM;
// 			}
// 		}
// 	}


// 	for (size_t idx = s_rel->keyrid_num - (s_rel->keyrid_num % (PTR_AVX512_UNIT_NUM)); idx < s_rel->keyrid_num; idx ++) {

// 		tmp_hashkey = IDHASH(s_rel->keyrids[idx].key, hashmask, NUM_RADIX_BITS);

// 		query_key = _mm512_set1_epi64(s_rel->keyrids[idx].key);
// 		query_rowid = _mm512_set1_epi64(s_rel->keyrids[idx].row_id);

//		start_idx = hist[ tmp_hashkey - (tmp_hashkey!=0) ]*(tmp_hashkey!=0);
// 		end_idx = hist[ tmp_hashkey ];

// 		while (start_idx < end_idx) {
// 			tup_array = _mm512_loadu_si512(r_tmp->keyrids + start_idx);

// 			res_mask = _mm512_cmpeq_epi64_mask(query_key, tup_array);
// 			avx_matched_cnt = _mm512_mask_add_epi64(avx_matched_cnt, res_mask, avx_matched_cnt, all_one);

// 			tmp_avx_checksum = _mm512_maskz_add_epi64(res_mask << 1, tup_array, query_rowid);
// 			avx_checksum = _mm512_mask_add_epi64(avx_checksum, res_mask << 1, avx_checksum, tmp_avx_checksum);

// 			start_idx += PTR_AVX512_UNIT_NUM;
// 		}
// 	}


// 	__m256i matched_cnt_1 = _mm512_extracti32x8_epi32(avx_matched_cnt, 0);
// 	__m256i matched_cnt_2 = _mm512_extracti32x8_epi32(avx_matched_cnt, 1);
// 	join_arg->matched_cnt += _mm256_extract_epi64(matched_cnt_1, 0) + _mm256_extract_epi64(matched_cnt_1, 2) +
// 		_mm256_extract_epi64(matched_cnt_2, 0) + _mm256_extract_epi64(matched_cnt_2, 2);
	
// 	__m256i checksum_1 = _mm512_extracti32x8_epi32(avx_checksum, 0);
// 	__m256i checksum_2 = _mm512_extracti32x8_epi32(avx_checksum, 1);
// 	join_arg->checksum += _mm256_extract_epi64(checksum_1, 1) + _mm256_extract_epi64(checksum_1, 3) +
// 		_mm256_extract_epi64(checksum_2, 1) + _mm256_extract_epi64(checksum_2, 3);


// #ifdef PHJ_MBP
// 	clock_gettime(CLOCK_REALTIME, &probe_end);
// #endif /* PHJ_MBP */


// #ifdef PHJ_MBP
// 	join_arg->memset_time = diff_sec(memset_start, memset_end);
// 	join_arg->build_time = diff_sec(build_start, build_end);
// 	join_arg->probe_time = diff_sec(probe_start, probe_end);
// #endif /* PHJ_MBP */
// }


void *ptr_phj_thread_buildpart_bw_reg(void * param) {
	ptr_phj_arg_bw_reg_t *phj_arg = (ptr_phj_arg_bw_reg_t *) param;

#ifdef WARMUP
	warmup_localize(phj_arg->r_rel.tuples, phj_arg->r_rel.tuple_num);
	warmup_localize(phj_arg->s_rel.tuples, phj_arg->s_rel.tuple_num);
#endif /* WARMUP */

	/* eliminate the page faults of the hash table */
	size_t memset_num_keyrids_thr;
	size_t memset_num_keyrids_size;
	memset_num_keyrids_thr = phj_arg->r_tmp.keyrid_num / BUILDPART_THREAD_NUM;
	memset_num_keyrids_size = (phj_arg->_tid == BUILDPART_THREAD_NUM - 1) ? KEYRID_T_SIZE * ( phj_arg->r_tmp.keyrid_num - phj_arg->_tid * memset_num_keyrids_thr ) : KEYRID_T_SIZE * memset_num_keyrids_thr;
	memset_localize(phj_arg->r_tmp.keyrids + phj_arg->_tid * memset_num_keyrids_thr, memset_num_keyrids_size);
	memset_localize(phj_arg->r_ext.keyrids + phj_arg->_tid * memset_num_keyrids_thr, memset_num_keyrids_size);
	memset_num_keyrids_thr = phj_arg->s_tmp.keyrid_num / BUILDPART_THREAD_NUM;
	memset_num_keyrids_size = (phj_arg->_tid == BUILDPART_THREAD_NUM - 1) ? KEYRID_T_SIZE * ( phj_arg->s_tmp.keyrid_num - phj_arg->_tid * memset_num_keyrids_thr ) : KEYRID_T_SIZE * memset_num_keyrids_thr;
	memset_localize(phj_arg->s_tmp.keyrids + phj_arg->_tid * memset_num_keyrids_thr, memset_num_keyrids_size);
	memset_localize(phj_arg->s_ext.keyrids + phj_arg->_tid * memset_num_keyrids_thr, memset_num_keyrids_size);

	int rv;

	size_t max_build_side_tup_num = 0;

	ptr_task_t *task;
	ptr_task_queue_t *part_queue = phj_arg->part_queue;

#if SKEW_HANDLING
	ptr_task_queue_t *skew_queue = phj_arg->skew_queue;
	ptr_task_t **skewtask = phj_arg->skewtask;
#endif /* SKEW_HANDLING */
	ptr_part_bw_reg_t part;

	part._tid = phj_arg->_tid;
	part.barrier = phj_arg->barrier;

	phj_arg->r_hist[phj_arg->_tid] = (size_t *) alloc_memory( sizeof(size_t) * 2 * FANOUT_PASS1, CACHELINE_SIZE);
	phj_arg->s_hist[phj_arg->_tid] = (size_t *) alloc_memory( sizeof(size_t) * 2 * FANOUT_PASS1, CACHELINE_SIZE);
	
#ifdef OUTPUT_OFFSET_NVM
	phj_arg->r_output_offset[phj_arg->_tid] = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS1);
	phj_arg->s_output_offset[phj_arg->_tid] = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	phj_arg->r_output_offset[phj_arg->_tid] = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS1, CACHELINE_SIZE);
	phj_arg->s_output_offset[phj_arg->_tid] = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS1, CACHELINE_SIZE);
#endif /* OUTPUT_OFFSET_NVM */

	size_t *r_output_offset = phj_arg->r_output_offset[phj_arg->_tid];
	size_t *s_output_offset = phj_arg->s_output_offset[phj_arg->_tid];

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM
	phj_arg->swwcb_s[phj_arg->_tid] = (ptr_swwcb_t *) new_alloc_nvm(sizeof(ptr_swwcb_t) * FANOUT_PASS1);
#else /* PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	phj_arg->swwcb_s[phj_arg->_tid] = (ptr_swwcb_t *) alloc_aligned_dram(sizeof(ptr_swwcb_t) * FANOUT_PASS1, CACHELINE_SIZE);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	ptr_swwcb_t *swwcb = phj_arg->swwcb_s[phj_arg->_tid];
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */


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
	if (phj_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */

	BARRIER_ARRIVE(phj_arg->barrier, rv);

	/**************** PARTITION PHASE ****************/
	if (phj_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &phj_arg->part_start);
	}

	/**************** PARTITIONING RUNTIME INCLUDES THE RUNTIME OF PRE-PARTITION MEMSET ****************/
	
	// // /* eliminate the page faults of the hash table */
	// size_t memset_num_keyrids_thr;
	// size_t memset_num_keyrids_size;
	// memset_num_keyrids_thr = phj_arg->r_tmp.keyrid_num / BUILDPART_THREAD_NUM;
	// memset_num_keyrids_size = (phj_arg->_tid == BUILDPART_THREAD_NUM - 1) ? KEYRID_T_SIZE * ( phj_arg->r_tmp.keyrid_num - phj_arg->_tid * memset_num_keyrids_thr ) : KEYRID_T_SIZE * memset_num_keyrids_thr;
	// memset_localize(phj_arg->r_tmp.keyrids + phj_arg->_tid * memset_num_keyrids_thr, memset_num_keyrids_size);
	// memset_localize(phj_arg->r_ext.keyrids + phj_arg->_tid * memset_num_keyrids_thr, memset_num_keyrids_size);
	// memset_num_keyrids_thr = phj_arg->s_tmp.keyrid_num / BUILDPART_THREAD_NUM;
	// memset_num_keyrids_size = (phj_arg->_tid == BUILDPART_THREAD_NUM - 1) ? KEYRID_T_SIZE * ( phj_arg->s_tmp.keyrid_num - phj_arg->_tid * memset_num_keyrids_thr ) : KEYRID_T_SIZE * memset_num_keyrids_thr;
	// memset_localize(phj_arg->s_tmp.keyrids + phj_arg->_tid * memset_num_keyrids_thr, memset_num_keyrids_size);
	// memset_localize(phj_arg->s_ext.keyrids + phj_arg->_tid * memset_num_keyrids_thr, memset_num_keyrids_size);

	// /* currently, we only consider the uniform pkfk scenario */
	// size_t tot_row_id_pair_num = phj_arg->org_relS->tuple_num * AVG_MATCH_NUM_PER_THR_SCALING_FACTOR / BUILDPART_THREAD_NUM;
	// row_id_pair_t *mater = (row_id_pair_t *) alloc_memory(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE, CACHELINE_SIZE);
	// memset_localize(mater, ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);


	memset_localize(phj_arg->r_hist[phj_arg->_tid] , sizeof(size_t) * 2 * FANOUT_PASS1);
	memset_localize(phj_arg->s_hist[phj_arg->_tid] , sizeof(size_t) * 2 * FANOUT_PASS1);

#ifdef OUTPUT_OFFSET_NVM
	pmem_memset_persist(r_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
	pmem_memset_persist(s_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	memset(r_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
	memset(s_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
#endif /* OUTPUT_OFFSET_NVM */

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM
	pmem_memset_persist(swwcb, 0, sizeof(ptr_swwcb_t) * FANOUT_PASS1);
#else /* PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	memset(swwcb, 0, sizeof(ptr_swwcb_t) * FANOUT_PASS1);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM */
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

	/**************** END OF PRE-PARTITION MEMSET ****************/
	/* wait for all threads finish the memset for immediates */
#ifdef SYNCSTATS
	SYNC_TIMER(phj_arg->localtimer->sync_prepart_memset);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_prepart_memset);
#endif /* SYNCSTATS */

	/**************** FIRST PASS OF PARTITIONING ****************/
	part.bitskip = 0;
	part.hashmask = FANOUT_PASS1-1;
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	part.swwcb_s = phj_arg->swwcb_s;
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

	/**************** PARTITIONING R ****************/
	part.rel = phj_arg->r_rel;
	part.tmp = phj_arg->r_tmp;
	part.hist = phj_arg->r_hist;
	part.output_offset = phj_arg->r_output_offset;;
	part.rel_flag = 0;

	part.tuple_org = phj_arg->org_relR->tuples;
	part.thr_tup_num =  phj_arg->r_thr_tup_num;
	part.last_thr_tup_num = phj_arg->r_last_thr_tup_num;

#ifdef SYNCSTATS
	part.sync_local_hist = & phj_arg->localtimer->sync_1st_hist[0];
	part.sync_global_hist = & phj_arg->globaltimer->sync_1st_hist[0];
	part.sync_local_prfx_sum = & phj_arg->localtimer->sync_1st_prfx_sum[0];
	part.sync_global_prfx_sum = & phj_arg->globaltimer->sync_1st_prfx_sum[0];
#endif /* SYNCSTATS */

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART

#ifdef SYNCSTATS
	part.sync_local_swwcb = & phj_arg->localtimer->sync_1st_swwcb[0];
	part.sync_global_swwcb = & phj_arg->globaltimer->sync_1st_swwcb[0];
#endif /* SYNCSTATS */

	ptr_parallel_radix_partition_optimized_first_bw_reg(&part, FANOUT_PASS2 * PTR_PADDING_UNIT_NUM, 0);
#else /* PTR_USE_SWWCB_OPTIMIZED_PART */
	ptr_parallel_radix_partition_first_bw_reg(&part, FANOUT_PASS2 * PTR_PADDING_UNIT_NUM);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */


	/**************** PARTITIONING S ****************/
	part.rel = phj_arg->s_rel;
	part.tmp = phj_arg->s_tmp;
	part.hist = phj_arg->s_hist;
	part.output_offset = phj_arg->s_output_offset;
	part.rel_flag = 1;

	part.tuple_org = phj_arg->org_relS->tuples;
	part.thr_tup_num =  phj_arg->s_thr_tup_num;
	part.last_thr_tup_num = phj_arg->s_last_thr_tup_num;

#ifdef SYNCSTATS
	part.sync_local_hist = & phj_arg->localtimer->sync_1st_hist[1];
	part.sync_global_hist = & phj_arg->globaltimer->sync_1st_hist[1];
	part.sync_local_prfx_sum = & phj_arg->localtimer->sync_1st_prfx_sum[1];
	part.sync_global_prfx_sum = & phj_arg->globaltimer->sync_1st_prfx_sum[1];
#endif /* SYNCSTATS */

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART

#ifdef SYNCSTATS
	part.sync_local_swwcb = & phj_arg->localtimer->sync_1st_swwcb[1];
	part.sync_global_swwcb = & phj_arg->globaltimer->sync_1st_swwcb[1];
#endif /* SYNCSTATS */

	ptr_parallel_radix_partition_optimized_first_bw_reg(&part, FANOUT_PASS2 * PTR_PADDING_UNIT_NUM, 0);
#else /* PTR_USE_SWWCB_OPTIMIZED_PART */
	ptr_parallel_radix_partition_first_bw_reg(&part, FANOUT_PASS2 * PTR_PADDING_UNIT_NUM);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */


	/* wait for all threads finish the 1st pass partitioning */
#ifdef SYNCSTATS
	SYNC_TIMER(phj_arg->localtimer->sync_1st_pass);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_1st_pass);
#endif /* SYNCSTATS */

	/********** end of 1st partitioning phase ******************/

	/* 3. first thread creates partitioning tasks for 2nd pass */
	size_t r_num_keyrid;
	size_t s_num_keyrid;
	if (phj_arg->_tid == 0) {
		for (size_t idx = 0; idx < FANOUT_PASS1; idx ++) {

			/* within the first thread, thus no memory leak */
			r_num_keyrid = (idx == (FANOUT_PASS1-1)) ?
				phj_arg->datameta->r_tuple_num - r_output_offset[idx] + idx * FANOUT_PASS2 * PTR_PADDING_UNIT_NUM :
				r_output_offset[idx+1] - r_output_offset[idx] - FANOUT_PASS2 * PTR_PADDING_UNIT_NUM;

			s_num_keyrid = (idx == (FANOUT_PASS1-1)) ?
				phj_arg->datameta->s_tuple_num - s_output_offset[idx] + idx * FANOUT_PASS2 * PTR_PADDING_UNIT_NUM :
				s_output_offset[idx+1] - s_output_offset[idx] - FANOUT_PASS2 * PTR_PADDING_UNIT_NUM;

#ifdef SKEW_HANDLING
			if (r_num_keyrid > THRESHOLD_0 || s_num_keyrid > THRESHOLD_0) {
		
				task = task_queue_get_slot(skew_queue);

				task->r_rel.keyrids = phj_arg->r_tmp.keyrids + r_output_offset[idx];
				task->r_rel.keyrid_num = r_num_keyrid;
				task->r_tmp.keyrids = phj_arg->r_ext.keyrids + r_output_offset[idx];

				task->s_rel.keyrids = phj_arg->s_tmp.keyrids + s_output_offset[idx];
				task->s_rel.keyrid_num = s_num_keyrid;
				task->s_tmp.keyrids = phj_arg->s_ext.keyrids + s_output_offset[idx];

				task->r_global_offset = r_output_offset[idx];
				task->s_global_offset = s_output_offset[idx];

				task_queue_add(skew_queue, task);

			} else
#endif /* SKEW_HANDLING */
			if (r_num_keyrid > 0 || s_num_keyrid > 0) {
				task = task_queue_get_slot(part_queue);

				task->r_rel.keyrids = phj_arg->r_tmp.keyrids + r_output_offset[idx];
				task->r_rel.keyrid_num = r_num_keyrid;
				task->r_tmp.keyrids = phj_arg->r_ext.keyrids + r_output_offset[idx];

				task->s_rel.keyrids = phj_arg->s_tmp.keyrids + s_output_offset[idx];
				task->s_rel.keyrid_num = s_num_keyrid;
				task->s_tmp.keyrids = phj_arg->s_ext.keyrids + s_output_offset[idx];

#if NUM_PASSES != 1
				task->r_global_offset = r_output_offset[idx];
				task->s_global_offset = s_output_offset[idx];
#endif /* NUM_PASSES != 1 */

				task_queue_add(part_queue, task);

				max_build_side_tup_num = max_build_side_tup_num >= r_num_keyrid ? max_build_side_tup_num : r_num_keyrid;
			}

		}
	}

	/* wait for the first thread add tasks to respective queues */
#ifdef SYNCSTATS
	SYNC_TIMER(phj_arg->localtimer->sync_1st_pass_queue);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_1st_pass_queue);
#endif /* SYNCSTATS */

	/************ SECOND PASS OF PARTITIONING ********************/
#if NUM_PASSES == 2
	/* since we have the 2nd pass, no need to keep this value of the 1st pass */
	max_build_side_tup_num = 0;

	while ( ( task = task_queue_get_atomic(part_queue) ) ) {
		ptr_serial_radix_partition_bw_reg(task, phj_arg->join_queue, 
			phj_arg->part_queue_2nd_pass, FANOUT_PASS2, 
			NUM_RADIX_BITS / NUM_PASSES, &max_build_side_tup_num, 
			PTR_PADDING_UNIT_NUM, phj_arg->_tid, 1
		);
	}

#ifdef SYNCSTATS
	SYNC_TIMER(phj_arg->localtimer->sync_2nd_pass_mid_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_2nd_pass_mid_hist);
#endif /* SYNCSTATS */

	if (phj_arg->_tid < BUILDPART_WRITE_THREAD_NUM) {
		while ( ( task = task_queue_get_atomic(phj_arg->part_queue_2nd_pass) ) ) {
			ptr_serial_radix_partition_bw_reg(task, phj_arg->join_queue, 
				NULL, FANOUT_PASS2, NUM_RADIX_BITS / NUM_PASSES, 
				&max_build_side_tup_num, PTR_PADDING_UNIT_NUM, 
				phj_arg->_tid, 0
			);
		}
	}

	/* store the value to the respective data slot of the sharing data structure */
	phj_arg->max_build_side_tup_num[phj_arg->_tid] = max_build_side_tup_num;
#endif /* NUM_PASSES == 2 */

	/* wait for all threads finish the partitioning phase */
#ifdef SYNCSTATS
	SYNC_TIMER(phj_arg->localtimer->sync_part);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_part);
#endif /* SYNCSTATS */


#ifdef SKEW_HANDLING
	/* SAME CONFIGURATIONS AS SECOND PASS PARTITIONING */
	part.bitskip = NUM_RADIX_BITS / NUM_PASSES;
	part.hashmask = FANOUT_PASS2-1;

	dealloc_memory(phj_arg->r_output_offset[phj_arg->_tid], sizeof(size_t) * FANOUT_PASS1);
	dealloc_memory(phj_arg->s_output_offset[phj_arg->_tid], sizeof(size_t) * FANOUT_PASS1);
#ifdef OUTPUT_OFFSET_NVM
	phj_arg->r_output_offset[phj_arg->_tid] = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS2 );
	phj_arg->s_output_offset[phj_arg->_tid] = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS2 );
#else /* OUTPUT_OFFSET_NVM */
	phj_arg->r_output_offset[phj_arg->_tid] = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS2, CACHELINE_SIZE);
	phj_arg->s_output_offset[phj_arg->_tid] = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS2, CACHELINE_SIZE);
#endif /* OUTPUT_OFFSET_NVM */
	r_output_offset = phj_arg->r_output_offset[phj_arg->_tid];
	s_output_offset = phj_arg->s_output_offset[phj_arg->_tid];

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	dealloc_memory( phj_arg->swwcb_s[phj_arg->_tid], sizeof(ptr_swwcb_t) * FANOUT_PASS1);
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM
	phj_arg->swwcb_s[phj_arg->_tid] = (ptr_swwcb_t *) new_alloc_nvm(sizeof(ptr_swwcb_t) * FANOUT_PASS2);
	pmem_memset_persist(phj_arg->swwcb_s[phj_arg->_tid], 0, sizeof(ptr_swwcb_t) * FANOUT_PASS2);
#else /* PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	phj_arg->swwcb_s[phj_arg->_tid] = (ptr_swwcb_t *) alloc_aligned_dram(sizeof(ptr_swwcb_t) * FANOUT_PASS2, CACHELINE_SIZE);
	memset(phj_arg->swwcb_s[phj_arg->_tid], 0, sizeof(ptr_swwcb_t) * FANOUT_PASS2);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	swwcb = phj_arg->swwcb_s[phj_arg->_tid];
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

	dealloc_memory(phj_arg->r_hist[phj_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS1);
	dealloc_memory(phj_arg->s_hist[phj_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS1);

	phj_arg->r_hist[phj_arg->_tid] = (size_t *) alloc_memory( sizeof(size_t) * 2 * FANOUT_PASS2, CACHELINE_SIZE);
	phj_arg->s_hist[phj_arg->_tid] = (size_t *) alloc_memory( sizeof(size_t) * 2 * FANOUT_PASS2, CACHELINE_SIZE);

	size_t skew_hand_num_per_thr;
	size_t skew_hand_r_num_keyrid, skew_hand_s_num_keyrid;
	ptr_task_t *skew_hand_task;

	while(1) {

		/* an explicit code segment to aggregate the accumulated idle time */
#ifdef SYNCSTATS

		/* must add a barrier to ensure the execution of the execution of 
		   SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_skew_part);
		   though it incurs additional overhead */
		BARRIER_ARRIVE(phj_arg->barrier, rv);
		phj_arg->sync_idle_skew_taskfetch_acc += diff_sec(phj_arg->localtimer->sync_skew_taskfetch, 
			phj_arg->globaltimer->sync_skew_taskfetch);
		phj_arg->sync_idle_skew_prepart_memset_acc += diff_sec(phj_arg->localtimer->sync_skew_prepart_memset, 
			phj_arg->globaltimer->sync_skew_prepart_memset);
		phj_arg->sync_idle_skew_hist_acc[0] += diff_sec(phj_arg->localtimer->sync_skew_hist[0],
			phj_arg->globaltimer->sync_skew_hist[0]);
		phj_arg->sync_idle_skew_prfx_sum_acc[0] += diff_sec(phj_arg->localtimer->sync_skew_prfx_sum[0],
			phj_arg->globaltimer->sync_skew_prfx_sum[0]);
		phj_arg->sync_idle_skew_hist_acc[1] += diff_sec(phj_arg->localtimer->sync_skew_hist[1],
			phj_arg->globaltimer->sync_skew_hist[1]);
		phj_arg->sync_idle_skew_prfx_sum_acc[1] += diff_sec(phj_arg->localtimer->sync_skew_prfx_sum[1],
			phj_arg->globaltimer->sync_skew_prfx_sum[1]);
		phj_arg->sync_idle_skew_part_acc += diff_sec(phj_arg->localtimer->sync_skew_part,
			phj_arg->globaltimer->sync_skew_part);
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
		phj_arg->sync_idle_skew_swwcb_acc[0] += diff_sec(phj_arg->localtimer->sync_skew_swwcb[0],
			phj_arg->globaltimer->sync_skew_swwcb[0]);
		phj_arg->sync_idle_skew_swwcb_acc[1] += diff_sec(phj_arg->localtimer->sync_skew_swwcb[1],
			phj_arg->globaltimer->sync_skew_swwcb[1]);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

#ifdef PHJ_SYNCSTATS
		struct timespec *tmp_start;

		// phj_arg->global_acc_skew_taskfetch += diff_sec();
		phj_arg->global_acc_skew_prepart_memset += diff_sec( phj_arg->globaltimer->sync_skew_taskfetch, 
			phj_arg->globaltimer->sync_skew_prepart_memset );
		phj_arg->global_acc_skew_hist[0] += diff_sec( phj_arg->globaltimer->sync_skew_prepart_memset,
			phj_arg->globaltimer->sync_skew_hist[0] );
		phj_arg->global_acc_skew_prfx_sum[0] += diff_sec( phj_arg->globaltimer->sync_skew_hist[0],
			phj_arg->globaltimer->sync_skew_prfx_sum[0] );

		tmp_start = & ( phj_arg->globaltimer->sync_skew_prfx_sum[0]) ;
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
		phj_arg->global_acc_skew_swwcb[0] += diff_sec( phj_arg->globaltimer->sync_skew_prfx_sum[0],
			phj_arg->globaltimer->sync_skew_swwcb[0] );

		tmp_start = & ( phj_arg->globaltimer->sync_skew_swwcb[0] );
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

		phj_arg->global_acc_skew_hist[1] += diff_sec( *tmp_start,
			phj_arg->globaltimer->sync_skew_hist[1] );
		phj_arg->global_acc_skew_prfx_sum[1] += diff_sec( phj_arg->globaltimer->sync_skew_hist[1],
			phj_arg->globaltimer->sync_skew_prfx_sum[1] );

		tmp_start = & ( phj_arg->globaltimer->sync_skew_prfx_sum[1] );
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
		phj_arg->global_acc_skew_swwcb[1] += diff_sec( phj_arg->globaltimer->sync_skew_prfx_sum[1],
			phj_arg->globaltimer->sync_skew_swwcb[1] );

		tmp_start = & ( phj_arg->globaltimer->sync_skew_swwcb[1] );
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

		phj_arg->global_acc_skew_part += diff_sec( *tmp_start,
			phj_arg->globaltimer->sync_skew_part );

#endif /* PHJ_SYNCSTATS */

#endif /* SYNCSTATS */


		if (phj_arg->_tid == 0) {
			*skewtask = task_queue_get_atomic(skew_queue);
		}

		/* wait for the first thread fetch the skewtask slot */
#ifdef SYNCSTATS
		SYNC_TIMER(phj_arg->localtimer->sync_skew_taskfetch);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_skew_taskfetch);
#endif /* SYNCSTATS */

		if ( *skewtask == NULL ) {
			break;
		}

		memset_localize(phj_arg->r_hist[phj_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS2);
		memset_localize(phj_arg->s_hist[phj_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS2);

		/* explicitly zero the skew_hand offset array */
#ifdef OUTPUT_OFFSET_NVM
		pmem_memset_persist(phj_arg->r_output_offset[phj_arg->_tid], 0, sizeof(size_t) * FANOUT_PASS2 );
		pmem_memset_persist(phj_arg->s_output_offset[phj_arg->_tid], 0, sizeof(size_t) * FANOUT_PASS2 );
#else /* OUTPUT_OFFSET_NVM */
		memset(phj_arg->r_output_offset[phj_arg->_tid], 0, sizeof(size_t) * FANOUT_PASS2 );
		memset(phj_arg->s_output_offset[phj_arg->_tid], 0, sizeof(size_t) * FANOUT_PASS2 );
#endif /* OUTPUT_OFFSET_NVM */

		/* memset the historgram for the skew handling phase*/
#ifdef SYNCSTATS
		SYNC_TIMER(phj_arg->localtimer->sync_skew_prepart_memset);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_skew_prepart_memset);
#endif /* SYNCSTATS */

		/**************** PARTITIONING R ****************/
		skew_hand_num_per_thr = (*skewtask)->r_rel.keyrid_num / BUILDPART_THREAD_NUM;

		part.ext.keyrids = (*skewtask)->r_rel.keyrids + phj_arg->_tid * skew_hand_num_per_thr;
		part.tmp = (*skewtask)->r_tmp;
		part.hist = phj_arg->r_hist;
		part.output_offset = phj_arg->r_output_offset;

		part.thr_tup_num = skew_hand_num_per_thr;
		part.last_thr_tup_num = ((*skewtask)->r_rel.keyrid_num - (BUILDPART_THREAD_NUM-1) * skew_hand_num_per_thr);
		part.keyrid_org = (*skewtask)->r_rel.keyrids;

		part.ext.keyrid_num = (phj_arg->_tid == BUILDPART_THREAD_NUM -1) ? 
			part.last_thr_tup_num : part.thr_tup_num;

		part.rel_flag = 0;

#ifdef SYNCSTATS
		part.sync_local_hist = & phj_arg->localtimer->sync_skew_hist[0];
		part.sync_global_hist = & phj_arg->globaltimer->sync_skew_hist[0];
		part.sync_local_prfx_sum = & phj_arg->localtimer->sync_skew_prfx_sum[0];
		part.sync_global_prfx_sum = & phj_arg->globaltimer->sync_skew_prfx_sum[0];
#endif /* SYNCSTATS */

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART

#ifdef SYNCSTATS
		part.sync_local_swwcb = & phj_arg->localtimer->sync_skew_swwcb[0];
		part.sync_global_swwcb = & phj_arg->globaltimer->sync_skew_swwcb[0];
#endif /* SYNCSTATS */

		ptr_parallel_radix_partition_optimized_following_bw_reg(&part, PTR_PADDING_UNIT_NUM, (*skewtask)->r_global_offset);

#else /* PTR_USE_SWWCB_OPTIMIZED_PART */
		ptr_parallel_radix_partition_following_bw_reg(&part, PTR_PADDING_UNIT_NUM);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

		/**************** PARTITIONING S ****************/
		skew_hand_num_per_thr = (*skewtask)->s_rel.keyrid_num / BUILDPART_THREAD_NUM;

		part.ext.keyrids = (*skewtask)->s_rel.keyrids + phj_arg->_tid * skew_hand_num_per_thr;
		part.tmp = (*skewtask)->s_tmp;
		part.hist = phj_arg->s_hist;
		part.output_offset = phj_arg->s_output_offset;

		part.thr_tup_num = skew_hand_num_per_thr;
		part.last_thr_tup_num = ((*skewtask)->s_rel.keyrid_num - (BUILDPART_THREAD_NUM-1) * skew_hand_num_per_thr);
		part.keyrid_org = (*skewtask)->s_rel.keyrids;

		part.ext.keyrid_num = (phj_arg->_tid == BUILDPART_THREAD_NUM -1) ? 
			part.last_thr_tup_num : part.thr_tup_num;

		part.rel_flag = 1;

#ifdef SYNCSTATS
		part.sync_local_hist = & phj_arg->localtimer->sync_skew_hist[1];
		part.sync_global_hist = & phj_arg->globaltimer->sync_skew_hist[1];
		part.sync_local_prfx_sum = & phj_arg->localtimer->sync_skew_prfx_sum[1];
		part.sync_global_prfx_sum = & phj_arg->globaltimer->sync_skew_prfx_sum[1];
#endif /* SYNCSTATS */

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART

#ifdef SYNCSTATS
		part.sync_local_swwcb = & phj_arg->localtimer->sync_skew_swwcb[1];
		part.sync_global_swwcb = & phj_arg->globaltimer->sync_skew_swwcb[1];
#endif /* SYNCSTATS */

		ptr_parallel_radix_partition_optimized_following_bw_reg(&part, PTR_PADDING_UNIT_NUM, (*skewtask)->s_global_offset);

#else /* PTR_USE_SWWCB_OPTIMIZED_PART */
		ptr_parallel_radix_partition_following_bw_reg(&part, PTR_PADDING_UNIT_NUM);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

		/* the partitioning in skew handling phase */
#ifdef SYNCSTATS
		SYNC_TIMER(phj_arg->localtimer->sync_skew_part);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_skew_part);
#endif /* SYNCSTATS */

		if (phj_arg -> _tid == 0) {

			for (size_t idx = 0; idx < FANOUT_PASS2; idx ++) {

				/* within the first thread, thus no memory leak */
				skew_hand_r_num_keyrid = (idx == FANOUT_PASS2 - 1) ?
					(*skewtask)->r_rel.keyrid_num - r_output_offset[idx] + idx * PTR_PADDING_UNIT_NUM :
					r_output_offset[idx+1] - r_output_offset[idx] - PTR_PADDING_UNIT_NUM;
				skew_hand_s_num_keyrid = (idx == FANOUT_PASS2 - 1) ?
					(*skewtask)->s_rel.keyrid_num - s_output_offset[idx] + idx * PTR_PADDING_UNIT_NUM :
					s_output_offset[idx+1] - s_output_offset[idx] - PTR_PADDING_UNIT_NUM;

				max_build_side_tup_num = max_build_side_tup_num > skew_hand_r_num_keyrid ? max_build_side_tup_num : skew_hand_r_num_keyrid;

				if (skew_hand_r_num_keyrid > THRESHOLD_1 || skew_hand_s_num_keyrid > THRESHOLD_1) {
					for (size_t jdx = 0; jdx < PROBEJOIN_THREAD_NUM; jdx ++) {
						skew_hand_task = task_queue_get_slot(part_queue);
						
						skew_hand_task->r_rel.keyrid_num = skew_hand_task->r_tmp.keyrid_num = skew_hand_r_num_keyrid;

						skew_hand_task->r_rel.keyrids = (*skewtask)->r_tmp.keyrids + r_output_offset[idx];
						skew_hand_task->r_tmp.keyrids = (*skewtask)->r_rel.keyrids + r_output_offset[idx];

						size_t skew_hand_large_task_num_keyrid = (jdx == PROBEJOIN_THREAD_NUM - 1) ?
							skew_hand_s_num_keyrid - jdx * (skew_hand_s_num_keyrid / PROBEJOIN_THREAD_NUM) :
							skew_hand_s_num_keyrid / PROBEJOIN_THREAD_NUM;

						skew_hand_task->s_rel.keyrid_num = skew_hand_task->s_tmp.keyrid_num = skew_hand_large_task_num_keyrid;

						skew_hand_task->s_rel.keyrids = (*skewtask)->s_tmp.keyrids + s_output_offset[idx] + jdx * ( skew_hand_s_num_keyrid / PROBEJOIN_THREAD_NUM );
						skew_hand_task->s_tmp.keyrids = (*skewtask)->s_rel.keyrids + s_output_offset[idx] + jdx * ( skew_hand_s_num_keyrid / PROBEJOIN_THREAD_NUM );

						task_queue_add(part_queue, skew_hand_task);

					}

				} else {
					if (skew_hand_r_num_keyrid > 0 && skew_hand_s_num_keyrid > 0) {

						skew_hand_task = task_queue_get_slot(phj_arg->join_queue);

						skew_hand_task->r_rel.keyrid_num = skew_hand_task->r_tmp.keyrid_num = skew_hand_r_num_keyrid;
						skew_hand_task->r_rel.keyrids = (*skewtask)->r_tmp.keyrids + r_output_offset[idx];
						skew_hand_task->r_tmp.keyrids = (*skewtask)->r_rel.keyrids + r_output_offset[idx];

						skew_hand_task->s_rel.keyrid_num = skew_hand_task->s_tmp.keyrid_num = skew_hand_s_num_keyrid;
						skew_hand_task->s_rel.keyrids = (*skewtask)->s_tmp.keyrids + s_output_offset[idx];
						skew_hand_task->s_tmp.keyrids = (*skewtask)->s_rel.keyrids + s_output_offset[idx];

						task_queue_add(phj_arg->join_queue, skew_hand_task);
					}
				}

			}
		}
	}

	/* add large join tasks in part_queue to the front of the join queue */
	if (phj_arg->_tid == 0) {
		while((skew_hand_task = task_queue_get_atomic(part_queue))) {
			task_queue_add(phj_arg->join_queue, skew_hand_task);
		}
	}

/* wait for the first thread add the skew handling tasks to join_queue */
#ifdef SYNCSTATS
	SYNC_TIMER(phj_arg->localtimer->sync_skew_queue);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_skew_queue);
#endif /* SYNCSTATS */

#endif /* SKEW_HANDLING */

	/* share the max_build_side_tup_num to all threads */
	if (phj_arg->_tid == 0) {
		for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
			if (max_build_side_tup_num < phj_arg->max_build_side_tup_num[idx]) {
				max_build_side_tup_num = phj_arg->max_build_side_tup_num[idx];
			}
		}
		for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
			phj_arg->max_build_side_tup_num[idx] = max_build_side_tup_num;
		}
	}

	/* wait for the first thread finishs counting the maximum size of hashtable */
#ifdef SYNCSTATS
	SYNC_TIMER(phj_arg->localtimer->sync_max_hashtable);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_max_hashtable);
#endif /* SYNCSTATS */

	if (phj_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &phj_arg->part_end);
	}

#ifdef USE_PMWATCH
	if (phj_arg->_tid == 0) {
		pmwEnd(PMWatch_output_buildpart);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, phj_arg->PAPI_values_buildpart);
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


#ifdef SKEW_HANDLING
	dealloc_memory(phj_arg->r_hist[phj_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS2);
	dealloc_memory(phj_arg->s_hist[phj_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS2);
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM
	pmem_unmap(swwcb, sizeof(ptr_swwcb_t) * FANOUT_PASS2);
#else /* PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	free(swwcb);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM */
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
#ifdef OUTPUT_OFFSET_NVM
	pmem_unmap(r_output_offset, sizeof(size_t) * FANOUT_PASS2);
	pmem_unmap(s_output_offset, sizeof(size_t) * FANOUT_PASS2);
#else /* OUTPUT_OFFSET_NVM */
	dealloc_dram(r_output_offset, sizeof(size_t) * FANOUT_PASS2);
	dealloc_dram(s_output_offset, sizeof(size_t) * FANOUT_PASS2);
#endif /* OUTPUT_OFFSET_NVM */
#else /* SKEW_HANDLING */
	dealloc_memory(phj_arg->r_hist[phj_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS1);
	dealloc_memory(phj_arg->s_hist[phj_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS1);
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM
	pmem_unmap(swwcb, sizeof(ptr_swwcb_t) * FANOUT_PASS1);
#else /* PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	free(swwcb);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM */
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
#ifdef OUTPUT_OFFSET_NVM
	pmem_unmap(r_output_offset, sizeof(size_t) * FANOUT_PASS1);
	pmem_unmap(s_output_offset, sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	dealloc_dram(r_output_offset, sizeof(size_t) * FANOUT_PASS1);
	dealloc_dram(s_output_offset, sizeof(size_t) * FANOUT_PASS1);
#endif /* OUTPUT_OFFSET_NVM */
#endif /* SKEW_HANDLING */

	return NULL;
}


void * ptr_phj_thread_probejoin_bw_reg(void * param) {
	ptr_phj_arg_bw_reg_t *phj_arg = (ptr_phj_arg_bw_reg_t *) param;
	ptr_join_arg_t join_arg;

	/* currently, we only consider the uniform pkfk scenario */
	size_t tot_row_id_pair_num = (size_t) phj_arg->org_relS->tuple_num * AVG_MATCH_NUM_PER_THR_SCALING_FACTOR / PROBEJOIN_THREAD_NUM;
	row_id_pair_t *mater;

#ifdef USE_NVM
	mater = (row_id_pair_t *) new_alloc_nvm(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);
#else /* USE_NVM */
#ifdef USE_HUGE
	mater = (row_id_pair_t *) alloc_huge_dram(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	mater = (row_id_pair_t *) alloc_aligned_dram(ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	memset_localize(mater, ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);

	int rv;
	ptr_task_t *task;

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
	if (phj_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */


	/**************** JOIN PHASE ****************/
	if (phj_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &phj_arg->join_start);
	}

	if (phj_arg->_tid < INTERMEDIATEMEMSET_THREAD_NUM) {
		size_t memset_size = (phj_arg->_tid < INTERMEDIATEMEMSET_THREAD_NUM-1) ?
			phj_arg->intermediate_size*PROBEJOIN_THREAD_NUM/INTERMEDIATEMEMSET_THREAD_NUM:
			phj_arg->intermediate_size*PROBEJOIN_THREAD_NUM - phj_arg->_tid*
			(phj_arg->intermediate_size*PROBEJOIN_THREAD_NUM/INTERMEDIATEMEMSET_THREAD_NUM);

		memset_localize(phj_arg->intermediate + phj_arg->_tid*(phj_arg->intermediate_size
			*PROBEJOIN_THREAD_NUM/INTERMEDIATEMEMSET_THREAD_NUM), memset_size);
	}

	/* wait for all threads finish their join tasks */
#ifdef SYNCSTATS
	SYNC_TIMER(phj_arg->localtimer->sync_intermediate_memset);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_intermediate_memset);
#endif /* SYNCSTATS */

	join_arg.intermediate = phj_arg->intermediate + phj_arg->_tid * phj_arg->intermediate_size;


#ifdef NONTEMP_STORE_MATER
#ifdef NONTEMP_STORE_MATER_ON_NVM
	join_arg.row_id_pair_swwcb = (row_id_pair_swwcb_t *) new_alloc_nvm(sizeof(row_id_pair_swwcb_t));
	pmem_memset_persist(join_arg.row_id_pair_swwcb, 0, sizeof(row_id_pair_swwcb_t));
#else /* NONTEMP_STORE_MATER_ON_NVM */
	join_arg.row_id_pair_swwcb = (row_id_pair_swwcb_t *) alloc_aligned_dram(sizeof(row_id_pair_swwcb_t), CACHELINE_SIZE);
	memset(join_arg.row_id_pair_swwcb, 0, sizeof(row_id_pair_swwcb_t));
#endif /* NONTEMP_STORE_MATER_ON_NVM */
#endif /* NONTEMP_STORE_MATER */


	join_arg.mater = mater + phj_arg->matched_cnt;
	while ( ( task = task_queue_get_atomic(phj_arg->join_queue) ) ) {
		phj_arg->joinfunction(&task->r_rel, &task->s_rel, &task->r_tmp, &join_arg);
		phj_arg->matched_cnt += join_arg.matched_cnt;

		(phj_arg->parts_processed) ++;

#ifndef NONTEMP_STORE_MATER
		join_arg.mater = mater + phj_arg->matched_cnt;
#endif /* NONTEMP_STORE_MATER */

#ifdef PHJ_MBP
		phj_arg->memset_time += join_arg.memset_time;
		phj_arg->build_time += join_arg.build_time;
		phj_arg->probe_time += join_arg.probe_time;
#endif /* PHJ_MBP */

	}

#ifdef NONTEMP_STORE_MATER
	size_t slot_mod = phj_arg->matched_cnt & (ROW_ID_PAIR_PER_SWWCB - 1);
	for (size_t idx = 0; idx < slot_mod; idx ++) {
		store_a_row_id_pair(join_arg.mater+phj_arg->matched_cnt-slot_mod+idx, 
			&join_arg.row_id_pair_swwcb->data.row_id_pairs[idx]);
	}
#endif /* NONTEMP_STORE_MATER */

	/* wait for all threads finish their join tasks */
#ifdef SYNCSTATS
	SYNC_TIMER(phj_arg->localtimer->sync_join);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(phj_arg->barrier, rv);				
#ifdef SYNCSTATS
	SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_join);
#endif /* SYNCSTATS */

	if (phj_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &phj_arg->join_end);
	}

#ifdef USE_PMWATCH
	if (phj_arg->_tid == 0) {
		pmwEnd(PMWatch_output_probejoin);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, phj_arg->PAPI_values_probejoin);
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

#ifdef NONTEMP_STORE_MATER
#ifdef NONTEMP_STORE_MATER_ON_NVM
	pmem_unmap(join_arg.row_id_pair_swwcb, sizeof(row_id_pair_swwcb_t));
#else /* NONTEMP_STORE_MATER_ON_NVM */
	free(join_arg.row_id_pair_swwcb);
#endif /* NONTEMP_STORE_MATER_ON_NVM */
#endif /* NONTEMP_STORE_MATER */

	phj_arg->mater[phj_arg->_tid] = mater;

	return NULL;
}


void * ptr_phj_thread_retrieve_bw_reg(void * param) {
	ptr_phj_arg_bw_reg_t *phj_arg = (ptr_phj_arg_bw_reg_t *) param;

	size_t start_offset = 0;
	size_t processed_num = 0;
	size_t processed_num_thr = 0;
	rowid_t tmp_checksum = 0;

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
	if (phj_arg->_tid == 0) {
		pmwInit();
		pmwStart();
	}
#endif /* USE_PMWATCH */

	/**************** RETRIEVE PHASE ****************/

	if (phj_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &phj_arg->retrieve_start);
	}

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		processed_num_thr = phj_arg->mater_cnt[idx] / RETRIEVE_THREAD_NUM;
		start_offset = phj_arg->_tid * processed_num_thr;
		processed_num = (phj_arg->_tid == RETRIEVE_THREAD_NUM - 1) ? 
			phj_arg->mater_cnt[idx] - phj_arg->_tid * processed_num_thr : processed_num_thr;
		ptr_retrieve(&tmp_checksum, phj_arg->mater[idx]+start_offset, processed_num, *phj_arg->org_relR, *phj_arg->org_relS);
		phj_arg->checksum += tmp_checksum;
		phj_arg->row_id_pairs_processed += processed_num;
	}

#ifdef SYNCSTATS
	SYNC_TIMER(phj_arg->localtimer->sync_retrieve);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_retrieve);
#endif /* SYNCSTATS */

	if (phj_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &phj_arg->retrieve_end);
	}

#ifdef USE_PMWATCH
	if (phj_arg->_tid == 0) {
		pmwEnd(PMWatch_output_retrieve);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, phj_arg->PAPI_values_retrieve);
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


void ptr_join_init_run_bw_reg(const datameta_t datameta, timekeeper_t * const timekeeper, 
	void (*f_join)(const ptr_relation_t * const, const ptr_relation_t * const, ptr_relation_t * const, ptr_join_arg_t * const)
	) {

	int rv;

	size_t matched_cnt = 0;
	rowid_t checksum = 0;

	/**************** LOAD DATA ****************/
	char r_path[CHAR_BUFFER_LEN], s_path[CHAR_BUFFER_LEN];
	snprintf(r_path, CHAR_BUFFER_LEN, "%s/%s", DATA_PATH_PREFIX, datameta.r_path_suffix);
	snprintf(s_path, CHAR_BUFFER_LEN, "%s/%s", DATA_PATH_PREFIX, datameta.s_path_suffix);

	tuple_t *r_original_addr = (tuple_t*) create_pmem_buffer(r_path, TUPLE_T_SIZE * datameta.r_tuple_num);
	tuple_t *s_original_addr = (tuple_t*) create_pmem_buffer(s_path, TUPLE_T_SIZE * datameta.s_tuple_num);

	relation_t r_relation, s_relation;

	r_relation.tuple_num = datameta.r_tuple_num;
	s_relation.tuple_num = datameta.s_tuple_num;

#ifdef USE_NVM
	r_relation.tuples = (tuple_t*) new_alloc_nvm(TUPLE_T_SIZE * r_relation.tuple_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	s_relation.tuples = (tuple_t*) new_alloc_nvm(TUPLE_T_SIZE * s_relation.tuple_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
#else /* USE_NVM */
#ifdef USE_HUGE
	r_relation.tuples = (tuple_t*) alloc_huge_dram(TUPLE_T_SIZE * r_relation.tuple_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	s_relation.tuples = (tuple_t*) alloc_huge_dram(TUPLE_T_SIZE * s_relation.tuple_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	r_relation.tuples = (tuple_t*) alloc_aligned_dram(TUPLE_T_SIZE * r_relation.tuple_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)), CACHELINE_SIZE);
	s_relation.tuples = (tuple_t*) alloc_aligned_dram(TUPLE_T_SIZE * s_relation.tuple_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)), CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memcpy(r_relation.tuples, r_original_addr, TUPLE_T_SIZE * r_relation.tuple_num);
	parallel_memcpy(s_relation.tuples, s_original_addr, TUPLE_T_SIZE * s_relation.tuple_num);
	parallel_memset_localize(r_relation.tuples + r_relation.tuple_num, PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	parallel_memset_localize(s_relation.tuples + s_relation.tuple_num, PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	pmem_unmap(r_original_addr, TUPLE_T_SIZE * r_relation.tuple_num);
	pmem_unmap(s_original_addr, TUPLE_T_SIZE * s_relation.tuple_num);

	/**************** CREATE INTERMEDIATES FOR PARTITIONING ****************/
	ptr_relation_t r_tmp_relation, r_ext_relation;
	r_tmp_relation.keyrid_num = r_ext_relation.keyrid_num = r_relation.tuple_num;
#ifdef USE_NVM
	r_tmp_relation.keyrids = (keyrid_t *) new_alloc_nvm(KEYRID_T_SIZE * r_tmp_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	r_ext_relation.keyrids = (keyrid_t *) new_alloc_nvm(KEYRID_T_SIZE * r_ext_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
#else /* USE_NVM */
#ifdef USE_HUGE
	r_tmp_relation.keyrids = (keyrid_t *) alloc_huge_dram(KEYRID_T_SIZE * r_tmp_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	r_ext_relation.keyrids = (keyrid_t *) alloc_huge_dram(KEYRID_T_SIZE * r_ext_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	r_tmp_relation.keyrids = (keyrid_t *) alloc_aligned_dram(KEYRID_T_SIZE * r_tmp_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)), CACHELINE_SIZE);
	r_ext_relation.keyrids = (keyrid_t *) alloc_aligned_dram(KEYRID_T_SIZE * r_ext_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)), CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memset_localize(r_tmp_relation.keyrids + r_tmp_relation.keyrid_num, PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	parallel_memset_localize(r_ext_relation.keyrids + r_ext_relation.keyrid_num, PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));

	ptr_relation_t s_tmp_relation, s_ext_relation;
	s_tmp_relation.keyrid_num = s_ext_relation.keyrid_num = s_relation.tuple_num;
#ifdef USE_NVM
	s_tmp_relation.keyrids = (keyrid_t *) new_alloc_nvm(KEYRID_T_SIZE * s_tmp_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	s_ext_relation.keyrids = (keyrid_t *) new_alloc_nvm(KEYRID_T_SIZE * s_ext_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
#else /* USE_NVM */
#ifdef USE_HUGE
	s_tmp_relation.keyrids = (keyrid_t *) alloc_huge_dram(KEYRID_T_SIZE * s_tmp_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	s_ext_relation.keyrids = (keyrid_t *) alloc_huge_dram(KEYRID_T_SIZE * s_ext_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	s_tmp_relation.keyrids = (keyrid_t *) alloc_aligned_dram(KEYRID_T_SIZE * s_tmp_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)), CACHELINE_SIZE);
	s_ext_relation.keyrids = (keyrid_t *) alloc_aligned_dram(KEYRID_T_SIZE * s_ext_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)), CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memset_localize(s_tmp_relation.keyrids + s_tmp_relation.keyrid_num, PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	parallel_memset_localize(s_ext_relation.keyrids + s_ext_relation.keyrid_num, PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));

	size_t **r_hist = (size_t **) alloc_memory( sizeof(size_t *) * BUILDPART_THREAD_NUM, CACHELINE_SIZE );
	size_t **s_hist = (size_t **) alloc_memory( sizeof(size_t *) * BUILDPART_THREAD_NUM, CACHELINE_SIZE );

	size_t **r_output_offset = (size_t **) alloc_memory( sizeof(size_t *) * BUILDPART_THREAD_NUM, CACHELINE_SIZE );
	size_t **s_output_offset = (size_t **) alloc_memory( sizeof(size_t *) * BUILDPART_THREAD_NUM, CACHELINE_SIZE );

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	ptr_swwcb_t **swwcb_s = (ptr_swwcb_t **) alloc_memory( sizeof(ptr_swwcb_t *) * BUILDPART_THREAD_NUM, CACHELINE_SIZE );
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

	ptr_task_queue_t *part_queue = task_queue_init(FANOUT_PASS1);
	ptr_task_queue_t *join_queue = task_queue_init(1 << NUM_RADIX_BITS);

#if NUM_PASSES != 1
	idx_2nd_pass = 0;

	ptr_task_queue_t *part_queue_2nd_pass = task_queue_init(FANOUT_PASS1);
	r_hist_2nd_pass = (size_t *) alloc_memory( sizeof(size_t) * 2 * FANOUT_PASS2 * FANOUT_PASS1, CACHELINE_SIZE);
	s_hist_2nd_pass = (size_t *) alloc_memory( sizeof(size_t) * 2 * FANOUT_PASS2 * FANOUT_PASS1, CACHELINE_SIZE);
	memset_localize(r_hist_2nd_pass, sizeof(size_t) * 2 * FANOUT_PASS2 * FANOUT_PASS1);
	memset_localize(s_hist_2nd_pass, sizeof(size_t) * 2 * FANOUT_PASS2 * FANOUT_PASS1);

#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM
	r_swwcb_2nd_pass_mem = new_alloc_nvm(sizeof(ptr_swwcb_t) * FANOUT_PASS2 * FANOUT_PASS1);
	s_swwcb_2nd_pass_mem = new_alloc_nvm(sizeof(ptr_swwcb_t) * FANOUT_PASS2 * FANOUT_PASS1);
	pmem_memset_persist(r_swwcb_2nd_pass_mem, 0, sizeof(ptr_swwcb_t) * FANOUT_PASS2 * FANOUT_PASS1);
	pmem_memset_persist(s_swwcb_2nd_pass_mem, 0, sizeof(ptr_swwcb_t) * FANOUT_PASS2 * FANOUT_PASS1);
#else /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
	r_swwcb_2nd_pass_mem = alloc_aligned_dram(sizeof(ptr_swwcb_t) * FANOUT_PASS2 * FANOUT_PASS1, CACHELINE_SIZE);
	s_swwcb_2nd_pass_mem = alloc_aligned_dram(sizeof(ptr_swwcb_t) * FANOUT_PASS2 * FANOUT_PASS1, CACHELINE_SIZE);
	memset(r_swwcb_2nd_pass_mem, 0, sizeof(ptr_swwcb_t) * FANOUT_PASS2 * FANOUT_PASS1);
	memset(s_swwcb_2nd_pass_mem, 0, sizeof(ptr_swwcb_t) * FANOUT_PASS2 * FANOUT_PASS1);
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */

#endif /* NUM_PASSES != 1 */


#if SKEW_HANDLING
	ptr_task_queue_t *skew_queue;
	ptr_task_t *skewtask = NULL;
	skew_queue = task_queue_init(FANOUT_PASS1);
#endif /* SKEW_HANDLING */

	row_id_pair_t **mater = (row_id_pair_t **) alloc_memory( sizeof(row_id_pair_t *) * PROBEJOIN_THREAD_NUM, CACHELINE_SIZE );
	size_t mater_cnt[PROBEJOIN_THREAD_NUM];
	memset(mater_cnt, 0, sizeof(size_t)*PROBEJOIN_THREAD_NUM);

	int max_thread_num = MAX( MAX(BUILDPART_THREAD_NUM, PROBEJOIN_THREAD_NUM), RETRIEVE_THREAD_NUM );

	size_t max_build_side_tup_num[max_thread_num];
	memset(max_build_side_tup_num, 0, sizeof(size_t)*max_thread_num);

	pthread_t threadpool[max_thread_num];
	ptr_phj_arg_bw_reg_t args[max_thread_num];

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
	pthread_barrier_t retrieve_barrier;
	rv = pthread_barrier_init(&retrieve_barrier, NULL, RETRIEVE_THREAD_NUM);
	if(rv){
		printf("[ERROR] Couldn't create the retrieve_barrier\n");
		exit(EXIT_FAILURE);
	}

#ifdef SYNCSTATS
	ptr_synctimer_phj_bw_reg_t *localtimer = (ptr_synctimer_phj_bw_reg_t *) malloc( max_thread_num * sizeof(ptr_synctimer_phj_bw_reg_t) );
	ptr_synctimer_phj_bw_reg_t *globaltimer = (ptr_synctimer_phj_bw_reg_t *) malloc( sizeof(ptr_synctimer_phj_bw_reg_t) );
	memset(localtimer, 0, max_thread_num * sizeof(ptr_synctimer_phj_bw_reg_t));
	memset(globaltimer, 0, sizeof(ptr_synctimer_phj_bw_reg_t));
#endif /* SYNCSTATS */

#ifdef PHJ_MBP
	double avg_memset_time = 0.0;
	double avg_build_time = 0.0;
	double avg_probe_time = 0.0;
	double avg_retrieve_time = 0.0;
#endif /* PHJ_MBP */

	cpu_set_t set;
	int cpu_idx;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	/** allocate a hashtable **/

	size_t r_tuple_num_thr = datameta.r_tuple_num / BUILDPART_THREAD_NUM;
	size_t s_tuple_num_thr = datameta.s_tuple_num / BUILDPART_THREAD_NUM;


#ifdef USE_PAPI
	long long PAPI_values[NUM_PAPI_EVENT];
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
	long long PAPI_values_retrieve[NUM_PAPI_EVENT];
	memset(PAPI_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_retrieve, 0, sizeof(long long) * NUM_PAPI_EVENT );

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

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &buildpart_barrier;
		(args+idx)->datameta = &datameta;

		(args+idx)->r_rel.tuple_num = (idx == BUILDPART_THREAD_NUM -1) ? (datameta.r_tuple_num - idx * r_tuple_num_thr) : r_tuple_num_thr;
		(args+idx)->r_rel.tuples = r_relation.tuples + r_tuple_num_thr * idx;
		(args+idx)->r_tmp = r_tmp_relation;
		(args+idx)->r_ext = r_ext_relation;
		(args+idx)->r_hist = r_hist;

		(args+idx)->s_rel.tuple_num = (idx == BUILDPART_THREAD_NUM -1) ? (datameta.s_tuple_num - idx * s_tuple_num_thr) : s_tuple_num_thr;
		(args+idx)->s_rel.tuples = s_relation.tuples + s_tuple_num_thr * idx;
		(args+idx)->s_tmp = s_tmp_relation;
		(args+idx)->s_ext = s_ext_relation;
		(args+idx)->s_hist = s_hist;

		(args+idx)->max_build_side_tup_num = max_build_side_tup_num;

		(args+idx)->part_queue = part_queue;
		(args+idx)->join_queue = join_queue;

#if NUM_PASSES != 1
		(args+idx)->part_queue_2nd_pass = part_queue_2nd_pass;
#endif /* NUM_PASSES != 1 */

#ifdef SKEW_HANDLING
		(args+idx)->skew_queue = skew_queue;
		(args+idx)->skewtask = &skewtask;
#endif /* SKEW_HANDLING */

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;

#ifdef SKEW_HANDLING
		(args+idx)->sync_idle_skew_taskfetch_acc = 0.0;
		(args+idx)->sync_idle_skew_prepart_memset_acc = 0.0;
		(args+idx)->sync_idle_skew_hist_acc[0] = 0.0;
		(args+idx)->sync_idle_skew_hist_acc[1] = 0.0;
		(args+idx)->sync_idle_skew_prfx_sum_acc[0] = 0.0;
		(args+idx)->sync_idle_skew_prfx_sum_acc[1] = 0.0;
		(args+idx)->sync_idle_skew_part_acc = 0.0;
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
		(args+idx)->sync_idle_skew_swwcb_acc[0] = 0.0;
		(args+idx)->sync_idle_skew_swwcb_acc[1] = 0.0;
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
#ifdef PHJ_SYNCSTATS
		(args+idx)->global_acc_skew_taskfetch = 0.0;
		(args+idx)->global_acc_skew_prepart_memset = 0.0;
		(args+idx)->global_acc_skew_hist[0] = 0.0;
		(args+idx)->global_acc_skew_hist[1] = 0.0;
		(args+idx)->global_acc_skew_prfx_sum[0] = 0.0;
		(args+idx)->global_acc_skew_prfx_sum[1] = 0.0;
		(args+idx)->global_acc_skew_part = 0.0;
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
		(args+idx)->global_acc_skew_swwcb[0] = 0.0;
		(args+idx)->global_acc_skew_swwcb[1] = 0.0;
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
#endif /* PHJ_SYNCSTATS */
#endif /* SKEW_HANDLING */
#endif /* SYNCSTATS */

		(args+idx)->org_relR = &r_relation;
		(args+idx)->org_relS = &s_relation;

		(args+idx)->r_output_offset = r_output_offset;
		(args+idx)->s_output_offset = s_output_offset;

		(args+idx)->r_thr_tup_num = r_tuple_num_thr;
		(args+idx)->r_last_thr_tup_num = datameta.r_tuple_num - (BUILDPART_THREAD_NUM-1) * r_tuple_num_thr;

		(args+idx)->s_thr_tup_num = s_tuple_num_thr;
		(args+idx)->s_last_thr_tup_num = datameta.s_tuple_num - (BUILDPART_THREAD_NUM-1) * s_tuple_num_thr;

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
		(args+idx)->swwcb_s = swwcb_s;
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, ptr_phj_thread_buildpart_bw_reg, args+idx);
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

	/* reset the join_queue */
#if NUM_PASSES == 1
	ptr_task_queue_t *swap = join_queue;
	join_queue = part_queue;
	part_queue = swap;
#endif /* NUM_PASSES == 1 */

	size_t intermediate_size;

	if (f_join == ptr_sc_bw_reg_join) {
		intermediate_size = PTR_PHJ_SC_BKT_T_SIZE*next_pow_2(max_build_side_tup_num[0])*INTERMEDIATE_SCALING_FACTOR/PHJ_SC_BUCKET_CAPACITY;
	} else {
		intermediate_size = KEY_T__SIZE*next_pow_2(max_build_side_tup_num[0])*INTERMEDIATE_SCALING_FACTOR;
	}

	void *intermediate;
#ifdef USE_NVM
	intermediate = new_alloc_nvm(intermediate_size*PROBEJOIN_THREAD_NUM);
#else /* USE_NVM */
	intermediate = alloc_aligned_dram(intermediate_size*PROBEJOIN_THREAD_NUM, CACHELINE_SIZE);
#endif /* USE_NVM */


#ifndef TEST_PARTITIONING

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &probejoin_barrier;

		(args+idx)->org_relR = &r_relation;
		(args+idx)->org_relS = &s_relation;

		(args+idx)->matched_cnt = 0;
		(args+idx)->parts_processed = 0;
		(args+idx)->joinfunction = f_join;

		(args+idx)->max_build_side_tup_num = max_build_side_tup_num;

		(args+idx)->join_queue = join_queue;

		(args+idx)->mater = mater;

		(args+idx)->intermediate_size = intermediate_size;
		(args+idx)->intermediate = intermediate;

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

#ifdef PHJ_MBP
		(args+idx)->memset_time = 0.0;
		(args+idx)->build_time = 0.0;
		(args+idx)->probe_time = 0.0;
#endif /* PHJ_MBP */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, ptr_phj_thread_probejoin_bw_reg, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
		matched_cnt += (args+idx)->matched_cnt;
		mater_cnt[idx] = (args+idx)->matched_cnt;
#ifdef PHJ_MBP
		avg_memset_time += (args+idx)->memset_time;
		avg_build_time += (args+idx)->build_time;
		avg_probe_time += (args+idx)->probe_time;
#endif /* PHJ_MBP */
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_probejoin[jdx] += (args+idx)->PAPI_values_probejoin[jdx];
		}
#endif /* USE_PAPI */
	}

	for (int idx = 0; idx < RETRIEVE_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &retrieve_barrier;

		(args+idx)->checksum = 0;

		(args+idx)->mater = mater;
		(args+idx)->mater_cnt = mater_cnt;

		(args+idx)->org_relR = &r_relation;
		(args+idx)->org_relS = &s_relation;

		(args+idx)->row_id_pairs_processed = 0;

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#endif /* SYNCSTATS */

#ifdef PHJ_MBP
		(args+idx)->retrieve_time = 0.0;
#endif /* PHJ_MBP */
#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_retrieve, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, ptr_phj_thread_retrieve_bw_reg, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < RETRIEVE_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
		checksum += (args+idx)->checksum;
#ifdef PHJ_MBP
		avg_retrieve_time += (args+idx)->retrieve_time;
#endif /* PHJ_MBP */
#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_values_retrieve[jdx] += (args+idx)->PAPI_values_retrieve[jdx];
		}
#endif /* USE_PAPI */
	}

#endif /* TEST_PARTITIONING */

	purple();
	printf("matched_cnt: %zu\tcheck_sum: %zu\n", matched_cnt, checksum);

#ifdef SYNCSTATS

	printf("[SYNCSTATS] TID\tPREPART_MEMSET\t1ST_HIST_R\t1ST_PRFX_SUM_R\t");
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	printf("1ST_SWWCB_R\t");
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
	printf("1ST_HIST_S\t1ST_PRFX_SUM_S\t");
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	printf("1ST_SWWCB_S\t");
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
	printf("1ST_PASS\t1ST_PASS_QUEUE\t");
#if NUM_PASSES != 1
	printf("2ND_PASS_MID_HIST\t");
#endif /* NUM_PASSES != 1 */
	printf("PART\t");

#ifdef SKEW_HANDLING
	printf("SKEW_TASKFETCH\tSKEW_PREPART_MEMSET\tSKEW_HIST_R\tSKEW_PRFX_SUM_R\t");
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	printf("SKEW_SWWCB_R\t");
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
	printf("SKEW_HIST_S\tSKEW_PRFX_SUM_S\t");
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	printf("SKEW_SWWCB_S\t");
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
	printf("SKEW_PART\tSKEW_QUEUE\t");
#endif /* SKEW_HANDLING */

	printf("MAX_HASHTABLE\n");

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%.9f\t%.9f\t%.9f\t", idx, 
			diff_sec( (args+idx)->localtimer->sync_prepart_memset,
				(args+idx)->globaltimer->sync_prepart_memset ),
			diff_sec( (args+idx)->localtimer->sync_1st_hist[0],
				(args+idx)->globaltimer->sync_1st_hist[0] ),
			diff_sec( (args+idx)->localtimer->sync_1st_prfx_sum[0],
				(args+idx)->globaltimer->sync_1st_prfx_sum[0] )
		);
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
		printf("%.9f\t", diff_sec( (args+idx)->localtimer->sync_1st_swwcb[0],
			(args+idx)->globaltimer->sync_1st_swwcb[0] )
		);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
		printf("%.9f\t%.9f\t", diff_sec( (args+idx)->localtimer->sync_1st_hist[1],
			(args+idx)->globaltimer->sync_1st_hist[1] ),
			diff_sec( (args+idx)->localtimer->sync_1st_prfx_sum[1],
				(args+idx)->globaltimer->sync_1st_prfx_sum[1] )
		);
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
		printf("%.9f\t", diff_sec( (args+idx)->localtimer->sync_1st_swwcb[1],
			(args+idx)->globaltimer->sync_1st_swwcb[1] )
		);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
		printf("%.9f\t%.9f\t", diff_sec( (args+idx)->localtimer->sync_1st_pass,
			(args+idx)->globaltimer->sync_1st_pass ),
			diff_sec( (args+idx)->localtimer->sync_1st_pass_queue,
				(args+idx)->globaltimer->sync_1st_pass_queue )
		);

#if NUM_PASSES != 1
		printf("%.9f\t", diff_sec( (args+idx)->localtimer->sync_2nd_pass_mid_hist,
			(args+idx)->globaltimer->sync_2nd_pass_mid_hist )
		);
#endif /* NUM_PASSES != 1 */
		printf("%.9f\t", diff_sec( (args+idx)->localtimer->sync_part,
			(args+idx)->globaltimer->sync_part )
		);

#ifdef SKEW_HANDLING
		printf("%.9f\t%.9f\t%.9f\t%.9f\t", (args+idx)->sync_idle_skew_taskfetch_acc,
			 (args+idx)->sync_idle_skew_prepart_memset_acc,
			 (args+idx)->sync_idle_skew_hist_acc[0],
			 (args+idx)->sync_idle_skew_prfx_sum_acc[0]
		);
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
		printf("%.9f\t", (args+idx)->sync_idle_skew_swwcb_acc[0]);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
		printf("%.9f\t%.9f\t", (args+idx)->sync_idle_skew_hist_acc[1],
			(args+idx)->sync_idle_skew_prfx_sum_acc[1]
		);
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
		printf("%.9f\t", (args+idx)->sync_idle_skew_swwcb_acc[1]);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
		printf("%.9f\t%.9f\t", (args+idx)->sync_idle_skew_part_acc, 
			diff_sec( (args+idx)->localtimer->sync_skew_queue,
				(args+idx)->globaltimer->sync_skew_queue
			)
		);
#endif /* SKEW_HANDLING */
		printf("%.9f\n",
			diff_sec( (args+idx)->localtimer->sync_max_hashtable,
				(args+idx)->globaltimer->sync_max_hashtable )
		);
	}



#ifdef PHJ_SYNCSTATS
	struct timespec *tmp_start;
	printf("[PHJ_SYNCSTATS] PREPART_MEMSET: %.9f\t1ST_HIST_R: %.9f\t1ST_PRFX_SUM_R: %.9f\t",
		diff_sec( args[0].part_start,
			args[0].globaltimer->sync_prepart_memset ),
		diff_sec( args[0].globaltimer->sync_prepart_memset,
			args[0].globaltimer->sync_1st_hist[0] ),
		diff_sec( args[0].globaltimer->sync_1st_hist[0],
			args[0].globaltimer->sync_1st_prfx_sum[0] )
	);
	tmp_start = & args[0].globaltimer->sync_1st_prfx_sum[0];

#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	printf("1ST_SWWCB_R: %.9f\t", 
		diff_sec( args[0].globaltimer->sync_1st_prfx_sum[0],
			args[0].globaltimer->sync_1st_swwcb[0] )
	);
	tmp_start = & args[0].globaltimer->sync_1st_swwcb[0];
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
	printf("1ST_HIST_S: %.9f\t1ST_PRFX_SUM_S: %.9f\t",
		diff_sec( *tmp_start,
			args[0].globaltimer->sync_1st_hist[1] ),
		diff_sec( args[0].globaltimer->sync_1st_hist[1],
			args[0].globaltimer->sync_1st_prfx_sum[1] )
	);
	tmp_start = & args[0].globaltimer->sync_1st_prfx_sum[1];
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	printf("1ST_SWWCB_S: %.9f\t",
		diff_sec( args[0].globaltimer->sync_1st_prfx_sum[1],
			args[0].globaltimer->sync_1st_swwcb[1] )
	);
	tmp_start = & args[0].globaltimer->sync_1st_swwcb[1];
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
	printf("1ST_PASS: %.9f\t1ST_PASS_QUEUE: %.9f\t",
		diff_sec( *tmp_start,
			args[0].globaltimer->sync_1st_pass ),
		diff_sec( args[0].globaltimer->sync_1st_pass,
			args[0].globaltimer->sync_1st_pass_queue )
	);
#if NUM_PASSES != 1
	printf("2ND_PASS_MID_HIST: %.9f\t", 
		diff_sec( args[0].globaltimer->sync_1st_pass_queue,
			args[0].globaltimer->sync_2nd_pass_mid_hist )
	);
#endif /* NUM_PASSES != 1 */
	printf("PART: %.9f\n", 
		diff_sec( args[0].globaltimer->sync_2nd_pass_mid_hist,
			args[0].globaltimer->sync_part )
	);

#ifdef SKEW_HANDLING
	// printf("[PHJ_SYNCSTATS] SKEW_TASKFETCH: \tSKEW_PREPART_MEMSET: \tSKEW_HIST_R: \t");
	printf("[PHJ_SYNCSTATS] SKEW_PREPART_MEMSET: %.9f\tSKEW_HIST_R: %.9f\tSKEW_PRFX_SUM_R: %.9f\t",
		args[0].global_acc_skew_prepart_memset, 
		args[0].global_acc_skew_hist[0],
		args[0].global_acc_skew_prfx_sum[0]
	);
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	printf("SKEW_SWWCB_R: %.9f\t",
		args[0].global_acc_skew_swwcb[0]
	);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
	printf("SKEW_HIST_S: %.9f\tSKEW_PRFX_SUM_S: %.9f\t", 
		args[0].global_acc_skew_hist[1],
		args[0].global_acc_skew_prfx_sum[1]
	);
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	printf("SKEW_SWWCB_S: %.9f\t",
		args[0].global_acc_skew_swwcb[1]
	);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
	printf("SKEW_PART: %.9f\tSKEW_QUEUE: %.9f\n",
		args[0].global_acc_skew_part,
		diff_sec( args[0].globaltimer->sync_skew_part,
			args[0].globaltimer->sync_skew_queue )
	);
#endif /* SKEW_HANDLING */
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

	printf("[SYNCSTATS] TID\tPAIRS\tRETRIEVE\n");
	for (int idx = 0; idx < RETRIEVE_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%zu\t%.9f\n", idx, (args+idx)->row_id_pairs_processed,
			diff_sec( (args+idx)->localtimer->sync_retrieve,
				(args+idx)->globaltimer->sync_retrieve )
		);
	}
#endif /* TEST_PARTITIONING */

#endif /* SYNCSTATS */

#ifdef PHJ_MBP
	printf("[PHJ_MBP] avg_memset_time: %.9f\tavg_build_time: %.9f\tavg_probe_time: %.9f\tavg_retrieve_time: %.9f\n",
		avg_memset_time/PROBEJOIN_THREAD_NUM, avg_build_time/PROBEJOIN_THREAD_NUM, avg_probe_time/PROBEJOIN_THREAD_NUM, avg_retrieve_time/RETRIEVE_THREAD_NUM
	);
#endif /* PHJ_MBP */
#ifdef USE_PMWATCH
	for (int idx = 0; idx < NVMCount; idx ++) {
		PMWatch_output[idx].total_bytes_read = PMWatch_output_buildpart[idx].total_bytes_read + 
			PMWatch_output_probejoin[idx].total_bytes_read + PMWatch_output_retrieve[idx].total_bytes_read;
		PMWatch_output[idx].total_bytes_written = PMWatch_output_buildpart[idx].total_bytes_written + 
			PMWatch_output_probejoin[idx].total_bytes_written + PMWatch_output_retrieve[idx].total_bytes_written;
		PMWatch_output[idx].media_read_ops = PMWatch_output_buildpart[idx].media_read_ops + 
			PMWatch_output_probejoin[idx].media_read_ops + PMWatch_output_retrieve[idx].media_read_ops;
		PMWatch_output[idx].media_write_ops = PMWatch_output_buildpart[idx].media_write_ops +
			PMWatch_output_probejoin[idx].media_write_ops + PMWatch_output_retrieve[idx].media_write_ops;
		PMWatch_output[idx].read_64B_ops_received = PMWatch_output_buildpart[idx].read_64B_ops_received + 
			PMWatch_output_probejoin[idx].read_64B_ops_received + PMWatch_output_retrieve[idx].read_64B_ops_received;
		PMWatch_output[idx].write_64B_ops_received = PMWatch_output_buildpart[idx].write_64B_ops_received + 
			PMWatch_output_probejoin[idx].write_64B_ops_received + PMWatch_output_retrieve[idx].write_64B_ops_received;
		PMWatch_output[idx].host_reads = PMWatch_output_buildpart[idx].host_reads + 
			PMWatch_output_probejoin[idx].host_reads + PMWatch_output_retrieve[idx].host_reads;
		PMWatch_output[idx].host_writes = PMWatch_output_buildpart[idx].host_writes +
			PMWatch_output_probejoin[idx].host_writes + PMWatch_output_retrieve[idx].host_writes;
		PMWatch_output[idx].read_hit_ratio = (PMWatch_output_buildpart[idx].read_hit_ratio + 
			PMWatch_output_probejoin[idx].read_hit_ratio + PMWatch_output_retrieve[idx].read_hit_ratio) / 3;
		PMWatch_output[idx].write_hit_ratio = (PMWatch_output_buildpart[idx].write_hit_ratio + 
			PMWatch_output_probejoin[idx].write_hit_ratio + PMWatch_output_retrieve[idx].write_hit_ratio) / 3;
		PMWatch_output[idx].read_amplification = (PMWatch_output_buildpart[idx].read_amplification + 
			PMWatch_output_probejoin[idx].read_amplification + PMWatch_output_retrieve[idx].read_amplification) / 3;
		PMWatch_output[idx].write_amplification = (PMWatch_output_buildpart[idx].write_amplification +
			PMWatch_output_probejoin[idx].write_amplification + PMWatch_output_retrieve[idx].write_amplification) / 3;
		printf("[PMWatch] DIMM: %d\n", idx);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_read: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].total_bytes_read, PMWatch_output_buildpart[idx].total_bytes_read, 
			PMWatch_output_probejoin[idx].total_bytes_read, PMWatch_output_retrieve[idx].total_bytes_read
		);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_written: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].total_bytes_written, PMWatch_output_buildpart[idx].total_bytes_written, 
			PMWatch_output_probejoin[idx].total_bytes_written, PMWatch_output_retrieve[idx].total_bytes_written
		);
		printf("[PMWatch] DIMM: %d\tmedia_read_ops: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].media_read_ops, PMWatch_output_buildpart[idx].media_read_ops, 
			PMWatch_output_probejoin[idx].media_read_ops, PMWatch_output_retrieve[idx].media_read_ops
		);
		printf("[PMWatch] DIMM: %d\tmedia_write_ops: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].media_write_ops, PMWatch_output_buildpart[idx].media_write_ops, 
			PMWatch_output_probejoin[idx].media_write_ops, PMWatch_output_retrieve[idx].media_write_ops
		);
		printf("[PMWatch] DIMM: %d\tread_64B_ops_received: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].read_64B_ops_received, PMWatch_output_buildpart[idx].read_64B_ops_received,
			PMWatch_output_probejoin[idx].read_64B_ops_received, PMWatch_output_retrieve[idx].read_64B_ops_received
		);
		printf("[PMWatch] DIMM: %d\twrite_64B_ops_received: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].write_64B_ops_received, PMWatch_output_buildpart[idx].write_64B_ops_received,
			PMWatch_output_probejoin[idx].write_64B_ops_received, PMWatch_output_retrieve[idx].write_64B_ops_received
		);
		printf("[PMWatch] DIMM: %d\thost_reads: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].host_reads, PMWatch_output_buildpart[idx].host_reads, 
			PMWatch_output_probejoin[idx].host_reads, PMWatch_output_retrieve[idx].host_reads
		);
		printf("[PMWatch] DIMM: %d\thost_writes: %zu\t%zu\t%zu\t%zu\n", idx, 
			PMWatch_output[idx].host_writes, PMWatch_output_buildpart[idx].host_writes, 
			PMWatch_output_probejoin[idx].host_writes, PMWatch_output_retrieve[idx].host_writes
		);
		printf("[PMWatch] DIMM: %d\tread_hit_ratio: %f\t%f\t%f\t%f\n", idx, 
			PMWatch_output[idx].read_hit_ratio, PMWatch_output_buildpart[idx].read_hit_ratio, 
			PMWatch_output_probejoin[idx].read_hit_ratio, PMWatch_output_retrieve[idx].read_hit_ratio
		);
		printf("[PMWatch] DIMM: %d\twrite_hit_ratio: %f\t%f\t%f\t%f\n", idx, 
			PMWatch_output[idx].write_hit_ratio, PMWatch_output_buildpart[idx].write_hit_ratio, 
			PMWatch_output_probejoin[idx].write_hit_ratio, PMWatch_output_retrieve[idx].write_hit_ratio
		);
		printf("[PMWatch] DIMM: %d\tread_amplification: %.9f\t%.9f\t %.9f\t %.9f\n", idx, 
			PMWatch_output[idx].read_amplification, PMWatch_output_buildpart[idx].read_amplification, 
			PMWatch_output_probejoin[idx].read_amplification, PMWatch_output_retrieve[idx].read_amplification
		);
		printf("[PMWatch] DIMM: %d\twrite_amplification: %.9f\t%.9f\t %.9f\t %.9f\n", idx, 
			PMWatch_output[idx].write_amplification, PMWatch_output_buildpart[idx].write_amplification, 
			PMWatch_output_probejoin[idx].write_amplification, PMWatch_output_retrieve[idx].write_amplification
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
		}
	}
#endif /* USE_PMWATCH */
#ifdef USE_PAPI
	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		PAPI_values[idx] += PAPI_values_buildpart[idx] + PAPI_values_probejoin[idx] + PAPI_values_retrieve[idx];
		printf("[PAPI] PAPI_event_name: %s", PAPI_event_names[idx]);
		printf("\ttotal_counter: %lld", PAPI_values[idx]);
		printf("\tbuildpart_counter: %lld", PAPI_values_buildpart[idx]);
		printf("\tprobejoin_counter: %lld", PAPI_values_probejoin[idx]);
		printf("\tretrieve_counter: %lld", PAPI_values_retrieve[idx]);
		printf("\n");
		if (cur_t != 0) {
			agg_PAPI_values[idx] += PAPI_values[idx];
			agg_PAPI_values_buildpart[idx] += PAPI_values_buildpart[idx];
			agg_PAPI_values_probejoin[idx] += PAPI_values_probejoin[idx];
		}
	}
	PAPI_shutdown();
#endif /* USE_PAPI */

	dealloc_memory(intermediate, intermediate_size*PROBEJOIN_THREAD_NUM);

	dealloc_memory(r_hist, sizeof(size_t *) * BUILDPART_THREAD_NUM);
	dealloc_memory(s_hist, sizeof(size_t *) * BUILDPART_THREAD_NUM);
	dealloc_memory(r_output_offset, sizeof(size_t *) * BUILDPART_THREAD_NUM);
	dealloc_memory(s_output_offset, sizeof(size_t *) * BUILDPART_THREAD_NUM);
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	dealloc_memory(swwcb_s, sizeof(ptr_swwcb_t *) * BUILDPART_THREAD_NUM);
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */

	dealloc_memory(r_tmp_relation.keyrids, KEYRID_T_SIZE * r_tmp_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	dealloc_memory(s_tmp_relation.keyrids, KEYRID_T_SIZE * s_tmp_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	dealloc_memory(r_ext_relation.keyrids, KEYRID_T_SIZE * r_ext_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	dealloc_memory(s_ext_relation.keyrids, KEYRID_T_SIZE * s_ext_relation.keyrid_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));

	task_queue_free(part_queue);
	task_queue_free(join_queue);

#if NUM_PASSES != 1
	task_queue_free(part_queue_2nd_pass);

#if SKEW_HANDLING
	task_queue_free(skew_queue);
#endif /* SKEW_HANDLING */

	dealloc_memory(r_hist_2nd_pass, sizeof(size_t) * 2 * FANOUT_PASS2 * FANOUT_PASS1);
	dealloc_memory(s_hist_2nd_pass, sizeof(size_t) * 2 * FANOUT_PASS2 * FANOUT_PASS1);

#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM
	pmem_unmap(r_swwcb_2nd_pass_mem, sizeof(ptr_swwcb_t) * FANOUT_PASS2 * FANOUT_PASS1);
	pmem_unmap(s_swwcb_2nd_pass_mem, sizeof(ptr_swwcb_t) * FANOUT_PASS2 * FANOUT_PASS1);
#else /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
	free(r_swwcb_2nd_pass_mem);
	free(s_swwcb_2nd_pass_mem);
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */

#endif /* NUM_PASSES != 1 */

#ifdef SYNCSTATS
	free(localtimer);
	free(globaltimer);
#endif /* SYNCSTATS */

	pthread_attr_destroy(&attr);
	pthread_barrier_destroy(&buildpart_barrier);
	pthread_barrier_destroy(&probejoin_barrier);
	pthread_barrier_destroy(&retrieve_barrier);
	dealloc_memory(r_relation.tuples, TUPLE_T_SIZE * r_relation.tuple_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	dealloc_memory(s_relation.tuples, TUPLE_T_SIZE * s_relation.tuple_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		size_t tot_row_id_pair_num = (size_t) s_relation.tuple_num * AVG_MATCH_NUM_PER_THR_SCALING_FACTOR / PROBEJOIN_THREAD_NUM;
		dealloc_memory(mater[idx], ROW_ID_PAIR_T_SIZE * tot_row_id_pair_num + AVX512_SIZE);
	}

	dealloc_memory(mater, sizeof(row_id_pair_t *) * PROBEJOIN_THREAD_NUM);

	timekeeper->buildpart = diff_sec(args[0].part_start, args[0].part_end);
	timekeeper->probejoin = diff_sec(args[0].join_start, args[0].join_end);
	timekeeper->total = timekeeper->buildpart + timekeeper->probejoin + diff_sec(args[0].retrieve_start, args[0].retrieve_end);
}


void ptr_phj_radix_bw_reg_bc(const datameta_t datameta, timekeeper_t * const timekeeper) {
	ptr_join_init_run_bw_reg(datameta, timekeeper, ptr_bc_bw_reg_join);
}

void ptr_phj_radix_bw_reg_lp(const datameta_t datameta, timekeeper_t * const timekeeper) {
	ptr_join_init_run_bw_reg(datameta, timekeeper, ptr_lp_bw_reg_join);
}

void ptr_phj_radix_bw_reg_sc(const datameta_t datameta, timekeeper_t * const timekeeper) {
	ptr_join_init_run_bw_reg(datameta, timekeeper, ptr_sc_bw_reg_join);
}

void ptr_phj_radix_bw_reg_hr(const datameta_t datameta, timekeeper_t * const timekeeper) {
	ptr_join_init_run_bw_reg(datameta, timekeeper, ptr_hr_bw_reg_join);
}

// void ptr_phj_radix_bw_reg_hro(const datameta_t datameta, timekeeper_t * const timekeeper) {
// 	ptr_join_init_run_bw_reg(datameta, timekeeper, ptr_hro_bw_reg_join);
// }

#endif /* RUN_PAYLOAD */
#endif /* BW_REG */