#include "libpmem.h"

#include "phj_shr_uni.h"
#include "../inc/memaccess.h"

#define __USE_GNU
#include <sched.h>
#include <pthread.h>

// #include "../inc/lock.h"
#include "../inc/task_pthread.h"


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
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_buildpart[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_probejoin[MaxNVMNum];
#endif /* USE_PMWATCH */

extern int cur_t;
typedef void (*shr_uni_joinfunction_t)(const relation_t * const, const relation_t * const, relation_t * const, join_arg_t * const);


/** holds the arguments passed to each thread */
typedef struct phj_shr_uni_arg_t {
	int _tid;
	pthread_barrier_t *barrier;

	const datameta_t * datameta;

	// tuple_t *r_org;
	relation_t r_rel;
	relation_t r_tmp;

	// tuple_t *s_org;
	relation_t s_rel;
	relation_t s_tmp;

	task_queue_t *part_queue;
	task_queue_t *join_queue;
#if SKEW_HANDLING
	task_queue_t *skew_queue;
	task_t ** skewtask;
#endif /* SKEW_HANDLING */

	shr_uni_joinfunction_t joinfunction;
	size_t *max_build_side_tup_num;

	size_t matched_cnt;
	rowid_t checksum;

	/* stats about the thread */
	size_t parts_processed;

	struct timespec part_start, part_end, join_start, join_end;

#ifdef SYNCSTATS
	synctimer_phj_shr_uni_t *localtimer;
	synctimer_phj_shr_uni_t *globaltimer;
#ifdef SKEW_HANDLING
	double sync_idle_skew_taskfetch_acc;
	double sync_idle_skew_prepart_memset_acc;
	double sync_idle_skew_buildside_acc;
	double sync_idle_skew_part_acc;
#ifdef PHJ_SYNCSTATS
	double global_acc_skew_taskfetch;
	double global_acc_skew_prepart_memset;
	double global_acc_skew_buildside;
	double global_acc_skew_part;
#endif /* PHJ_SYNCSTATS */
#endif /* SKEW_HANDLING */
#endif /* SYNCSTATS */

#ifdef PHJ_MBP
	double memset_time, build_time, probe_time;
#endif /* PHJ_MBP */

#ifdef USE_PAPI
	long long PAPI_values_buildpart[NUM_PAPI_EVENT];
	long long PAPI_values_probejoin[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

	// lock_t *latches;
	pthread_mutex_t *latches;
	size_t *r_part_tup_num, *s_part_tup_num;
	size_t r_avg_part_scaling_offset, s_avg_part_scaling_offset;

#ifdef SKEW_HANDLING
	size_t *skew_hand_r_part_tup_num, *skew_hand_s_part_tup_num;
#endif /* SKEW_HANDLING */

	size_t intermediate_size;
	void *intermediate;
} phj_shr_uni_arg_t;


typedef struct shr_uni_part_t {
	int _tid;
	pthread_barrier_t *barrier;

	key_t_ hashmask;
	key_t_ bitskip;

	relation_t rel;
	relation_t tmp;

	size_t *output_offset;

	short rel_flag;		/* 0: R, 1: S */
} shr_uni_part_t;


void shr_uni_parallel_partition(shr_uni_part_t * const part, const size_t padding_num, 
	const size_t avg_part_scaling_offset, pthread_mutex_t *latches) {
	key_t_ hashmask = (part->hashmask) << part->bitskip;
	key_t_ bitskip = part->bitskip;

	size_t *output_offset = part->output_offset;
	size_t tmp_offset;

	relation_t rel = part->rel;
	relation_t tmp = part->tmp;

	hashkey_t tmp_hashkey;


	/* copy tuples to their corresponding clusters */
	for (size_t idx = 0; idx < rel.tuple_num; idx ++) {
		tmp_hashkey = IDHASH(rel.tuples[idx].key, hashmask, bitskip);

		// pthread_mutex_lock(& latches[tmp_hashkey]);

		// tmp_offset = output_offset[tmp_hashkey] ++;

		// pthread_mutex_unlock(& latche[tmp_hashkey]);

		tmp_offset = __sync_fetch_and_add(output_offset+tmp_hashkey, 1);

		store_a_tuple(&tmp.tuples[ tmp_offset+tmp_hashkey*(avg_part_scaling_offset+padding_num) ], 
			&rel.tuples[idx]);
	}
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


#if NUM_PASSES != 1


void shr_uni_cluster(relation_t out_rel, relation_t in_rel, size_t *hist, const size_t fanout, 
	const key_t_ bitskip, const size_t padding_num, const size_t avg_part_scaling_offset) {
	key_t_ hashmask = (fanout - 1) << bitskip;

	hashkey_t tmp_hashkey;

	for (size_t idx = 0; idx < in_rel.tuple_num; idx ++) {
		tmp_hashkey = IDHASH(in_rel.tuples[idx].key, hashmask, bitskip);
		store_a_tuple(& out_rel.tuples[ hist[tmp_hashkey]+tmp_hashkey*(avg_part_scaling_offset+padding_num) ] , 
			&in_rel.tuples[idx]
		);
		hist[tmp_hashkey] ++;
	}
}


#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
void shr_uni_cluster_optimized(relation_t out_rel, relation_t in_rel, size_t *hist, const size_t fanout, const key_t_ bitskip, 
	swwcb_t * const swwcb, const size_t padding_num, const size_t avg_part_scaling_offset, const size_t global_offset) {
	key_t_ hashmask = (fanout - 1) << bitskip;

	hashkey_t tmp_hashkey;

	for (size_t idx = 0; idx < fanout; idx ++) {
		swwcb[idx].data.slot = idx*(avg_part_scaling_offset+padding_num)+global_offset;
	}

	/* copy tuples to their corresponding SWWCBs */
	size_t slot;
	size_t slot_mod;
	size_t remainder_start_pos;
	size_t starting_offset;
	tuple_t * tmp_swwcb;

	for (size_t idx = 0; idx < in_rel.tuple_num; idx ++) {
		tmp_hashkey = IDHASH(in_rel.tuples[idx].key, hashmask, bitskip);
		slot = swwcb[tmp_hashkey].data.slot;
		tmp_swwcb = (tuple_t *) (swwcb + tmp_hashkey);
		slot_mod = slot & (TUPLE_PER_SWWCB - 1);
		tmp_swwcb[slot_mod] = in_rel.tuples[idx];

		if (slot_mod == TUPLE_PER_SWWCB - 1) {
			/* non-temporal store a SWWCB */
			nontemp_store_swwcb( & out_rel.tuples[slot-(TUPLE_PER_SWWCB-1)-global_offset] , tmp_swwcb);
		}

		swwcb[tmp_hashkey].data.slot = slot + 1;
	}

	/* write out the remainders in the swwcbs */
	for (size_t idx = 0; idx < fanout; idx ++) {
		starting_offset = idx*(avg_part_scaling_offset+padding_num)+global_offset;
		slot = swwcb[idx].data.slot;
		hist[idx] = slot - starting_offset;
		slot_mod = slot & (TUPLE_PER_SWWCB - 1);
		slot -= slot_mod;
		remainder_start_pos = (slot < starting_offset ) ? ( starting_offset - slot) : 0;
		for (size_t jdx = remainder_start_pos; jdx < slot_mod; jdx ++) {
			store_a_tuple(& out_rel.tuples[slot+jdx-global_offset] , & swwcb[idx].data.tuples[jdx]);
		}
	}
}
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */


void shr_uni_serial_partition(task_t * const task, task_queue_t * join_queue, const size_t fanout, const key_t_ bitskip, 
	size_t *r_hist, size_t *s_hist, void *swwcb_mem, size_t *max_build_side_tup_num, const size_t padding_num, 
	const size_t r_avg_part_scaling_offset, const size_t s_avg_part_scaling_offset) {

	memset_localize(r_hist, sizeof(size_t) * fanout);
	memset_localize(s_hist, sizeof(size_t) * fanout);

#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
	swwcb_t *swwcb = (swwcb_t *) swwcb_mem;
	shr_uni_cluster_optimized(task->r_tmp, task->r_rel, r_hist, fanout, bitskip, swwcb, padding_num, r_avg_part_scaling_offset, task->r_global_offset);
	shr_uni_cluster_optimized(task->s_tmp, task->s_rel, s_hist, fanout, bitskip, swwcb, padding_num, s_avg_part_scaling_offset, task->s_global_offset);
#else /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
	shr_uni_cluster(task->r_tmp, task->r_rel, r_hist, fanout, bitskip, padding_num, r_avg_part_scaling_offset);
	shr_uni_cluster(task->s_tmp, task->s_rel, s_hist, fanout, bitskip, padding_num, s_avg_part_scaling_offset);
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */

	task_t *tmp_task;

	size_t r_current_offset = 0;
	size_t s_current_offset = 0;

	for (size_t idx = 0; idx < fanout; idx ++) {
		if (r_hist[idx] > 0 && s_hist[idx] > 0) {

			r_current_offset = idx * (r_avg_part_scaling_offset + padding_num);
			s_current_offset = idx * (s_avg_part_scaling_offset + padding_num);

			tmp_task = task_queue_get_slot_atomic(join_queue);

			tmp_task->r_rel.tuple_num = r_hist[idx];
			tmp_task->r_rel.tuples = task->r_tmp.tuples + r_current_offset;
			tmp_task->r_tmp.tuples = task->r_rel.tuples + r_current_offset;

			tmp_task->s_rel.tuple_num = s_hist[idx];
			tmp_task->s_rel.tuples = task->s_tmp.tuples + s_current_offset;
			tmp_task->s_tmp.tuples = task->s_rel.tuples + s_current_offset;

			task_queue_add_atomic(join_queue, tmp_task);

			*max_build_side_tup_num = *max_build_side_tup_num > r_hist[idx] ? *max_build_side_tup_num : r_hist[idx];

		}
	}
}

#endif /* NUM_PASSES != 1 */


void shr_uni_bc_join(const relation_t * const r_rel, const relation_t * const s_rel, relation_t * const r_tmp, join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
	join_arg -> checksum = 0;

	hashkey_t max_hashkey = next_pow_2(r_rel->tuple_num);
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
	memset_localize(join_arg->intermediate, HASHKEY_T_SIZE * max_hashkey + KEY_T__SIZE * r_rel->tuple_num);
#ifdef PHJ_MBP	
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

	/* build the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */

	bucket = (rowid_t *) (join_arg -> intermediate);
	next = (rowid_t *) (join_arg -> intermediate + HASHKEY_T_SIZE * max_hashkey);

	for (size_t idx = 0; idx < r_rel->tuple_num;) {
		tmp_hashkey = IDHASH( r_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);
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

	short matched_flag;

	for (size_t idx = 0; idx < s_rel->tuple_num; idx ++ ) {
		tmp_hashkey = IDHASH(s_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);

		for (rowid_t hit = bucket[tmp_hashkey]; hit > 0; hit = next[hit-1]) {
			// if (s_rel->tuples[idx].key == r_rel->tuples[hit-1].key) {
			// 	(join_arg -> matched_cnt) ++;
			// 	(join_arg -> checksum) += r_rel->tuples[hit-1].row_id + s_rel->tuples[idx].row_id;
			// }

			matched_flag = (s_rel->tuples[idx].key == r_rel->tuples[hit-1].key);
			join_arg->matched_cnt += matched_flag;
			join_arg->checksum += (r_rel->tuples[hit-1].row_id + s_rel->tuples[idx].row_id)*matched_flag;
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


void shr_uni_lp_join(const relation_t * const r_rel, const relation_t * const s_rel, relation_t * const r_tmp, join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
	join_arg -> checksum = 0;

	hashkey_t max_hashkey = next_pow_2(r_rel->tuple_num);

	/* the & mask for hashing, for we use some leftmost bits for radix partitioning */
	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	/* the & mask for modulo to max_hashkey buckets */
	key_t_ hashmask_0 = max_hashkey - 1;

	hashkey_t tmp_hashkey;

	tuple_t* lp_table;

#ifdef PHJ_MBP
	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
#endif /* PHJ_MBP */

	/* memset the intermediate */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_start);
#endif /* PHJ_MBP */

	memset_localize(join_arg->intermediate, TUPLE_T_SIZE * max_hashkey);

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

	/* build the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */

	lp_table = (tuple_t *) (join_arg -> intermediate);

	for (size_t idx = 0; idx < r_rel->tuple_num; idx ++ ) {
		tmp_hashkey = IDHASH(r_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);

		while (  (lp_table + tmp_hashkey)->key ) {									// the min_key is at least 1
			tmp_hashkey = (tmp_hashkey + 1) & hashmask_0;
		}

		store_a_tuple(lp_table + tmp_hashkey, & r_rel->tuples[idx]);
	}

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_end);
#endif /* PHJ_MBP */

	/* probe the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_start);
#endif /* PHJ_MBP */
	short matched_flag;

	for (size_t idx = 0; idx < s_rel->tuple_num; idx ++) {
		tmp_hashkey = IDHASH(s_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);
#ifdef TEST_SELECTIVITY
		bool flag = true;
		hashkey_t pos_0 = tmp_hashkey;


		while ( ( (lp_table + tmp_hashkey)->key ) 										// the min_key is at least 1
			&& ( (lp_table + tmp_hashkey)->key != s_rel->tuples[idx].key)
			&& ( tmp_hashkey != pos_0 || flag) ) {
			flag = false;
			tmp_hashkey = (tmp_hashkey + 1) & hashmask_0;

		}

		// join_arg->matched_cnt += ( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) ) ? 1: 0;
		// join_arg->checksum +=  ( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) ) ?
		// 	s_rel->tuples[idx].row_id + (lp_table + tmp_hashkey)->row_id : 0;

		matched_flag = ( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) );
		join_arg->matched_cnt += matched_flag;
		join_arg->checksum += (s_rel->tuples[idx].row_id + (lp_table + tmp_hashkey)->row_id)*matched_flag;
#else /* TEST_SELECTIVITY */
		while ( ( (lp_table + tmp_hashkey)->key ) 									// the min_key is at least 1
			&& ( (lp_table + tmp_hashkey)->key != s_rel->tuples[idx].key) ) {
			tmp_hashkey = (tmp_hashkey + 1) & hashmask_0;
		}

		// join_arg->matched_cnt += ( (lp_table + tmp_hashkey)->key ) ? 1: 0;
		// join_arg->checksum +=  ( (lp_table + tmp_hashkey)->key ) ?
		// 	s_rel->tuples[idx].row_id + (lp_table + tmp_hashkey)->row_id : 0;

		matched_flag = ( (lp_table + tmp_hashkey)->key != 0 );
		join_arg->matched_cnt += matched_flag;
		join_arg->checksum += (s_rel->tuples[idx].row_id + (lp_table + tmp_hashkey)->row_id)*matched_flag;
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


void shr_uni_sc_join(const relation_t * const r_rel, const relation_t * const s_rel, relation_t * const r_tmp, join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
	join_arg -> checksum = 0;

	hashkey_t max_hashkey = next_pow_2(r_rel->tuple_num) / PHJ_SC_BUCKET_CAPACITY;
	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	hashkey_t tmp_hashkey;

	phj_sc_bkt_t *buckets, *free, *curr, *nxt, *tmp;
	tuple_t *dest;
	size_t free_idx = 0;

#ifdef PHJ_MBP
	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
#endif /* PHJ_MBP */

	/* memset the intermediate */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_start);
#endif /* PHJ_MBP */
	memset_localize(join_arg->intermediate, PHJ_SC_BKT_T_SIZE * max_hashkey * 2);
#ifdef PHJ_MBP	
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

	/* build the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */

	buckets = (phj_sc_bkt_t *) (join_arg -> intermediate);
	free = (phj_sc_bkt_t *) (join_arg -> intermediate + PHJ_SC_BKT_T_SIZE * max_hashkey);

	for (size_t idx = 0; idx < r_rel->tuple_num; idx ++) {
		tmp_hashkey = IDHASH(r_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);

		curr = buckets + tmp_hashkey;
		nxt = curr->next;

		if (curr->count == PHJ_SC_BUCKET_CAPACITY) {
			if (!nxt || nxt->count == PHJ_SC_BUCKET_CAPACITY) {
				tmp = free + free_idx;
				free_idx ++;
				curr->next = tmp;
				tmp->next = nxt;
				tmp->count = 1;
				dest = tmp->tuples;
			} else {
				dest = nxt->tuples + nxt->count;
				(nxt->count) ++;
			}
		} else {
			dest = curr->tuples + curr->count;
			(curr->count) ++;
		}

		store_a_tuple(dest, & r_rel->tuples[idx]);
	}

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_end);
#endif /* PHJ_MBP */

	/* probe the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_start);
#endif /* PHJ_MBP */
	short matched_flag;

	for (size_t idx = 0; idx < s_rel->tuple_num; idx ++) {
		tmp_hashkey = IDHASH(s_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);

		tmp = buckets + tmp_hashkey;

		do {
			for (size_t jdx = 0; jdx < tmp->count; jdx ++) {
				// if (s_rel->tuples[idx].key == tmp->tuples[jdx].key) {
				// 	(join_arg -> matched_cnt) ++;
				// 	(join_arg -> checksum) += tmp->tuples[jdx].row_id + s_rel->tuples[idx].row_id;
				// }
				matched_flag = (s_rel->tuples[idx].key == tmp->tuples[jdx].key);
				join_arg->matched_cnt += matched_flag;
				join_arg->checksum += (tmp->tuples[jdx].row_id + s_rel->tuples[idx].row_id)*matched_flag;
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


void shr_uni_hr_join(const relation_t * const r_rel, const relation_t * const s_rel, relation_t * const r_tmp, join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
	join_arg -> checksum = 0;

	hashkey_t max_hashkey = next_pow_2(r_rel->tuple_num);

	/* 4 times smaller hash table index, thus 4 time more comparison than bc_join * lp_join */
	// max_hashkey >>= HR_HRO_FACTOR;
	// if (max_hashkey < (1<<HR_HRO_FACTOR)) {
	// 	max_hashkey = (1<<HR_HRO_FACTOR);
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

	for (size_t idx = 0; idx < r_rel->tuple_num; idx ++) {
		tmp_hashkey = IDHASH(r_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);
		hist[tmp_hashkey] ++;
	}

	for (size_t idx = 0; idx < max_hashkey; idx ++) {
		accum_num += hist[idx];
		hist[idx] = accum_num - hist[idx];
	}

	for (size_t idx = 0; idx < r_rel->tuple_num; idx ++) {
		tmp_hashkey = IDHASH(r_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);
		store_a_tuple(r_tmp->tuples + hist[tmp_hashkey], & r_rel->tuples[idx]);
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

	short matched_flag;
	for (size_t idx = 0; idx < s_rel->tuple_num; idx ++) {
		tmp_hashkey = IDHASH(s_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);
		start_idx = hist[ tmp_hashkey-(tmp_hashkey!=0) ]*(tmp_hashkey!=0);
		end_idx = hist[tmp_hashkey];
	
		for (size_t jdx = start_idx; jdx < end_idx; jdx ++) {
			// if (s_rel->tuples[idx].key == r_tmp->tuples[jdx].key) {
			// 	(join_arg->matched_cnt) ++;
			// 	(join_arg->checksum) += r_tmp->tuples[jdx].row_id + s_rel->tuples[idx].row_id;
			// }

			matched_flag = (s_rel->tuples[idx].key == r_tmp->tuples[jdx].key);
			join_arg->matched_cnt += matched_flag;
			join_arg->checksum += (r_tmp->tuples[jdx].row_id + s_rel->tuples[idx].row_id)*matched_flag;
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


#if TUPLE_T_SIZE <= AVX512_SIZE
void shr_uni_hro_join(const relation_t * const r_rel, const relation_t * const s_rel, relation_t * const r_tmp, join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
	join_arg -> checksum = 0;

	hashkey_t max_hashkey = next_pow_2(r_rel->tuple_num);

	/* shrink the hashtable size according to AVX512_SIZE/TUPLE_T_SIZE */
	max_hashkey >>= HR_HRO_FACTOR;
	if (max_hashkey < (1<<HR_HRO_FACTOR)) {
		max_hashkey = (1<<HR_HRO_FACTOR);
	}
	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	hashkey_t tmp_hashkey;

	size_t *hist;
	size_t accum_num = 0;
#ifdef PHJ_MBP
	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
#endif /* PHJ_MBP */

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_start);
#endif /* PHJ_MBP */

	memset_localize(join_arg->intermediate, HASHKEY_T_SIZE * max_hashkey + AVX512_SIZE * 3);				// 3 buffers for hashkeys, keys, and row_ids
	/* memset the padding region, in case the padding region happens to hold some matching keys */
	memset_localize(r_tmp->tuples + r_rel->tuple_num, AVX512_SIZE);

#ifdef PHJ_MBP	
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */
	hist = (size_t *) (join_arg -> intermediate);

	for (size_t idx = 0; idx < r_rel->tuple_num; idx ++) {
		tmp_hashkey = IDHASH(r_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);
		hist[tmp_hashkey] ++;
	}

	for (size_t idx = 0; idx < max_hashkey; idx ++) {
		accum_num += hist[idx];
		hist[idx] = accum_num - hist[idx];
	}

	for (size_t idx = 0; idx < r_rel->tuple_num; idx ++) {
		tmp_hashkey = IDHASH(r_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);
		store_a_tuple(r_tmp->tuples + hist[tmp_hashkey], & r_rel->tuples[idx]);
		hist[tmp_hashkey] ++;
	}
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_end);
#endif /* PHJ_MBP */


#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_start);
#endif /* PHJ_MBP */
	hashkey_t *hashkey_buffer = (hashkey_t *) (join_arg -> intermediate + HASHKEY_T_SIZE * max_hashkey);
	key_t_ *key_buffer = (key_t_ *) (hashkey_buffer + AVX512_SIZE/HASHKEY_T_SIZE);							// AVX512 can hold 8 hashkeys
	rowid_t *rowid_buffer = (rowid_t *) (key_buffer + AVX512_SIZE/KEY_T__SIZE);								// AVX512 can hold 8 keys

	__m512i query_key;
	__m512i query_rowid;
	__m512i tup_array;

	__m512i tmp_avx_checksum;
	__m512i avx_checksum = _mm512_set1_epi64(0);
	__m512i avx_matched_cnt = _mm512_set1_epi64(0);

	__mmask8 res_mask;
	__m512i all_one = _mm512_set1_epi64(1);

	size_t start_idx;
	size_t end_idx;


	for (size_t idx = 0; idx < s_rel->tuple_num / (AVX512_UNIT_NUM); idx ++) {

		for (int jdx = 0; jdx < AVX512_UNIT_NUM; jdx ++) {
			key_buffer[jdx] = s_rel->tuples[idx*AVX512_UNIT_NUM+jdx].key;
			rowid_buffer[jdx] = s_rel->tuples[idx*AVX512_UNIT_NUM+jdx].row_id;
			tmp_hashkey = IDHASH(key_buffer[jdx], hashmask, NUM_RADIX_BITS);
			hashkey_buffer[jdx] = tmp_hashkey;
			// prefetch(r_tmp->tuples + hist[tmp_hashkey]);
			prefetch(r_tmp->tuples + hist[tmp_hashkey-(tmp_hashkey!=0)]*(tmp_hashkey!=0) );
		}

		for (int jdx = 0; jdx < AVX512_UNIT_NUM; jdx ++) {
			start_idx = hist[ hashkey_buffer[jdx]-(hashkey_buffer[jdx]!=0) ]*(hashkey_buffer[jdx]!=0);
			end_idx = hist[ hashkey_buffer[jdx] ];
			query_key = _mm512_set1_epi64(key_buffer[jdx]);
			query_rowid = _mm512_set1_epi64(rowid_buffer[jdx]);

			while (start_idx < end_idx) {
				tup_array = _mm512_loadu_si512(r_tmp->tuples + start_idx);

				res_mask = _mm512_cmpeq_epi64_mask(query_key, tup_array);
				avx_matched_cnt = _mm512_mask_add_epi64(avx_matched_cnt, res_mask, avx_matched_cnt, all_one);

				tmp_avx_checksum = _mm512_maskz_add_epi64(res_mask << 1, tup_array, query_rowid);
				avx_checksum = _mm512_mask_add_epi64(avx_checksum, res_mask << 1, avx_checksum, tmp_avx_checksum);
				
				start_idx += AVX512_UNIT_NUM;
			}
		}
	}


	for (size_t idx = s_rel->tuple_num - (s_rel->tuple_num % (AVX512_UNIT_NUM)); idx < s_rel->tuple_num; idx ++) {

		tmp_hashkey = IDHASH(s_rel->tuples[idx].key, hashmask, NUM_RADIX_BITS);

		query_key = _mm512_set1_epi64(s_rel->tuples[idx].key);
		query_rowid = _mm512_set1_epi64(s_rel->tuples[idx].row_id);

		start_idx = hist[ tmp_hashkey-(tmp_hashkey!=0) ]*(tmp_hashkey!=0);
		end_idx = hist[ tmp_hashkey ];

		while (start_idx < end_idx) {
			tup_array = _mm512_loadu_si512(r_tmp->tuples + start_idx);

			res_mask = _mm512_cmpeq_epi64_mask(query_key, tup_array);
			avx_matched_cnt = _mm512_mask_add_epi64(avx_matched_cnt, res_mask, avx_matched_cnt, all_one);

			tmp_avx_checksum = _mm512_maskz_add_epi64(res_mask << 1, tup_array, query_rowid);
			avx_checksum = _mm512_mask_add_epi64(avx_checksum, res_mask << 1, avx_checksum, tmp_avx_checksum);

			start_idx += AVX512_UNIT_NUM;
		}
	}


	__m256i matched_cnt_1 = _mm512_extracti32x8_epi32(avx_matched_cnt, 0);
	__m256i matched_cnt_2 = _mm512_extracti32x8_epi32(avx_matched_cnt, 1);
	join_arg->matched_cnt += _mm256_extract_epi64(matched_cnt_1, 0) + _mm256_extract_epi64(matched_cnt_1, 2) +
		_mm256_extract_epi64(matched_cnt_2, 0) + _mm256_extract_epi64(matched_cnt_2, 2);
	
	__m256i checksum_1 = _mm512_extracti32x8_epi32(avx_checksum, 0);
	__m256i checksum_2 = _mm512_extracti32x8_epi32(avx_checksum, 1);
	join_arg->checksum += _mm256_extract_epi64(checksum_1, 1) + _mm256_extract_epi64(checksum_1, 3) +
		_mm256_extract_epi64(checksum_2, 1) + _mm256_extract_epi64(checksum_2, 3);


#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_end);
#endif /* PHJ_MBP */


#ifdef PHJ_MBP
	join_arg->memset_time = diff_sec(memset_start, memset_end);
	join_arg->build_time = diff_sec(build_start, build_end);
	join_arg->probe_time = diff_sec(probe_start, probe_end);
#endif /* PHJ_MBP */
}
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */


void *shr_uni_prj_thread_buildpart(void *param) {
	phj_shr_uni_arg_t *phj_arg = (phj_shr_uni_arg_t *) param;

#ifdef WARMUP
	warmup_localize(phj_arg->r_rel.tuples, phj_arg->r_rel.tuple_num);
	warmup_localize(phj_arg->s_rel.tuples, phj_arg->s_rel.tuple_num);
#endif /* WARMUP */

	/* eliminate the page faults of the hash table */
	size_t memset_num_tuples_thr;
	size_t memset_num_tuples_size;
	memset_num_tuples_thr = phj_arg->r_tmp.tuple_num / BUILDPART_THREAD_NUM;
	memset_num_tuples_size = (phj_arg->_tid == BUILDPART_THREAD_NUM - 1) ? TUPLE_T_SIZE * ( phj_arg->r_tmp.tuple_num - phj_arg->_tid * memset_num_tuples_thr ) : TUPLE_T_SIZE * memset_num_tuples_thr;
	memset_localize(phj_arg->r_tmp.tuples + phj_arg->_tid * memset_num_tuples_thr, memset_num_tuples_size);
	memset_num_tuples_thr = phj_arg->s_tmp.tuple_num / BUILDPART_THREAD_NUM;
	memset_num_tuples_size = (phj_arg->_tid == BUILDPART_THREAD_NUM - 1) ? TUPLE_T_SIZE * ( phj_arg->s_tmp.tuple_num - phj_arg->_tid * memset_num_tuples_thr ) : TUPLE_T_SIZE * memset_num_tuples_thr;
	memset_localize(phj_arg->s_tmp.tuples + phj_arg->_tid * memset_num_tuples_thr, memset_num_tuples_size);

	int rv;

	size_t max_build_side_tup_num = 0;

	task_t *task;
	task_queue_t *part_queue = phj_arg->part_queue;

#if SKEW_HANDLING
	task_queue_t *skew_queue = phj_arg->skew_queue;
	task_t **skewtask = phj_arg->skewtask;
#endif /* SKEW_HANDLING */
	shr_uni_part_t part;

	part._tid = phj_arg->_tid;
	part.barrier = phj_arg->barrier;
	

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
	// size_t memset_num_tuples_thr;
	// size_t memset_num_tuples_size;
	// memset_num_tuples_thr = phj_arg->r_tmp.tuple_num / BUILDPART_THREAD_NUM;
	// memset_num_tuples_size = (phj_arg->_tid == BUILDPART_THREAD_NUM - 1) ? TUPLE_T_SIZE * ( phj_arg->r_tmp.tuple_num - phj_arg->_tid * memset_num_tuples_thr ) : TUPLE_T_SIZE * memset_num_tuples_thr;
	// memset_localize(phj_arg->r_tmp.tuples + phj_arg->_tid * memset_num_tuples_thr, memset_num_tuples_size);
	// memset_num_tuples_thr = phj_arg->s_tmp.tuple_num / BUILDPART_THREAD_NUM;
	// memset_num_tuples_size = (phj_arg->_tid == BUILDPART_THREAD_NUM - 1) ? TUPLE_T_SIZE * ( phj_arg->s_tmp.tuple_num - phj_arg->_tid * memset_num_tuples_thr ) : TUPLE_T_SIZE * memset_num_tuples_thr;
	// memset_localize(phj_arg->s_tmp.tuples + phj_arg->_tid * memset_num_tuples_thr, memset_num_tuples_size);


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

	/**************** PARTITIONING R ****************/
	part.rel = phj_arg->r_rel;
	part.tmp = phj_arg->r_tmp;
	part.output_offset = phj_arg->r_part_tup_num;
	part.rel_flag = 0;

	shr_uni_parallel_partition(&part, FANOUT_PASS2 * PADDING_UNIT_NUM, 
		phj_arg->r_avg_part_scaling_offset, phj_arg->latches);

	/* wait for all threads release the latches */
#ifdef SYNCSTATS
	SYNC_TIMER(phj_arg->localtimer->sync_1st_pass_buildside);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_1st_pass_buildside);
#endif /* SYNCSTATS */

	/**************** PARTITIONING S ****************/
	part.rel = phj_arg->s_rel;
	part.tmp = phj_arg->s_tmp;
	part.output_offset = phj_arg->s_part_tup_num;
	part.rel_flag = 1;

	shr_uni_parallel_partition(&part, FANOUT_PASS2 * PADDING_UNIT_NUM, 
		phj_arg->s_avg_part_scaling_offset, phj_arg->latches);

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
	size_t r_num_tup;
	size_t s_num_tup;
	size_t r_current_offset;
	size_t s_current_offset;
	if (phj_arg->_tid == 0) {

		for (size_t idx = 0; idx < FANOUT_PASS1; idx ++) {

			/* within the first thread, thus no memory leak */
			r_num_tup = phj_arg->r_part_tup_num[idx];
			s_num_tup = phj_arg->s_part_tup_num[idx];

			r_current_offset = idx * (phj_arg->r_avg_part_scaling_offset + FANOUT_PASS2 * PADDING_UNIT_NUM);
			s_current_offset = idx * (phj_arg->s_avg_part_scaling_offset + FANOUT_PASS2 * PADDING_UNIT_NUM);

#ifdef SKEW_HANDLING
			if ( r_num_tup > 0 && s_num_tup > 0 && (r_num_tup > THRESHOLD_0 || s_num_tup > THRESHOLD_0) ) {

				task = task_queue_get_slot(skew_queue);

				task->r_rel.tuples = phj_arg->r_tmp.tuples + r_current_offset;
				task->r_rel.tuple_num = r_num_tup;
				task->r_tmp.tuples = phj_arg->r_rel.tuples + r_current_offset;

				task->s_rel.tuples = phj_arg->s_tmp.tuples + s_current_offset;
				task->s_rel.tuple_num = s_num_tup;
				task->s_tmp.tuples = phj_arg->s_rel.tuples + s_current_offset;

				task->r_global_offset = r_current_offset;
				task->s_global_offset = s_current_offset;

				task_queue_add(skew_queue, task);

			} else
#endif /* SKEW_HANDLING */
			if (r_num_tup > 0 && s_num_tup > 0) {
				task = task_queue_get_slot(part_queue);

				task->r_rel.tuples = phj_arg->r_tmp.tuples + r_current_offset;
				task->r_rel.tuple_num = r_num_tup;
				task->r_tmp.tuples = phj_arg->r_rel.tuples + r_current_offset;
				task->s_rel.tuples = phj_arg->s_tmp.tuples + s_current_offset;
				task->s_rel.tuple_num = s_num_tup;
				task->s_tmp.tuples = phj_arg->s_rel.tuples + s_current_offset;

#if NUM_PASSES != 1
				task->r_global_offset = r_current_offset;
				task->s_global_offset = s_current_offset;
#endif /* NUM_PASSES != 1 */

				task_queue_add(part_queue, task);

				max_build_side_tup_num = max_build_side_tup_num >= r_num_tup ? max_build_side_tup_num : r_num_tup;
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
	size_t *ser_r_hist = (size_t *) alloc_memory( sizeof(size_t) * FANOUT_PASS2, CACHELINE_SIZE);
	size_t *ser_s_hist = (size_t *) alloc_memory( sizeof(size_t) * FANOUT_PASS2, CACHELINE_SIZE);

	memset_localize(ser_r_hist, sizeof(size_t) * FANOUT_PASS2);
	memset_localize(ser_s_hist, sizeof(size_t) * FANOUT_PASS2);

	void *ser_swwcb = NULL;
#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM
	ser_swwcb = new_alloc_nvm(sizeof(swwcb_t) * FANOUT_PASS2);
	pmem_memset_persist(ser_swwcb, 0, sizeof(swwcb_t) * FANOUT_PASS2);
#else /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
	ser_swwcb = alloc_aligned_dram(sizeof(swwcb_t) * FANOUT_PASS2, CACHELINE_SIZE);
	memset(ser_swwcb, 0, sizeof(swwcb_t) * FANOUT_PASS2);
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */

	/* since we have the 2nd pass, no need to keep this value of the 1st pass */
	max_build_side_tup_num = 0;

	while ( ( task = task_queue_get_atomic(part_queue) ) ) {
		shr_uni_serial_partition(task, phj_arg->join_queue, 
			FANOUT_PASS2, NUM_RADIX_BITS / NUM_PASSES, 
			ser_r_hist, ser_s_hist, ser_swwcb, &max_build_side_tup_num, PADDING_UNIT_NUM, 
			phj_arg->r_avg_part_scaling_offset/FANOUT_PASS2, phj_arg->s_avg_part_scaling_offset/FANOUT_PASS2
		);
	}
	/* store the value to the respective data slot of the sharing data structure */
	phj_arg->max_build_side_tup_num[phj_arg->_tid] = max_build_side_tup_num;

#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM
	pmem_unmap(ser_swwcb, sizeof(swwcb_t) * FANOUT_PASS2);
#else /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
	free(ser_swwcb);
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */
	dealloc_memory(ser_r_hist, sizeof(size_t) * FANOUT_PASS2);
	dealloc_memory(ser_s_hist, sizeof(size_t) * FANOUT_PASS2);
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

	phj_arg->r_avg_part_scaling_offset = phj_arg->r_avg_part_scaling_offset / FANOUT_PASS2;
	phj_arg->s_avg_part_scaling_offset = phj_arg->s_avg_part_scaling_offset / FANOUT_PASS2;

	size_t skew_hand_num_per_thr;
	size_t skew_hand_r_num_tup, skew_hand_s_num_tup;
	size_t skew_hand_r_current_offset, skew_hand_s_current_offset;
	task_t *skew_hand_task;
	size_t skew_hand_large_task_num_tup;

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
		phj_arg->sync_idle_skew_buildside_acc += diff_sec(phj_arg->localtimer->sync_skew_buildside,
			phj_arg->globaltimer->sync_skew_buildside);
		phj_arg->sync_idle_skew_part_acc += diff_sec(phj_arg->localtimer->sync_skew_part,
			phj_arg->globaltimer->sync_skew_part);
#ifdef PHJ_SYNCSTATS
		// phj_arg->global_acc_skew_taskfetch += diff_sec();
		phj_arg->global_acc_skew_prepart_memset += diff_sec( phj_arg->globaltimer->sync_skew_taskfetch, 
			phj_arg->globaltimer->sync_skew_prepart_memset );
		phj_arg->global_acc_skew_buildside += diff_sec( phj_arg->globaltimer->sync_skew_prepart_memset,
			phj_arg->globaltimer->sync_skew_buildside );
		phj_arg->global_acc_skew_part += diff_sec( phj_arg->globaltimer->sync_skew_buildside,
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

		/* explicitly zero the skew_hand offset array */
#ifdef OUTPUT_OFFSET_NVM
		pmem_memset_persist(phj_arg->skew_hand_r_part_tup_num, 0, sizeof(size_t) * FANOUT_PASS2 );
		pmem_memset_persist(phj_arg->skew_hand_s_part_tup_num, 0, sizeof(size_t) * FANOUT_PASS2 );
#else /* OUTPUT_OFFSET_NVM */
		memset(phj_arg->skew_hand_r_part_tup_num, 0, sizeof(size_t) * FANOUT_PASS2 );
		memset(phj_arg->skew_hand_s_part_tup_num, 0, sizeof(size_t) * FANOUT_PASS2 );
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

		skew_hand_num_per_thr = (*skewtask)->r_rel.tuple_num / BUILDPART_THREAD_NUM;

		part.rel.tuples = (*skewtask)->r_rel.tuples + phj_arg->_tid * skew_hand_num_per_thr;
		part.tmp = (*skewtask)->r_tmp;
		part.output_offset = phj_arg->skew_hand_r_part_tup_num;

		part.rel.tuple_num = (phj_arg->_tid == BUILDPART_THREAD_NUM -1) ? 
			((*skewtask)->r_rel.tuple_num - phj_arg->_tid * skew_hand_num_per_thr) : skew_hand_num_per_thr;

		part.rel_flag = 0;

		shr_uni_parallel_partition(&part, PADDING_UNIT_NUM, phj_arg->r_avg_part_scaling_offset, phj_arg->latches);

		/* wait for all threads release the latches */
#ifdef SYNCSTATS
		SYNC_TIMER(phj_arg->localtimer->sync_skew_buildside);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(phj_arg->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(phj_arg->_tid, phj_arg->globaltimer->sync_skew_buildside);
#endif /* SYNCSTATS */

		/**************** PARTITIONING S ****************/
		skew_hand_num_per_thr = (*skewtask)->s_rel.tuple_num / BUILDPART_THREAD_NUM;

		part.rel.tuples = (*skewtask)->s_rel.tuples + phj_arg->_tid * skew_hand_num_per_thr;
		part.tmp = (*skewtask)->s_tmp;
		part.output_offset = phj_arg->skew_hand_s_part_tup_num;

		part.rel.tuple_num = (phj_arg->_tid == BUILDPART_THREAD_NUM -1) ? 
			((*skewtask)->s_rel.tuple_num - phj_arg->_tid * skew_hand_num_per_thr) : 
			skew_hand_num_per_thr;

		part.rel_flag = 1;

		shr_uni_parallel_partition(&part, PADDING_UNIT_NUM, phj_arg->s_avg_part_scaling_offset, phj_arg->latches);

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
				skew_hand_r_num_tup = phj_arg->skew_hand_r_part_tup_num[idx];
				skew_hand_s_num_tup = phj_arg->skew_hand_s_part_tup_num[idx];
				
				skew_hand_r_current_offset = idx*(phj_arg->r_avg_part_scaling_offset+PADDING_UNIT_NUM);
				skew_hand_s_current_offset = idx*(phj_arg->s_avg_part_scaling_offset+PADDING_UNIT_NUM);

				max_build_side_tup_num = max_build_side_tup_num > skew_hand_r_num_tup ? max_build_side_tup_num : skew_hand_r_num_tup;

				if ( skew_hand_r_num_tup > 0 && skew_hand_s_num_tup > 0 && (skew_hand_r_num_tup > THRESHOLD_1 || skew_hand_s_num_tup > THRESHOLD_1) ) {
					for (size_t jdx = 0; jdx < PROBEJOIN_THREAD_NUM; jdx ++) {
						skew_hand_task = task_queue_get_slot(part_queue);
						
						skew_hand_task->r_rel.tuple_num = skew_hand_task->r_tmp.tuple_num = skew_hand_r_num_tup;

						skew_hand_task->r_rel.tuples = (*skewtask)->r_tmp.tuples + skew_hand_r_current_offset;
						skew_hand_task->r_tmp.tuples = (*skewtask)->r_rel.tuples + skew_hand_r_current_offset;

						skew_hand_large_task_num_tup = (jdx == PROBEJOIN_THREAD_NUM - 1) ?
							skew_hand_s_num_tup - jdx * (skew_hand_s_num_tup / PROBEJOIN_THREAD_NUM) :
							skew_hand_s_num_tup / PROBEJOIN_THREAD_NUM;

						skew_hand_task->s_rel.tuple_num = skew_hand_task->s_tmp.tuple_num = skew_hand_large_task_num_tup;

						skew_hand_task->s_rel.tuples = (*skewtask)->s_tmp.tuples + skew_hand_s_current_offset + jdx * ( skew_hand_s_num_tup / PROBEJOIN_THREAD_NUM );
						skew_hand_task->s_tmp.tuples = (*skewtask)->s_rel.tuples + skew_hand_s_current_offset + jdx * ( skew_hand_s_num_tup / PROBEJOIN_THREAD_NUM );

						task_queue_add(part_queue, skew_hand_task);
					}

				} else {
					if (skew_hand_r_num_tup > 0 && skew_hand_s_num_tup > 0) {

						skew_hand_task = task_queue_get_slot(phj_arg->join_queue);

						skew_hand_task->r_rel.tuple_num = skew_hand_task->r_tmp.tuple_num = skew_hand_r_num_tup;
						skew_hand_task->r_rel.tuples = (*skewtask)->r_tmp.tuples + skew_hand_r_current_offset;
						skew_hand_task->r_tmp.tuples = (*skewtask)->r_rel.tuples + skew_hand_r_current_offset;

						skew_hand_task->s_rel.tuple_num = skew_hand_task->s_tmp.tuple_num = skew_hand_s_num_tup;
						skew_hand_task->s_rel.tuples = (*skewtask)->s_tmp.tuples + skew_hand_s_current_offset;
						skew_hand_task->s_tmp.tuples = (*skewtask)->s_rel.tuples + skew_hand_s_current_offset;

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

	return NULL;
}


void *shr_uni_prj_thread_probejoin(void *param) {
	phj_shr_uni_arg_t *phj_arg = (phj_shr_uni_arg_t *) param;
	join_arg_t join_arg;

	int rv;
	task_t *task;

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

	while ( ( task = task_queue_get_atomic(phj_arg->join_queue) ) ) {
		phj_arg->joinfunction(&task->r_rel, &task->s_rel, &task->r_tmp, &join_arg);

		phj_arg->matched_cnt += join_arg.matched_cnt;
		phj_arg->checksum += join_arg.checksum;
		(phj_arg->parts_processed) ++;

#ifdef PHJ_MBP
		phj_arg->memset_time += join_arg.memset_time;
		phj_arg->build_time += join_arg.build_time;
		phj_arg->probe_time += join_arg.probe_time;
#endif /* PHJ_MBP */

	}

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

	return NULL;
}


void shr_uni_join_init_run(const datameta_t datameta, timekeeper_t * const timekeeper, 
	void (*f_join)(const relation_t * const, const relation_t * const, relation_t * const, join_arg_t * const)
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

	size_t r_rel_tuples_size = TUPLE_T_SIZE * r_relation.tuple_num * SHR_UNI_SCALING_FACTOR + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS));
	size_t s_rel_tuples_size = TUPLE_T_SIZE * s_relation.tuple_num * SHR_UNI_SCALING_FACTOR + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS));
#ifdef USE_NVM
	r_relation.tuples = (tuple_t*) new_alloc_nvm(r_rel_tuples_size);
	s_relation.tuples = (tuple_t*) new_alloc_nvm(s_rel_tuples_size);
#else /* USE_NVM */
#ifdef USE_HUGE
	r_relation.tuples = (tuple_t*) alloc_huge_dram(r_rel_tuples_size);
	s_relation.tuples = (tuple_t*) alloc_huge_dram(s_rel_tuples_size);
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	r_relation.tuples = (tuple_t*) alloc_aligned_dram(r_rel_tuples_size, CACHELINE_SIZE);
	s_relation.tuples = (tuple_t*) alloc_aligned_dram(s_rel_tuples_size, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memcpy(r_relation.tuples, r_original_addr, TUPLE_T_SIZE*r_relation.tuple_num);
	parallel_memcpy(s_relation.tuples, s_original_addr, TUPLE_T_SIZE*s_relation.tuple_num);
	parallel_memset_localize(r_relation.tuples + r_relation.tuple_num, r_rel_tuples_size-TUPLE_T_SIZE*r_relation.tuple_num);
	parallel_memset_localize(s_relation.tuples + s_relation.tuple_num, s_rel_tuples_size-TUPLE_T_SIZE*s_relation.tuple_num);
	pmem_unmap(r_original_addr, TUPLE_T_SIZE * r_relation.tuple_num);
	pmem_unmap(s_original_addr, TUPLE_T_SIZE * s_relation.tuple_num);

	/**************** CREATE INTERMEDIATES FOR PARTITIONING ****************/
	relation_t r_tmp_relation;
	r_tmp_relation.tuple_num = r_relation.tuple_num;
#ifdef USE_NVM
	r_tmp_relation.tuples = (tuple_t *) new_alloc_nvm(r_rel_tuples_size);
#else /* USE_NVM */
#ifdef USE_HUGE
	r_tmp_relation.tuples = (tuple_t *) alloc_huge_dram(r_rel_tuples_size);
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	r_tmp_relation.tuples = (tuple_t *) alloc_aligned_dram(r_rel_tuples_size, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memset_localize(r_tmp_relation.tuples + r_tmp_relation.tuple_num, r_rel_tuples_size-TUPLE_T_SIZE*r_tmp_relation.tuple_num);

	relation_t s_tmp_relation;
	s_tmp_relation.tuple_num = s_relation.tuple_num;
#ifdef USE_NVM
	s_tmp_relation.tuples = (tuple_t *) new_alloc_nvm(s_rel_tuples_size);
#else /* USE_NVM */
#ifdef USE_HUGE
	s_tmp_relation.tuples = (tuple_t *) alloc_huge_dram(s_rel_tuples_size);
#else /* USE_HUGE */
	/* aligned on a 64-byte boundary */
	s_tmp_relation.tuples = (tuple_t *) alloc_aligned_dram(s_rel_tuples_size, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memset_localize(s_tmp_relation.tuples + s_tmp_relation.tuple_num, s_rel_tuples_size-TUPLE_T_SIZE*s_tmp_relation.tuple_num);


	/**************** CREATE SHARING LOCKS FOR PARTITIONING ****************/
	// lock_t *latches;
	pthread_mutex_t *latches;
#ifdef SHARE_LOCK_ON_NVM
	// latches = (lock_t *) new_alloc_nvm( sizeof(lock_t) * (1 << NUM_RADIX_BITS) );
	// pmem_memset_persist( (void *) latches, 0, sizeof(lock_t) *  (1 << NUM_RADIX_BITS) );
	latches = (pthread_mutex_t *) new_alloc_nvm( sizeof(pthread_mutex_t) * (1 << NUM_RADIX_BITS) );
	pmem_memset_persist( (void *) latches, 0, sizeof(pthread_mutex_t) *  (1 << NUM_RADIX_BITS) );
#else /* SHARE_LOCK_ON_NVM */
	// latches = (lock_t *) alloc_dram( sizeof(lock_t) * (1 << NUM_RADIX_BITS), CACHELINE_SIZE);
	// memset( (void *) latches, 0, sizeof(lock_t) * (1 << NUM_RADIX_BITS) );
	latches = (pthread_mutex_t *) alloc_dram( sizeof(pthread_mutex_t) * (1 << NUM_RADIX_BITS), CACHELINE_SIZE );
	memset( (void *) latches, 0, sizeof(pthread_mutex_t) *  (1 << NUM_RADIX_BITS) );
#endif /* SHARE_LOCK_ON_NVM */
	for (size_t idx = 0; idx < (1 << NUM_RADIX_BITS); idx ++) {
		pthread_mutex_init( latches+idx, NULL );
	}


	/**************** CREATE PART_TUP_NUM FOR PARTITIONING ****************/
	size_t *r_part_tup_num, *s_part_tup_num;	
#ifdef OUTPUT_OFFSET_NVM
	r_part_tup_num = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS1);
	s_part_tup_num = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS1);
	pmem_memset_persist(r_part_tup_num, 0, sizeof(size_t) * FANOUT_PASS1);
	pmem_memset_persist(s_part_tup_num, 0, sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	r_part_tup_num = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS1, CACHELINE_SIZE);
	s_part_tup_num = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS1, CACHELINE_SIZE);
	memset(r_part_tup_num, 0, sizeof(size_t) * FANOUT_PASS1);
	memset(s_part_tup_num, 0, sizeof(size_t) * FANOUT_PASS1);
#endif /* OUTPUT_OFFSET_NVM */

#ifdef SKEW_HANDLING
	size_t *skew_hand_r_part_tup_num, *skew_hand_s_part_tup_num;
#ifdef OUTPUT_OFFSET_NVM
	skew_hand_r_part_tup_num = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS2 );
	skew_hand_s_part_tup_num = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS2 );
#else /* OUTPUT_OFFSET_NVM */
	skew_hand_r_part_tup_num = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS2, CACHELINE_SIZE);
	skew_hand_s_part_tup_num = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS2, CACHELINE_SIZE);
#endif /* OUTPUT_OFFSET_NVM */
#endif /* SKEW_HANDLING */

	task_queue_t *part_queue = task_queue_init(FANOUT_PASS1);
	task_queue_t *join_queue = task_queue_init(1 << NUM_RADIX_BITS);

#if SKEW_HANDLING
	task_queue_t *skew_queue;
	task_t *skewtask = NULL;
	skew_queue = task_queue_init(FANOUT_PASS1);
#endif /* SKEW_HANDLING */

	int max_thread_num = MAX(BUILDPART_THREAD_NUM, PROBEJOIN_THREAD_NUM);

	size_t max_build_side_tup_num[max_thread_num];
	memset(max_build_side_tup_num, 0, sizeof(size_t)*max_thread_num);

	pthread_t threadpool[max_thread_num];
	phj_shr_uni_arg_t args[max_thread_num];

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

#ifdef SYNCSTATS
	synctimer_phj_shr_uni_t *localtimer = (synctimer_phj_shr_uni_t *) malloc( max_thread_num * sizeof(synctimer_phj_shr_uni_t) );
	synctimer_phj_shr_uni_t *globaltimer = (synctimer_phj_shr_uni_t *) malloc( sizeof(synctimer_phj_shr_uni_t) );
	memset(localtimer, 0, max_thread_num * sizeof(synctimer_phj_shr_uni_t));
	memset(globaltimer, 0, sizeof(synctimer_phj_shr_uni_t));
#endif /* SYNCSTATS */

#ifdef PHJ_MBP
	double avg_memset_time = 0.0;
	double avg_build_time = 0.0;
	double avg_probe_time = 0.0;
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
	memset(PAPI_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );

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

		// (args+idx)-> r_org = r_relation.tuples;
		(args+idx)->r_rel.tuple_num = (idx == BUILDPART_THREAD_NUM -1) ? (datameta.r_tuple_num - idx * r_tuple_num_thr) : r_tuple_num_thr;
		(args+idx)->r_rel.tuples = r_relation.tuples + r_tuple_num_thr * idx;
		(args+idx)->r_tmp = r_tmp_relation;

		// (args+idx) -> s_org = s_relation.tuples;
		(args+idx)->s_rel.tuple_num = (idx == BUILDPART_THREAD_NUM -1) ? (datameta.s_tuple_num - idx * s_tuple_num_thr) : s_tuple_num_thr;
		(args+idx)->s_rel.tuples = s_relation.tuples + s_tuple_num_thr * idx;
		(args+idx)->s_tmp = s_tmp_relation;

		(args+idx)->max_build_side_tup_num = max_build_side_tup_num;

		(args+idx)->part_queue = part_queue;
		(args+idx)->join_queue = join_queue;
#ifdef SKEW_HANDLING
		(args+idx)->skew_queue = skew_queue;
		(args+idx)->skewtask = &skewtask;
#endif /* SKEW_HANDLING */

#ifdef SYNCSTATS
		(args+idx)->localtimer = localtimer + idx;
		(args+idx)->globaltimer = globaltimer;
#ifdef SKEW_HANDLING
		(args+idx)->sync_idle_skew_taskfetch_acc = 0.0;
		(args+idx)->sync_idle_skew_buildside_acc = 0.0;
		(args+idx)->sync_idle_skew_prepart_memset_acc = 0.0;
		(args+idx)->sync_idle_skew_part_acc = 0.0;
#ifdef PHJ_SYNCSTATS
		(args+idx)->global_acc_skew_taskfetch = 0.0;
		(args+idx)->global_acc_skew_prepart_memset = 0.0;
		(args+idx)->global_acc_skew_buildside = 0.0;
		(args+idx)->global_acc_skew_part = 0.0;
#endif /* PHJ_SYNCSTATS */
#endif /* SKEW_HANDLING */
#endif /* SYNCSTATS */

		(args+idx)->latches = latches;
		(args+idx)->r_part_tup_num = r_part_tup_num;
		(args+idx)->s_part_tup_num = s_part_tup_num;
		(args+idx)->r_avg_part_scaling_offset = SHR_UNI_SCALING_FACTOR * r_relation.tuple_num / FANOUT_PASS1;
		(args+idx)->s_avg_part_scaling_offset = SHR_UNI_SCALING_FACTOR * s_relation.tuple_num / FANOUT_PASS1;

#ifdef SKEW_HANDLING
		(args+idx)->skew_hand_r_part_tup_num = skew_hand_r_part_tup_num;
		(args+idx)->skew_hand_s_part_tup_num = skew_hand_s_part_tup_num;
#endif /* SKEW_HANDLING */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, shr_uni_prj_thread_buildpart, args+idx);
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
	task_queue_t *swap = join_queue;
	join_queue = part_queue;
	part_queue = swap;
#endif /* NUM_PASSES == 1 */

	size_t intermediate_size;
#if TUPLE_T_SIZE > 16
	if (f_join == shr_uni_lp_join) {
		intermediate_size = TUPLE_T_SIZE*next_pow_2(max_build_side_tup_num[0])*INTERMEDIATE_SCALING_FACTOR;
	} else {
		if (f_join == shr_uni_sc_join) {
			intermediate_size = PHJ_SC_BKT_T_SIZE*next_pow_2(max_build_side_tup_num[0])*INTERMEDIATE_SCALING_FACTOR/PHJ_SC_BUCKET_CAPACITY;
		} else {
			intermediate_size = KEY_T__SIZE*next_pow_2(max_build_side_tup_num[0])*INTERMEDIATE_SCALING_FACTOR;
		}
	}
#else /* TUPLE_T_SIZE > 16 */
	/* default setting: 16B tuple */
	if (f_join == shr_uni_sc_join) {
		intermediate_size = PHJ_SC_BKT_T_SIZE*next_pow_2(max_build_side_tup_num[0])*INTERMEDIATE_SCALING_FACTOR/PHJ_SC_BUCKET_CAPACITY;
	} else {
		intermediate_size = KEY_T__SIZE*next_pow_2(max_build_side_tup_num[0])*INTERMEDIATE_SCALING_FACTOR;
	}
#endif /* TUPLE_T_SIZE > 16 */

	void *intermediate = alloc_memory(intermediate_size*PROBEJOIN_THREAD_NUM, CACHELINE_SIZE);

#ifndef TEST_PARTITIONING

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		cpu_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->barrier = &probejoin_barrier;

		(args+idx)->matched_cnt = 0;
		(args+idx)->checksum = 0;
		(args+idx)->parts_processed = 0;
		(args+idx)->joinfunction = f_join;

		(args+idx)->max_build_side_tup_num = max_build_side_tup_num;

		(args+idx)->join_queue = join_queue;

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

		rv = pthread_create(&threadpool[idx], &attr, shr_uni_prj_thread_probejoin, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < PROBEJOIN_THREAD_NUM; idx ++) {
		pthread_join(threadpool[idx], NULL);
		matched_cnt += (args+idx)->matched_cnt;
		checksum += (args+idx)->checksum;
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

#endif /* TEST_PARTITIONING */

	purple();
	printf("matched_cnt: %zu\tcheck_sum: %zu\n", matched_cnt, checksum);

#ifdef SYNCSTATS

	printf("[SYNCSTATS] TID\tPREPART_MEMSET\t1ST_PASS_BUILDSIDE\t1ST_PASS\t1ST_PASS_QUEUE\tPART\t");

#ifdef SKEW_HANDLING
	printf("SKEW_TASKFETCH\tSKEW_PREPART_MEMSET\tSKEW_BUILDSIDE\tSKEW_PART\tSKEW_QUEUE\t");
#endif /* SKEW_HANDLING */

	printf("MAX_HASHTABLE\n");

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%.9f\t%.9f\t%.9f\t%.9f\t%.9f\t", 
			idx, 
			diff_sec( (args+idx)->localtimer->sync_prepart_memset,
				(args+idx)->globaltimer->sync_prepart_memset ),
			diff_sec( (args+idx)->localtimer->sync_1st_pass_buildside,
				(args+idx)->globaltimer->sync_1st_pass_buildside),
			diff_sec( (args+idx)->localtimer->sync_1st_pass,
				(args+idx)->globaltimer->sync_1st_pass ),
			diff_sec( (args+idx)->localtimer->sync_1st_pass_queue,
				(args+idx)->globaltimer->sync_1st_pass_queue ),
			diff_sec( (args+idx)->localtimer->sync_part,
				(args+idx)->globaltimer->sync_part )
		);

#ifdef SKEW_HANDLING
		printf("%.9f\t%.9f\t%.9f\t%.9f\t%.9f\t", 
			(args+idx)->sync_idle_skew_taskfetch_acc,
			(args+idx)->sync_idle_skew_prepart_memset_acc,
			(args+idx)->sync_idle_skew_buildside_acc,
			(args+idx)->sync_idle_skew_part_acc, 
			diff_sec( (args+idx)->localtimer->sync_skew_queue,
				(args+idx)->globaltimer->sync_skew_queue)
		);
#endif /* SKEW_HANDLING */
		printf("%.9f\n",
			diff_sec( (args+idx)->localtimer->sync_max_hashtable,
				(args+idx)->globaltimer->sync_max_hashtable )
		);
	}


#ifdef PHJ_SYNCSTATS
	printf("[PHJ_SYNCSTATS] PREPART_MEMSET: %.9f\t1ST_PASS_BUILDSIDE: %.9f\t1ST_PASS: %.9f\t1ST_PASS_QUEUE: %.9f\tPART: %.9f\n",
		diff_sec( args[0].part_start,
			args[0].globaltimer->sync_prepart_memset ),
		diff_sec( args[0].globaltimer->sync_prepart_memset,
			args[0].globaltimer->sync_1st_pass_buildside ),
		diff_sec( args[0].globaltimer->sync_1st_pass_buildside,
			args[0].globaltimer->sync_1st_pass ),
		diff_sec( args[0].globaltimer->sync_1st_pass,
			args[0].globaltimer->sync_1st_pass_queue ),
		diff_sec( args[0].globaltimer->sync_1st_pass_queue,
			args[0].globaltimer->sync_part )
	);
#ifdef SKEW_HANDLING
	// printf("[PHJ_SYNCSTATS] SKEW_TASKFETCH: \tSKEW_PREPART_MEMSET: \tSKEW_HIST_R: \t");
	printf("[PHJ_SYNCSTATS] SKEW_PREPART_MEMSET: %.9f\tSKEW_BUILDSIDE: %.9f\tSKEW_PART: %.9f\tSKEW_QUEUE: %.9f\n",
		args[0].global_acc_skew_prepart_memset, 
		args[0].global_acc_skew_buildside,
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
#endif /* TEST_PARTITIONING */

#endif /* SYNCSTATS */

#ifdef PHJ_MBP
	printf("[PHJ_MBP] avg_memset_time: %.9f\tavg_build_time: %.9f\tavg_probe_time: %.9f\n",
		avg_memset_time/PROBEJOIN_THREAD_NUM, avg_build_time/PROBEJOIN_THREAD_NUM, avg_probe_time/PROBEJOIN_THREAD_NUM
	);
#endif /* PHJ_MBP */
#ifdef USE_PMWATCH
	for (int idx = 0; idx < NVMCount; idx ++) {
		PMWatch_output[idx].total_bytes_read = PMWatch_output_buildpart[idx].total_bytes_read + 
			PMWatch_output_probejoin[idx].total_bytes_read;
		PMWatch_output[idx].total_bytes_written = PMWatch_output_buildpart[idx].total_bytes_written + 
			PMWatch_output_probejoin[idx].total_bytes_written;
		PMWatch_output[idx].media_read_ops = PMWatch_output_buildpart[idx].media_read_ops + 
			PMWatch_output_probejoin[idx].media_read_ops;
		PMWatch_output[idx].media_write_ops = PMWatch_output_buildpart[idx].media_write_ops +
			PMWatch_output_probejoin[idx].media_write_ops;
		PMWatch_output[idx].read_64B_ops_received = PMWatch_output_buildpart[idx].read_64B_ops_received + 
			PMWatch_output_probejoin[idx].read_64B_ops_received;
		PMWatch_output[idx].write_64B_ops_received = PMWatch_output_buildpart[idx].write_64B_ops_received + 
			PMWatch_output_probejoin[idx].write_64B_ops_received;
		PMWatch_output[idx].host_reads = PMWatch_output_buildpart[idx].host_reads + 
			PMWatch_output_probejoin[idx].host_reads;
		PMWatch_output[idx].host_writes = PMWatch_output_buildpart[idx].host_writes +
			PMWatch_output_probejoin[idx].host_writes;
		PMWatch_output[idx].read_hit_ratio = (PMWatch_output_buildpart[idx].read_hit_ratio + 
			PMWatch_output_probejoin[idx].read_hit_ratio) / 2;
		PMWatch_output[idx].write_hit_ratio = (PMWatch_output_buildpart[idx].write_hit_ratio + 
			PMWatch_output_probejoin[idx].write_hit_ratio) / 2;
		PMWatch_output[idx].read_amplification = (PMWatch_output_buildpart[idx].read_amplification + 
			PMWatch_output_probejoin[idx].read_amplification) / 2;
		PMWatch_output[idx].write_amplification = (PMWatch_output_buildpart[idx].write_amplification +
			PMWatch_output_probejoin[idx].write_amplification) / 2;
		printf("[PMWatch] DIMM: %d\n", idx);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_read: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].total_bytes_read, 
			PMWatch_output_buildpart[idx].total_bytes_read, PMWatch_output_probejoin[idx].total_bytes_read);
		printf("[PMWatch] DIMM: %d\ttotal_bytes_written: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].total_bytes_written,
			PMWatch_output_buildpart[idx].total_bytes_written, PMWatch_output_probejoin[idx].total_bytes_written);
		printf("[PMWatch] DIMM: %d\tmedia_read_ops: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].media_read_ops,
			PMWatch_output_buildpart[idx].media_read_ops, PMWatch_output_probejoin[idx].media_read_ops);
		printf("[PMWatch] DIMM: %d\tmedia_write_ops: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].media_write_ops,
			PMWatch_output_buildpart[idx].media_write_ops, PMWatch_output_probejoin[idx].media_write_ops);
		printf("[PMWatch] DIMM: %d\tread_64B_ops_received: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].read_64B_ops_received,
			PMWatch_output_buildpart[idx].read_64B_ops_received, PMWatch_output_probejoin[idx].read_64B_ops_received);
		printf("[PMWatch] DIMM: %d\twrite_64B_ops_received: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].write_64B_ops_received,
			PMWatch_output_buildpart[idx].write_64B_ops_received, PMWatch_output_probejoin[idx].write_64B_ops_received);
		printf("[PMWatch] DIMM: %d\thost_reads: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].host_reads,
			PMWatch_output_buildpart[idx].host_reads, PMWatch_output_probejoin[idx].host_reads);
		printf("[PMWatch] DIMM: %d\thost_writes: %zu\t%zu\t%zu\n", idx, PMWatch_output[idx].host_writes,
			PMWatch_output_buildpart[idx].host_writes, PMWatch_output_probejoin[idx].host_writes);
		printf("[PMWatch] DIMM: %d\tread_hit_ratio: %f\t%f\t%f\n", idx, PMWatch_output[idx].read_hit_ratio,
			PMWatch_output_buildpart[idx].read_hit_ratio, PMWatch_output_probejoin[idx].read_hit_ratio);
		printf("[PMWatch] DIMM: %d\twrite_hit_ratio: %f\t%f\t%f\n", idx, PMWatch_output[idx].write_hit_ratio,
			PMWatch_output_buildpart[idx].write_hit_ratio, PMWatch_output_probejoin[idx].write_hit_ratio);
		printf("[PMWatch] DIMM: %d\tread_amplification: %.9f\t%.9f\t %.9f\n", idx, PMWatch_output[idx].read_amplification,
			PMWatch_output_buildpart[idx].read_amplification, PMWatch_output_probejoin[idx].read_amplification);
		printf("[PMWatch] DIMM: %d\twrite_amplification: %.9f\t%.9f\t %.9f\n", idx, PMWatch_output[idx].write_amplification,
			PMWatch_output_buildpart[idx].write_amplification, PMWatch_output_probejoin[idx].write_amplification);
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
		PAPI_values[idx] += PAPI_values_buildpart[idx] + PAPI_values_probejoin[idx];
		printf("[PAPI] PAPI_event_name: %s", PAPI_event_names[idx]);
		printf("\ttotal_counter: %lld", PAPI_values[idx]);
		printf("\tbuildpart_counter: %lld", PAPI_values_buildpart[idx]);
		printf("\tprobejoin_counter: %lld", PAPI_values_probejoin[idx]);
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

	task_queue_free(part_queue);
	task_queue_free(join_queue);

#if SKEW_HANDLING
	task_queue_free(skew_queue);

#ifdef OUTPUT_OFFSET_NVM
	pmem_unmap(skew_hand_r_part_tup_num, sizeof(size_t) * FANOUT_PASS2);
	pmem_unmap(skew_hand_s_part_tup_num, sizeof(size_t) * FANOUT_PASS2);
#else /* OUTPUT_OFFSET_NVM */
	dealloc_dram(skew_hand_r_part_tup_num, sizeof(size_t) * FANOUT_PASS2);
	dealloc_dram(skew_hand_s_part_tup_num, sizeof(size_t) * FANOUT_PASS2);
#endif /* OUTPUT_OFFSET_NVM */
#endif /* SKEW_HANDLING */

#ifdef OUTPUT_OFFSET_NVM
	pmem_unmap(r_part_tup_num, sizeof(size_t) * FANOUT_PASS1);
	pmem_unmap(s_part_tup_num, sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	dealloc_dram(r_part_tup_num, sizeof(size_t) * FANOUT_PASS1);
	dealloc_dram(s_part_tup_num, sizeof(size_t) * FANOUT_PASS1);
#endif /* OUTPUT_OFFSET_NVM */

	dealloc_memory(r_tmp_relation.tuples, r_rel_tuples_size);
	dealloc_memory(s_tmp_relation.tuples, s_rel_tuples_size);

	for (size_t idx = 0; idx < (1 << NUM_RADIX_BITS); idx ++) {
		pthread_mutex_destroy(latches+idx);
	}
#ifdef SHARE_LOCK_ON_NVM
	// pmem_unmap(latches, sizeof(lock_t)*(1<<NUM_RADIX_BITS));
	pmem_unmap(latches, sizeof(pthread_mutex_t)*(1<<NUM_RADIX_BITS));
#else /* SHARE_LOCK_ON_NVM */
	// dealloc_dram((void*)latches, sizeof(lock_t)*(1<<NUM_RADIX_BITS));
	dealloc_dram(latches, sizeof(pthread_mutex_t)*(1<<NUM_RADIX_BITS));
#endif /* SHARE_LOCK_ON_NVM */


#ifdef SYNCSTATS
	free(localtimer);
	free(globaltimer);
#endif /* SYNCSTATS */

	pthread_attr_destroy(&attr);
	pthread_barrier_destroy(&buildpart_barrier);
	pthread_barrier_destroy(&probejoin_barrier);
	dealloc_memory(r_relation.tuples, r_rel_tuples_size);
	dealloc_memory(s_relation.tuples, s_rel_tuples_size);

	timekeeper->buildpart = diff_sec(args[0].part_start, args[0].part_end);
	timekeeper->probejoin = diff_sec(args[0].join_start, args[0].join_end);
	timekeeper->total = timekeeper->buildpart + timekeeper->probejoin;
}


void phj_shr_uni_bc(const datameta_t datameta, timekeeper_t * const timekeeper) {
	shr_uni_join_init_run(datameta, timekeeper, shr_uni_bc_join);
}

void phj_shr_uni_lp(const datameta_t datameta, timekeeper_t * const timekeeper) {
	shr_uni_join_init_run(datameta, timekeeper, shr_uni_lp_join);
}

void phj_shr_uni_sc(const datameta_t datameta, timekeeper_t * const timekeeper) {
	shr_uni_join_init_run(datameta, timekeeper, shr_uni_sc_join);
}

void phj_shr_uni_hr(const datameta_t datameta, timekeeper_t * const timekeeper) {
	shr_uni_join_init_run(datameta, timekeeper, shr_uni_hr_join);
}

#if TUPLE_T_SIZE <= AVX512_SIZE
void phj_shr_uni_hro(const datameta_t datameta, timekeeper_t * const timekeeper) {
	shr_uni_join_init_run(datameta, timekeeper, shr_uni_hro_join);
}
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */