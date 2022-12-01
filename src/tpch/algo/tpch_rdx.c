#include "tpch_rdx.h"


#ifdef USE_PAPI
#include "../../papi/mypapi.h"
extern char* PAPI_event_names[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
#include "../../pmw/mypmw.h"
extern unsigned int NVMCount;
extern MY_PMWATCH_OP_BUF_NODE PMWatch_output_buildpart[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE PMWatch_output_probejoin[MaxNVMNum];
extern MY_PMWATCH_OP_BUF_NODE PMWatch_output_aggregate[MaxNVMNum];
#endif /* USE_PMWATCH */


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
size_t **global_part_output_offset;
size_t **global_lineitem_output_offset;
#ifdef USE_SWWCB_OPTIMIZED_PART
tpch_fltrq_part_swwcb_t **global_tpch_fltrq_part_swwcb_s;
tpch_fltrq_lineitem_swwcb_t **global_tpch_fltrq_lineitem_swwcb_s;
#endif /* USE_SWWCB_OPTIMIZED_PART */


void tpch_init_part_mem(bool use, void *org_tup, void **tmp_tup,
	size_t t_num, size_t fltrq_t_size) {
	if (use) {
#ifdef USE_NVM
		*tmp_tup = new_alloc_nvm(fltrq_t_size * t_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
#else /* USE_NVM */
#ifdef USE_HUGE
		*tmp_tup = alloc_huge_dram(fltrq_t_size * t_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
#else /* USE_HUGE */
		*tmp_tup = alloc_aligned_dram(fltrq_t_size * t_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)), CACHELINE_SIZE);
#endif /* USE_HUGE */
#endif /* USE_NVM */
		parallel_memset_localize(org_tup + fltrq_t_size * t_num, PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
		parallel_memset_localize(*tmp_tup + fltrq_t_size * t_num, PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
	}
}


void tpch_memset_part_mem(bool use, int _tid, 
	void *tmp_tup, size_t tup_num, size_t t_size) {
	if (use) {
		size_t memset_num_tuples_thr = tup_num / BUILDPART_THREAD_NUM;
		size_t memset_num_tuples_size = (_tid == BUILDPART_THREAD_NUM - 1) ? 
			t_size * ( tup_num - _tid * memset_num_tuples_thr ) : t_size * memset_num_tuples_thr;
		memset_localize(tmp_tup + _tid * memset_num_tuples_thr, memset_num_tuples_size);

	}
}


void parallel_radix_partition_part(part_arg_t * const part_arg) {
	int rv;

	tpch_key_t hashmask = (part_arg->hashmask) << part_arg->bitskip;
	tpch_key_t bitskip = part_arg->bitskip;

	size_t fanout = part_arg->hashmask + 1;
	size_t **hist = part_arg->hist;
	size_t *myhist =  part_arg->hist[part_arg->_tid];
	size_t *output_offset = part_arg->output_offset;
	size_t padding_num = part_arg->padding_num;
	size_t org_tup_num = part_arg->org_tup_num;

	fltrq_part_t *org_tup = (fltrq_part_t *) part_arg->org_tup;
	fltrq_part_t *tmp_tup = (fltrq_part_t *) part_arg->tmp_tup;

	/* compute the local histogram */
	tpch_key_t tmp_hashkey;
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->p_partkey, 
			hashmask, bitskip
		);
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
	SYNC_TIMER(*part_arg->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each cluster */
	for (size_t idx = 0; idx < part_arg->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			output_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part_arg->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
		for (size_t jdx = 1; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			output_offset[jdx] += hist[idx][(jdx-1)*2+1] ;
		}
	}

	/* copy tuples to their corresponding clusters */
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->p_partkey, 
			hashmask, bitskip
		);
		memcpy(&tmp_tup[output_offset[tmp_hashkey]+tmp_hashkey*padding_num],
			org_tup+idx, FLTRQ_PART_T_SIZE);
		output_offset[tmp_hashkey] ++;
	}

	/* move back the output_offset pointer to the starting address */
	/* this code segments could be moved to main and only be executed for the 1st thread */
	for (size_t idx = 0; idx < fanout; idx ++) {
		output_offset[idx] -= myhist[idx*2];
		output_offset[idx] += idx * padding_num;
	}
}


void parallel_radix_partition_lineitem(part_arg_t * const part_arg) {
	int rv;

	tpch_key_t hashmask = (part_arg->hashmask) << part_arg->bitskip;
	tpch_key_t bitskip = part_arg->bitskip;

	size_t fanout = part_arg->hashmask + 1;
	size_t **hist = part_arg->hist;
	size_t *myhist =  part_arg->hist[part_arg->_tid];
	size_t *output_offset = part_arg->output_offset;
	size_t padding_num = part_arg->padding_num;
	size_t org_tup_num = part_arg->org_tup_num;

	fltrq_lineitem_t *org_tup = (fltrq_lineitem_t *) part_arg->org_tup;
	fltrq_lineitem_t *tmp_tup = (fltrq_lineitem_t *) part_arg->tmp_tup;

	/* compute the local histogram */
	tpch_key_t tmp_hashkey;
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->l_partkey, 
			hashmask, bitskip
		);
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
	SYNC_TIMER(*part_arg->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each cluster */
	for (size_t idx = 0; idx < part_arg->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			output_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part_arg->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
		for (size_t jdx = 1; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			output_offset[jdx] += hist[idx][(jdx-1)*2+1] ;
		}
	}

	/* copy tuples to their corresponding clusters */
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->l_partkey, 
			hashmask, bitskip
		);
		memcpy(&tmp_tup[output_offset[tmp_hashkey]+tmp_hashkey*padding_num],
			org_tup+idx, FLTRQ_LINEITEM_T_SIZE);
		output_offset[tmp_hashkey] ++;
	}

	/* move back the output_offset pointer to the starting address */
	/* this code segments could be moved to main and only be executed for the 1st thread */
	for (size_t idx = 0; idx < fanout; idx ++) {
		output_offset[idx] -= myhist[idx*2];
		output_offset[idx] += idx * padding_num;
	}
}


void parallel_radix_partition_bw_reg_part(part_arg_t * const part_arg) {
	int rv;

	tpch_key_t hashmask = (part_arg->hashmask) << part_arg->bitskip;
	tpch_key_t bitskip = part_arg->bitskip;

	size_t fanout = part_arg->hashmask + 1;
	size_t **hist = part_arg->hist;
	size_t *myhist =  part_arg->hist[part_arg->_tid];
	size_t *myoutput_offset = global_part_output_offset[part_arg->_tid];
	size_t padding_num = part_arg->padding_num;
	size_t org_tup_num = part_arg->org_tup_num;

	fltrq_part_t *org_tup = (fltrq_part_t *) part_arg->org_tup;
	fltrq_part_t *tmp_tup = (fltrq_part_t *) part_arg->tmp_tup;

	/* compute the local histogram */
	tpch_key_t tmp_hashkey;
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->p_partkey, 
			hashmask, bitskip
		);
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
	SYNC_TIMER(*part_arg->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each cluster */
	for (size_t idx = 0; idx < part_arg->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part_arg->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
		for (size_t jdx = 1; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][(jdx-1)*2+1] ;
		}
	}

#ifdef SYNCSTATS
	SYNC_TIMER(*part_arg->sync_local_prfx_sum);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_prfx_sum);
#endif /* SYNCSTATS */

	/* copy tuples to their corresponding clusters */
	if (part_arg->_tid < BUILDPART_WRITE_THREAD_NUM) {
		int tmp_tid = part_arg->_tid;
		size_t tmp_fltrq_part_num;
		fltrq_part_t *tmp_fltrq_part;
		while (tmp_tid < BUILDPART_THREAD_NUM) {
			tmp_fltrq_part_num = every_thread_part_num[tmp_tid];
			tmp_fltrq_part = part_start_addr + tmp_tid * initial_part_tuple_num_thr;

			for (size_t idx = 0; idx < tmp_fltrq_part_num; idx ++) {
				tmp_hashkey = IDHASH(
					(tmp_fltrq_part+idx)->p_partkey,
					hashmask, bitskip
				);
				memcpy(& tmp_tup[global_part_output_offset[tmp_tid][tmp_hashkey]+tmp_hashkey*padding_num],
					tmp_fltrq_part+idx, FLTRQ_PART_T_SIZE);
				global_part_output_offset[tmp_tid][tmp_hashkey] ++;
			}

			/* move back the output_offset pointer to the starting address */
			/* this code segments could be moved to main and only be executed for the 1st thread */
			for (size_t idx = 0; idx < fanout; idx ++) {
				global_part_output_offset[tmp_tid][idx] -= hist[tmp_tid][idx*2];
				global_part_output_offset[tmp_tid][idx] += idx * padding_num;
			}

			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}
	}
}


