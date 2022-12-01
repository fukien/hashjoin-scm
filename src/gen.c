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
void gen_dup_uniform(const size_t pk_record_num, const size_t, const char* subdirectory, const char* pk_flag, const char* fk_flag);
void gen_fk_zipfian(const size_t record_num, const key_t_ min_key, const key_t_ max_key, const double theta);
void gen_fk_zipfian_eth(const size_t record_num, const size_t pk_record_num, double theta);
void gen_fk_selectivity(const size_t pk_record_num, const size_t fk_record_num, const key_t_ min_key, const key_t_ max_key, const double selectivity);
void gen_density(const size_t record_num, const size_t min_key, const size_t max_key, const double density, const char* flag);
void gen_sparsity(const size_t record_num, const size_t min_key, const size_t max_key, const double sparsity, const char* flag);
void payload_gen_fk_from_pk(const void* pk_data, const size_t pk_record_num, const size_t fk_record_num, const char* subdirectory, const char* flag);
void payload_gen_pkfk_uniform(const size_t pk_record_num, const size_t fk_record_num, const char* subdirectory, const char* pk_flag, const char* fk_flag);


void generate_workload(int seed) {
#define NUM_ZIPF_THETA 5
#define NUM_SELECTIVITY 4
#define NUM_DENSITY 4
#define NUM_SPARSITY 10

#ifndef RUN_BILLION
#ifndef RUN_FNDLY
	double zipf_theta[NUM_ZIPF_THETA] = {0.050, 0.250, 0.500, 0.750, 0.900};
	double selectivity[NUM_SELECTIVITY] = {0.200, 0.400, 0.600, 0.800};
	double density[NUM_DENSITY] = {0.200, 0.400, 0.600, 0.800};
#endif /* RUN_FNDLY */
#endif /* RUN_BILLION */


#ifdef RUN_BILLION
	size_t workload_A_R_num = 1 * pow(2, 30);
	size_t workload_ABC_S_num = 16 * pow(2, 30);
#elif RUN_FNDLY
	// /* L */
	// size_t workload_A_R_num = 512 * pow(2, 20);
	// size_t workload_ABC_S_num = 16384 * pow(2, 20);

	/* R */
	size_t workload_A_R_num = 128 * pow(2, 20);
	size_t workload_ABC_S_num = 1024 * pow(2, 20);
#else /* RUN_BILLION */
	size_t workload_A_R_num = 16 * pow(2, 20);
	size_t workload_B_R_num = 64 * pow(2, 20);
	size_t workload_C_R_num = 256 * pow(2, 20);
	size_t workload_ABC_S_num = 256 * pow(2, 20);
#ifdef RUN_MINI
	workload_A_R_num = 1 * pow(2, 10);
	workload_B_R_num = 4 * pow(2, 10);
	workload_C_R_num = 16 * pow(2, 10);
	workload_ABC_S_num = 16 * pow(2, 10);
#endif /* RUN_MINI */
#endif /* RUN_BILLION */

	srand(seed);
	gen_pkfk_uniform(workload_A_R_num, workload_ABC_S_num, "uniform", "AR", "AS");
#ifndef RUN_FNDLY
	srand(seed);
	gen_dup_uniform(workload_A_R_num, workload_ABC_S_num, "uniform", "DR", "DS");
#endif /* RUN_FNDLY */

#ifndef RUN_BILLION
#ifndef RUN_FNDLY
	srand(seed);
	gen_pkfk_uniform(workload_B_R_num, workload_ABC_S_num, "uniform", "BR", "BS");
	srand(seed);
	gen_pkfk_uniform(workload_C_R_num, workload_ABC_S_num, "uniform", "CR", "CS");
	
	srand(seed);
	gen_dup_uniform(workload_B_R_num, workload_ABC_S_num, "uniform", "ER", "ES");
	srand(seed);
	gen_dup_uniform(workload_C_R_num, workload_ABC_S_num, "uniform", "FR", "FS");

	for (size_t idx = 0; idx < NUM_ZIPF_THETA; idx ++) {
		srand(seed);
		// gen_fk_zipfian(workload_ABC_S_num, 1, workload_A_R_num, zipf_theta[idx]);
		gen_fk_zipfian_eth(workload_ABC_S_num, workload_A_R_num, zipf_theta[idx]);
	}

	for (size_t idx = 0; idx < NUM_SELECTIVITY; idx ++) {
		srand(seed);
		gen_fk_selectivity(workload_A_R_num, workload_ABC_S_num, 1, workload_A_R_num, selectivity[idx]);
	}

	for (size_t idx = 0; idx < NUM_DENSITY; idx ++) {
		srand(seed);
		gen_density(workload_A_R_num, 1, workload_A_R_num, density[idx], "AR");
		gen_density(workload_ABC_S_num, 1, workload_A_R_num, density[idx], "AS");
	}

	// for (size_t idx = 0; idx < NUM_SPARSITY; idx ++) {
	// 	srand(seed);
	// 	gen_sparsity(workload_A_R_num, 1, workload_A_R_num, (double) 1 / ( 2 * ( idx + 1 ) ), "AR");
	// 	gen_sparsity(workload_ABC_S_num, 1, workload_A_R_num, (double) 1 / ( 2 * ( idx + 1 ) ), "AS");
	// }
#endif /* RUN_FNDLY */
#endif /* RUN_BILLION */
}


