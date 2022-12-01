#include <assert.h>
#include <dirent.h>		// DIR, closedir, opendir
#include <errno.h>
#include <math.h>
#include <stdarg.h>		// va_list va_arg va_start va_end
#include <stdint.h>		// uintptr_t uin32_t SIZE_MAX
#include <stdio.h>
#include <stdlib.h>		// exit system
#include <sys/mman.h>	// PROT_READ PROT_WRITE MAP_PRIVATE MAP_ANONYMOUS MADV_HUGEPAGE
#include <string.h>
#include <time.h>		// timespec timespec_get gmtime strftime
#include <unistd.h>		// access

#define __USE_GNU
#include <sched.h>
#include <pthread.h>

#include <libconfig.h>
#include <libpmem.h>

#include "utils.h"


size_t file_idx = 0;
char work_dir[CHAR_BUFFER_LEN];
int local_cores[SINGLE_SOCKET_CORE_NUM];
int remote_cores[SINGLE_SOCKET_CORE_NUM];

void red() {
	printf("\033[1;31m");
}

void yellow() {
	printf("\033[1;33m");
}

void blue() {
	printf("\033[0;34m");
}

void green() {
	printf("\033[0;32m");
}

void cyan() {
	printf("\033[0;36m");
}

void purple() {
	printf("\033[0;35m");
}

void white() {
	printf("\033[0;37m");
}

void black() {
	printf("\033[0;30m");
}

void reset() {
	printf("\033[0m");
}

void touchfile(const char * filename) {
	char command[CHAR_BUFFER_LEN] = {'\0'};
	strcat(strcpy(command, "touch "), filename);
	int val = system(command);
	if (val != 0) {
		printf("\"%s\" failed, errno: %d\n", command, errno);
		perror(command);
	}
}

void mkdir(const char * dirpath) {
	char command[CHAR_BUFFER_LEN] = {'\0'};
	strcat(strcpy(command, "mkdir -p "), dirpath);
	int val = system(command);
	if (val != 0) {
		printf("\"%s\" failed, errno: %d\n", command, errno);
		perror(command);
	}
}

void deldir(const char * dirpath) {
	char command[CHAR_BUFFER_LEN] = {'\0'};
	strcat(strcpy(command, "rm -r "), dirpath);
	int val = system(command);
	if (val != 0) {
		printf("\"%s\" failed, errno: %d\n", command, errno);
		perror(command);
	}
}

void delfile(const char * filepath) {
	char command[CHAR_BUFFER_LEN] = {'\0'};
	strcat(strcpy(command, "rm "), filepath);
	int val = system(command);
	if (val != 0) {
		printf("\"%s\" failed, errno: %d\n", command, errno);
		perror(command);
	}
}

void renamefile(const char * oldname, const char * newname) {
	char command[CHAR_BUFFER_LEN] = {'\0'};
	strcat(strcat(strcat(strcpy(command, "mv "), oldname), " "), newname);
	int val = system(command);
	if (val != 0) {
		printf("\"%s\" failed, errno: %d\n", command, errno);
		perror(command);
	}
}

int file_exists(const char * filepath) {
	int val;
	int res = access(filepath, F_OK);
	if (res == 0) {
		val = 1;
	} else {
		val = 0;
		printf("%s access failed, errno: %d\n", filepath, errno);
		perror(filepath);
	}
	return val;
}

int dir_exists(const char * dirpath) {
	int val;
	DIR *dir = opendir(dirpath);
	if (dir) {
		val = 1;
		closedir(dir);
	} else {
		val = 0;
		printf("%s path not found, errno: %d\n", dirpath, errno);
		perror(dirpath);
	}
	return val;
}

size_t count_lines_num(const char * filepath) {
	size_t num = 0;
	FILE *fp = fopen(filepath, "r");
	char c;
	while (!feof(fp)) {
		c = fgetc(fp);
		if (c=='\n') {
			num++;
		}
	}
	fclose(fp);
	return num;
}

