#include "libpmem.h"

#include "phj_shr.h"
#include "../inc/memaccess.h"

#define __USE_GNU
#include <sched.h>
#include <pthread.h>

#include "../inc/lock.h"
#include "../inc/task_pthread_shr.h"


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


static size_t r_shr_free_buffers_num_pass_1 = 0;
static size_t r_shr_free_buffers_idx_pass_1 = 0;
static shr_part_buffer_t *r_shr_free_buffers_pass_1;

static size_t s_shr_free_buffers_num_pass_1 = 0;
static size_t s_shr_free_buffers_idx_pass_1 = 0;
static shr_part_buffer_t *s_shr_free_buffers_pass_1;

static size_t r_shr_free_buffers_num_pass_2 = 0;
static size_t r_shr_free_buffers_idx_pass_2 = 0;
static size_t s_shr_free_buffers_num_pass_2 = 0;
static size_t s_shr_free_buffers_idx_pass_2 = 0;

#if NUM_PASSES != 1
static shr_part_buffer_t *r_shr_free_buffers_pass_2;
static shr_part_buffer_t *s_shr_free_buffers_pass_2;
#else /* NUM_PASSES != 1 */
#ifdef SKEW_HANDLING
static shr_part_buffer_t *r_shr_free_buffers_pass_2;
static shr_part_buffer_t *s_shr_free_buffers_pass_2;
#endif /* SKEW_HANDLING */
#endif /* NUM_PASSES != 1 */

extern int cur_t;
typedef void (*shr_joinfunction_t)(task_t * const, join_arg_t * const);


/** holds the arguments passed to each thread */
typedef struct phj_shr_arg_t {
	int _tid;
	pthread_barrier_t *barrier;

	const datameta_t * datameta;

	relation_t r_rel;
	shr_part_buffer_t *r_part_buffer;

	relation_t s_rel;
	shr_part_buffer_t *s_part_buffer;

	task_queue_t *part_queue;
	task_queue_t *join_queue;
#if SKEW_HANDLING
	task_queue_t *skew_queue;
	task_t ** skewtask;
#endif /* SKEW_HANDLING */

	shr_joinfunction_t joinfunction;
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
	double sync_idle_skew_part_acc;
#ifdef PHJ_SYNCSTATS
	double global_acc_skew_taskfetch;
	double global_acc_skew_prepart_memset;
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

	size_t *r_part_hist, *s_part_hist;

#ifdef SKEW_HANDLING
	size_t *skew_hand_r_part_hist, *skew_hand_s_part_hist;
#endif /* SKEW_HANDLING */

	size_t intermediate_size;
	void *intermediate;
} phj_shr_arg_t;


typedef struct shr_part_t {
	int _tid;
	pthread_barrier_t *barrier;

	key_t_ hashmask;
	key_t_ bitskip;

	relation_t rel;
	shr_part_buffer_t *part_buffer;

	size_t *output_offset;

	short rel_flag;		/* 0: R, 1: S */
} shr_part_t;


void shr_parallel_partition_first(shr_part_t * const part) {
	key_t_ hashmask = (part->hashmask) << part->bitskip;
	key_t_ bitskip = part->bitskip;

	size_t *output_offset = part->output_offset;

	// size_t free_buffes_num;
	size_t *free_buffers_idx;
	shr_part_buffer_t *free_buffers;
	if (part->rel_flag == 0) {
		// free_buffes_num = r_shr_free_buffers_num_pass_1;
		free_buffers_idx = &r_shr_free_buffers_idx_pass_1;
		free_buffers = r_shr_free_buffers_pass_1;
	} else {
		// free_buffes_num = s_shr_free_buffers_num_pass_1;
		free_buffers_idx = &s_shr_free_buffers_idx_pass_1;
		free_buffers = s_shr_free_buffers_pass_1;
	}

	tuple_t *dest;
	hashkey_t tmp_hashkey;
	shr_part_buffer_t *cur, *nxt, *new;

 	/* copy tuples to their corresponding clusters */
	for (size_t idx = 0; idx < part->rel.tuple_num; idx ++) {
		tmp_hashkey = IDHASH(part->rel.tuples[idx].key, hashmask, bitskip);
		cur = part->part_buffer+tmp_hashkey;

		lock( & (part->part_buffer+tmp_hashkey)->latch );

		nxt = cur->next;
		if (cur->count == PART_BUFFER_T_CAPACITY) {
			if (!nxt || nxt->count == PART_BUFFER_T_CAPACITY) {
				new = free_buffers + __sync_fetch_and_add(free_buffers_idx, 1);
				cur->next = new;
				new->next = nxt;
				new->count = 1;
				dest = new->tuples;

			} else {
				dest = nxt->tuples + nxt->count;
				(nxt->count) ++;
			}
		} else {
			dest = cur->tuples + cur->count;
			(cur->count) ++;
		}

		output_offset[tmp_hashkey] ++;

		unlock( & (part->part_buffer+tmp_hashkey)->latch );

		store_a_tuple(dest, &part->rel.tuples[idx]);
	}
}


#ifdef SKEW_HANDLING
void shr_parallel_partition_following(shr_skew_part_t *part) {

	if (part->link_buffer_num == 0) {
		return;
	}

	key_t_ hashmask = (part->hashmask) << part->bitskip;
	key_t_ bitskip = part->bitskip;

	// size_t free_buffes_num;
	size_t *free_buffers_idx;
	shr_part_buffer_t *free_buffers;
	if (part->rel_flag == 0) {
		// free_buffes_num = r_shr_free_buffers_num_pass_2;
		free_buffers_idx = &r_shr_free_buffers_idx_pass_2;
		free_buffers = r_shr_free_buffers_pass_2;
	} else {
		// free_buffes_num = s_shr_free_buffers_num_pass_2;
		free_buffers_idx = &s_shr_free_buffers_idx_pass_2;
		free_buffers = s_shr_free_buffers_pass_2;
	}

	tuple_t *dest;
	hashkey_t tmp_hashkey;
	shr_part_buffer_t *cur, *nxt, *new;

	shr_part_buffer_t *tmp_in_buf = part->in_part;

	size_t current_link_buffer_idx = 0;
	size_t current_link_buffer_num = 0;

	while (current_link_buffer_idx < part->link_buffer_start_idx) {
		tmp_in_buf = tmp_in_buf->next;
		current_link_buffer_idx ++;
	}

	do {

		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {

			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, bitskip);

			cur = part->part_buffer+tmp_hashkey;

			lock( & (part->part_buffer+tmp_hashkey)->latch );

			nxt = cur->next;
			if (cur->count == PART_BUFFER_T_CAPACITY) {
				if (!nxt || nxt->count == PART_BUFFER_T_CAPACITY) {
					new = free_buffers + __sync_fetch_and_add(free_buffers_idx, 1);
					cur->next = new;
					new->next = nxt;
					new->count = 1;
					dest = new->tuples;

				} else {
					dest = nxt->tuples + nxt->count;
					(nxt->count) ++;
				}
			} else {
				dest = cur->tuples + cur->count;
				(cur->count) ++;
			}


			part->output_offset[tmp_hashkey] ++;

			unlock( & (part->part_buffer+tmp_hashkey)->latch );

			store_a_tuple(dest, &tmp_in_buf->tuples[idx]);
		}

		tmp_in_buf = tmp_in_buf->next;

		current_link_buffer_num ++;
		if (current_link_buffer_num == part->link_buffer_num) {
			break;
		}

	} while (tmp_in_buf);
}
#endif	/* SKEW_HANDLING */


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

