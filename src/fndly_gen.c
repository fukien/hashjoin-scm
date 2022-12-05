#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libpmem.h>

#include "inc/rng.h"
#include "inc/store.h"
#include "inc/utils.h"

#include "inc/eth_genzipf.h"

void knuth_shuffle(void* const dram_data, const size_t record_num);
void gen_fk_from_pk(const void* pk_data, const size_t pk_record_num, const size_t fk_record_num, const char* subdirectory, const char* flag);
void gen_pkfk_uniform(const size_t pk_record_num, const size_t fk_record_num, const char* subdirectory, const char* pk_flag, const char* fk_flag);


void generate_workload(int seed) {
#ifdef RUN_FNDLY
	size_t workload_A_R_num = FNDLY_R_CARDINALITY * pow(2, 20);
	size_t workload_ABC_S_num = FNDLY_S_CARDINALITY * pow(2, 20);
	srand(seed);
	gen_pkfk_uniform(workload_A_R_num, workload_ABC_S_num, "uniform", "AR", "AS");
#endif /* RUN_FNDLY */
}


int main(int argc, char** argv) {

	mkdir(DATA_PATH_PREFIX);

	/* Start measuring time */
	struct timespec begin, end; 
	clock_gettime(CLOCK_REALTIME, &begin);

	/* generate the seed */
	// srand((unsigned) time(NULL));
	// int seed = rand();
	int seed = 12345;

	generate_workload(seed);

	/* Stop measuring time and calculate the elapsed time */
	clock_gettime(CLOCK_REALTIME, &end);
	double elapsed = diff_sec(begin, end);
	printf("Time elapsed: %.9f seconds.\n", elapsed);

	return 0;
}


void knuth_shuffle(void* const dram_data, const size_t record_num) {
	tuple_t tuple;
	size_t jdx;
	
	for (size_t idx = 0; idx < record_num; idx ++) {
		jdx = rand() % record_num;
		memcpy(&tuple, dram_data + jdx * TUPLE_T_SIZE, TUPLE_T_SIZE);
		memcpy(dram_data + jdx * TUPLE_T_SIZE, dram_data + idx * TUPLE_T_SIZE, TUPLE_T_SIZE);
		memcpy(dram_data + idx * TUPLE_T_SIZE, &tuple, TUPLE_T_SIZE);
	}
}


void gen_fk_from_pk(const void* pk_data, const size_t pk_record_num, const size_t fk_record_num, const char* subdirectory, const char* flag) {
	void* dram_data = malloc(fk_record_num * TUPLE_T_SIZE);
	size_t memcpy_size = pow(2, 12);

	for (size_t idx = 0; idx < (double) fk_record_num / pk_record_num; idx ++) {
		for (size_t jdx = 0; jdx < pk_record_num * TUPLE_T_SIZE / memcpy_size; jdx ++) {
			memcpy(dram_data + idx * pk_record_num * TUPLE_T_SIZE + jdx * memcpy_size, pk_data + jdx * memcpy_size, memcpy_size);
		}
	}

	knuth_shuffle(dram_data, fk_record_num);

	char path[CHAR_BUFFER_LEN];
	snprintf(path, CHAR_BUFFER_LEN, "%s/%s",  DATA_PATH_PREFIX, subdirectory);
	mkdir(path);
	snprintf(path, CHAR_BUFFER_LEN, "%s/%s/%s_%s.bin", DATA_PATH_PREFIX, subdirectory, "fk", flag);
	void* nvm_data = create_pmem_buffer(path, fk_record_num * TUPLE_T_SIZE);

	pmem_memcpy_persist(nvm_data, dram_data, fk_record_num * TUPLE_T_SIZE);

	free(dram_data);
	pmem_unmap(nvm_data, fk_record_num * TUPLE_T_SIZE);
}


void gen_pkfk_uniform(const size_t pk_record_num, const size_t fk_record_num, const char* subdirectory, const char* pk_flag, const char* fk_flag) {
	void* dram_data = malloc(pk_record_num * TUPLE_T_SIZE);
	tuple_t tuple;
	
	for (size_t idx = 0; idx < pk_record_num; idx ++) {
		tuple.key = idx + 1;
		tuple.row_id = idx + 1;
		memcpy(dram_data + idx * TUPLE_T_SIZE, &tuple, TUPLE_T_SIZE);
	}

	knuth_shuffle(dram_data, pk_record_num);

	char path[CHAR_BUFFER_LEN];
	snprintf(path, CHAR_BUFFER_LEN, "%s/%s", DATA_PATH_PREFIX, subdirectory);
	mkdir(path);
	snprintf(path, CHAR_BUFFER_LEN, "%s/%s/%s_%s.bin", DATA_PATH_PREFIX, subdirectory, "pk", pk_flag);
	void* nvm_data = create_pmem_buffer(path, pk_record_num * TUPLE_T_SIZE);
	
	pmem_memcpy_persist(nvm_data, dram_data, pk_record_num * TUPLE_T_SIZE);
	
	gen_fk_from_pk(dram_data, pk_record_num, fk_record_num, subdirectory, fk_flag);

	free(dram_data);
	pmem_unmap(nvm_data, pk_record_num * TUPLE_T_SIZE);
}