char * current_timestamp() {
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	char *buff1 = (char*) malloc( CHAR_BUFFER_LEN * sizeof(char) );
	char *buff2 = (char*) malloc( CHAR_BUFFER_LEN * sizeof(char) );	
	strftime(buff1, CHAR_BUFFER_LEN, "%F-%T", gmtime(&ts.tv_sec));
	snprintf(buff2, CHAR_BUFFER_LEN, "%s.%09ld", buff1, ts.tv_nsec);
	free(buff1);
	return buff2;
}

char * concat_str_by(const char * concat, const int count, ...) {
	size_t concat_len = strlen(concat);
	size_t malloc_len = concat_len * (count - 1);
	va_list argument;
	char * str_array[count];
	va_start(argument, count);
	for (int idx = 0; idx < count; idx ++) {
		str_array[idx] = va_arg(argument, char *);
		malloc_len += strlen(str_array[idx]);
	}
	va_end(argument);
	malloc_len ++; // add one more byte for the ending char '\0'
	char * concat_str = (char*) malloc( (malloc_len) * sizeof(char) );
	memset(concat_str, '\0', (malloc_len) * sizeof(char) );
	for (int idx = 0; idx < count - 1; idx ++) {
		strcat(strcat(concat_str, str_array[idx]), concat);
	}
	strcat(concat_str, str_array[count-1]);
	// printf("%zu\n", malloc_len);
	return concat_str;
}

char * substr(const char * src, int m, int n) {
	// get the length of the destination string
	int len = n - m;
 
	// allocate (len + 1) chars for destination (+1 for extra null character)
	char *dest = (char*) malloc( sizeof(char) * (len + 1) );
 	memset(dest, '\0',  sizeof(char) * (len + 1) );
	// extracts characters between m'th and n'th index from source string
	// and copy them into the destination string
	for (int i = m; i < n && (*(src + i) != '\0'); i++) {
		*dest = *(src + i);
		dest++;
	}
 
	// null-terminate the destination string
	*dest = '\0';
 
	// return the destination string
	return dest - len;
}

char * get_command_output_short(const char * command) {
	FILE *fp;
	fp = popen(command, "r");
	if (fp == NULL) {
		printf("Failed to run command: %s, errno: %d\n", command, errno);
		exit(1);
	}

	char path[CHAR_BUFFER_LEN] = {'\0'};	
	/* Read the output a line at a time - output it. */
	while (fgets(path, sizeof(path), fp) == NULL) {
		printf("get command \"%s\" output failed, errno: %d", command, errno);
	}

	/* close */
	pclose(fp);

	size_t malloc_len = strlen(path) + 1;	// +1 to accomodate for the null terminator
	char *res = (char*) malloc( (malloc_len) * sizeof(char) );
	memset(res, '\0', (malloc_len) * sizeof(char) );
	strcpy(res, path);

	return res;
}