void shr_cluster(shr_cluster_arg_t *arg) {
	key_t_ hashmask = (arg->fanout - 1) << (arg->bitskip);
	key_t_ bitskip = arg->bitskip;

	// size_t free_buffes_num;
	size_t *free_buffers_idx;
	shr_part_buffer_t *free_buffers;
	if (arg->rel_flag == 0) {
		// free_buffes_num = r_shr_free_buffers_num_pass_2;
		free_buffers_idx = &r_shr_free_buffers_idx_pass_2;
		free_buffers = r_shr_free_buffers_pass_2;
	} else {
		// free_buffes_num = s_shr_free_buffers_num_pass_2;
		free_buffers_idx = &s_shr_free_buffers_idx_pass_2;
		free_buffers = s_shr_free_buffers_pass_2;
	}

	tuple_t *dest;
	hashkey_t tmp_hashkey;
	shr_part_buffer_t *cur, *nxt, *new;

	shr_part_buffer_t *tmp_in_buf = arg->in_part;

	do {

		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {
			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, bitskip);;
			cur = arg->part_buffer+tmp_hashkey;
			nxt = cur->next;
			if (cur->count == PART_BUFFER_T_CAPACITY) {
				if (!nxt || nxt->count == PART_BUFFER_T_CAPACITY) {
					new = free_buffers + __sync_fetch_and_add(free_buffers_idx, 1);
					cur->next = new;
					new->next = nxt;
					new->count = 1;
					dest = new->tuples;

				} else {
					dest = nxt->tuples + nxt->count;
					(nxt->count) ++;
				}
			} else {
				dest = cur->tuples + cur->count;
				(cur->count) ++;
			}
			store_a_tuple(dest, &tmp_in_buf->tuples[idx]);

			arg->hist[tmp_hashkey] ++;
		}

		tmp_in_buf = tmp_in_buf->next;
	} while (tmp_in_buf);
}


#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
void shr_cluster_optimized(shr_cluster_arg_t *arg, swwcb_t * const swwcb) {
	key_t_ hashmask = (arg->fanout - 1) << (arg->bitskip);
	key_t_ bitskip = arg->bitskip;

	// size_t free_buffes_num;
	size_t *free_buffers_idx;
	shr_part_buffer_t *free_buffers;
	if (arg->rel_flag == 0) {
		// free_buffes_num = r_shr_free_buffers_num_pass_2;
		free_buffers_idx = &r_shr_free_buffers_idx_pass_2;
		free_buffers = r_shr_free_buffers_pass_2;
	} else {
		// free_buffes_num = s_shr_free_buffers_num_pass_2;
		free_buffers_idx = &s_shr_free_buffers_idx_pass_2;
		free_buffers = s_shr_free_buffers_pass_2;
	}

	tuple_t *dest;
	hashkey_t tmp_hashkey;
	shr_part_buffer_t *cur, *nxt, *new;

	shr_part_buffer_t *tmp_in_buf = arg->in_part;

	/* copy tuples to their corresponding SWWCBs */
	size_t slot;
	size_t slot_mod;
	// size_t remainder_start_pos;
	// size_t starting_offset;
	tuple_t * tmp_swwcb;

	do {
		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {
			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, bitskip);;
			cur = arg->part_buffer+tmp_hashkey;
			nxt = cur->next;
			if (cur->count == PART_BUFFER_T_CAPACITY) {
				if (!nxt || nxt->count == PART_BUFFER_T_CAPACITY) {
					new = free_buffers + __sync_fetch_and_add(free_buffers_idx, 1);
					cur->next = new;
					new->next = nxt;
					slot = new->count = 1;
					dest = new->tuples;
				} else {
					dest = nxt->tuples + nxt->count;
					(nxt->count) ++;
					slot = nxt->count;
				}
			} else {
				dest = cur->tuples + cur->count;
				(cur->count) ++;
				slot = cur->count;
			}

			/* automatical decrement */
			slot --;
			slot_mod = slot & (TUPLE_PER_SWWCB - 1);
			tmp_swwcb = (tuple_t *) (swwcb + tmp_hashkey);
			tmp_swwcb[slot_mod] = tmp_in_buf->tuples[idx];

			if (slot_mod == TUPLE_PER_SWWCB - 1) {
				/* non-temporal store a SWWCB */
				nontemp_store_swwcb(dest-TUPLE_PER_SWWCB+1, tmp_swwcb);
			}
			arg->hist[tmp_hashkey] ++;
			swwcb[tmp_hashkey].data.slot = slot + 1;
		}

		tmp_in_buf = tmp_in_buf->next;
	} while (tmp_in_buf);

	for (size_t idx = 0; idx < arg->fanout; idx ++) {

		slot_mod = swwcb[idx].data.slot & (TUPLE_PER_SWWCB-1);
		for (size_t jdx = 0; jdx < slot_mod; jdx ++) {
			cur = arg->part_buffer+idx;
			nxt = cur->next;
			if (cur->count == PART_BUFFER_T_CAPACITY) {
				/** 
				 * since a part_buffer can hold multiple complete SWWCBs,
				 * so, will not allocate a new part buffer here
				 */
				dest = nxt->tuples + nxt->count - slot_mod + jdx;
			} else {
				dest = cur->tuples + cur->count - slot_mod + jdx;
			}

			store_a_tuple(dest, & swwcb[idx].data.tuples[jdx]);
		}

		/* explicitly identify that the SWWCB is empty */
		swwcb[idx].data.slot = 0;
	}

}
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */


void shr_serial_partition(task_t * const task, task_queue_t *join_queue, shr_serial_part_t *part) {

	memset_localize(part->r_hist, sizeof(size_t)*part->fanout);
	memset_localize(part->s_hist, sizeof(size_t)*part->fanout);

	shr_cluster_arg_t shr_cluster_arg;
	shr_cluster_arg.fanout = part->fanout;
	shr_cluster_arg.bitskip = part->bitskip;

#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
	swwcb_t *swwcb = (swwcb_t *) part->swwcb_mem;
	shr_cluster_arg.part_buffer = part->r_part_buffer;
	shr_cluster_arg.in_part = task->r_part_buffer;
	shr_cluster_arg.hist = part->r_hist;
	shr_cluster_arg.rel_flag = 0;
	shr_cluster_optimized(&shr_cluster_arg, swwcb);

	shr_cluster_arg.part_buffer = part->s_part_buffer;
	shr_cluster_arg.in_part = task->s_part_buffer;
	shr_cluster_arg.hist = part->s_hist;
	shr_cluster_arg.rel_flag = 1;
	shr_cluster_optimized(&shr_cluster_arg, swwcb);
#else	/* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */

	shr_cluster_arg.part_buffer = part->r_part_buffer;
	shr_cluster_arg.in_part = task->r_part_buffer;
	shr_cluster_arg.hist = part->r_hist;
	shr_cluster_arg.rel_flag = 0;
	shr_cluster(&shr_cluster_arg);

	shr_cluster_arg.part_buffer = part->s_part_buffer;
	shr_cluster_arg.in_part = task->s_part_buffer;
	shr_cluster_arg.hist = part->s_hist;
	shr_cluster_arg.rel_flag = 1;
	shr_cluster(&shr_cluster_arg);

#endif	/* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */

	task_t *tmp_task;
	for (size_t idx = 0; idx < part->fanout; idx ++) {
		if (part->r_hist[idx] > 0 && part->s_hist[idx] > 0) {
			tmp_task = task_queue_get_slot_atomic(join_queue);
			tmp_task->r_tuple_num = part->r_hist[idx];
			tmp_task->r_part_buffer = part->r_part_buffer + idx;
			tmp_task->s_tuple_num = part->s_hist[idx];
			tmp_task->s_part_buffer = part->s_part_buffer + idx;

#ifdef SKEW_HANDLING
			tmp_task->skew_skew_flag = 0;
#endif /* SKEW_HANDLING */

			task_queue_add_atomic(join_queue, tmp_task);
			*(part->max_build_side_tup_num) = *(part->max_build_side_tup_num) > part->r_hist[idx] ? 
				*(part->max_build_side_tup_num) : part->r_hist[idx];
		}
	}
}