void parallel_radix_partition_bw_reg_lineitem(part_arg_t * const part_arg) {
	int rv;

	tpch_key_t hashmask = (part_arg->hashmask) << part_arg->bitskip;
	tpch_key_t bitskip = part_arg->bitskip;

	size_t fanout = part_arg->hashmask + 1;
	size_t **hist = part_arg->hist;
	size_t *myhist =  part_arg->hist[part_arg->_tid];
	size_t *myoutput_offset = global_lineitem_output_offset[part_arg->_tid];
	size_t padding_num = part_arg->padding_num;
	size_t org_tup_num = part_arg->org_tup_num;

	fltrq_lineitem_t *org_tup = (fltrq_lineitem_t *) part_arg->org_tup;
	fltrq_lineitem_t *tmp_tup = (fltrq_lineitem_t *) part_arg->tmp_tup;

	/* compute the local histogram */
	tpch_key_t tmp_hashkey;
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->l_partkey, 
			hashmask, bitskip
		);
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
	SYNC_TIMER(*part_arg->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each cluster */
	for (size_t idx = 0; idx < part_arg->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part_arg->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
		for (size_t jdx = 1; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][(jdx-1)*2+1] ;
		}
	}

#ifdef SYNCSTATS
	SYNC_TIMER(*part_arg->sync_local_prfx_sum);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_prfx_sum);
#endif /* SYNCSTATS */

	/* copy tuples to their corresponding clusters */
	if (part_arg->_tid < BUILDPART_WRITE_THREAD_NUM) {
		int tmp_tid = part_arg->_tid;
		size_t tmp_fltrq_lineitem_num;
		fltrq_lineitem_t *tmp_fltrq_lineitem;
		while (tmp_tid < BUILDPART_THREAD_NUM) {
			tmp_fltrq_lineitem_num = every_thread_lineitem_num[tmp_tid];
			tmp_fltrq_lineitem = lineitem_start_addr + tmp_tid * initial_lineitem_tuple_num_thr;

			for (size_t idx = 0; idx < tmp_fltrq_lineitem_num; idx ++) {
				tmp_hashkey = IDHASH(
					(tmp_fltrq_lineitem+idx)->l_partkey,
					hashmask, bitskip
				);
				memcpy(& tmp_tup[global_lineitem_output_offset[tmp_tid][tmp_hashkey]+tmp_hashkey*padding_num],
					tmp_fltrq_lineitem+idx, FLTRQ_LINEITEM_T_SIZE);
				global_lineitem_output_offset[tmp_tid][tmp_hashkey] ++;
			}

			/* move back the output_offset pointer to the starting address */
			/* this code segments could be moved to main and only be executed for the 1st thread */
			for (size_t idx = 0; idx < fanout; idx ++) {
				global_lineitem_output_offset[tmp_tid][idx] -= hist[tmp_tid][idx*2];
				global_lineitem_output_offset[tmp_tid][idx] += idx * padding_num;
			}

			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}
	}
}


#ifdef USE_SWWCB_OPTIMIZED_PART
void parallel_radix_partition_optimized_part(part_arg_t * const part_arg, 
	tpch_fltrq_part_swwcb_t * const swwcb, const size_t global_offset) {
	int rv;

	tpch_key_t hashmask = (part_arg->hashmask) << part_arg->bitskip;
	tpch_key_t bitskip = part_arg->bitskip;

	size_t fanout = part_arg->hashmask + 1;
	size_t **hist = part_arg->hist;
	size_t *myhist =  part_arg->hist[part_arg->_tid];
	size_t *output_offset = part_arg->output_offset;
	size_t padding_num = part_arg->padding_num;
	size_t org_tup_num = part_arg->org_tup_num;

	fltrq_part_t *org_tup = (fltrq_part_t *) part_arg->org_tup;
	fltrq_part_t *tmp_tup = (fltrq_part_t *) part_arg->tmp_tup;

	/* compute the local histogram */
	tpch_key_t tmp_hashkey;
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->p_partkey, 
			hashmask, bitskip
		);
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
	SYNC_TIMER(*part_arg->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each SWWCB */
	for (size_t idx = 0; idx < part_arg->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			output_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part_arg->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
		for (size_t jdx = 1; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			output_offset[jdx] += hist[idx][(jdx-1)*2+1];
		}
	}

	/* initial updates on SWWCB slots */
	for (size_t idx = 0; idx < fanout; idx ++) {
		output_offset[idx] += idx * padding_num;
		swwcb[idx].data.slot = output_offset[idx] + global_offset;
	}

	/* copy tuples to their corresponding SWWCBs */
	size_t slot;
	size_t slot_mod;
	size_t remainder_start_pos;
	fltrq_part_t *tmp_swwcb;
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->p_partkey, 
			hashmask, bitskip
		);
		slot = swwcb[tmp_hashkey].data.slot;
		tmp_swwcb = (fltrq_part_t *) (swwcb + tmp_hashkey);
		slot_mod = slot & (FLTRQ_PART_PER_SWWCB - 1);
		tmp_swwcb[slot_mod] = org_tup[idx];

		if (slot_mod == FLTRQ_PART_PER_SWWCB - 1) {
			/* non-temporal store a SWWCB */
			nontemp_store_swwcb(
				& tmp_tup[slot-(FLTRQ_PART_PER_SWWCB-1)-global_offset],
				tmp_swwcb
			);
		}

		swwcb[tmp_hashkey].data.slot = slot + 1;
	}

	/* nontemp store */
#ifdef SYNCSTATS
	SYNC_TIMER(*part_arg->sync_local_swwcb);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_swwcb);
#endif /* SYNCSTATS */

	/* write out the remainders in the swwcbs */
	for (size_t idx = 0; idx < fanout; idx ++) {
		slot = swwcb[idx].data.slot;
		slot_mod = slot & (FLTRQ_PART_PER_SWWCB - 1);
		slot -= slot_mod;

		remainder_start_pos = (slot < output_offset[idx] + global_offset ) ? (output_offset[idx] + global_offset - slot) : 0;
		for (size_t jdx = remainder_start_pos; jdx < slot_mod; jdx ++) {
			memcpy(&tmp_tup[slot+jdx-global_offset], 
				&swwcb[idx].data.tuples[jdx], FLTRQ_PART_T_SIZE);
		}
	}
}


void parallel_radix_partition_optimized_lineitem(part_arg_t * const part_arg, 
	tpch_fltrq_lineitem_swwcb_t * const swwcb, const size_t global_offset) {
	int rv;

	tpch_key_t hashmask = (part_arg->hashmask) << part_arg->bitskip;
	tpch_key_t bitskip = part_arg->bitskip;

	size_t fanout = part_arg->hashmask + 1;
	size_t **hist = part_arg->hist;
	size_t *myhist =  part_arg->hist[part_arg->_tid];
	size_t *output_offset = part_arg->output_offset;
	size_t padding_num = part_arg->padding_num;
	size_t org_tup_num = part_arg->org_tup_num;

	fltrq_lineitem_t *org_tup = (fltrq_lineitem_t *) part_arg->org_tup;
	fltrq_lineitem_t *tmp_tup = (fltrq_lineitem_t *) part_arg->tmp_tup;

	/* compute the local histogram */
	tpch_key_t tmp_hashkey;
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->l_partkey, 
			hashmask, bitskip
		);
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
	SYNC_TIMER(*part_arg->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each SWWCB */
	for (size_t idx = 0; idx < part_arg->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			output_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part_arg->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
		for (size_t jdx = 1; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			output_offset[jdx] += hist[idx][(jdx-1)*2+1];
		}
	}

	/* initial updates on SWWCB slots */
	for (size_t idx = 0; idx < fanout; idx ++) {
		output_offset[idx] += idx * padding_num;
		swwcb[idx].data.slot = output_offset[idx] + global_offset;
	}

	/* copy tuples to their corresponding SWWCBs */
	size_t slot;
	size_t slot_mod;
	size_t remainder_start_pos;
	fltrq_lineitem_t *tmp_swwcb;
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->l_partkey, 
			hashmask, bitskip
		);
		slot = swwcb[tmp_hashkey].data.slot;
		tmp_swwcb = (fltrq_lineitem_t *) (swwcb + tmp_hashkey);
		slot_mod = slot & (FLTRQ_LINEITEM_PER_SWWCB - 1);
		tmp_swwcb[slot_mod] = org_tup[idx];

		if (slot_mod == FLTRQ_LINEITEM_PER_SWWCB - 1) {
			/* non-temporal store a SWWCB */
			nontemp_store_swwcb(
				& tmp_tup[slot-(FLTRQ_LINEITEM_PER_SWWCB-1)-global_offset],
				tmp_swwcb
			);
		}

		swwcb[tmp_hashkey].data.slot = slot + 1;
	}

	/* nontemp store */
#ifdef SYNCSTATS
	SYNC_TIMER(*part_arg->sync_local_swwcb);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_swwcb);
#endif /* SYNCSTATS */

	/* write out the remainders in the swwcbs */
	for (size_t idx = 0; idx < fanout; idx ++) {
		slot = swwcb[idx].data.slot;
		slot_mod = slot & (FLTRQ_LINEITEM_PER_SWWCB - 1);
		slot -= slot_mod;

		remainder_start_pos = (slot < output_offset[idx] + global_offset ) ? (output_offset[idx] + global_offset - slot) : 0;
		for (size_t jdx = remainder_start_pos; jdx < slot_mod; jdx ++) {
			memcpy(&tmp_tup[slot+jdx-global_offset], 
				&swwcb[idx].data.tuples[jdx], FLTRQ_LINEITEM_T_SIZE);
		}
	}
}