void data_cfg(datameta_t * const datameta, const char * cfg_path) {
	const char *tmp_r_path_suffix;
	const char *tmp_s_path_suffix;
	
	long long tmp_r_tuple_num = 0;
	long long tmp_s_tuple_num = 0;
	long long tmp_r_tuple_size = 0;
	long long tmp_s_tuple_size = 0;
	long long tmp_min_key = 0;
	long long tmp_max_key = 0;

	const char *tmp_workload_name;

	config_t cfg;
	config_init(&cfg);
	if(! config_read_file(&cfg, cfg_path)) {
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
			config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		exit(1);
	}

	config_lookup_string(&cfg, "r_path_suffix", &tmp_r_path_suffix);
	config_lookup_string(&cfg, "s_path_suffix", &tmp_s_path_suffix);

	config_lookup_float(&cfg, "theta", &(datameta->theta));
	config_lookup_float(&cfg, "selectivity", &(datameta->selectivity));
	config_lookup_float(&cfg, "density", &(datameta->density));
	config_lookup_float(&cfg, "sparsity", &(datameta->sparsity));

	config_lookup_int64(&cfg, "r_tuple_num", &tmp_r_tuple_num);
	config_lookup_int64(&cfg, "s_tuple_num", &tmp_s_tuple_num);
	config_lookup_int64(&cfg, "r_tuple_size", &tmp_r_tuple_size);
	config_lookup_int64(&cfg, "s_tuple_size", &tmp_s_tuple_size);
	config_lookup_int64(&cfg, "min_key", &tmp_min_key);
	config_lookup_int64(&cfg, "max_key", &tmp_max_key);

	config_lookup_string(&cfg, "workload_name", &tmp_workload_name);

	datameta->r_tuple_num = tmp_r_tuple_num;
	datameta->s_tuple_num = tmp_s_tuple_num;
	datameta->r_tuple_size = tmp_r_tuple_size;
	datameta->s_tuple_size = tmp_s_tuple_size;
	datameta->min_key = tmp_min_key;
	datameta->max_key = tmp_max_key;

	datameta->r_path_suffix = (char*) malloc(strlen(tmp_r_path_suffix));
	datameta->s_path_suffix = (char*) malloc(strlen(tmp_s_path_suffix));
	datameta->workload_name = (char*) malloc(strlen(tmp_workload_name));
	strcpy(datameta->r_path_suffix, tmp_r_path_suffix);
	strcpy(datameta->s_path_suffix, tmp_s_path_suffix);
	strcpy(datameta->workload_name, tmp_workload_name);

	config_destroy(&cfg);
}


void mc_cfg(int *local_cores, int *remote_cores) {
	char cfg_path[CHAR_BUFFER_LEN];
	char* hostname = get_command_output_short("hostname");

	if (strncmp(hostname, "ocna1", strlen(hostname)-1) == 0) {
		snprintf(cfg_path, CHAR_BUFFER_LEN, "/work/huang/workspace/pm-join/config/mc/%s.cfg", substr( hostname, 0, strlen(hostname) - 1 ));
	} else {
		printf("[ERROR] Unknown hostname: %s\n", hostname);
		exit(EXIT_FAILURE);
	}

	config_t cfg;
	config_init(&cfg);

	if(! config_read_file(&cfg, cfg_path)) {
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
			config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		exit(1);
	}

	int length;
	config_setting_t* setting;

	setting = config_lookup(&cfg, "cores.local_core_id_arrays");
	length = config_setting_length(setting);
	if(setting != NULL) {
		for (int idx = 0; idx < length; idx ++) {
			local_cores[idx] = config_setting_get_int_elem(setting, idx);
		}
	}

	setting = config_lookup(&cfg, "cores.remote_core_id_arrays");
	length = config_setting_length(setting);
	if(setting != NULL) {
		for (int idx = 0; idx < length; idx ++) {
			remote_cores[idx] = config_setting_get_int_elem(setting, idx);
		}
	}

	config_destroy(&cfg);
}