#endif /* NUM_PASSES != 1 */


void shr_lp_join(task_t * const task, join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
	join_arg -> checksum = 0;

#ifdef SKEW_HANDLING
	size_t s_current_link_buffer_idx = 0;
	size_t s_current_link_buffer_num = 0;

	if (task->skew_skew_flag == 1) {
		if (task->skew_skew_s_buffer_num == 0) {
			return;
		}

		shr_part_buffer_t *s_cursor;
		s_cursor = task->s_part_buffer;

		while (s_current_link_buffer_idx < task->skew_skew_s_start_idx) {
			s_cursor = s_cursor->next;
			s_current_link_buffer_idx ++;
		}

		task->s_part_buffer = s_cursor;
	}
#endif /* SKEW_HANDLING */

	hashkey_t max_hashkey = next_pow_2(task->r_tuple_num);

	/* the & mask for hashing, for we use some leftmost bits for radix partitioning */
	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	/* the & mask for modulo to max_hashkey buckets */
	key_t_ hashmask_0 = max_hashkey - 1;

	hashkey_t tmp_hashkey;

	shr_part_buffer_t *tmp_in_buf;

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

	tmp_in_buf = task->r_part_buffer;

	do {
		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {
			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, NUM_RADIX_BITS);
 			store_a_tuple(lp_table + tmp_hashkey, & tmp_in_buf->tuples[idx]);
		}
		tmp_in_buf = tmp_in_buf->next;
	} while (tmp_in_buf);

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_end);
#endif /* PHJ_MBP */

	/* probe the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_start);
#endif /* PHJ_MBP */

	short matched_flag;

	tmp_in_buf = task->s_part_buffer;

	do {

		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {
			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, NUM_RADIX_BITS);
#ifdef TEST_SELECTIVITY
			bool flag = true;
			hashkey_t pos_0 = tmp_hashkey;


			while ( ( (lp_table + tmp_hashkey)->key ) 										// the min_key is at least 1
				&& ( (lp_table + tmp_hashkey)->key != tmp_in_buf->tuples[idx].key)
				&& ( tmp_hashkey != pos_0 || flag) ) {
				flag = false;
				tmp_hashkey = (tmp_hashkey + 1) & hashmask_0;

			}

			// join_arg->matched_cnt += ( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) ) ? 1: 0;
			// join_arg->checksum +=  ( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) ) ?
			// 	tmp_in_buf->tuples[idx].row_id + (lp_table + tmp_hashkey)->row_id : 0;

			matched_flag = ( (lp_table + tmp_hashkey)->key && ( tmp_hashkey != pos_0 || flag ) );
			join_arg->matched_cnt += matched_flag;
			join_arg->checksum += (tmp_in_buf->tuples[idx].row_id + (lp_table + tmp_hashkey)->row_id) * matched_flag;
#else /* TEST_SELECTIVITY */
			while ( ( (lp_table + tmp_hashkey)->key ) 									// the min_key is at least 1
				&& ( (lp_table + tmp_hashkey)->key != tmp_in_buf->tuples[idx].key) ) {
				tmp_hashkey = (tmp_hashkey + 1) & hashmask_0;
			}

			// join_arg->matched_cnt += ( (lp_table + tmp_hashkey)->key ) ? 1: 0;
			// join_arg->checksum +=  ( (lp_table + tmp_hashkey)->key ) ?
			// 	tmp_in_buf->tuples[idx].row_id + (lp_table + tmp_hashkey)->row_id : 0;

			matched_flag = ( (lp_table + tmp_hashkey)->key != 0 );
			join_arg->matched_cnt += matched_flag;
			join_arg->checksum += (tmp_in_buf->tuples[idx].row_id + (lp_table + tmp_hashkey)->row_id)*matched_flag;
#endif/* TEST_SELECTIVITY */
		}

		tmp_in_buf = tmp_in_buf->next;

#ifdef SKEW_HANDLING
		s_current_link_buffer_num ++;
		if (task->skew_skew_flag && s_current_link_buffer_num == task->skew_skew_s_buffer_num) {
			break;
		}
#endif /* SKEW_HANDLING */

	} while (tmp_in_buf);

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_end);
#endif /* PHJ_MBP */

#ifdef PHJ_MBP
	join_arg->memset_time = diff_sec(memset_start, memset_end);
	join_arg->build_time = diff_sec(build_start, build_end);
	join_arg->probe_time = diff_sec(probe_start, probe_end);
#endif /* PHJ_MBP */

}


void shr_sc_join(task_t * const task, join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
	join_arg -> checksum = 0;

#ifdef SKEW_HANDLING
	size_t s_current_link_buffer_idx = 0;
	size_t s_current_link_buffer_num = 0;

	if (task->skew_skew_flag == 1) {
		if (task->skew_skew_s_buffer_num == 0) {
			return;
		}

		shr_part_buffer_t *s_cursor;
		s_cursor = task->s_part_buffer;

		while (s_current_link_buffer_idx < task->skew_skew_s_start_idx) {
			s_cursor = s_cursor->next;
			s_current_link_buffer_idx ++;
		}

		task->s_part_buffer = s_cursor;
	}
#endif /* SKEW_HANDLING */

	hashkey_t max_hashkey = next_pow_2(task->r_tuple_num) / PHJ_SC_BUCKET_CAPACITY;
	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	hashkey_t tmp_hashkey;

	phj_sc_bkt_t *buckets, *free, *curr, *nxt, *tmp;
	tuple_t *dest;
	size_t free_idx = 0;

	shr_part_buffer_t *tmp_in_buf;

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

	tmp_in_buf = task->r_part_buffer;

	do {
		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {
			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, NUM_RADIX_BITS);

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

			store_a_tuple(dest, & tmp_in_buf->tuples[idx]);
		}
		tmp_in_buf = tmp_in_buf->next;
	} while (tmp_in_buf);

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_end);
#endif /* PHJ_MBP */

	/* probe the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_start);
#endif /* PHJ_MBP */

	short matched_flag;

	tmp_in_buf = task->s_part_buffer;

	do {
		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {
			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, NUM_RADIX_BITS);

			tmp = buckets + tmp_hashkey;

			do {
				for (size_t jdx = 0; jdx < tmp->count; jdx ++) {
					// if (tmp_in_buf->tuples[idx].key == tmp->tuples[jdx].key) {
					// 	(join_arg -> matched_cnt) ++;
					// 	(join_arg -> checksum) += tmp->tuples[jdx].row_id + tmp_in_buf->tuples[idx].row_id;
					// }

					matched_flag = (tmp_in_buf->tuples[idx].key == tmp->tuples[jdx].key);
					join_arg->matched_cnt += matched_flag;
					join_arg->checksum += (tmp->tuples[jdx].row_id + tmp_in_buf->tuples[idx].row_id)*matched_flag;
				}

				tmp = tmp->next;
			} while (tmp);
		}

		tmp_in_buf = tmp_in_buf->next;

#ifdef SKEW_HANDLING
		s_current_link_buffer_num ++;
		if (task->skew_skew_flag && s_current_link_buffer_num == task->skew_skew_s_buffer_num) {
			break;
		}
#endif /* SKEW_HANDLING */

	} while (tmp_in_buf);

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_end);
#endif /* PHJ_MBP */