void generate_payload(int seed) {
	srand(seed);
	size_t tmp_len = 10;
	char pk_flag[tmp_len];
	char fk_flag[tmp_len];
	snprintf(pk_flag, tmp_len, "%zu_R", (size_t) TUPLE_T_SIZE);
	snprintf(fk_flag, tmp_len, "%zu_S", (size_t) TUPLE_T_SIZE);
	size_t workload_R_num = 16 * pow(2, 20);
	size_t workload_S_num = 256 * pow(2, 20);
	payload_gen_pkfk_uniform(workload_R_num, workload_S_num, "payload", pk_flag, fk_flag);
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

#ifdef RUN_PAYLOAD
	generate_payload(seed);
#else /* RUN_PAYLOAD */
	generate_workload(seed);
#endif /* RUN_PAYLOAD */

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


void payload_gen_fk_from_pk(const void* pk_data, const size_t pk_record_num, const size_t fk_record_num, const char* subdirectory, const char* flag) {
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

	for (size_t idx = 0; idx < fk_record_num; idx ++) {
		( (tuple_t *) (dram_data + idx * TUPLE_T_SIZE) )->row_id = idx + 1;
	}

	void* nvm_data = create_pmem_buffer(path, fk_record_num * TUPLE_T_SIZE);

	pmem_memcpy_persist(nvm_data, dram_data, fk_record_num * TUPLE_T_SIZE);

	free(dram_data);
	pmem_unmap(nvm_data, fk_record_num * TUPLE_T_SIZE);
}


void payload_gen_pkfk_uniform(const size_t pk_record_num, const size_t fk_record_num, const char* subdirectory, const char* pk_flag, const char* fk_flag) {
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
	
	payload_gen_fk_from_pk(dram_data, pk_record_num, fk_record_num, subdirectory, fk_flag);

	for (size_t idx = 0; idx < pk_record_num; idx ++) {
		( (tuple_t *) (dram_data + idx * TUPLE_T_SIZE) )->row_id = idx + 1;
	}

	pmem_memcpy_persist(nvm_data, dram_data, pk_record_num * TUPLE_T_SIZE);
	free(dram_data);
	pmem_unmap(nvm_data, pk_record_num * TUPLE_T_SIZE);
}


void gen_dup_uniform(const size_t pk_record_num, const size_t fk_record_num, const char* subdirectory, const char* pk_flag, const char* fk_flag) {
	void* dram_data = malloc(pk_record_num * TUPLE_T_SIZE);
	tuple_t tuple;

	/* see https://people.sc.fsu.edu/~jburkardt/c_src/uniform/uniform.c */
	const int i4_huge = 2147483647;
	int k;
	double r;
	int uni_seed = rand() % RAND_MAX;

	const key_t_ min_key = 1;
	const key_t_ max_key = pk_record_num;

	for (size_t idx = 0; idx < pk_record_num; idx ++) {
		tuple.row_id = idx + 1;
		k = uni_seed / 127773;
		uni_seed = 16807 * ( uni_seed - k * 127773 ) - k * 2836;
		if (uni_seed < 0) {
			uni_seed = uni_seed + i4_huge;
		}
		r = (double) (uni_seed) * 4.656612875E-10;
		r = ( 1.0 - r ) * ( (double) (min_key) - 0.5 ) + r * ( (double)(max_key) + 0.5);
		tuple.key = (int) round(r);
		if (tuple.key < min_key) {
			tuple.key = min_key;
		}
		if (tuple.key > max_key) {
			tuple.key = max_key;
		}
		memcpy(dram_data + idx * TUPLE_T_SIZE, &tuple, TUPLE_T_SIZE);
	}
	knuth_shuffle(dram_data, pk_record_num);

	char path[CHAR_BUFFER_LEN];
	snprintf(path, CHAR_BUFFER_LEN, "%s/%s", DATA_PATH_PREFIX, subdirectory);
	mkdir(path);
	snprintf(path, CHAR_BUFFER_LEN, "%s/%s/%s_%s.bin", DATA_PATH_PREFIX, subdirectory, "dup", pk_flag);
	void* nvm_data = create_pmem_buffer(path, pk_record_num * TUPLE_T_SIZE);
	
	pmem_memcpy_persist(nvm_data, dram_data, pk_record_num * TUPLE_T_SIZE);

	gen_fk_from_pk(dram_data, pk_record_num, fk_record_num, subdirectory, fk_flag);

	free(dram_data);
	pmem_unmap(nvm_data, pk_record_num * TUPLE_T_SIZE);
}


void gen_fk_zipfian(const size_t record_num, const key_t_ min_key, const key_t_ max_key, const double theta) {
	void* dram_data = malloc(record_num * TUPLE_T_SIZE);

	tuple_t tuple;

	const double alpha = 1 / ( 1 - theta );	
	double zetan = zeta_func(max_key - min_key + 1, theta);
	double eta = eta_func(theta, zetan, min_key, max_key);

	double u = 0.0;
	double uz = 0.0;

	for (size_t idx = 0; idx < record_num; idx ++) {

		u = ( (double) rand() ) / RAND_MAX;
		uz = u * zetan;

		if (uz < 1) {
			tuple.key = min_key;
		} else if (uz < 1 + pow(0.5, theta)) {
			tuple.key = min_key + 1;
		} else {
			tuple.key = min_key + (key_t_) ( (max_key - min_key + 1) * pow( eta * u - eta + 1, alpha ) );
		}
		tuple.row_id = idx + 1;

		memcpy(dram_data + idx * TUPLE_T_SIZE, &tuple, TUPLE_T_SIZE);
	}

	char path[CHAR_BUFFER_LEN];
	snprintf(path, CHAR_BUFFER_LEN, "%s/zipfian", DATA_PATH_PREFIX);
	mkdir(path);
	snprintf(path, CHAR_BUFFER_LEN, "%s/zipfian/%s_%.3f_AS.bin", DATA_PATH_PREFIX, "fk", 1 + theta);
	void* nvm_data = create_pmem_buffer(path, record_num * TUPLE_T_SIZE);
	pmem_memcpy_persist(nvm_data, dram_data, record_num * TUPLE_T_SIZE);
	
	free(dram_data);
	pmem_unmap(nvm_data, record_num * TUPLE_T_SIZE);
}


void gen_fk_zipfian_eth(const size_t record_num, const size_t pk_record_num, double theta) {
	void* dram_data = malloc(record_num * TUPLE_T_SIZE);

	/* make the skewness to be 1+theta so that it's consistent with previous works */
	theta = 1 + theta;
	gen_zipf(record_num, pk_record_num, theta, (tuple_t**) &dram_data);

	char path[CHAR_BUFFER_LEN];
	snprintf(path, CHAR_BUFFER_LEN, "%s/zipfian", DATA_PATH_PREFIX);
	mkdir(path);
	snprintf(path, CHAR_BUFFER_LEN, "%s/zipfian/%s_%.3f_AS.bin", DATA_PATH_PREFIX, "fk", theta);
	void* nvm_data = create_pmem_buffer(path, record_num * TUPLE_T_SIZE);
	pmem_memcpy_persist(nvm_data, dram_data, record_num * TUPLE_T_SIZE);
	
	free(dram_data);
	pmem_unmap(nvm_data, record_num * TUPLE_T_SIZE);
}


void gen_fk_selectivity(const size_t pk_record_num, const size_t fk_record_num, const key_t_ min_key, const key_t_ max_key, const double selectivity) {
	void* dram_data = malloc(fk_record_num * TUPLE_T_SIZE);

	tuple_t tuple;

	key_t_ floor = min_key;
	key_t_ ceiling = (key_t_) ( min_key + (max_key - min_key + 1) * selectivity );
	key_t_ upstairs = max_key + 1;

	key_t_ cur;

	for (size_t idx = 0; idx < (double) fk_record_num/pk_record_num; idx ++) {
		cur = floor;
		for (size_t jdx = 0; jdx < pk_record_num; jdx ++) {
			if (cur == ceiling) {
				cur = upstairs;
			}
			tuple.key = cur;
			tuple.row_id = idx + 1;
			cur ++;
			memcpy(dram_data + (jdx + idx * pk_record_num) * TUPLE_T_SIZE, &tuple, TUPLE_T_SIZE);
		}
	}
	knuth_shuffle(dram_data, fk_record_num);

	char path[CHAR_BUFFER_LEN];
	snprintf(path, CHAR_BUFFER_LEN, "%s/selectivity", DATA_PATH_PREFIX);
	mkdir(path);
	snprintf(path, CHAR_BUFFER_LEN, "%s/selectivity/%s_%.3f_AS.bin", DATA_PATH_PREFIX, "fk", selectivity);
	void* nvm_data = create_pmem_buffer(path, fk_record_num * TUPLE_T_SIZE);

	pmem_memcpy_persist(nvm_data, dram_data, fk_record_num * TUPLE_T_SIZE);

	free(dram_data);
	pmem_unmap(nvm_data, fk_record_num * TUPLE_T_SIZE);
}


void gen_density(const size_t record_num, const size_t min_key, const size_t max_key, const double density, const char* flag) {
	void * dram_data = malloc(record_num * TUPLE_T_SIZE);

	tuple_t tuple;

	/* density = new_range / old_range */
	size_t range = (size_t) (max_key - min_key + 1) * density;
	// key_t_ floor =  (size_t) ( (double) ( (min_key + max_key) - range ) / 2 );
	// key_t_ ceiling = (size_t) ( (double) ( (min_key + max_key) + range ) / 2 );
	key_t_ floor = min_key;
	key_t_ ceiling = min_key + range;

	key_t_ cur = floor;

	for (size_t idx = 0; idx < record_num; idx ++) {
		if (cur == ceiling ) {
			cur = floor;
		}
		tuple.key = cur;
		cur ++;
		tuple.row_id = idx + 1;
		memcpy(dram_data + idx * TUPLE_T_SIZE, &tuple, TUPLE_T_SIZE);
	}
	knuth_shuffle(dram_data, record_num);

	char path[CHAR_BUFFER_LEN];
	snprintf(path, CHAR_BUFFER_LEN, "%s/density", DATA_PATH_PREFIX);
	mkdir(path);
	snprintf(path, CHAR_BUFFER_LEN, "%s/density/%.3f_%s.bin", DATA_PATH_PREFIX, density, flag);
	void* nvm_data = create_pmem_buffer(path, record_num * TUPLE_T_SIZE);

	pmem_memcpy_persist(nvm_data, dram_data, record_num * TUPLE_T_SIZE);

	free(dram_data);
	pmem_unmap(nvm_data, record_num * TUPLE_T_SIZE);
}


void gen_sparsity(const size_t record_num, const size_t min_key, const size_t max_key, const double sparsity, const char* flag) {
	void * dram_data = malloc(record_num * TUPLE_T_SIZE);

	tuple_t tuple;

	/* sparsity = old_range / new_range */
	size_t range = (size_t) (max_key - min_key + 1) / sparsity;
	size_t stride = (size_t) 1 / sparsity;

	key_t_ cur = min_key;
	key_t_ ceiling = min_key + range;

	assert( (ceiling-min_key)%stride == 0 );

	for (size_t idx = 0; idx < record_num; idx ++) {
		tuple.key = cur;
		cur += stride;
		if (cur == ceiling) {
			cur = min_key;
		}
		tuple.row_id = idx + 1;
		memcpy(dram_data + idx * TUPLE_T_SIZE, &tuple, TUPLE_T_SIZE);
	}
	knuth_shuffle(dram_data, record_num);

	char path[CHAR_BUFFER_LEN];
	snprintf(path, CHAR_BUFFER_LEN, "%s/sparsity", DATA_PATH_PREFIX);
	mkdir(path);
	snprintf(path, CHAR_BUFFER_LEN, "%s/sparsity/%.3f_%s.bin", DATA_PATH_PREFIX, sparsity, flag);
	void *nvm_data = create_pmem_buffer(path, record_num * TUPLE_T_SIZE);

	pmem_memcpy_persist(nvm_data, dram_data, record_num * TUPLE_T_SIZE);

	free(dram_data);
	pmem_unmap(nvm_data, record_num * TUPLE_T_SIZE);
}