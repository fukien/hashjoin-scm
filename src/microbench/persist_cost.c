#include "libpmem.h"

#include "../inc/memaccess.h"
#include "../inc/store.h"
#include "../inc/utils.h"

extern char work_dir[CHAR_BUFFER_LEN];

const size_t buffer_size = 1 << 28;				// 256MB
const size_t max_idx = buffer_size / TUPLE_T_SIZE - 1;
const size_t iter_num = 1 << 24;

int main(int argc, char **argv) {
	int seed = 32768;
	srand(seed);

	snprintf(work_dir, CHAR_BUFFER_LEN, "%s/%s", WORK_PATH_PREFIX, current_timestamp());
	mkdir(work_dir);

	struct timespec clock_start, clock_end;

	tuple_t org_tuple;

	void* addr;
	addr = new_alloc_nvm(buffer_size);
	memset_localize(addr, buffer_size);


	clock_gettime(CLOCK_REALTIME, &clock_start);

	for (size_t idx = 0; idx < iter_num; idx ++) {
		store_a_tuple(addr + TUPLE_T_SIZE * ( rand() % max_idx ), &org_tuple);
	}

	clock_gettime(CLOCK_REALTIME, &clock_end);


	pmem_unmap(addr, buffer_size);

	deldir(work_dir);

	printf("elapsed time: %.8f\n", diff_sec(clock_start, clock_end));

	return 0;
}