#ifdef PHJ_MBP
	join_arg->memset_time = diff_sec(memset_start, memset_end);
	join_arg->build_time = diff_sec(build_start, build_end);
	join_arg->probe_time = diff_sec(probe_start, probe_end);
#endif /* PHJ_MBP */
}


void shr_hr_join(task_t * const task, join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
	join_arg -> checksum = 0;

#ifdef SKEW_HANDLING
	size_t s_current_link_buffer_idx = 0;
	size_t s_current_link_buffer_num = 0;

	if (task->skew_skew_flag == 1) {
		if (task->skew_skew_s_buffer_num == 0) {
			return;
		}

		shr_part_buffer_t *s_cursor;
		s_cursor = task->s_part_buffer;

		while (s_current_link_buffer_idx < task->skew_skew_s_start_idx) {
			s_cursor = s_cursor->next;
			s_current_link_buffer_idx ++;
		}

		task->s_part_buffer = s_cursor;
	}
#endif /* SKEW_HANDLING */

	hashkey_t max_hashkey = next_pow_2(task->r_tuple_num);

	/* 4 times smaller hash table index, thus 4 time more comparison than bc_join * lp_join */
	// max_hashkey >>= HR_HRO_FACTOR;
	// if (max_hashkey < (1<<HR_HRO_FACTOR)) {
	// 	max_hashkey = (1<<HR_HRO_FACTOR);
	// }

	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	hashkey_t tmp_hashkey;

	size_t *hist;
	size_t accum_num = 0;
	tuple_t *dest;
	shr_part_buffer_t *tmp_in_buf;

#ifdef PHJ_MBP
	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
#endif /* PHJ_MBP */

	/* memset the intermediate */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_start);
#endif /* PHJ_MBP */
	memset_localize(join_arg->intermediate, HASHKEY_T_SIZE*max_hashkey+TUPLE_T_SIZE*task->r_tuple_num);
#ifdef PHJ_MBP	
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

	/* build the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */

	hist = (hashkey_t *) (join_arg->intermediate);
	dest = (tuple_t *) (join_arg->intermediate+HASHKEY_T_SIZE*max_hashkey);

	tmp_in_buf = task->r_part_buffer;

	do {
		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {
			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, NUM_RADIX_BITS);
 			hist[tmp_hashkey] ++;
		}
		tmp_in_buf = tmp_in_buf->next;
	} while (tmp_in_buf);

	for (size_t idx = 0; idx < max_hashkey; idx ++) {
		accum_num += hist[idx];
		hist[idx] = accum_num - hist[idx];
	}

	tmp_in_buf = task->r_part_buffer;

	do {
		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {
			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, NUM_RADIX_BITS);
			store_a_tuple(dest+hist[tmp_hashkey], & tmp_in_buf->tuples[idx]);
 			hist[tmp_hashkey] ++;
		}
		tmp_in_buf = tmp_in_buf->next;
	} while (tmp_in_buf);

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
	tmp_in_buf = task->s_part_buffer;

	do {
		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {
			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, NUM_RADIX_BITS);
			start_idx = hist[ tmp_hashkey-(tmp_hashkey!=0) ]*(tmp_hashkey!=0);
			end_idx = hist[tmp_hashkey];

			for (size_t jdx = start_idx; jdx < end_idx; jdx ++) {
				// if (tmp_in_buf->tuples[idx].key == dest[jdx].key) {
				// 	(join_arg->matched_cnt) ++;
				// 	(join_arg->checksum) += tmp_in_buf->tuples[idx].row_id + dest[jdx].row_id;
				// }

				matched_flag = (tmp_in_buf->tuples[idx].key == dest[jdx].key);
				join_arg->matched_cnt += matched_flag;
				join_arg->checksum += (tmp_in_buf->tuples[idx].row_id + dest[jdx].row_id)*matched_flag;
			}

		}

		tmp_in_buf = tmp_in_buf->next;

#ifdef SKEW_HANDLING
		s_current_link_buffer_num ++;
		if (task->skew_skew_flag && s_current_link_buffer_num == task->skew_skew_s_buffer_num) {
			break;
		}
#endif /* SKEW_HANDLING */

	} while (tmp_in_buf);

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
void shr_hro_join(task_t * const task, join_arg_t * const join_arg) {
	join_arg -> matched_cnt = 0;
	join_arg -> checksum = 0;

#ifdef SKEW_HANDLING
	size_t s_current_link_buffer_idx = 0;
	size_t s_current_link_buffer_num = 0;

	if (task->skew_skew_flag == 1) {
		if (task->skew_skew_s_buffer_num == 0) {
			return;
		}

		shr_part_buffer_t *s_cursor;
		s_cursor = task->s_part_buffer;

		while (s_current_link_buffer_idx < task->skew_skew_s_start_idx) {
			s_cursor = s_cursor->next;
			s_current_link_buffer_idx ++;
		}

		task->s_part_buffer = s_cursor;
	}
#endif /* SKEW_HANDLING */

	hashkey_t max_hashkey = next_pow_2(task->r_tuple_num);

	/* shrink the hashtable size according to AVX512_SIZE/TUPLE_T_SIZE */
	max_hashkey >>= HR_HRO_FACTOR;
	if (max_hashkey < (1<<HR_HRO_FACTOR)) {
		max_hashkey = (1<<HR_HRO_FACTOR);
	}
	key_t_ hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	hashkey_t tmp_hashkey;

	size_t *hist;
	size_t accum_num = 0;
	tuple_t *dest;
	shr_part_buffer_t *tmp_in_buf;

#ifdef PHJ_MBP
	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
#endif /* PHJ_MBP */

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_start);
#endif /* PHJ_MBP */

	memset_localize(join_arg->intermediate, 
		HASHKEY_T_SIZE*max_hashkey + 
		TUPLE_T_SIZE*task->r_tuple_num + 
		AVX512_SIZE*4);

#ifdef PHJ_MBP	
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */

	hist = (hashkey_t *) (join_arg->intermediate);
	dest = (tuple_t *) (join_arg->intermediate+HASHKEY_T_SIZE*max_hashkey);

	tmp_in_buf = task->r_part_buffer;

	do {
		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {
			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, NUM_RADIX_BITS);
 			hist[tmp_hashkey] ++;
		}
		tmp_in_buf = tmp_in_buf->next;
	} while (tmp_in_buf);

	for (size_t idx = 0; idx < max_hashkey; idx ++) {
		accum_num += hist[idx];
		hist[idx] = accum_num - hist[idx];
	}

	tmp_in_buf = task->r_part_buffer;

	do {
		for (size_t idx = 0; idx < tmp_in_buf->count; idx ++) {
			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, NUM_RADIX_BITS);
			store_a_tuple(dest+hist[tmp_hashkey], & tmp_in_buf->tuples[idx]);
 			hist[tmp_hashkey] ++;
		}
		tmp_in_buf = tmp_in_buf->next;
	} while (tmp_in_buf);

#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_end);
#endif /* PHJ_MBP */