void parallel_radix_partition_optimized_bw_reg_part(part_arg_t * const part_arg, const size_t global_offset) {
	int rv;

	tpch_key_t hashmask = (part_arg->hashmask) << part_arg->bitskip;
	tpch_key_t bitskip = part_arg->bitskip;

	size_t fanout = part_arg->hashmask + 1;
	size_t **hist = part_arg->hist;
	size_t *myhist =  part_arg->hist[part_arg->_tid];
	size_t *myoutput_offset = global_part_output_offset[part_arg->_tid];
	size_t padding_num = part_arg->padding_num;
	size_t org_tup_num = part_arg->org_tup_num;

	tpch_fltrq_part_swwcb_t *myswwcb = global_tpch_fltrq_part_swwcb_s[part_arg->_tid];

	fltrq_part_t *org_tup = (fltrq_part_t *) part_arg->org_tup;
	fltrq_part_t *tmp_tup = (fltrq_part_t *) part_arg->tmp_tup;

	/* compute the local histogram */
	tpch_key_t tmp_hashkey;
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->p_partkey,
			hashmask, bitskip
		);
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
	SYNC_TIMER(*part_arg->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each SWWCB */
	for (size_t idx = 0; idx < part_arg->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part_arg->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
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
	SYNC_TIMER(*part_arg->sync_local_prfx_sum);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_prfx_sum);
#endif /* SYNCSTATS */

	/* copy tuples to their corresponding SWWCBs */
	if (part_arg->_tid < BUILDPART_WRITE_THREAD_NUM) {
		int tmp_tid = part_arg->_tid;

		size_t slot;
		size_t slot_mod;
		size_t remainder_start_pos;
		fltrq_part_t *tmp_swwcb;	

		size_t tmp_fltrq_part_num;
		fltrq_part_t *tmp_fltrq_part;

		while (tmp_tid < BUILDPART_THREAD_NUM) {
			tmp_fltrq_part_num = every_thread_part_num[tmp_tid];
			tmp_fltrq_part = part_start_addr + tmp_tid * initial_part_tuple_num_thr;

			for (size_t idx = 0; idx < tmp_fltrq_part_num; idx ++) {
				tmp_hashkey = IDHASH(
					(tmp_fltrq_part+idx)->p_partkey,
					hashmask, bitskip
				);
				slot = global_tpch_fltrq_part_swwcb_s[tmp_tid][tmp_hashkey].data.slot;
				tmp_swwcb = (fltrq_part_t *) (global_tpch_fltrq_part_swwcb_s[tmp_tid]+tmp_hashkey);
				slot_mod = slot & (FLTRQ_PART_PER_SWWCB-1);
				tmp_swwcb[slot_mod] = tmp_fltrq_part[idx];

				if (slot_mod == FLTRQ_PART_PER_SWWCB-1) {
					nontemp_store_swwcb( & tmp_tup[slot-(FLTRQ_PART_PER_SWWCB-1)-global_offset],
						tmp_swwcb);
				}
				global_tpch_fltrq_part_swwcb_s[tmp_tid][tmp_hashkey].data.slot = slot + 1;
			}

			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}

		/* for barrier balancing among threads */
#ifdef SYNCSTATS
		SYNC_TIMER(*part_arg->sync_local_swwcb);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_swwcb);
#endif /* SYNCSTATS */

		tmp_tid = part_arg->_tid;
		while (tmp_tid < BUILDPART_THREAD_NUM) {
			for (size_t idx = 0; idx < fanout; idx ++) {
				slot = global_tpch_fltrq_part_swwcb_s[tmp_tid][idx].data.slot;
				slot_mod = slot & (FLTRQ_PART_PER_SWWCB-1);
				slot -= slot_mod;

				remainder_start_pos = (slot < global_part_output_offset[tmp_tid][idx] + global_offset) ?
					(global_part_output_offset[tmp_tid][idx] + global_offset - slot) : 0;

				for (size_t jdx = remainder_start_pos; jdx < slot_mod; jdx ++) {
					memcpy(&tmp_tup[slot+jdx-global_offset], 
						&global_tpch_fltrq_part_swwcb_s[tmp_tid][idx].data.tuples[jdx], 
						FLTRQ_PART_T_SIZE
					);
				}
			}
			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}

	} else {
		/* for barrier balancing among threads */
#ifdef SYNCSTATS
		SYNC_TIMER(*part_arg->sync_local_swwcb);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_swwcb);
#endif /* SYNCSTATS */
	}
}


void parallel_radix_partition_optimized_bw_reg_lineitem(part_arg_t * const part_arg, const size_t global_offset) {
	int rv;

	tpch_key_t hashmask = (part_arg->hashmask) << part_arg->bitskip;
	tpch_key_t bitskip = part_arg->bitskip;

	size_t fanout = part_arg->hashmask + 1;
	size_t **hist = part_arg->hist;
	size_t *myhist =  part_arg->hist[part_arg->_tid];
	size_t *myoutput_offset = global_lineitem_output_offset[part_arg->_tid];
	size_t padding_num = part_arg->padding_num;
	size_t org_tup_num = part_arg->org_tup_num;

	tpch_fltrq_lineitem_swwcb_t *myswwcb = global_tpch_fltrq_lineitem_swwcb_s[part_arg->_tid];

	fltrq_lineitem_t *org_tup = (fltrq_lineitem_t *) part_arg->org_tup;
	fltrq_lineitem_t *tmp_tup = (fltrq_lineitem_t *) part_arg->tmp_tup;

	/* compute the local histogram */
	tpch_key_t tmp_hashkey;
	for (size_t idx = 0; idx < org_tup_num; idx ++) {
		tmp_hashkey = IDHASH(
			(org_tup+idx)->l_partkey, 
			hashmask, bitskip
		);
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
	SYNC_TIMER(*part_arg->sync_local_hist);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_hist);
#endif /* SYNCSTATS */

	/* determine the start and end of each SWWCB */
	for (size_t idx = 0; idx < part_arg->_tid; idx ++) {
		for (size_t jdx = 0; jdx < fanout; jdx ++) {
			/* tuple count prefix sum of current partition of all threads */
			myoutput_offset[jdx] += hist[idx][jdx*2+1];
		}
	}

	for (size_t idx = part_arg->_tid; idx < BUILDPART_THREAD_NUM; idx ++) {
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
	SYNC_TIMER(*part_arg->sync_local_prfx_sum);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_prfx_sum);
#endif /* SYNCSTATS */

	/* copy tuples to their corresponding SWWCBs */
	if (part_arg->_tid < BUILDPART_WRITE_THREAD_NUM) {
		int tmp_tid = part_arg->_tid;

		size_t slot;
		size_t slot_mod;
		size_t remainder_start_pos;
		fltrq_lineitem_t *tmp_swwcb;	

		size_t tmp_fltrq_lineitem_num;
		fltrq_lineitem_t *tmp_fltrq_lineitem;

		while (tmp_tid < BUILDPART_THREAD_NUM) {
			tmp_fltrq_lineitem_num = every_thread_lineitem_num[tmp_tid];
			tmp_fltrq_lineitem = lineitem_start_addr + tmp_tid * initial_lineitem_tuple_num_thr;

			for (size_t idx = 0; idx < tmp_fltrq_lineitem_num; idx ++) {
				tmp_hashkey = IDHASH(
					(tmp_fltrq_lineitem+idx)->l_partkey,
					hashmask, bitskip
				);
				slot = global_tpch_fltrq_lineitem_swwcb_s[tmp_tid][tmp_hashkey].data.slot;
				tmp_swwcb = (fltrq_lineitem_t *) (global_tpch_fltrq_lineitem_swwcb_s[tmp_tid]+tmp_hashkey);
				slot_mod = slot & (FLTRQ_LINEITEM_PER_SWWCB-1);
				tmp_swwcb[slot_mod] = tmp_fltrq_lineitem[idx];

				if (slot_mod == FLTRQ_LINEITEM_PER_SWWCB-1) {
					nontemp_store_swwcb( & tmp_tup[slot-(FLTRQ_LINEITEM_PER_SWWCB-1)-global_offset],
						tmp_swwcb);
				}
				global_tpch_fltrq_lineitem_swwcb_s[tmp_tid][tmp_hashkey].data.slot = slot + 1;
			}

			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}

		/* for barrier balancing among threads */
#ifdef SYNCSTATS
		SYNC_TIMER(*part_arg->sync_local_swwcb);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_swwcb);
#endif /* SYNCSTATS */

		tmp_tid = part_arg->_tid;
		while (tmp_tid < BUILDPART_THREAD_NUM) {
			for (size_t idx = 0; idx < fanout; idx ++) {
				slot = global_tpch_fltrq_lineitem_swwcb_s[tmp_tid][idx].data.slot;
				slot_mod = slot & (FLTRQ_LINEITEM_PER_SWWCB-1);
				slot -= slot_mod;

				remainder_start_pos = (slot < global_lineitem_output_offset[tmp_tid][idx] + global_offset) ?
					(global_lineitem_output_offset[tmp_tid][idx] + global_offset - slot) : 0;

				for (size_t jdx = remainder_start_pos; jdx < slot_mod; jdx ++) {
					memcpy(&tmp_tup[slot+jdx-global_offset], 
						&global_tpch_fltrq_lineitem_swwcb_s[tmp_tid][idx].data.tuples[jdx], 
						FLTRQ_LINEITEM_T_SIZE
					);
				}
			}
			tmp_tid += BUILDPART_WRITE_THREAD_NUM;
		}

	} else {
		/* for barrier balancing among threads */
#ifdef SYNCSTATS
		SYNC_TIMER(*part_arg->sync_local_swwcb);
#endif /* SYNCSTATS */
		BARRIER_ARRIVE(part_arg->barrier, rv);
#ifdef SYNCSTATS
		SYNC_GLOBAL(part_arg->_tid, *part_arg->sync_global_swwcb);
#endif /* SYNCSTATS */
	}
}
#endif /* USE_SWWCB_OPTIMIZED_PART */


void *tpch_run_rdx_part(void *arg) {
	tpch_arg_t *tpch_arg = (tpch_arg_t *) arg;

#ifdef WARMUP
	tpch_warmup_localize(tpch_arg->tpch_query_meta->use_part, 
		tpch_arg->part_org_tup, tpch_arg->self_part_num, FLTRQ_PART_T_SIZE
	);
	tpch_warmup_localize(tpch_arg->tpch_query_meta->use_lineitem, 
		tpch_arg->lineitem_org_tup, tpch_arg->self_lineitem_num, FLTRQ_LINEITEM_T_SIZE
	);
#endif /* WARMUP */

	tpch_memset_part_mem(tpch_arg->tpch_query_meta->use_part, 
		tpch_arg->_tid, tpch_arg->part_tmp_tup,
		fltrq_part_num, FLTRQ_PART_T_SIZE
	);
	tpch_memset_part_mem(tpch_arg->tpch_query_meta->use_lineitem, 
		tpch_arg->_tid, tpch_arg->lineitem_tmp_tup,
		fltrq_lineitem_num, FLTRQ_LINEITEM_T_SIZE
	);

	int rv;

	size_t max_build_side_tup_num = 0;

	tpch_task_t *task;
	tpch_task_queue_t *part_queue = tpch_arg->part_queue;

// #if SKEW_HANDLING
// 	tpch_task_queue_t *skew_queue = tpch_arg->skew_queue;
// 	tpch_task_t **skewtask = tpch_arg->skewtask;
// #endif /* SKEW_HANDLING */

	part_arg_t part_arg;

	part_arg._tid = tpch_arg->_tid;
	part_arg.barrier = tpch_arg->barrier;

	tpch_arg->part_hist[tpch_arg->_tid] = (size_t *) alloc_memory( sizeof(size_t) * 2 * FANOUT_PASS1, CACHELINE_SIZE);
	tpch_arg->lineitem_hist[tpch_arg->_tid] = (size_t *) alloc_memory( sizeof(size_t) * 2 * FANOUT_PASS1, CACHELINE_SIZE);
	
	size_t *part_output_offset, *lineitem_output_offset;
#ifdef OUTPUT_OFFSET_NVM
	part_output_offset = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS1);
	lineitem_output_offset = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	part_output_offset = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS1, CACHELINE_SIZE);
	lineitem_output_offset = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS1, CACHELINE_SIZE);
#endif /* OUTPUT_OFFSET_NVM */

#ifdef USE_SWWCB_OPTIMIZED_PART
	tpch_fltrq_part_swwcb_t *tpch_fltrq_part_swwcb;
	tpch_fltrq_lineitem_swwcb_t *tpch_fltrq_lineitem_swwcb;
#ifdef USE_SWWCB_OPTIMIZED_PART_ON_NVM
	tpch_fltrq_part_swwcb = (tpch_fltrq_part_swwcb_t *) new_alloc_nvm(sizeof(tpch_fltrq_part_swwcb_t) * FANOUT_PASS1);
	tpch_fltrq_lineitem_swwcb = (tpch_fltrq_lineitem_swwcb_t *) new_alloc_nvm(sizeof(tpch_fltrq_lineitem_swwcb_t) * FANOUT_PASS1);
#else /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	tpch_fltrq_part_swwcb = (tpch_fltrq_part_swwcb_t *) alloc_aligned_dram(sizeof(tpch_fltrq_part_swwcb_t) * FANOUT_PASS1, CACHELINE_SIZE);
	tpch_fltrq_lineitem_swwcb = (tpch_fltrq_lineitem_swwcb_t *) alloc_aligned_dram(sizeof(tpch_fltrq_lineitem_swwcb_t) * FANOUT_PASS1, CACHELINE_SIZE);
#endif /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
#endif /* USE_SWWCB_OPTIMIZED_PART */

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
		clock_gettime(CLOCK_REALTIME, &tpch_arg->buildpart_start);
	}

	memset_localize(tpch_arg->part_hist[tpch_arg->_tid] , sizeof(size_t) * 2 * FANOUT_PASS1);
	memset_localize(tpch_arg->lineitem_hist[tpch_arg->_tid] , sizeof(size_t) * 2 * FANOUT_PASS1);

#ifdef OUTPUT_OFFSET_NVM
	pmem_memset_persist(part_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
	pmem_memset_persist(lineitem_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	memset(part_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
	memset(lineitem_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
#endif /* OUTPUT_OFFSET_NVM */

#ifdef USE_SWWCB_OPTIMIZED_PART
#ifdef USE_SWWCB_OPTIMIZED_PART_ON_NVM
	pmem_memset_persist(tpch_fltrq_part_swwcb, 0, sizeof(tpch_fltrq_part_swwcb_t) * FANOUT_PASS1);
	pmem_memset_persist(tpch_fltrq_lineitem_swwcb, 0, sizeof(tpch_fltrq_lineitem_swwcb_t) * FANOUT_PASS1);
#else /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	memset(tpch_fltrq_part_swwcb, 0, sizeof(tpch_fltrq_part_swwcb_t) * FANOUT_PASS1);
	memset(tpch_fltrq_lineitem_swwcb, 0, sizeof(tpch_fltrq_lineitem_swwcb_t) * FANOUT_PASS1);
#endif /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
#endif /* USE_SWWCB_OPTIMIZED_PART */

	/**************** END OF PRE-PARTITION MEMSET ****************/
	/* wait for all threads finish the memset for immediates */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_prepart_memset);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_prepart_memset);
#endif /* SYNCSTATS */

	/**************** FIRST PASS OF PARTITIONING ****************/
	part_arg.bitskip = 0;
	part_arg.hashmask = FANOUT_PASS1-1;

	/**************** PARTITIONING R ****************/
	part_arg.org_tup_num = tpch_arg->self_part_num;
	part_arg.org_tup = (void*) tpch_arg->part_org_tup;
	part_arg.tmp_tup = (void*) tpch_arg->part_tmp_tup;

	part_arg.hist = tpch_arg->part_hist;
	part_arg.output_offset = part_output_offset;
	part_arg.padding_num = FANOUT_PASS2 * FLTRQ_PART_PADDING_UNIT_NUM;

#ifdef SYNCSTATS
	part_arg.sync_local_hist = & tpch_arg->localtimer->sync_1st_hist[0];
	part_arg.sync_global_hist = & tpch_arg->globaltimer->sync_1st_hist[0];
#endif /* SYNCSTATS */

#ifdef USE_SWWCB_OPTIMIZED_PART

#ifdef SYNCSTATS
	part_arg.sync_local_swwcb = & tpch_arg->localtimer->sync_1st_swwcb[0];
	part_arg.sync_global_swwcb = & tpch_arg->globaltimer->sync_1st_swwcb[0];
#endif /* SYNCSTATS */

	parallel_radix_partition_optimized_part(&part_arg, tpch_fltrq_part_swwcb, 0);
#else /* USE_SWWCB_OPTIMIZED_PART */
	parallel_radix_partition_part(&part_arg);
#endif /* USE_SWWCB_OPTIMIZED_PART */


	/**************** PARTITIONING S ****************/
	part_arg.org_tup_num = tpch_arg->self_lineitem_num;
	part_arg.org_tup = (void*) tpch_arg->lineitem_org_tup;
	part_arg.tmp_tup = (void*) tpch_arg->lineitem_tmp_tup;

	part_arg.hist = tpch_arg->lineitem_hist;
	part_arg.output_offset = lineitem_output_offset;
	part_arg.padding_num = FANOUT_PASS2 * FLTRQ_LINEITEM_PADDING_UNIT_NUM;

#ifdef SYNCSTATS
	part_arg.sync_local_hist = & tpch_arg->localtimer->sync_1st_hist[1];
	part_arg.sync_global_hist = & tpch_arg->globaltimer->sync_1st_hist[1];
#endif /* SYNCSTATS */

#ifdef USE_SWWCB_OPTIMIZED_PART

#ifdef SYNCSTATS
	part_arg.sync_local_swwcb = & tpch_arg->localtimer->sync_1st_swwcb[1];
	part_arg.sync_global_swwcb = & tpch_arg->globaltimer->sync_1st_swwcb[1];
#endif /* SYNCSTATS */

	parallel_radix_partition_optimized_lineitem(&part_arg, tpch_fltrq_lineitem_swwcb, 0);
#else /* USE_SWWCB_OPTIMIZED_PART */
	parallel_radix_partition_lineitem(&part_arg);
#endif /* USE_SWWCB_OPTIMIZED_PART */


	/* wait for all threads finish the 1st pass partitioning */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_1st_pass);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_1st_pass);
#endif /* SYNCSTATS */

	/********** end of 1st partitioning phase ******************/

	/* 3. first thread creates partitioning tasks for 2nd pass */
	size_t part_num_tup;
	size_t lineitem_num_tup;
	if (tpch_arg->_tid == 0) {
		for (size_t idx = 0; idx < FANOUT_PASS1; idx ++) {

			/* within the first thread, thus no memory leak */
			if (idx != FANOUT_PASS1-1) {
				part_num_tup = part_output_offset[idx+1] - part_output_offset[idx] - FANOUT_PASS2 * FLTRQ_PART_PADDING_UNIT_NUM;
				lineitem_num_tup = lineitem_output_offset[idx+1] - lineitem_output_offset[idx] - FANOUT_PASS2 * FLTRQ_LINEITEM_PADDING_UNIT_NUM;
			} else {
				part_num_tup = 0;
				lineitem_num_tup = 0;
				for (int jdx = 0; jdx < BUILDPART_THREAD_NUM; jdx ++) {
					part_num_tup += tpch_arg->part_hist[jdx][2*idx];
					lineitem_num_tup += tpch_arg->lineitem_hist[jdx][2*idx];
				}
			}

			if (part_num_tup > 0 && lineitem_num_tup > 0) {
				task = tpch_task_queue_get_slot(part_queue);

				task->fltrq_part = tpch_arg->part_tmp_tup + part_output_offset[idx];
				task->self_part_num = part_num_tup;
				task->fltrq_part_tmp = ( (fltrq_part_t *) tpch_arg->part_org_tup ) + part_output_offset[idx];

				task->fltrq_lineitem = tpch_arg->lineitem_tmp_tup + lineitem_output_offset[idx];
				task->self_lineitem_num = lineitem_num_tup;
				task->fltrq_lineitem_tmp = ( (fltrq_lineitem_t *) tpch_arg->lineitem_org_tup ) + lineitem_output_offset[idx];

#if NUM_PASSES != 1
				task->part_global_offset = part_output_offset[idx];
				task->lineitem_global_offset = lineitem_output_offset[idx];
#endif /* NUM_PASSES != 1 */

				tpch_task_queue_add(part_queue, task);

				max_build_side_tup_num = max_build_side_tup_num >= lineitem_num_tup ? max_build_side_tup_num : lineitem_num_tup;
			}
		}
	}

	/* wait for the first thread add tasks to respective queues */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_1st_pass_queue);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_1st_pass_queue);
#endif /* SYNCSTATS */


	/* wait for all threads finish the partitioning phase */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_part);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_part);
#endif /* SYNCSTATS */


	/* share the max_build_side_tup_num to all threads */
	if (tpch_arg->_tid == 0) {
		for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
			if (max_build_side_tup_num < tpch_arg->max_build_side_tup_num[idx]) {
				max_build_side_tup_num = tpch_arg->max_build_side_tup_num[idx];
			}
		}
		for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
			tpch_arg->max_build_side_tup_num[idx] = max_build_side_tup_num;
		}
	}

	/* wait for the first thread finishs counting the maximum size of hashtable */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_max_hashtable);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_max_hashtable);
#endif /* SYNCSTATS */

	if (tpch_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &tpch_arg->buildpart_end);
	}

#ifdef USE_PMWATCH
	if (tpch_arg->_tid == 0) {
		pmwEnd(PMWatch_output_buildpart);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, tpch_arg->PAPI_values_buildpart);
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


#ifdef USE_SWWCB_OPTIMIZED_PART
#ifdef USE_SWWCB_OPTIMIZED_PART_ON_NVM
	pmem_unmap(tpch_fltrq_part_swwcb, sizeof(tpch_fltrq_part_swwcb_t) * FANOUT_PASS1);
	pmem_unmap(tpch_fltrq_lineitem_swwcb, sizeof(tpch_fltrq_lineitem_swwcb_t) * FANOUT_PASS1);
#else /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	free(tpch_fltrq_part_swwcb);
	free(tpch_fltrq_lineitem_swwcb);
#endif /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
#endif /* USE_SWWCB_OPTIMIZED_PART */

// #ifdef SKEW_HANDLING
// 	dealloc_memory(tpch_arg->part_hist[tpch_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS2);
// 	dealloc_memory(tpch_arg->lineitem_hist[tpch_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS2);
// #else /* SKEW_HANDLING */
	dealloc_memory(tpch_arg->part_hist[tpch_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS1);
	dealloc_memory(tpch_arg->lineitem_hist[tpch_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS1);
// #endif /* SKEW_HANDLING */

#ifdef OUTPUT_OFFSET_NVM
	pmem_unmap(part_output_offset, sizeof(size_t) * FANOUT_PASS1);
	pmem_unmap(lineitem_output_offset, sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	dealloc_dram(part_output_offset, sizeof(size_t) * FANOUT_PASS1);
	dealloc_dram(lineitem_output_offset, sizeof(size_t) * FANOUT_PASS1);
#endif /* OUTPUT_OFFSET_NVM */

	return NULL;
}


void *tpch_run_rdx_part_bw_reg(void *arg) {
	tpch_arg_t *tpch_arg = (tpch_arg_t *) arg;

#ifdef WARMUP
	tpch_warmup_localize(tpch_arg->tpch_query_meta->use_part, 
		tpch_arg->part_org_tup, tpch_arg->self_part_num, FLTRQ_PART_T_SIZE
	);
	tpch_warmup_localize(tpch_arg->tpch_query_meta->use_lineitem, 
		tpch_arg->lineitem_org_tup, tpch_arg->self_lineitem_num, FLTRQ_LINEITEM_T_SIZE
	);
#endif /* WARMUP */

	tpch_memset_part_mem(tpch_arg->tpch_query_meta->use_part, 
		tpch_arg->_tid, tpch_arg->part_tmp_tup,
		fltrq_part_num, FLTRQ_PART_T_SIZE
	);
	tpch_memset_part_mem(tpch_arg->tpch_query_meta->use_lineitem, 
		tpch_arg->_tid, tpch_arg->lineitem_tmp_tup,
		fltrq_lineitem_num, FLTRQ_LINEITEM_T_SIZE
	);

	int rv;

	size_t max_build_side_tup_num = 0;

	tpch_task_t *task;
	tpch_task_queue_t *part_queue = tpch_arg->part_queue;

// #if SKEW_HANDLING
// 	tpch_task_queue_t *skew_queue = tpch_arg->skew_queue;
// 	tpch_task_t **skewtask = tpch_arg->skewtask;
// #endif /* SKEW_HANDLING */

	part_arg_t part_arg;

	part_arg._tid = tpch_arg->_tid;
	part_arg.barrier = tpch_arg->barrier;

	tpch_arg->part_hist[tpch_arg->_tid] = (size_t *) alloc_memory( sizeof(size_t) * 2 * FANOUT_PASS1, CACHELINE_SIZE);
	tpch_arg->lineitem_hist[tpch_arg->_tid] = (size_t *) alloc_memory( sizeof(size_t) * 2 * FANOUT_PASS1, CACHELINE_SIZE);
	
#ifdef OUTPUT_OFFSET_NVM
	global_part_output_offset[tpch_arg->_tid] = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS1 );
	global_lineitem_output_offset[tpch_arg->_tid] = (size_t *) new_alloc_nvm( sizeof(size_t) * FANOUT_PASS1 );
#else /* OUTPUT_OFFSET_NVM */
	global_part_output_offset[tpch_arg->_tid] = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS1, CACHELINE_SIZE );
	global_lineitem_output_offset[tpch_arg->_tid] = (size_t *) alloc_dram( sizeof(size_t) * FANOUT_PASS1, CACHELINE_SIZE );
#endif /* OUTPUT_OFFSET_NVM */
	size_t *part_output_offset = global_part_output_offset[tpch_arg->_tid];
	size_t *lineitem_output_offset = global_lineitem_output_offset[tpch_arg->_tid];

#ifdef USE_SWWCB_OPTIMIZED_PART
#ifdef USE_SWWCB_OPTIMIZED_PART_ON_NVM
	global_tpch_fltrq_part_swwcb_s[tpch_arg->_tid] = (tpch_fltrq_part_swwcb_t *) new_alloc_nvm(sizeof(tpch_fltrq_part_swwcb_t) * FANOUT_PASS1);
	global_tpch_fltrq_lineitem_swwcb_s[tpch_arg->_tid] = (tpch_fltrq_lineitem_swwcb_t *) new_alloc_nvm(sizeof(tpch_fltrq_lineitem_swwcb_t) * FANOUT_PASS1);
#else /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	global_tpch_fltrq_part_swwcb_s[tpch_arg->_tid] = (tpch_fltrq_part_swwcb_t *) alloc_aligned_dram(sizeof(tpch_fltrq_part_swwcb_t) * FANOUT_PASS1, CACHELINE_SIZE);
	global_tpch_fltrq_lineitem_swwcb_s[tpch_arg->_tid] = (tpch_fltrq_lineitem_swwcb_t *) alloc_aligned_dram(sizeof(tpch_fltrq_lineitem_swwcb_t) * FANOUT_PASS1, CACHELINE_SIZE);
#endif /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
#endif /* USE_SWWCB_OPTIMIZED_PART */

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
		clock_gettime(CLOCK_REALTIME, &tpch_arg->buildpart_start);
	}

	memset_localize(tpch_arg->part_hist[tpch_arg->_tid] , sizeof(size_t) * 2 * FANOUT_PASS1);
	memset_localize(tpch_arg->lineitem_hist[tpch_arg->_tid] , sizeof(size_t) * 2 * FANOUT_PASS1);

#ifdef OUTPUT_OFFSET_NVM
	pmem_memset_persist(part_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
	pmem_memset_persist(lineitem_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	memset(part_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
	memset(lineitem_output_offset, 0, sizeof(size_t) * FANOUT_PASS1);
#endif /* OUTPUT_OFFSET_NVM */

#ifdef USE_SWWCB_OPTIMIZED_PART
#ifdef USE_SWWCB_OPTIMIZED_PART_ON_NVM
	pmem_memset_persist(global_tpch_fltrq_part_swwcb_s[tpch_arg->_tid], 0, sizeof(tpch_fltrq_part_swwcb_t) * FANOUT_PASS1);
	pmem_memset_persist(global_tpch_fltrq_lineitem_swwcb_s[tpch_arg->_tid], 0, sizeof(tpch_fltrq_lineitem_swwcb_t) * FANOUT_PASS1);
#else /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	memset(global_tpch_fltrq_part_swwcb_s[tpch_arg->_tid], 0, sizeof(tpch_fltrq_part_swwcb_t) * FANOUT_PASS1);
	memset(global_tpch_fltrq_lineitem_swwcb_s[tpch_arg->_tid], 0, sizeof(tpch_fltrq_lineitem_swwcb_t) * FANOUT_PASS1);
#endif /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
#endif /* USE_SWWCB_OPTIMIZED_PART */

	/**************** END OF PRE-PARTITION MEMSET ****************/
	/* wait for all threads finish the memset for immediates */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_prepart_memset);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_prepart_memset);
#endif /* SYNCSTATS */

	/**************** FIRST PASS OF PARTITIONING ****************/
	part_arg.bitskip = 0;
	part_arg.hashmask = FANOUT_PASS1-1;

	/**************** PARTITIONING R ****************/
	part_arg.org_tup_num = tpch_arg->self_part_num;
	part_arg.org_tup = (void*) tpch_arg->part_org_tup;
	part_arg.tmp_tup = (void*) tpch_arg->part_tmp_tup;

	part_arg.hist = tpch_arg->part_hist;
	part_arg.padding_num = FANOUT_PASS2 * FLTRQ_PART_PADDING_UNIT_NUM;

#ifdef SYNCSTATS
	part_arg.sync_local_hist = & tpch_arg->localtimer->sync_1st_hist[0];
	part_arg.sync_global_hist = & tpch_arg->globaltimer->sync_1st_hist[0];
	part_arg.sync_local_prfx_sum = & tpch_arg->localtimer->sync_1st_prfx_sum[0];
	part_arg.sync_global_prfx_sum = & tpch_arg->globaltimer->sync_1st_prfx_sum[0];
#endif /* SYNCSTATS */

#ifdef USE_SWWCB_OPTIMIZED_PART

#ifdef SYNCSTATS
	part_arg.sync_local_swwcb = & tpch_arg->localtimer->sync_1st_swwcb[0];
	part_arg.sync_global_swwcb = & tpch_arg->globaltimer->sync_1st_swwcb[0];
#endif /* SYNCSTATS */

	parallel_radix_partition_optimized_bw_reg_part(&part_arg, 0);
#else /* USE_SWWCB_OPTIMIZED_PART */
	parallel_radix_partition_bw_reg_part(&part_arg);
#endif /* USE_SWWCB_OPTIMIZED_PART */


	/**************** PARTITIONING S ****************/
	part_arg.org_tup_num = tpch_arg->self_lineitem_num;
	part_arg.org_tup = (void*) tpch_arg->lineitem_org_tup;
	part_arg.tmp_tup = (void*) tpch_arg->lineitem_tmp_tup;

	part_arg.hist = tpch_arg->lineitem_hist;
	part_arg.padding_num = FANOUT_PASS2 * FLTRQ_LINEITEM_PADDING_UNIT_NUM;

#ifdef SYNCSTATS
	part_arg.sync_local_hist = & tpch_arg->localtimer->sync_1st_hist[1];
	part_arg.sync_global_hist = & tpch_arg->globaltimer->sync_1st_hist[1];
	part_arg.sync_local_prfx_sum = & tpch_arg->localtimer->sync_1st_prfx_sum[1];
	part_arg.sync_global_prfx_sum = & tpch_arg->globaltimer->sync_1st_prfx_sum[1];
#endif /* SYNCSTATS */

#ifdef USE_SWWCB_OPTIMIZED_PART

#ifdef SYNCSTATS
	part_arg.sync_local_swwcb = & tpch_arg->localtimer->sync_1st_swwcb[1];
	part_arg.sync_global_swwcb = & tpch_arg->globaltimer->sync_1st_swwcb[1];
#endif /* SYNCSTATS */

	parallel_radix_partition_optimized_bw_reg_lineitem(&part_arg, 0);
#else /* USE_SWWCB_OPTIMIZED_PART */
	parallel_radix_partition_bw_reg_lineitem(&part_arg);
#endif /* USE_SWWCB_OPTIMIZED_PART */


	/* wait for all threads finish the 1st pass partitioning */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_1st_pass);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_1st_pass);
#endif /* SYNCSTATS */

	/********** end of 1st partitioning phase ******************/

	/* 3. first thread creates partitioning tasks for 2nd pass */
	size_t part_num_tup;
	size_t lineitem_num_tup;
	if (tpch_arg->_tid == 0) {
		for (size_t idx = 0; idx < FANOUT_PASS1; idx ++) {

			/* within the first thread, thus no memory leak */
			if (idx != FANOUT_PASS1-1) {
				part_num_tup = part_output_offset[idx+1] - part_output_offset[idx] - FANOUT_PASS2 * FLTRQ_PART_PADDING_UNIT_NUM;
				lineitem_num_tup = lineitem_output_offset[idx+1] - lineitem_output_offset[idx] - FANOUT_PASS2 * FLTRQ_LINEITEM_PADDING_UNIT_NUM;
			} else {
				part_num_tup = 0;
				lineitem_num_tup = 0;
				for (int jdx = 0; jdx < BUILDPART_THREAD_NUM; jdx ++) {
					part_num_tup += tpch_arg->part_hist[jdx][2*idx];
					lineitem_num_tup += tpch_arg->lineitem_hist[jdx][2*idx];
				}
			}

			if (part_num_tup > 0 && lineitem_num_tup > 0) {
				task = tpch_task_queue_get_slot(part_queue);

				task->fltrq_part = tpch_arg->part_tmp_tup + part_output_offset[idx];
				task->self_part_num = part_num_tup;
				task->fltrq_part_tmp = ( (fltrq_part_t *) tpch_arg->part_org_tup ) + part_output_offset[idx];

				task->fltrq_lineitem = tpch_arg->lineitem_tmp_tup + lineitem_output_offset[idx];
				task->self_lineitem_num = lineitem_num_tup;
				task->fltrq_lineitem_tmp = ( (fltrq_lineitem_t *) tpch_arg->lineitem_org_tup ) + lineitem_output_offset[idx];

#if NUM_PASSES != 1
				task->part_global_offset = part_output_offset[idx];
				task->lineitem_global_offset = lineitem_output_offset[idx];
#endif /* NUM_PASSES != 1 */

				tpch_task_queue_add(part_queue, task);

				max_build_side_tup_num = max_build_side_tup_num >= lineitem_num_tup ? max_build_side_tup_num : lineitem_num_tup;
			}
		}
	}

	/* wait for the first thread add tasks to respective queues */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_1st_pass_queue);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_1st_pass_queue);
#endif /* SYNCSTATS */


	/* wait for all threads finish the partitioning phase */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_part);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_part);
#endif /* SYNCSTATS */


	/* share the max_build_side_tup_num to all threads */
	if (tpch_arg->_tid == 0) {
		for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
			if (max_build_side_tup_num < tpch_arg->max_build_side_tup_num[idx]) {
				max_build_side_tup_num = tpch_arg->max_build_side_tup_num[idx];
			}
		}
		for (int idx = 0; idx < BUILDPART_THREAD_NUM; idx ++) {
			tpch_arg->max_build_side_tup_num[idx] = max_build_side_tup_num;
		}
	}

	/* wait for the first thread finishs counting the maximum size of hashtable */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_max_hashtable);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_max_hashtable);
#endif /* SYNCSTATS */

	if (tpch_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &tpch_arg->buildpart_end);
	}

#ifdef USE_PMWATCH
	if (tpch_arg->_tid == 0) {
		pmwEnd(PMWatch_output_buildpart);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, tpch_arg->PAPI_values_buildpart);
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


#ifdef USE_SWWCB_OPTIMIZED_PART
#ifdef USE_SWWCB_OPTIMIZED_PART_ON_NVM
	pmem_unmap(global_tpch_fltrq_part_swwcb_s[tpch_arg->_tid], sizeof(tpch_fltrq_part_swwcb_t) * FANOUT_PASS1);
	pmem_unmap(global_tpch_fltrq_lineitem_swwcb_s[tpch_arg->_tid], sizeof(tpch_fltrq_lineitem_swwcb_t) * FANOUT_PASS1);
#else /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
	free(global_tpch_fltrq_part_swwcb_s[tpch_arg->_tid]);
	free(global_tpch_fltrq_lineitem_swwcb_s[tpch_arg->_tid]);
#endif /* USE_SWWCB_OPTIMIZED_PART_ON_NVM */
#endif /* USE_SWWCB_OPTIMIZED_PART */

// #ifdef SKEW_HANDLING
// 	dealloc_memory(tpch_arg->part_hist[tpch_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS2);
// 	dealloc_memory(tpch_arg->lineitem_hist[tpch_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS2);
// #else /* SKEW_HANDLING */
	dealloc_memory(tpch_arg->part_hist[tpch_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS1);
	dealloc_memory(tpch_arg->lineitem_hist[tpch_arg->_tid], sizeof(size_t) * 2 * FANOUT_PASS1);
// #endif /* SKEW_HANDLING */

#ifdef OUTPUT_OFFSET_NVM
	pmem_unmap(part_output_offset, sizeof(size_t) * FANOUT_PASS1);
	pmem_unmap(lineitem_output_offset, sizeof(size_t) * FANOUT_PASS1);
#else /* OUTPUT_OFFSET_NVM */
	dealloc_dram(part_output_offset, sizeof(size_t) * FANOUT_PASS1);
	dealloc_dram(lineitem_output_offset, sizeof(size_t) * FANOUT_PASS1);
#endif /* OUTPUT_OFFSET_NVM */

	return NULL;
}



void tpch_bc_join(tpch_task_t *task, join_arg_t *join_arg){
	join_arg->matched_cnt = 0;

#if QUERY == Q14
	join_arg->numerator = 0;
	join_arg->denominator = 0;
#elif QUERY == Q19
	join_arg->revenue = 0;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

	tpch_key_t max_hashkey = next_pow_2(task->self_lineitem_num);
	tpch_key_t hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	tpch_key_t tmp_hashkey;

	tpch_key_t *bucket, *next;

#if QUERY == Q19
	fltrq_match_t fltrq_match;
#else /* QUERY == Q19 */
#endif /* QUERY == Q19 */


#ifdef PHJ_MBP
	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
#endif /* PHJ_MBP */

	/* memset the intermediate */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_start);
#endif /* PHJ_MBP */
	memset_localize(join_arg->intermediate, TPCH_KEY_T__SIZE * max_hashkey + TPCH_KEY_T__SIZE * task->self_lineitem_num);
#ifdef PHJ_MBP	
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

	/* build the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */

	bucket = (tpch_key_t *) (join_arg->intermediate);
	next = (tpch_key_t *) (join_arg->intermediate + TPCH_KEY_T__SIZE * max_hashkey);

	for (size_t idx = 0; idx < task->self_lineitem_num;) {
		tmp_hashkey = IDHASH( 
			task->fltrq_lineitem[idx].l_partkey, 
			hashmask, NUM_RADIX_BITS
		);
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

	for (size_t idx = 0; idx < task->self_part_num; idx ++) {
		tmp_hashkey = IDHASH(
			task->fltrq_part[idx].p_partkey,
			hashmask, NUM_RADIX_BITS
		);

		for (tpch_key_t hit = bucket[tmp_hashkey]; hit > 0; hit = next[hit-1]) {
// 			if (task->fltrq_part[idx].p_partkey == task->fltrq_lineitem[hit-1].l_partkey) {
// #if QUERY == Q14
// 				(join_arg->matched_cnt) ++;
// 				if ( strncmp(task->fltrq_part[idx].p_type, "PROMO", 5) == 0 ) {
// 					(join_arg->numerator) += 
// 						task->fltrq_lineitem[hit-1].l_extendedprice *
// 						( 1 - task->fltrq_lineitem[hit-1].l_discount );
// 				}
// 				(join_arg->denominator) += 
// 					task->fltrq_lineitem[hit-1].l_extendedprice *
// 					( 1 - task->fltrq_lineitem[hit-1].l_discount );
// #elif QUERY == Q19
// 				memcpy(fltrq_match.p_brand, task->fltrq_part[idx].p_brand, 10);
// 				fltrq_match.p_size = task->fltrq_part[idx].p_size;
// 				memcpy(fltrq_match.p_container, task->fltrq_part[idx].p_container, 10);
// 				fltrq_match.l_quantity = task->fltrq_lineitem[hit-1].l_quantity;
// 				memcpy(fltrq_match.l_shipinstruct, task->fltrq_lineitem[hit-1].l_shipinstruct, 25);
// 				memcpy(fltrq_match.l_shipmode, task->fltrq_lineitem[hit-1].l_shipmode, 10);
// 				if ( match_filter(&fltrq_match) ) {
// 					(join_arg->matched_cnt) ++;
// 					(join_arg->revenue) += task->fltrq_lineitem[hit-1].l_extendedprice *
// 						( 1 - task->fltrq_lineitem[hit-1].l_discount );
// 				}
// #else /* QUERY == Q14 */
// #endif /* QUERY == Q14 */
// 			}

			short matched_flag = (task->fltrq_part[idx].p_partkey == task->fltrq_lineitem[hit-1].l_partkey);
#if QUERY == Q14
			(join_arg->matched_cnt) += matched_flag;
			(join_arg->numerator) += task->fltrq_lineitem[hit-1].l_extendedprice *
				( 1 - task->fltrq_lineitem[hit-1].l_discount ) * 
				( matched_flag & (strncmp(task->fltrq_part[idx].p_type, "PROMO", 5) == 0) );
			(join_arg->denominator) += task->fltrq_lineitem[hit-1].l_extendedprice *
				( 1 - task->fltrq_lineitem[hit-1].l_discount ) * matched_flag;
#elif QUERY == Q19
				memcpy(fltrq_match.p_brand, task->fltrq_part[idx].p_brand, 10);
				fltrq_match.p_size = task->fltrq_part[idx].p_size;
				memcpy(fltrq_match.p_container, task->fltrq_part[idx].p_container, 10);
				fltrq_match.l_quantity = task->fltrq_lineitem[hit-1].l_quantity;
				memcpy(fltrq_match.l_shipinstruct, task->fltrq_lineitem[hit-1].l_shipinstruct, 25);
				memcpy(fltrq_match.l_shipmode, task->fltrq_lineitem[hit-1].l_shipmode, 10);
				matched_flag = matched_flag &  match_filter(&fltrq_match);
				(join_arg->matched_cnt) += matched_flag;
				(join_arg->revenue) += task->fltrq_lineitem[hit-1].l_extendedprice *
					( 1 - task->fltrq_lineitem[hit-1].l_discount ) * matched_flag;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

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


void tpch_hr_join(tpch_task_t *task, join_arg_t *join_arg){
	join_arg->matched_cnt = 0;

#if QUERY == Q14
	join_arg->numerator = 0;
	join_arg->denominator = 0;
#elif QUERY == Q19
	join_arg->revenue = 0;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

#if QUERY == Q19
	fltrq_match_t fltrq_match;
#else /* QUERY == Q19 */
#endif /* QUERY == Q19 */

	tpch_key_t max_hashkey = next_pow_2(task->self_lineitem_num);

	/* 4 times smaller hash table index, thus 4 time more comparison than bc_join * lp_join */
	// max_hashkey >>= HR_HRO_FACTOR;
	// if (max_hashkey < (1<<HR_HRO_FACTOR)) {
	// 	max_hashkey = (1<<HR_HRO_FACTOR);
	// }

	tpch_key_t hashmask = ( max_hashkey - 1 ) << NUM_RADIX_BITS;
	tpch_key_t tmp_hashkey;

	size_t *hist;
	size_t accum_num = 0;
#ifdef PHJ_MBP
	struct timespec memset_start, memset_end, build_start, build_end, probe_start, probe_end;
#endif /* PHJ_MBP */

	/* memset the intermediate */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &memset_start);
#endif /* PHJ_MBP */
	memset_localize(join_arg->intermediate, TPCH_KEY_T__SIZE * max_hashkey);
#ifdef PHJ_MBP	
	clock_gettime(CLOCK_REALTIME, &memset_end);
#endif /* PHJ_MBP */

	/* build the hash table */
#ifdef PHJ_MBP
	clock_gettime(CLOCK_REALTIME, &build_start);
#endif /* PHJ_MBP */

	hist = (tpch_key_t *) (join_arg -> intermediate);

	for (size_t idx = 0; idx < task->self_lineitem_num; idx ++) {
		tmp_hashkey = IDHASH( 
			task->fltrq_lineitem[idx].l_partkey, 
			hashmask, NUM_RADIX_BITS
		);
		hist[tmp_hashkey] ++;
	}

	for (size_t idx = 0; idx < max_hashkey; idx ++) {
		accum_num += hist[idx];
		hist[idx] = accum_num - hist[idx];
	}

	for (size_t idx = 0; idx < task->self_lineitem_num; idx ++) {
		tmp_hashkey = IDHASH( 
			task->fltrq_lineitem[idx].l_partkey, 
			hashmask, NUM_RADIX_BITS
		);
		task->fltrq_lineitem_tmp[hist[tmp_hashkey]] = task->fltrq_lineitem[idx];

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

	for (size_t idx = 0; idx < task->self_part_num; idx ++) {
		tmp_hashkey = IDHASH(
			task->fltrq_part[idx].p_partkey,
			hashmask, NUM_RADIX_BITS
		);

		start_idx = hist[ tmp_hashkey - (tmp_hashkey!=0) ] * (tmp_hashkey!=0);
		end_idx = hist[tmp_hashkey];
	
		for (size_t jdx = start_idx; jdx < end_idx; jdx ++) {

// 			if (task->fltrq_part[idx].p_partkey == task->fltrq_lineitem_tmp[jdx].l_partkey) {
// #if QUERY == Q14
// 				(join_arg->matched_cnt) ++;
// 				if ( strncmp(task->fltrq_part[idx].p_type, "PROMO", 5) == 0 ) {
// 					(join_arg->numerator) += 
// 						task->fltrq_lineitem_tmp[jdx].l_extendedprice *
// 						( 1 - task->fltrq_lineitem_tmp[jdx].l_discount );
// 				}
// 				(join_arg->denominator) += 
// 					task->fltrq_lineitem_tmp[jdx].l_extendedprice *
// 					( 1 - task->fltrq_lineitem_tmp[jdx].l_discount );
// #elif QUERY == Q19
// 				memcpy(fltrq_match.p_brand, task->fltrq_part[idx].p_brand, 10);
// 				fltrq_match.p_size = task->fltrq_part[idx].p_size;
// 				memcpy(fltrq_match.p_container, task->fltrq_part[idx].p_container, 10);
// 				fltrq_match.l_quantity = task->fltrq_lineitem_tmp[jdx].l_quantity;
// 				memcpy(fltrq_match.l_shipinstruct, task->fltrq_lineitem_tmp[jdx].l_shipinstruct, 25);
// 				memcpy(fltrq_match.l_shipmode, task->fltrq_lineitem_tmp[jdx].l_shipmode, 10);
// 				if ( match_filter(&fltrq_match) ) {
// 					(join_arg->matched_cnt) ++;
// 					(join_arg->revenue) += task->fltrq_lineitem_tmp[jdx].l_extendedprice *
// 						( 1 - task->fltrq_lineitem_tmp[jdx].l_discount );
// 				}
// #else /* QUERY == Q14 */
// #endif /* QUERY == Q14 */
// 			}

			short matched_flag = (task->fltrq_part[idx].p_partkey == task->fltrq_lineitem_tmp[jdx].l_partkey);
#if QUERY == Q14
			(join_arg->matched_cnt) += matched_flag;
			(join_arg->numerator) += task->fltrq_lineitem_tmp[jdx].l_extendedprice *
				( 1 - task->fltrq_lineitem_tmp[jdx].l_discount ) * 
				( matched_flag & ( strncmp(task->fltrq_part[idx].p_type, "PROMO", 5) == 0 ) );
			(join_arg->denominator) += task->fltrq_lineitem_tmp[jdx].l_extendedprice *
				( 1 - task->fltrq_lineitem_tmp[jdx].l_discount ) * matched_flag;
#elif QUERY == Q19
			memcpy(fltrq_match.p_brand, task->fltrq_part[idx].p_brand, 10);
			fltrq_match.p_size = task->fltrq_part[idx].p_size;
			memcpy(fltrq_match.p_container, task->fltrq_part[idx].p_container, 10);
			fltrq_match.l_quantity = task->fltrq_lineitem_tmp[jdx].l_quantity;
			memcpy(fltrq_match.l_shipinstruct, task->fltrq_lineitem_tmp[jdx].l_shipinstruct, 25);
			memcpy(fltrq_match.l_shipmode, task->fltrq_lineitem_tmp[jdx].l_shipmode, 10);
			matched_flag = matched_flag & match_filter(&fltrq_match);
			(join_arg->matched_cnt) += matched_flag;
			(join_arg->revenue) += task->fltrq_lineitem_tmp[jdx].l_extendedprice *
				( 1 - task->fltrq_lineitem_tmp[jdx].l_discount ) * matched_flag;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */
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


void tpch_run_rdx_join(void *arg, void (*f_join)(tpch_task_t *, join_arg_t *)) {
	tpch_arg_t *tpch_arg = (tpch_arg_t *) arg;
	join_arg_t join_arg;

	int rv;
	tpch_task_t *task;

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

	/**************** JOIN PHASE ****************/
	if (tpch_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &tpch_arg->probejoin_start);
	}

	if (tpch_arg->_tid < INTERMEDIATEMEMSET_THREAD_NUM) {
		size_t memset_size = (tpch_arg->_tid < INTERMEDIATEMEMSET_THREAD_NUM-1) ?
			tpch_arg->intermediate_size*PROBEJOIN_THREAD_NUM/INTERMEDIATEMEMSET_THREAD_NUM:
			tpch_arg->intermediate_size*PROBEJOIN_THREAD_NUM - tpch_arg->_tid*
			(tpch_arg->intermediate_size*PROBEJOIN_THREAD_NUM/INTERMEDIATEMEMSET_THREAD_NUM);

		memset_localize(tpch_arg->intermediate + tpch_arg->_tid*(tpch_arg->intermediate_size
			*PROBEJOIN_THREAD_NUM/INTERMEDIATEMEMSET_THREAD_NUM), memset_size);
	}

	/* wait for all threads finish their join tasks */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_intermediate_memset);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_intermediate_memset);
#endif /* SYNCSTATS */

	join_arg.intermediate = tpch_arg->intermediate + tpch_arg->_tid * tpch_arg->intermediate_size;

	while ( ( task = tpch_task_queue_get_atomic(tpch_arg->join_queue) ) ) {
		f_join(task, &join_arg);
		tpch_arg->matched_cnt += join_arg.matched_cnt;

#if QUERY == Q14
		tpch_arg->numerator += join_arg.numerator;
		tpch_arg->denominator += join_arg.denominator;
#elif QUERY == Q19
		tpch_arg->revenue += join_arg.revenue;
#else /* QUERY == Q14 */
#endif /* QUERY == Q14 */

		(tpch_arg->parts_processed) ++;
#ifdef PHJ_MBP
		tpch_arg->memset_time += join_arg.memset_time;
		tpch_arg->build_time += join_arg.build_time;
		tpch_arg->probe_time += join_arg.probe_time;
#endif /* PHJ_MBP */
	}

	/* wait for all threads finish their join tasks */
#ifdef SYNCSTATS
	SYNC_TIMER(tpch_arg->localtimer->sync_join);
#endif /* SYNCSTATS */
	BARRIER_ARRIVE(tpch_arg->barrier, rv);				
#ifdef SYNCSTATS
	SYNC_GLOBAL(tpch_arg->_tid, tpch_arg->globaltimer->sync_join);
#endif /* SYNCSTATS */

	if (tpch_arg->_tid == 0) {
		clock_gettime(CLOCK_REALTIME, &tpch_arg->probejoin_end);
	}

#ifdef USE_PMWATCH
	if (tpch_arg->_tid == 0) {
		pmwEnd(PMWatch_output_probejoin);
		pmwClear();
	}
#endif /* USE_PMWATCH */

#ifdef USE_PAPI
	PAPI_stop(eventset, tpch_arg->PAPI_values_probejoin);
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

}


void *tpch_run_rdx_bc_join(void *arg) {
	tpch_run_rdx_join(arg, tpch_bc_join);
	return NULL;
}

void *tpch_run_rdx_hr_join(void *arg) {
	tpch_run_rdx_join(arg, tpch_hr_join);
	return NULL;
}