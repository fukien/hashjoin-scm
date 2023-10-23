#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "algo/eth.h"
#include "algo/nphj.h"
#include "algo/phj.h"
#if NUM_PASSES == 2
#include "algo/phj_asymm_radix.h"
#endif /* NUM_PASSES == 2 */
#ifdef RUN_PAYLOAD
#include "algo/ptr_nphj.h"
#include "algo/ptr_phj.h"
#endif /* RUN_PAYLOAD */
#include "algo/phj_ind.h"
#include "algo/phj_shr.h"
#include "algo/phj_ind_uni.h"
#include "algo/phj_shr_uni.h"
#include "inc/utils.h"

#ifdef BW_REG
#include "algo/phj_bw_reg.h"
#if NUM_PASSES == 2
#include "algo/phj_asymm_radix_bw_reg.h"
#endif /* NUM_PASSES == 2 */
#ifdef RUN_PAYLOAD
#include "algo/ptr_phj_bw_reg.h"
#endif /* RUN_PAYLOAD */
#endif /* BW_REG */


#ifdef USE_NVM
#define MEM_TYPE "nvm"
#else /* USE_NVM */
#define MEM_TYPE "dram"
#endif /* USE_NVM */


char cfg_path[CHAR_BUFFER_LEN];
char dump_dir[CHAR_BUFFER_LEN];
extern char work_dir[CHAR_BUFFER_LEN];
extern int local_cores[SINGLE_SOCKET_CORE_NUM];
extern int remote_cores[SINGLE_SOCKET_CORE_NUM];