#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &probe_start);
#endif /* PHJ_MBP */

	hashkey_t *hashkey_buffer = (hashkey_t *) (join_arg->intermediate + 
		+ HASHKEY_T_SIZE*max_hashkey + TUPLE_T_SIZE*task->r_tuple_num + AVX512_SIZE);		// additional AVX512_SIZE padding for SIMD store
	key_t_ *key_buffer = (key_t_ *) (hashkey_buffer + AVX512_SIZE/HASHKEY_T_SIZE);			// AVX512 can hold 8 hashkeys
	rowid_t *rowid_buffer = (rowid_t *) (key_buffer + AVX512_SIZE/KEY_T__SIZE);				// AVX512 can hold 8 keys

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

	tmp_in_buf = task->s_part_buffer;

	do {
		for (size_t idx = 0; idx < tmp_in_buf->count/(AVX512_UNIT_NUM); idx ++) {
			for (size_t jdx = 0; jdx < AVX512_UNIT_NUM; jdx ++) {
				key_buffer[jdx] = tmp_in_buf->tuples[idx*AVX512_UNIT_NUM+jdx].key;
				rowid_buffer[jdx] = tmp_in_buf->tuples[idx*AVX512_UNIT_NUM+jdx].row_id;
				tmp_hashkey = IDHASH(key_buffer[jdx], hashmask, NUM_RADIX_BITS);
				hashkey_buffer[jdx] = tmp_hashkey;
				// prefetch(dest + hist[tmp_hashkey]);
				prefetch( dest + hist[tmp_hashkey-(tmp_hashkey!=0)]*(tmp_hashkey!=0) );
			}

			for (int jdx = 0; jdx < AVX512_UNIT_NUM; jdx ++) {
				start_idx = hist[ hashkey_buffer[jdx] - (hashkey_buffer[jdx]!=0) ]*(hashkey_buffer[jdx]!=0);
				end_idx = hist[ hashkey_buffer[jdx] ];
				query_key = _mm512_set1_epi64(key_buffer[jdx]);
				query_rowid = _mm512_set1_epi64(rowid_buffer[jdx]);

				while (start_idx < end_idx) {
					tup_array = _mm512_loadu_si512(dest + start_idx);

					res_mask = _mm512_cmpeq_epi64_mask(query_key, tup_array);
					avx_matched_cnt = _mm512_mask_add_epi64(avx_matched_cnt, res_mask, avx_matched_cnt, all_one);

					tmp_avx_checksum = _mm512_maskz_add_epi64(res_mask << 1, tup_array, query_rowid);
					avx_checksum = _mm512_mask_add_epi64(avx_checksum, res_mask << 1, avx_checksum, tmp_avx_checksum);
					
					start_idx += AVX512_UNIT_NUM;
				}
			}
		}

		for (size_t idx = tmp_in_buf->count-(tmp_in_buf->count%(AVX512_UNIT_NUM)); idx < tmp_in_buf->count; idx ++) {

			tmp_hashkey = IDHASH(tmp_in_buf->tuples[idx].key, hashmask, NUM_RADIX_BITS);

			query_key = _mm512_set1_epi64(tmp_in_buf->tuples[idx].key);
			query_rowid = _mm512_set1_epi64(tmp_in_buf->tuples[idx].row_id);

			start_idx = hist[ tmp_hashkey-(tmp_hashkey!=0) ]*(tmp_hashkey!=0);
			end_idx = hist[ tmp_hashkey ];

			while (start_idx < end_idx) {
				tup_array = _mm512_loadu_si512(dest + start_idx);

				res_mask = _mm512_cmpeq_epi64_mask(query_key, tup_array);
				avx_matched_cnt = _mm512_mask_add_epi64(avx_matched_cnt, res_mask, avx_matched_cnt, all_one);

				tmp_avx_checksum = _mm512_maskz_add_epi64(res_mask << 1, tup_array, query_rowid);
				avx_checksum = _mm512_mask_add_epi64(avx_checksum, res_mask << 1, avx_checksum, tmp_avx_checksum);

				start_idx += AVX512_UNIT_NUM;
			}
		}

		tmp_in_buf = tmp_in_buf->next;

#ifdef SKEW_HANDLING
		s_current_link_buffer_num ++;
		if (task->skew_skew_flag && s_current_link_buffer_num == task->skew_skew_s_buffer_num) {
			break;
		}
#endif /* SKEW_HANDLING */

	} while (tmp_in_buf);

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