int get_cpu_id(int thread_id) {
#ifdef USE_NUMA
	if (thread_id < SINGLE_SOCKET_CORE_NUM) {
		return remote_cores[thread_id % SINGLE_SOCKET_CORE_NUM];
	} else {
#ifdef USE_HYPERTHREADING
		return remote_cores[thread_id % SINGLE_SOCKET_CORE_NUM] + CORE_NUM; 
#else /* USE_HYPERTHREADING */
		return remote_cores[thread_id % SINGLE_SOCKET_CORE_NUM];
#endif  /* USE_HYPERTHREADING */
#else /* USE_NUMA */
	if (thread_id < SINGLE_SOCKET_CORE_NUM) {
		return local_cores[thread_id % SINGLE_SOCKET_CORE_NUM];
	} else {
#ifdef USE_HYPERTHREADING
		return local_cores[thread_id % SINGLE_SOCKET_CORE_NUM] + CORE_NUM; 
#else /* USE_HYPERTHREADING */
		return local_cores[thread_id % SINGLE_SOCKET_CORE_NUM];
#endif  /* USE_HYPERTHREADING */
#endif /* USE_NUMA */
	}
}


size_t next_pow_2(size_t x) {
	return pow(2, ceil(log(x)/log(2)));
}


size_t cnt_power(size_t N) {
	size_t res = 0;
	while (N) {
		N >>= 1;
		res ++;
	}
	return res - 1;
}


double diff_sec(const struct timespec start, const struct timespec end) {
	return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
}


size_t time_index_descending_qsort_partition(double * time, size_t lower, size_t upper, size_t * index) {
	size_t i = lower;
	double pivot = time[upper];
	size_t j;
	for (j = lower; j < upper; j ++) {

		if (time[j] > pivot) {
			double tmp = time[j];
			time[j] = time[i];
			time[i] = tmp;
			size_t idx = index[j];
			index[j] = index[i];
			index[i] = idx;
			i++;
		}
	}
	double tmp = time[upper];
	time[upper] = time[i];
	time[i] = tmp;
	size_t idx = index[upper];
	index[upper] = index[i];
	index[i] = idx;
	return i;
}


void time_index_descending_qsort(double * time, size_t lower, size_t upper, size_t * index) {
	if (upper > lower) {

		size_t partitionIndex = time_index_descending_qsort_partition(time, lower, upper, index);

		if ( partitionIndex > 0 ) {			
			time_index_descending_qsort(time, lower, partitionIndex - 1, index);
		} 
		if ( partitionIndex < SIZE_MAX ) {
			time_index_descending_qsort(time, partitionIndex + 1, upper, index);
		}

	}
}


size_t size_index_descending_qsort_partition(size_t * size, size_t lower, size_t upper, size_t * index) {
	size_t i = lower;
	size_t pivot = size[upper];
	size_t j;
	for (j = lower; j < upper; j ++) {

		if (size[j] > pivot) {
			size_t tmp = size[j];
			size[j] = size[i];
			size[i] = tmp;
			size_t idx = index[j];
			index[j] = index[i];
			index[i] = idx;
			i++;
		}
	}
	size_t tmp = size[upper];
	size[upper] = size[i];
	size[i] = tmp;
	size_t idx = index[upper];
	index[upper] = index[i];
	index[i] = idx;
	return i;
}


void size_index_descending_qsort(size_t * size, size_t lower, size_t upper, size_t * index) {
	if (upper > lower) {

		size_t partitionIndex = size_index_descending_qsort_partition(size, lower, upper, index);

		if ( partitionIndex > 0 ) {
			size_index_descending_qsort(size, lower, partitionIndex - 1, index);
		}

		if ( partitionIndex < SIZE_MAX ) {
			size_index_descending_qsort(size, partitionIndex + 1, upper, index);
		}

	}
}


void * alloc_aligned_dram(const size_t size, const size_t alignment) {
	void *ret;
	int rv;
	rv = posix_memalign((void**)&ret, alignment, size);

	assert (rv == 0);

	/**
	if (rv) {
		ERROR_RETURN(errno);
		return 0;
	}
	*/

	return ret;
}


void * alloc_huge_dram(const size_t size) {
	void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	madvise(addr, size, MADV_HUGEPAGE);
	return addr;
}


void dealloc_huge_dram(void *addr, const size_t size) {
	munmap(addr, size);	
}


void * create_pmem_buffer(const char *filepath, const size_t buffer_size) {
	void *pmemaddr;
	int is_pmem = 0;
	size_t mapped_len = 0;

	int res = access(filepath, F_OK);
	if (res == 0) {
		pmemaddr = (char*) pmem_map_file(filepath, buffer_size, 
			PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
	} else {
		// printf("create mapped filepath: %s\n", filepath);
		pmemaddr = (char*) pmem_map_file(filepath, buffer_size,
			PMEM_FILE_CREATE|PMEM_FILE_EXCL, 0666, &mapped_len, &is_pmem);
	}

	if (pmemaddr == NULL) {
		printf("%s pmem_map_file failed, errno: %d\n", filepath, errno);
		printf("arguments, buffer_size: %zu, mapped_len: %zu, is_pmem: %d\n", buffer_size, mapped_len, is_pmem);
		perror("pmem_map_file failed");
		exit(1);
	}

	// if (is_pmem) {
	// 	// printf("successfully mapped\n");
	// } else {
	// 	printf("%s error, not pmem, is_pmem: %d", filepath, is_pmem);
	// 	exit(1);
	// }

	if (!is_pmem) {
		printf("%s error, not pmem, is_pmem: %d", filepath, is_pmem);
		exit(1);
	}

	return pmemaddr;
}


char * new_nvm_filepath(const char* const directory, const size_t file_idx, const size_t thread_id) {
	char *buff = (char*) malloc( sizeof(char) * CHAR_BUFFER_LEN *2 );
	snprintf(buff, CHAR_BUFFER_LEN * 2, "%s/%zu-%zu", directory, thread_id, file_idx);
	return buff;
}


void * new_alloc_nvm(const size_t buffer_size) {
	char file_path[ CHAR_BUFFER_LEN * 2 ];
	snprintf(file_path, CHAR_BUFFER_LEN * 2, "%s/%lu_%zu.bin", work_dir, pthread_self(), file_idx ++);
	return create_pmem_buffer(file_path, buffer_size);
}


void * alloc_dram(const size_t buffer_size, const size_t alignment) {
#ifdef USE_HUGE
	return alloc_huge_dram(buffer_size);
#else /* USE_HUGE */
#ifdef USE_ALIGNMENT
	return alloc_aligned_dram(buffer_size, alignment);
#else /* USE_ALIGNMENT */
	return malloc(buffer_size);
#endif /* USE_ALIGNMENT */
#endif /* USE_HUGE */
}


void dealloc_dram(void * addr, const size_t size) {
#ifdef USE_HUGE
	dealloc_huge_dram(addr, size);
#else /* USE_HUGE */
#ifdef USE_ALIGNMENT
	free(addr);
#else /* USE_ALIGNMENT */
	free(addr);
#endif /* USE_ALIGNMENT */
#endif /* USE_HUGE */
}

void * alloc_memory(const size_t buffer_size, const size_t alignment) {
#ifdef USE_NVM
	char file_path[ CHAR_BUFFER_LEN * 2 ];
	snprintf(file_path, CHAR_BUFFER_LEN * 2, "%s/%lu_%zu.bin", work_dir, pthread_self(), file_idx ++);
	return create_pmem_buffer(file_path, buffer_size);
#else /* USE_NVM */
	return alloc_dram(buffer_size, alignment);
#endif /* USE_NVM */
}

void dealloc_memory(void * addr, const size_t size) {
#ifdef USE_NVM
	pmem_unmap(addr, size);
#else /* USE_NVM */
	dealloc_dram(addr, size);
#endif /* USE_NVM */
}

void memset_localize(void * addr, const size_t size) {
#ifdef USE_NVM
	pmem_memset_persist(addr, 0, size);
#else /* USE_NVM */
	memset(addr, 0, size);
#endif /* USE_NVM */
}

void warmup_localize(const tuple_t * const addr, const size_t tuplpe_num) {
	size_t sum = 0;
	tuple_t tmp;
	for (size_t idx = 0; idx < tuplpe_num; idx ++ ) {
		memcpy(&tmp, addr+idx, TUPLE_T_SIZE);
		sum += tmp.key + tmp.row_id;
	}
}

void memsync(const void * const mem_addr, const size_t size) {
#ifdef USE_NVM
	pmem_persist(mem_addr, size);
#else /* USE_NVM */
	pmem_msync(mem_addr, size);
#endif /* USE_NVM */
}

void *parallel_memcpy_thr(void* param) {
	par_mem_arg_t *par_mcpy_arg = (par_mem_arg_t *) param;
#ifdef USE_NVM
	pmem_memcpy_persist(par_mcpy_arg->dst, par_mcpy_arg->src, par_mcpy_arg->size);
#else /* USE_NVM */
	memcpy(par_mcpy_arg->dst, par_mcpy_arg->src, par_mcpy_arg->size);
	pmem_msync(par_mcpy_arg->dst, par_mcpy_arg->size);
#endif /* USE_NVM */
	return NULL;
}

void parallel_memcpy(void * const dst, void * const src, const size_t size) {
	int thread_num;
#ifdef USE_NVM
	// thread_num = BUILDPART_THREAD_NUM;
	thread_num = XSOCKET_THREAD_NUM_DEFAULT;
#else /* USE_NVM */
	thread_num = THREAD_NUM;
#endif /* USE_NVM */

	size_t avg_size = (size_t)(size/thread_num);

	pthread_t threadpool[thread_num];
	par_mem_arg_t args[thread_num];

	cpu_set_t set;
	int cpu_idx;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	for (int idx = 0; idx < thread_num; idx ++) {
		cpu_idx = local_cores[idx % SINGLE_SOCKET_CORE_NUM];
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->dst = dst + idx * avg_size;
		(args+idx)->src = src + idx * avg_size;
		(args+idx)->size = (idx==thread_num-1) ? size-idx*avg_size : avg_size;

		int rv = pthread_create(&threadpool[idx], &attr, parallel_memcpy_thr, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < thread_num; idx ++) {
		pthread_join(threadpool[idx], NULL);
	}
	pthread_attr_destroy(&attr);
}

void *parallel_memset_localize_thr(void* param) {
	par_mem_arg_t *par_mset_arg = (par_mem_arg_t *) param;
#ifdef USE_NVM
	pmem_memset_persist(par_mset_arg->dst, 0, par_mset_arg->size);
#else /* USE_NVM */
	memset(par_mset_arg->dst, 0, par_mset_arg->size);
	pmem_msync(par_mset_arg->dst, par_mset_arg->size);
#endif /* USE_NVM */
	return NULL;
}

void parallel_memset_localize(void * const dst, const size_t size) {
	int thread_num;
#ifdef USE_NVM
	// thread_num = BUILDPART_THREAD_NUM;
	thread_num = XSOCKET_THREAD_NUM_DEFAULT;
#else /* USE_NVM */
	thread_num = THREAD_NUM;
#endif /* USE_NVM */

	size_t avg_size = (size_t)(size/thread_num);

	pthread_t threadpool[thread_num];
	par_mem_arg_t args[thread_num];

	cpu_set_t set;
	int cpu_idx;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	for (int idx = 0; idx < thread_num; idx ++) {
		cpu_idx = local_cores[idx % SINGLE_SOCKET_CORE_NUM];
		CPU_ZERO(&set);
		CPU_SET(cpu_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		(args+idx)->_tid = idx;
		(args+idx)->dst = dst + idx * avg_size;
		(args+idx)->size = (idx==thread_num-1) ? size-idx*avg_size : avg_size;

		int rv = pthread_create(&threadpool[idx], &attr, parallel_memset_localize_thr, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < thread_num; idx ++) {
		pthread_join(threadpool[idx], NULL);
	}
	pthread_attr_destroy(&attr);
}


void ptr_retrieve(rowid_t *checksum, row_id_pair_t * const mater, const size_t tot_row_id_pair_num, 
	const relation_t org_relR, const relation_t org_relS) {
	*checksum = 0;
	size_t r_idx = 0;
	size_t s_idx = 0;
	row_id_pair_t *tmp;
	for (size_t idx = 0; idx < tot_row_id_pair_num; idx ++) {
		tmp = mater+idx;
		r_idx = tmp->r_row_id - 1;
		s_idx = tmp->s_row_id - 1;
		*checksum += (org_relR.tuples+r_idx)->row_id + (org_relS.tuples+s_idx)->row_id;
	}
}