#ifdef USE_PAPI
#include "papi/mypapi.h"
extern char* PAPI_event_names[NUM_PAPI_EVENT];
long long agg_PAPI_values[NUM_PAPI_EVENT];
long long agg_PAPI_values_buildpart[NUM_PAPI_EVENT];
long long agg_PAPI_values_probejoin[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
#include "pmw/mypmw.h"
extern unsigned int NVMCount;
MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output[MaxNVMNum];
MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_buildpart[MaxNVMNum];
MY_PMWATCH_OP_BUF_NODE agg_PMWatch_output_probejoin[MaxNVMNum];
#endif /* USE_PMWATCH */


typedef struct algo_t {
	char name[128];
	void (*joinalgo)(const datameta_t, timekeeper_t* const);
} algo_t;


algo_t algos [] = {
	{"nphj_sc", nphj_sc},
	{"nphj_lp", nphj_lp},
	{"phj_radix_bc", phj_radix_bc},
	{"phj_radix_sc", phj_radix_sc},
	{"phj_radix_lp", phj_radix_lp},
	{"phj_radix_hr", phj_radix_hr},
#if NUM_PASSES == 2
	{"phj_asymm_radix_bc", phj_asymm_radix_bc},
	{"phj_asymm_radix_sc", phj_asymm_radix_sc},
	{"phj_asymm_radix_hr", phj_asymm_radix_hr},
#endif /* NUM_PASSES == 2 */
	{"eth_pro", eth_pro},
	{"eth_prh", eth_prh},
#ifdef RUN_PAYLOAD
	{"ptr_nphj_sc", ptr_nphj_sc},
	{"ptr_nphj_lp", ptr_nphj_lp},
	{"ptr_phj_radix_bc", ptr_phj_radix_bc},
	{"ptr_phj_radix_sc", ptr_phj_radix_sc},
	{"ptr_phj_radix_lp", ptr_phj_radix_lp},
	{"ptr_phj_radix_hr", ptr_phj_radix_hr},
#endif /* RUN_PAYLOAD */
	{"phj_ind_sc", phj_ind_sc},
	{"phj_ind_lp", phj_ind_lp},
	{"phj_ind_hr", phj_ind_hr},
	{"phj_shr_sc", phj_shr_sc},
	{"phj_shr_lp", phj_shr_lp},
	{"phj_shr_hr", phj_shr_hr},
	{"phj_ind_uni_bc", phj_ind_uni_bc},
	{"phj_ind_uni_sc", phj_ind_uni_sc},
	{"phj_ind_uni_lp", phj_ind_uni_lp},
	{"phj_ind_uni_hr", phj_ind_uni_hr},
	{"phj_shr_uni_bc", phj_shr_uni_bc},
	{"phj_shr_uni_sc", phj_shr_uni_sc},
	{"phj_shr_uni_lp", phj_shr_uni_lp},
	{"phj_shr_uni_hr", phj_shr_uni_hr},
#ifdef BW_REG
	{"phj_radix_bw_reg_bc", phj_radix_bw_reg_bc},
	{"phj_radix_bw_reg_lp", phj_radix_bw_reg_lp},
	{"phj_radix_bw_reg_sc", phj_radix_bw_reg_sc},
	{"phj_radix_bw_reg_hr", phj_radix_bw_reg_hr},
#if NUM_PASSES == 2
	{"phj_asymm_radix_bw_reg_bc", phj_asymm_radix_bw_reg_bc},
	{"phj_asymm_radix_bw_reg_sc", phj_asymm_radix_bw_reg_sc},
	{"phj_asymm_radix_bw_reg_hr", phj_asymm_radix_bw_reg_hr},
#endif /* NUM_PASSES == 2 */
#ifdef RUN_PAYLOAD
	{"ptr_phj_radix_bw_reg_bc", ptr_phj_radix_bw_reg_bc},
	{"ptr_phj_radix_bw_reg_sc", ptr_phj_radix_bw_reg_sc},
	{"ptr_phj_radix_bw_reg_lp", ptr_phj_radix_bw_reg_lp},
	{"ptr_phj_radix_bw_reg_hr", ptr_phj_radix_bw_reg_hr},
#endif /* RUN_PAYLOAD */
#endif /* BW_REG */
#if TUPLE_T_SIZE <= AVX512_SIZE
	{"phj_radix_hro", phj_radix_hro},
#if NUM_PASSES == 2
	{"phj_asymm_radix_hro", phj_asymm_radix_hro},
#endif /* NUM_PASSES == 2 */
	{"eth_prho", eth_prho},
	{"phj_ind_hro", phj_ind_hro},
	{"phj_shr_hro", phj_shr_hro},
	{"phj_ind_uni_hro", phj_ind_uni_hro},
	{"phj_shr_uni_hro", phj_shr_uni_hro},
#ifdef BW_REG
	{"phj_radix_bw_reg_hro", phj_radix_bw_reg_hro},
#if NUM_PASSES == 2
	{"phj_asymm_radix_bw_reg_hro", phj_asymm_radix_bw_reg_hro},
#endif /* NUM_PASSES == 2 */
#endif /* BW_REG */
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */
	// {"ptr_phj_radix_hro", ptr_phj_radix_hro},
};

int cur_t = 0;

int main(int argc, char** argv) {

#ifdef USE_PAPI
	memset(agg_PAPI_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(agg_PAPI_values_buildpart, 0, sizeof(long long) * NUM_PAPI_EVENT );
	memset(agg_PAPI_values_probejoin, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

#ifdef USE_PMWATCH
	memset(agg_PMWatch_output, 0, sizeof(MY_PMWATCH_OP_BUF_NODE) * MaxNVMNum );
	memset(agg_PMWatch_output_buildpart, 0, sizeof(MY_PMWATCH_OP_BUF_NODE) * MaxNVMNum );
	memset(agg_PMWatch_output_probejoin, 0, sizeof(MY_PMWATCH_OP_BUF_NODE) * MaxNVMNum );
#endif /* USE_PMWATCH */

	srand(time(NULL));
	// int seed = 32768;
	// srand(seed);

	/**************** CPU MAPPING INITIALIZATION ****************/
	mc_cfg(local_cores, remote_cores);

	timekeeper_t global_time_keeper = {0.0, 0.0, 0.0};

	/* main algo workload subtype param */
	assert( argc == 5);
	algo_t* algo = &algos[0];
	datameta_t datameta;
	char* workload;
	char* subtype;
	char* param;

	int command_arg_char;
	int i = 0;
	int found = 0;
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"algo",		required_argument,	NULL,	0},
			{"workload",	required_argument,	NULL,	0},
			{"subtype",		required_argument, 	NULL,	0},
			{"param",		required_argument, 	NULL,	0},
			{NULL,			0,					NULL,	0}
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
						while (algos[i].joinalgo) {
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
						workload = optarg;
						break;
					case 2:
						subtype = optarg;
						break;
					case 3:
						param = optarg;
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

#ifdef RUN_BILLION
		snprintf(cfg_path, CHAR_BUFFER_LEN, "%s/billion-data/%s/%s_%s.cfg", DATA_CFG_PATH_PREFIX, workload, param, subtype);
#elif RUN_FNDLY
		snprintf(cfg_path, CHAR_BUFFER_LEN, "%s/fndly-data/%s/%s_%s.cfg", DATA_CFG_PATH_PREFIX, workload, param, subtype);
#else /* RUN_BILLION */
		snprintf(cfg_path, CHAR_BUFFER_LEN, "%s/data/%s/%s_%s.cfg", DATA_CFG_PATH_PREFIX, workload, param, subtype);
#endif /* RUN_BILLION */

	snprintf(work_dir, CHAR_BUFFER_LEN, "%s/%s_%s_%s_%s", WORK_PATH_PREFIX, workload, param, subtype, current_timestamp());
	snprintf(dump_dir, CHAR_BUFFER_LEN, "%s/%s_%s_%s_%s", DUMP_PATH_PREFIX, workload, param, subtype, current_timestamp());
	assert(file_exists(cfg_path));
	data_cfg(&datameta, cfg_path);

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
#ifdef UN_PREPOPULATE
	printf("\tUN_PREPOPULATE");
#endif /* UN_PREPOPULATE */
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
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART
	printf("\tPTR_USE_SWWCB_OPTIMIZED_PART");
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART */
#ifdef PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM
	printf("\tPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM");
#endif /* PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM */
#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
	printf("\tPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER");
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */
#ifdef PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM
	printf("\tPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM");
#endif /* PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM */
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

	for (cur_t = 0; cur_t < TRIAL_NUM; cur_t ++) {
		mkdir(work_dir);
		mkdir(dump_dir);
		timekeeper_t tmp_time_keeper = {0.0, 0.0, 0.0};
		(*(algo->joinalgo)) (datameta, &tmp_time_keeper);
		if (cur_t != 0) {
			global_time_keeper.total += tmp_time_keeper.total;
			global_time_keeper.buildpart += tmp_time_keeper.buildpart;
			global_time_keeper.probejoin += tmp_time_keeper.probejoin;
		}
		cyan();
		printf("memory_type: %s\t", MEM_TYPE);
		printf("algo: %s\t",  algo->name);
		printf("workload: %s\t", datameta.workload_name);
		printf("buildpart_time: %.9f\t", tmp_time_keeper.buildpart);
		printf("probejoin_time: %.9f\t", tmp_time_keeper.probejoin);
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
	printf("workload: %s\t", datameta.workload_name);
	printf("buildpart_time: %.9f\t", global_time_keeper.buildpart/(TRIAL_NUM-1));
	printf("probejoin_time: %.9f\t", global_time_keeper.probejoin/(TRIAL_NUM-1));
	printf("total_time: %.9f\n\n", global_time_keeper.total/(TRIAL_NUM-1));

#ifdef USE_PAPI
	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		agg_PAPI_values[idx] /= (TRIAL_NUM-1);
		agg_PAPI_values_buildpart[idx] /= (TRIAL_NUM-1);
		agg_PAPI_values_probejoin[idx] /= (TRIAL_NUM-1);
		printf("[agg_PAPI] PAPI_event_name: %s", PAPI_event_names[idx]);
		printf("\ttotal_counter: %lld", agg_PAPI_values[idx]);
		printf("\tbuildpart_counter: %lld", agg_PAPI_values_buildpart[idx]);
		printf("\tprobejoin_counter: %lld", agg_PAPI_values_probejoin[idx]);
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

		printf("[agg_PMWatch] DIMM: %d\n", idx);
		printf("[agg_PMWatch] DIMM: %d\ttotal_bytes_read: %zu\t%zu\t%zu\n", idx, agg_PMWatch_output[idx].total_bytes_read, 
			agg_PMWatch_output_buildpart[idx].total_bytes_read, agg_PMWatch_output_probejoin[idx].total_bytes_read);
		printf("[agg_PMWatch] DIMM: %d\ttotal_bytes_written: %zu\t%zu\t%zu\n", idx, agg_PMWatch_output[idx].total_bytes_written,
			agg_PMWatch_output_buildpart[idx].total_bytes_written, agg_PMWatch_output_probejoin[idx].total_bytes_written);
		printf("[agg_PMWatch] DIMM: %d\tmedia_read_ops: %zu\t%zu\t%zu\n", idx, agg_PMWatch_output[idx].media_read_ops,
			agg_PMWatch_output_buildpart[idx].media_read_ops, agg_PMWatch_output_probejoin[idx].media_read_ops);
		printf("[agg_PMWatch] DIMM: %d\tmedia_write_ops: %zu\t%zu\t%zu\n", idx, agg_PMWatch_output[idx].media_write_ops,
			agg_PMWatch_output_buildpart[idx].media_write_ops, agg_PMWatch_output_probejoin[idx].media_write_ops);
		printf("[agg_PMWatch] DIMM: %d\tread_64B_ops_received: %zu\t%zu\t%zu\n", idx, agg_PMWatch_output[idx].read_64B_ops_received,
			agg_PMWatch_output_buildpart[idx].read_64B_ops_received, agg_PMWatch_output_probejoin[idx].read_64B_ops_received);
		printf("[agg_PMWatch] DIMM: %d\twrite_64B_ops_received: %zu\t%zu\t%zu\n", idx, agg_PMWatch_output[idx].write_64B_ops_received,
			agg_PMWatch_output_buildpart[idx].write_64B_ops_received, agg_PMWatch_output_probejoin[idx].write_64B_ops_received);
		printf("[agg_PMWatch] DIMM: %d\thost_reads: %zu\t%zu\t%zu\n", idx, agg_PMWatch_output[idx].host_reads,
			agg_PMWatch_output_buildpart[idx].host_reads, agg_PMWatch_output_probejoin[idx].host_reads);
		printf("[agg_PMWatch] DIMM: %d\thost_writes: %zu\t%zu\t%zu\n", idx, agg_PMWatch_output[idx].host_writes,
			agg_PMWatch_output_buildpart[idx].host_writes, agg_PMWatch_output_probejoin[idx].host_writes);
		printf("[agg_PMWatch] DIMM: %d\tread_hit_ratio: %f\t%f\t%f\n", idx, agg_PMWatch_output[idx].read_hit_ratio,
			agg_PMWatch_output_buildpart[idx].read_hit_ratio, agg_PMWatch_output_probejoin[idx].read_hit_ratio);
		printf("[agg_PMWatch] DIMM: %d\twrite_hit_ratio: %f\t%f\t%f\n", idx, agg_PMWatch_output[idx].write_hit_ratio,
			agg_PMWatch_output_buildpart[idx].write_hit_ratio, agg_PMWatch_output_probejoin[idx].write_hit_ratio);
		printf("[agg_PMWatch] DIMM: %d\tread_amplification: %.9f\t%.9f\t%.9f\n", idx, agg_PMWatch_output[idx].read_amplification,
			agg_PMWatch_output_buildpart[idx].read_amplification, agg_PMWatch_output_probejoin[idx].read_amplification);
		printf("[agg_PMWatch] DIMM: %d\twrite_amplification: %.9f\t%.9f\t%.9f\n", idx, agg_PMWatch_output[idx].write_amplification,
			agg_PMWatch_output_buildpart[idx].write_amplification, agg_PMWatch_output_probejoin[idx].write_amplification);
		printf("\n");
	}
#endif /* USE_PMWATCH */

#endif /* TRIAL_NUM != 1 */
	reset();
	printf("\n\n\n\n");


	return 0;
}