void *shr_prj_thread_buildpart(void *param) {
	phj_shr_arg_t *phj_arg = (phj_shr_arg_t *) param;

#ifdef WARMUP
	warmup_localize(phj_arg->r_rel.tuples, phj_arg->r_rel.tuple_num);
	warmup_localize(phj_arg->s_rel.tuples, phj_arg->s_rel.tuple_num);
#endif /* WARMUP */

	int rv;

	size_t max_build_side_tup_num = 0;

	task_t *task;
	task_queue_t *part_queue = phj_arg->part_queue;

#if SKEW_HANDLING
	task_queue_t *skew_queue = phj_arg->skew_queue;
	task_t **skewtask = phj_arg->skewtask;
#endif /* SKEW_HANDLING */
	shr_part_t part;

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
	part.part_buffer = phj_arg->r_part_buffer;
	part.output_offset = phj_arg->r_part_hist;
	part.rel_flag = 0;
	shr_parallel_partition_first(&part);

	/**************** PARTITIONING S ****************/
	part.rel = phj_arg->s_rel;
	part.part_buffer = phj_arg->s_part_buffer;
	part.output_offset = phj_arg->s_part_hist;
	part.rel_flag = 1;
	shr_parallel_partition_first(&part);

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
	if (phj_arg->_tid == 0) {

		for (size_t idx = 0; idx < FANOUT_PASS1; idx ++) {

#ifdef SKEW_HANDLING
			if (phj_arg->r_part_hist[idx] > THRESHOLD_0 || phj_arg->s_part_hist[idx] > THRESHOLD_0) {
				task = task_queue_get_slot(skew_queue);
				task->r_tuple_num = phj_arg->r_part_hist[idx];
				task->r_part_buffer = phj_arg->r_part_buffer + idx;
				task->s_tuple_num = phj_arg->s_part_hist[idx];
				task->s_part_buffer = phj_arg->s_part_buffer + idx;

				task->r_link_buffer_num = (size_t) ceil( (double) task->r_tuple_num/PART_BUFFER_T_CAPACITY);
				task->s_link_buffer_num = (size_t) ceil( (double) task->s_tuple_num/PART_BUFFER_T_CAPACITY);

				task->skew_skew_flag = 0;
				task_queue_add(skew_queue, task);
			} else
#endif /* SKEW_HANDLING */
			if (phj_arg->r_part_hist[idx] > 0 || phj_arg->s_part_hist[idx] > 0) {
				task = task_queue_get_slot(part_queue);
				task->r_tuple_num = phj_arg->r_part_hist[idx];
				task->r_part_buffer = phj_arg->r_part_buffer + idx;
				task->s_tuple_num = phj_arg->s_part_hist[idx];
				task->s_part_buffer = phj_arg->s_part_buffer + idx;
				task_queue_add(part_queue, task);

				max_build_side_tup_num = max_build_side_tup_num >= phj_arg->r_part_hist[idx] ? 
					max_build_side_tup_num : phj_arg->r_part_hist[idx];
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
	size_t part_queue_num = part_queue->count;
	phj_arg->r_part_buffer += FANOUT_PASS1;
	phj_arg->s_part_buffer += FANOUT_PASS1;

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

	size_t shr_serial_part_count = 0;

	shr_serial_part_t shr_serial_part;

	shr_serial_part.fanout = FANOUT_PASS2;
	shr_serial_part.bitskip = NUM_RADIX_BITS / NUM_PASSES;
	shr_serial_part.r_hist = ser_r_hist;
	shr_serial_part.s_hist = ser_s_hist;
	shr_serial_part.swwcb_mem = ser_swwcb;
	shr_serial_part.max_build_side_tup_num = &max_build_side_tup_num;

	while ( ( task = task_queue_get_count_atomic(part_queue, &shr_serial_part_count) ) ) {
		shr_serial_part.r_part_buffer = phj_arg->r_part_buffer+FANOUT_PASS2*(shr_serial_part_count-1);
		shr_serial_part.s_part_buffer = phj_arg->s_part_buffer+FANOUT_PASS2*(shr_serial_part_count-1);
		shr_serial_partition(task, phj_arg->join_queue, &shr_serial_part);
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

	phj_arg->r_part_buffer += FANOUT_PASS2*part_queue_num;
	phj_arg->s_part_buffer += FANOUT_PASS2*part_queue_num;
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
	shr_skew_part_t skew_part;

	/* SAME CONFIGURATIONS AS SECOND PASS PARTITIONING */
	skew_part.bitskip = NUM_RADIX_BITS / NUM_PASSES;
	skew_part.hashmask = FANOUT_PASS2-1;

	task_t *skew_hand_task;

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
		phj_arg->sync_idle_skew_part_acc += diff_sec(phj_arg->localtimer->sync_skew_part,
			phj_arg->globaltimer->sync_skew_part);
#ifdef PHJ_SYNCSTATS
		// phj_arg->global_acc_skew_taskfetch += diff_sec();
		phj_arg->global_acc_skew_prepart_memset += diff_sec( phj_arg->globaltimer->sync_skew_taskfetch, 
			phj_arg->globaltimer->sync_skew_prepart_memset );
		phj_arg->global_acc_skew_part += diff_sec( phj_arg->globaltimer->sync_skew_prepart_memset,
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
		pmem_memset_persist(phj_arg->skew_hand_r_part_hist, 0, sizeof(size_t) * FANOUT_PASS2 );
		pmem_memset_persist(phj_arg->skew_hand_s_part_hist, 0, sizeof(size_t) * FANOUT_PASS2 );
#else /* OUTPUT_OFFSET_NVM */
		memset(phj_arg->skew_hand_r_part_hist, 0, sizeof(size_t) * FANOUT_PASS2 );
		memset(phj_arg->skew_hand_s_part_hist, 0, sizeof(size_t) * FANOUT_PASS2 );
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
		skew_part.part_buffer = phj_arg->r_part_buffer;
		skew_part.in_part = (*skewtask)->r_part_buffer;
		skew_part.output_offset = phj_arg->skew_hand_r_part_hist;
		skew_part.rel_flag = 0;

		skew_part.link_buffer_start_idx = (phj_arg->_tid == 0) ? 0 : 
			phj_arg->_tid * ( (*skewtask)->r_link_buffer_num / BUILDPART_THREAD_NUM);
		skew_part.link_buffer_num = (phj_arg->_tid < BUILDPART_THREAD_NUM-1) ? 
			(*skewtask)->r_link_buffer_num / BUILDPART_THREAD_NUM :
			(*skewtask)->r_link_buffer_num - phj_arg->_tid * ( (*skewtask)->r_link_buffer_num / BUILDPART_THREAD_NUM );

		shr_parallel_partition_following(&skew_part);

		/**************** PARTITIONING S ****************/
		skew_part.part_buffer = phj_arg->s_part_buffer;
		skew_part.in_part = (*skewtask)->s_part_buffer;
		skew_part.output_offset = phj_arg->skew_hand_s_part_hist;
		skew_part.rel_flag = 1;

		skew_part.link_buffer_start_idx = (phj_arg->_tid == 0) ? 0 : 
			phj_arg->_tid * ( (*skewtask)->s_link_buffer_num / BUILDPART_THREAD_NUM);
		skew_part.link_buffer_num = (phj_arg->_tid < BUILDPART_THREAD_NUM-1) ? 
			(*skewtask)->s_link_buffer_num / BUILDPART_THREAD_NUM :
			(*skewtask)->s_link_buffer_num - phj_arg->_tid * ( (*skewtask)->s_link_buffer_num / BUILDPART_THREAD_NUM );

		shr_parallel_partition_following(&skew_part);

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

				max_build_side_tup_num = max_build_side_tup_num > phj_arg->skew_hand_r_part_hist[idx] ?
					max_build_side_tup_num : phj_arg->skew_hand_r_part_hist[idx];

				if (phj_arg->skew_hand_r_part_hist[idx] > THRESHOLD_1 && phj_arg->skew_hand_s_part_hist[idx] > THRESHOLD_1) {
					
					size_t s_link_buffer_num = (size_t) ceil( (double) phj_arg->skew_hand_s_part_hist[idx] / PART_BUFFER_T_CAPACITY );
					
					for (int jdx = 0; jdx < BUILDPART_THREAD_NUM; jdx ++) {
						skew_hand_task = task_queue_get_slot(part_queue);
						skew_hand_task->r_tuple_num = phj_arg->skew_hand_r_part_hist[idx];
						skew_hand_task->r_part_buffer = phj_arg->r_part_buffer + idx;

						skew_hand_task->s_part_buffer = phj_arg->s_part_buffer + idx;

						skew_hand_task->skew_skew_flag = 1;
						skew_hand_task->skew_skew_s_start_idx = (jdx == 0) ? 0 :
							jdx * ( s_link_buffer_num / BUILDPART_THREAD_NUM );
						skew_hand_task->skew_skew_s_buffer_num = (jdx < BUILDPART_THREAD_NUM-1) ?
							s_link_buffer_num / BUILDPART_THREAD_NUM :
							s_link_buffer_num - jdx * (s_link_buffer_num / BUILDPART_THREAD_NUM);

						task_queue_add(part_queue, skew_hand_task);
					}
				} else {
					if (phj_arg->skew_hand_r_part_hist[idx] > 0 && phj_arg->skew_hand_s_part_hist[idx] > 0) {
						skew_hand_task = task_queue_get_slot(phj_arg->join_queue);
						skew_hand_task->r_tuple_num = phj_arg->skew_hand_r_part_hist[idx];
						skew_hand_task->r_part_buffer = phj_arg->r_part_buffer + idx;
						skew_hand_task->s_tuple_num = phj_arg->skew_hand_s_part_hist[idx];
						skew_hand_task->s_part_buffer = phj_arg->s_part_buffer + idx;
						skew_hand_task->skew_skew_flag = 0;
						task_queue_add(phj_arg->join_queue, skew_hand_task);
					}

				}
			}
		}

		phj_arg->r_part_buffer += FANOUT_PASS2;
		phj_arg->s_part_buffer += FANOUT_PASS2;

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


void *shr_prj_thread_probejoin(void *param) {
	phj_shr_arg_t *phj_arg = (phj_shr_arg_t *) param;
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
		phj_arg->joinfunction(task, &join_arg);
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


void shr_join_init_run(const datameta_t datameta, timekeeper_t * const timekeeper, 
	void (*f_join)(task_t * const, join_arg_t * const)
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

	size_t r_rel_tuples_size = TUPLE_T_SIZE*r_relation.tuple_num;
	size_t s_rel_tuples_size = TUPLE_T_SIZE*s_relation.tuple_num;
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
	pmem_unmap(r_original_addr, TUPLE_T_SIZE*r_relation.tuple_num);
	pmem_unmap(s_original_addr, TUPLE_T_SIZE*s_relation.tuple_num);


	/**************** INITIALIZATION FOR SHARE_FREE_BUFFERS ****************/
	r_shr_free_buffers_idx_pass_1 = 0;
	s_shr_free_buffers_idx_pass_1 = 0;

	r_shr_free_buffers_idx_pass_2 = 0;
	s_shr_free_buffers_idx_pass_2 = 0;

	r_shr_free_buffers_num_pass_1 = (size_t) ceil( (double) r_relation.tuple_num / PART_BUFFER_T_CAPACITY);
	r_shr_free_buffers_num_pass_1 += FANOUT_PASS1;

	r_shr_free_buffers_num_pass_2 = (size_t) ceil( (double) r_relation.tuple_num / PART_BUFFER_T_CAPACITY);
	r_shr_free_buffers_num_pass_2 += FANOUT_PASS1 * FANOUT_PASS2;

	s_shr_free_buffers_num_pass_1 = (size_t) ceil( (double) s_relation.tuple_num / PART_BUFFER_T_CAPACITY);
	s_shr_free_buffers_num_pass_1 += FANOUT_PASS1;

	s_shr_free_buffers_num_pass_2 = (size_t) ceil( (double) s_relation.tuple_num / PART_BUFFER_T_CAPACITY);
	s_shr_free_buffers_num_pass_2 += FANOUT_PASS1 * FANOUT_PASS2;

#ifdef USE_NVM
	r_shr_free_buffers_pass_1 = (shr_part_buffer_t *) new_alloc_nvm(SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_1);
	s_shr_free_buffers_pass_1 = (shr_part_buffer_t *) new_alloc_nvm(SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_1);
#else /* USE_NVM */
#ifdef USE_HUGE
	r_shr_free_buffers_pass_1 = (shr_part_buffer_t *) alloc_huge_dram(SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_1);
	s_shr_free_buffers_pass_1 = (shr_part_buffer_t *) alloc_huge_dram(SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_1);
#else /* USE_HUGE */
	r_shr_free_buffers_pass_1 = (shr_part_buffer_t *) alloc_aligned_dram(SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_1, CACHELINE_SIZE);
	s_shr_free_buffers_pass_1 = (shr_part_buffer_t *) alloc_aligned_dram(SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_1, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memset_localize(r_shr_free_buffers_pass_1, SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_1);
	parallel_memset_localize(s_shr_free_buffers_pass_1, SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_1);


#if NUM_PASSES != 1
#ifdef USE_NVM	
	r_shr_free_buffers_pass_2 = (shr_part_buffer_t *) new_alloc_nvm(SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_2);
	s_shr_free_buffers_pass_2 = (shr_part_buffer_t *) new_alloc_nvm(SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_2);
#else /* USE_NVM */
#ifdef USE_HUGE
	r_shr_free_buffers_pass_2 = (shr_part_buffer_t *) alloc_huge_dram(SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_2);
	s_shr_free_buffers_pass_2 = (shr_part_buffer_t *) alloc_huge_dram(SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_2);
#else /* USE_HUGE */
	r_shr_free_buffers_pass_2 = (shr_part_buffer_t *) alloc_aligned_dram(SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_2, CACHELINE_SIZE);
	s_shr_free_buffers_pass_2 = (shr_part_buffer_t *) alloc_aligned_dram(SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_2, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memset_localize(r_shr_free_buffers_pass_2, SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_2);
	parallel_memset_localize(s_shr_free_buffers_pass_2, SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_2);
#else /* NUM_PASSES != 1 */
#ifdef SKEW_HANDLING
#ifdef USE_NVM	
	r_shr_free_buffers_pass_2 = (shr_part_buffer_t *) new_alloc_nvm(SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_2);
	s_shr_free_buffers_pass_2 = (shr_part_buffer_t *) new_alloc_nvm(SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_2);
#else /* USE_NVM */
#ifdef /* USE_HUGE */
	r_shr_free_buffers_pass_2 = (shr_part_buffer_t *) alloc_huge_dram(SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_2);
	s_shr_free_buffers_pass_2 = (shr_part_buffer_t *) alloc_huge_dram(SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_2);
#else /* USE_HUGE */
	r_shr_free_buffers_pass_2 = (shr_part_buffer_t *) alloc_aligned_dram(SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_2, CACHELINE_SIZE);
	s_shr_free_buffers_pass_2 = (shr_part_buffer_t *) alloc_aligned_dram(SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_2, CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memset_localize(r_shr_free_buffers_pass_2, SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_2);
	parallel_memset_localize(s_shr_free_buffers_pass_2, SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_2);
#endif /* SKEW_HANDLING */
#endif /* NUM_PASSES != 1 */


	/**************** CREATE INTERMEDIATES FOR PARTITIONING ****************/

	shr_part_buffer_t *r_part_buffer, *s_part_buffer;
#ifdef USE_NVM
	r_part_buffer = (shr_part_buffer_t *) new_alloc_nvm(SHR_PART_BUFFER_T_SIZE*(FANOUT_PASS1+FANOUT_PASS1*FANOUT_PASS2));
	s_part_buffer = (shr_part_buffer_t *) new_alloc_nvm(SHR_PART_BUFFER_T_SIZE*(FANOUT_PASS1+FANOUT_PASS1*FANOUT_PASS2));
#else /* USE_NVM */
#ifdef USE_HUGE
	r_part_buffer = (shr_part_buffer_t *) alloc_huge_dram(SHR_PART_BUFFER_T_SIZE*(FANOUT_PASS1+FANOUT_PASS1*FANOUT_PASS2));
	s_part_buffer = (shr_part_buffer_t *) alloc_huge_dram(SHR_PART_BUFFER_T_SIZE*(FANOUT_PASS1+FANOUT_PASS1*FANOUT_PASS2));
#else /* USE_HUGE */
	r_part_buffer = (shr_part_buffer_t *) alloc_aligned_dram(SHR_PART_BUFFER_T_SIZE*(FANOUT_PASS1+FANOUT_PASS1*FANOUT_PASS2), 
		CACHELINE_SIZE);
	s_part_buffer = (shr_part_buffer_t *) alloc_aligned_dram(SHR_PART_BUFFER_T_SIZE*(FANOUT_PASS1+FANOUT_PASS1*FANOUT_PASS2), 
		CACHELINE_SIZE);	
#endif /* USE_HUGE */
#endif /* USE_NVM */
	parallel_memset_localize(r_part_buffer, SHR_PART_BUFFER_T_SIZE*(FANOUT_PASS1+FANOUT_PASS1*FANOUT_PASS2));
	parallel_memset_localize(s_part_buffer, SHR_PART_BUFFER_T_SIZE*(FANOUT_PASS1+FANOUT_PASS1*FANOUT_PASS2));


	/**************** CREATE PART_TUP_NUM FOR PARTITIONING ****************/
	size_t *r_part_hist, *s_part_hist;	
#ifdef OUTPUT_OFFSET_NVM
	r_part_hist = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS1);
	s_part_hist = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS1);
	pmem_memset_persist(r_part_hist, 0, sizeof(size_t) * FANOUT_PASS1);
	pmem_memset_persist(s_part_hist, 0, sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	r_part_hist = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS1, CACHELINE_SIZE);
	s_part_hist = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS1, CACHELINE_SIZE);
	memset(r_part_hist, 0, sizeof(size_t) * FANOUT_PASS1);
	memset(s_part_hist, 0, sizeof(size_t) * FANOUT_PASS1);
#endif /* OUTPUT_OFFSET_NVM */


#ifdef SKEW_HANDLING
	size_t *skew_hand_r_part_hist, *skew_hand_s_part_hist;
#ifdef OUTPUT_OFFSET_NVM
	skew_hand_r_part_hist = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS2 );
	skew_hand_s_part_hist = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS2 );
#else /* OUTPUT_OFFSET_NVM */
	skew_hand_r_part_hist = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS2, CACHELINE_SIZE);
	skew_hand_s_part_hist = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS2, CACHELINE_SIZE);
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
	phj_shr_arg_t args[max_thread_num];

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

		(args+idx)->r_rel.tuple_num = (idx == BUILDPART_THREAD_NUM -1) ? 
			(datameta.r_tuple_num - idx * r_tuple_num_thr) : r_tuple_num_thr;
		(args+idx)->r_rel.tuples = r_relation.tuples + r_tuple_num_thr * idx;
		(args+idx)->r_part_buffer = r_part_buffer;

		(args+idx)->s_rel.tuple_num = (idx == BUILDPART_THREAD_NUM -1) ? 
			(datameta.s_tuple_num - idx * s_tuple_num_thr) : s_tuple_num_thr;
		(args+idx)->s_rel.tuples = s_relation.tuples + s_tuple_num_thr * idx;
		(args+idx)->s_part_buffer = s_part_buffer;

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
		(args+idx)->sync_idle_skew_prepart_memset_acc = 0.0;
		(args+idx)->sync_idle_skew_part_acc = 0.0;
#ifdef PHJ_SYNCSTATS
		(args+idx)->global_acc_skew_taskfetch = 0.0;
		(args+idx)->global_acc_skew_prepart_memset = 0.0;
		(args+idx)->global_acc_skew_part = 0.0;
#endif /* PHJ_SYNCSTATS */
#endif /* SKEW_HANDLING */
#endif /* SYNCSTATS */

		(args+idx)->r_part_hist = r_part_hist;
		(args+idx)->s_part_hist = s_part_hist;

#ifdef SKEW_HANDLING
		(args+idx)->skew_hand_r_part_hist = skew_hand_r_part_hist;
		(args+idx)->skew_hand_s_part_hist = skew_hand_s_part_hist;
#endif /* SKEW_HANDLING */

#ifdef USE_PAPI
		memset( (args+idx)->PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, shr_prj_thread_buildpart, args+idx);
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
	if (f_join == shr_sc_join) {
		intermediate_size = PHJ_SC_BKT_T_SIZE*next_pow_2(max_build_side_tup_num[0])*INTERMEDIATE_SCALING_FACTOR/PHJ_SC_BUCKET_CAPACITY;
	} else {
		intermediate_size = TUPLE_T_SIZE*next_pow_2(max_build_side_tup_num[0])*INTERMEDIATE_SCALING_FACTOR;
	}
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

		rv = pthread_create(&threadpool[idx], &attr, shr_prj_thread_probejoin, args+idx);
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

	printf("[SYNCSTATS] TID\tPREPART_MEMSET\t1ST_PASS\t1ST_PASS_QUEUE\tPART\t");

#ifdef SKEW_HANDLING
	printf("SKEW_TASKFETCH\tSKEW_PREPART_MEMSET\tSKEW_PART\tSKEW_QUEUE\t");
#endif /* SKEW_HANDLING */

	printf("MAX_HASHTABLE\n");

	for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
		printf("[SYNCSTATS] %d\t%.9f\t%.9f\t%.9f\t%.9f\t", 
			idx, 
			diff_sec( (args+idx)->localtimer->sync_prepart_memset,
				(args+idx)->globaltimer->sync_prepart_memset ),
			diff_sec( (args+idx)->localtimer->sync_1st_pass,
				(args+idx)->globaltimer->sync_1st_pass ),
			diff_sec( (args+idx)->localtimer->sync_1st_pass_queue,
				(args+idx)->globaltimer->sync_1st_pass_queue ),
			diff_sec( (args+idx)->localtimer->sync_part,
				(args+idx)->globaltimer->sync_part )
		);
#ifdef SKEW_HANDLING
		printf("%.9f\t%.9f\t%.9f\t%.9f\t", 
			(args+idx)->sync_idle_skew_taskfetch_acc,
			(args+idx)->sync_idle_skew_prepart_memset_acc,
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
	printf("[PHJ_SYNCSTATS] PREPART_MEMSET: %.9f\t1ST_PASS: %.9f\t1ST_PASS_QUEUE: %.9f\tPART: %.9f\n",
		diff_sec( args[0].part_start,
			args[0].globaltimer->sync_prepart_memset ),
		diff_sec( args[0].globaltimer->sync_prepart_memset,
			args[0].globaltimer->sync_1st_pass ),
		diff_sec( args[0].globaltimer->sync_1st_pass,
			args[0].globaltimer->sync_1st_pass_queue ),
		diff_sec( args[0].globaltimer->sync_1st_pass_queue,
			args[0].globaltimer->sync_part )
	);

#ifdef SKEW_HANDLING
	// printf("[PHJ_SYNCSTATS] SKEW_TASKFETCH: \tSKEW_PREPART_MEMSET: \tSKEW_HIST_R: \t");
	printf("[PHJ_SYNCSTATS] SKEW_PREPART_MEMSET: %.9f\tSKEW_PART: %.9f\tSKEW_QUEUE: %.9f\n",
		args[0].global_acc_skew_prepart_memset, 
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
	pmem_unmap(skew_hand_r_part_hist, sizeof(size_t) * FANOUT_PASS2);
	pmem_unmap(skew_hand_s_part_hist, sizeof(size_t) * FANOUT_PASS2);
#else /* OUTPUT_OFFSET_NVM */
	dealloc_dram(skew_hand_r_part_hist, sizeof(size_t) * FANOUT_PASS2);
	dealloc_dram(skew_hand_s_part_hist, sizeof(size_t) * FANOUT_PASS2);
#endif /* OUTPUT_OFFSET_NVM */
#endif /* SKEW_HANDLING */

#ifdef OUTPUT_OFFSET_NVM
	pmem_unmap(r_part_hist, sizeof(size_t) * FANOUT_PASS1);
	pmem_unmap(s_part_hist, sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	dealloc_dram(r_part_hist, sizeof(size_t) * FANOUT_PASS1);
	dealloc_dram(s_part_hist, sizeof(size_t) * FANOUT_PASS1);
#endif /* OUTPUT_OFFSET_NVM */

	dealloc_memory(r_shr_free_buffers_pass_1, SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_1);
	dealloc_memory(s_shr_free_buffers_pass_1, SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_1);
#if NUM_PASSES != 1
	dealloc_memory(r_shr_free_buffers_pass_2, SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_2);
	dealloc_memory(s_shr_free_buffers_pass_2, SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_2);
#else /* NUM_PASSES != 1 */
#ifdef SKEW_HANDLING
	dealloc_memory(r_shr_free_buffers_pass_2, SHR_PART_BUFFER_T_SIZE*r_shr_free_buffers_num_pass_2);
	dealloc_memory(s_shr_free_buffers_pass_2, SHR_PART_BUFFER_T_SIZE*s_shr_free_buffers_num_pass_2);
#endif /* SKEW_HANDLING */
#endif /* NUM_PASSES != 1 */

	dealloc_memory(r_part_buffer, SHR_PART_BUFFER_T_SIZE*(FANOUT_PASS1+FANOUT_PASS1*FANOUT_PASS2));
	dealloc_memory(s_part_buffer, SHR_PART_BUFFER_T_SIZE*(FANOUT_PASS1+FANOUT_PASS1*FANOUT_PASS2));

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


void phj_shr_lp(const datameta_t datameta, timekeeper_t * const timekeeper) {
	shr_join_init_run(datameta, timekeeper, shr_lp_join);
}

void phj_shr_sc(const datameta_t datameta, timekeeper_t * const timekeeper) {
	shr_join_init_run(datameta, timekeeper, shr_sc_join);
}

void phj_shr_hr(const datameta_t datameta, timekeeper_t * const timekeeper) {
	shr_join_init_run(datameta, timekeeper, shr_hr_join);
}

#if TUPLE_T_SIZE <= AVX512_SIZE
void phj_shr_hro(const datameta_t datameta, timekeeper_t * const timekeeper) {
	shr_join_init_run(datameta, timekeeper, shr_hro_join);
}
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */