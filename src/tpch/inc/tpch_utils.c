#include "tpch_utils.h"
#include "../../inc/utils.h"

int sf;

size_t part_num;
size_t lineitem_num;

size_t fltrq_part_num;
size_t fltrq_lineitem_num;

size_t every_thread_part_num[PROBEJOIN_THREAD_NUM];
size_t every_thread_lineitem_num[PROBEJOIN_THREAD_NUM];
fltrq_part_t *part_start_addr;
fltrq_lineitem_t *lineitem_start_addr;
size_t initial_part_tuple_num_thr;
size_t initial_lineitem_tuple_num_thr;


void get_cardinality(int sf) {
	switch (sf) {
		case 1: 
			part_num = 200000;
			lineitem_num = 6001215;
			break;
		case 10:
			part_num = 2000000;
			lineitem_num = 59986052;
			break;
		case 100:
			part_num = 20000000;
			lineitem_num = 600037902;
			break;
		default:
			printf("unsupported scaling factor %d\n", sf);
			exit(1);
	}

	// part_num = 200000 * sf;
	// lineitem_num = 6000000 * sf;
}


bool part_filter(part_t *tuple) {
#if QUERY == Q19
	int brand = fast_atoi(tuple->p_brand + 6);

	return (
				(
					( brand == 12 ) &&
					( tuple->p_size >= 1 && tuple->p_size <= 5 ) &&
					(
						strncmp(tuple->p_container, "SM CASE", 10) == 0
						|| strncmp(tuple->p_container, "SM BOX", 10) == 0 
						|| strncmp(tuple->p_container, "SM PACK", 10) == 0 
						|| strncmp(tuple->p_container, "SM PKG", 10) == 0 
					)
				)
			||
				(
					( brand == 23 ) &&
					( tuple->p_size >= 1 && tuple->p_size <= 10 ) &&
					(
						strncmp(tuple->p_container, "MED BAG", 10) == 0
						|| strncmp(tuple->p_container, "MED BOX", 10) == 0 
						|| strncmp(tuple->p_container, "MED PKG", 10) == 0 
						|| strncmp(tuple->p_container, "MED PACK", 10) == 0
					)
				)
			||
				(
					( brand == 34 ) && 
					( tuple->p_size >= 1 && tuple->p_size <= 15 ) &&
					(
						strncmp(tuple->p_container, "LG CASE", 10) == 0
						|| strncmp(tuple->p_container, "LG BOX", 10) == 0 
						|| strncmp(tuple->p_container, "LG PACK", 10) == 0 
						|| strncmp(tuple->p_container, "LG PKG", 10) == 0
					)
				)

		); 
#else /* QUERY == Q19 */
	return true;
#endif /* QUERY == Q19 */
}


bool lineitem_filter(lineitem_t *tuple) {
#if QUERY == Q14
	return ( 
			( strncmp(tuple->l_shipdate, "1995-09-01", 8) >= 0 ) 
			&& ( strncmp(tuple->l_shipdate, "1995-10-01", 8) < 0 )
		);
#elif QUERY == Q19
	return (
					( strncmp(tuple->l_shipinstruct, "DELIVER IN PERSON", 25) == 0 ) 
				&&
					(
						strncmp(tuple->l_shipmode, "AIR", 10) == 0 
						|| strncmp(tuple->l_shipmode, "AIR REG", 10) == 0 
					)
				&& 	(
						( tuple->l_quantity >= 1 && tuple->l_quantity <= 11 ) 
						||	( tuple->l_quantity >= 10 && tuple->l_quantity <= 20 )
						||	( tuple->l_quantity >= 20 && tuple->l_quantity <= 30 )
					)
		);
#else /* QUERY == Q14 */
	return true;
#endif /* QUERY == Q14 */
}


#if QUERY == Q19
bool match_filter(fltrq_match_t *fltrq_match) {
		int brand = fast_atoi(fltrq_match->p_brand + 6);

	return (
					( strncmp(fltrq_match->l_shipinstruct, "DELIVER IN PERSON", 25) == 0 ) 
				&&
					(
						strncmp(fltrq_match->l_shipmode, "AIR", 10) == 0 
						|| strncmp(fltrq_match->l_shipmode, "AIR REG", 10) == 0 
					)
				&&
					(
							(
								( fltrq_match->l_quantity >= 1 && fltrq_match->l_quantity <= 11 ) &&
								( brand == 12 ) &&
								( fltrq_match->p_size >= 1 && fltrq_match->p_size <= 5 ) &&
								(
									strncmp(fltrq_match->p_container, "SM CASE", 10) == 0
									|| strncmp(fltrq_match->p_container, "SM BOX", 10) == 0 
									|| strncmp(fltrq_match->p_container, "SM PACK", 10) == 0 
									|| strncmp(fltrq_match->p_container, "SM PKG", 10) == 0 
								)
							)
						||
							(
								( fltrq_match->l_quantity >= 10 && fltrq_match->l_quantity <= 20 ) &&
								( brand == 23 ) &&
								( fltrq_match->p_size >= 1 && fltrq_match->p_size <= 10 ) &&
								(
									strncmp(fltrq_match->p_container, "MED BAG", 10) == 0
									|| strncmp(fltrq_match->p_container, "MED BOX", 10) == 0 
									|| strncmp(fltrq_match->p_container, "MED PKG", 10) == 0 
									|| strncmp(fltrq_match->p_container, "MED PACK", 10) == 0 
								)
							)
						||
							(
								( fltrq_match->l_quantity >= 20 && fltrq_match->l_quantity <= 30 ) &&
								( brand == 34 ) &&
								( fltrq_match->p_size >= 1 && fltrq_match->p_size <= 15 ) &&
								(
									strncmp(fltrq_match->p_container, "LG CASE", 10) == 0
									|| strncmp(fltrq_match->p_container, "LG BOX", 10) == 0 
									|| strncmp(fltrq_match->p_container, "LG PACK", 10) == 0 
									|| strncmp(fltrq_match->p_container, "LG PKG", 10) == 0 
								)
							)
					)
		);
}
#else /* QUERY == Q19 */
#endif /* QUERY == Q19 */


void tpch_query_meta_cfg(tpch_query_meta_t *tpch_query_meta) {
	tpch_query_meta->use_part = false;
	tpch_query_meta->use_supplier = false;
	tpch_query_meta->use_partsupp = false;
	tpch_query_meta->use_customer = false;
	tpch_query_meta->use_order = false;
	tpch_query_meta->use_lineitem = false;
	tpch_query_meta->use_nation = false;
	tpch_query_meta->use_region = false;

	tpch_query_meta->ff_part = 1;
	tpch_query_meta->ff_supplier = 1;
	tpch_query_meta->ff_partsupp = 1;
	tpch_query_meta->ff_customer = 1;
	tpch_query_meta->ff_order = 1;
	tpch_query_meta->ff_lineitem = 1;
	tpch_query_meta->ff_nation = 1;
	tpch_query_meta->ff_region = 1;

	switch (QUERY) {
		case Q14:
			tpch_query_meta->use_part = true;
			tpch_query_meta->use_lineitem = true;
			tpch_query_meta->ff_part = 1;
			tpch_query_meta->ff_lineitem = 1;
			break;
		case Q19:
			tpch_query_meta->use_part = true;
			tpch_query_meta->use_lineitem = true;
			tpch_query_meta->ff_part = 1;
			tpch_query_meta->ff_lineitem = 1;
			break;
		default:
			break;
	}
}


void tpch_org_map(bool use, char *path, void *org_addr, void **tmp_addr, 
	void **org_tup, size_t tup_num, size_t tup_t_size, size_t fltrq_t_size) {
	if (use) {
		org_addr = create_pmem_buffer(path, tup_t_size * tup_num);
#ifdef USE_NVM
		*tmp_addr = new_alloc_nvm(tup_t_size * tup_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
		*org_tup = new_alloc_nvm(fltrq_t_size * tup_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
#else /* USE_NVM */
#ifdef USE_HUGE
		*tmp_addr = alloc_huge_dram(tup_t_size * tup_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
		*org_tup = alloc_huge_dram(fltrq_t_size * tup_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)));
#else /* USE_HUGE */
		*tmp_addr = alloc_aligned_dram(tup_t_size * tup_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)), CACHELINE_SIZE );
		*org_tup = alloc_aligned_dram(fltrq_t_size * tup_num + PADDING_SIZE * ((size_t)(1 << NUM_RADIX_BITS)), CACHELINE_SIZE );
#endif /* USE_HUGE */
#endif /* USE_NVM */
		parallel_memcpy(*tmp_addr, org_addr, tup_num * tup_t_size);
		pmem_unmap(org_addr, tup_num * tup_t_size);
	}
}


void tpch_unmap(bool use, void *addr, size_t buffer_size) {
	if (use) {
		dealloc_memory(addr, buffer_size);
	}
}


void tpch_warmup_localize(bool use, void *addr, const size_t tuplpe_num, const size_t tuple_size) {
	if (use) {
		size_t sum = 0;
		void *tmp = malloc(tuple_size);
		for (size_t idx = 0; idx < tuplpe_num; idx ++) {
			memcpy(tmp, addr, tuple_size);
			sum += *( (tpch_key_t *) tmp);
		}
		free(tmp);
	}
}






