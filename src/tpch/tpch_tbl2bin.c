#include <string.h>

#include <libpmem.h>

#include "inc/schema.h"
#include "inc/tpch_utils.h"

#include "../inc/utils.h"


char *delimiter = "|";

#define TPCH_DIR_PATH "/work/huang/workspace/pm-join/tpch-data"
#define NVM_DIR_PATH "/dcpmm/huang/pm-join/tpch-data"


extern size_t part_num;
extern size_t lineitem_num;


void tbl2bin_part(int sf, size_t tuple_num) {
	char input_filepath[CHAR_BUFFER_LEN];
	char output_filepath[CHAR_BUFFER_LEN];

	snprintf(input_filepath, CHAR_BUFFER_LEN, "%s/sf%d/%s", TPCH_DIR_PATH, sf, "part.tbl");
	snprintf(output_filepath, CHAR_BUFFER_LEN, "%s/sf%d/%s", NVM_DIR_PATH, sf, "part.bin");

	part_t *parts = (part_t *) create_pmem_buffer(output_filepath, tuple_num * PART_T_SIZE);

	FILE *fp;
	fp = fopen(input_filepath, "r");

	char tmp_text[CHAR_BUFFER_LEN];
	char *pch;
	size_t line_num = 0;

	while (fgets(tmp_text, CHAR_BUFFER_LEN, fp)) {
		pch = strtok(tmp_text, delimiter);
		(parts+line_num)->p_partkey = atol(pch);

		pch = strtok(NULL, delimiter);
		strncpy( (parts+line_num)->p_name, pch, 55);

		pch = strtok(NULL, delimiter);
		strncpy( (parts+line_num)->p_mfgr, pch, 25);

		pch = strtok(NULL, delimiter);
		strncpy( (parts+line_num)->p_brand, pch, 10);

		pch = strtok(NULL, delimiter);
		strncpy( (parts+line_num)->p_type, pch, 25);

		pch = strtok(NULL, delimiter);
		(parts+line_num)->p_size = atoi(pch);

		pch = strtok(NULL, delimiter);
		strncpy( (parts+line_num)->p_container, pch, 10);

		pch = strtok(NULL, delimiter);
		(parts+line_num)->p_retailprice = (double) atof(pch);

		pch = strtok(NULL, delimiter);
		strncpy( (parts+line_num)->p_comment, pch, 23);

		line_num ++;
	}

	fclose(fp);
	assert(line_num == tuple_num);

	pmem_unmap(parts, tuple_num * PART_T_SIZE);
}



void tbl2bin_lineitem(int sf, size_t tuple_num) {
	char input_filepath[CHAR_BUFFER_LEN];
	char output_filepath[CHAR_BUFFER_LEN];

	snprintf(input_filepath, CHAR_BUFFER_LEN, "%s/sf%d/%s", TPCH_DIR_PATH, sf, "lineitem.tbl");
	snprintf(output_filepath, CHAR_BUFFER_LEN, "%s/sf%d/%s", NVM_DIR_PATH, sf, "lineitem.bin");

	lineitem_t *lineitems = (lineitem_t *) create_pmem_buffer(output_filepath, tuple_num * LINEITEM_T_SIZE);

	FILE *fp;
	fp = fopen(input_filepath, "r");

	char tmp_text[CHAR_BUFFER_LEN];
	char *pch;
	size_t line_num = 0;

	while (fgets(tmp_text, CHAR_BUFFER_LEN, fp)) {
		pch = strtok(tmp_text, delimiter);
		(lineitems+line_num)->l_orderkey = atol(pch);

		pch = strtok(NULL, delimiter);
		(lineitems+line_num)->l_partkey = atol(pch);

		pch = strtok(NULL, delimiter);
		(lineitems+line_num)->l_suppkey = atol(pch);

		pch = strtok(NULL, delimiter);
		(lineitems+line_num)->l_linenumber = atoi(pch);

		pch = strtok(NULL, delimiter);
		(lineitems+line_num)->l_quantity = (double) atof(pch);

		pch = strtok(NULL, delimiter);
		(lineitems+line_num)->l_extendedprice = (double) atof(pch);

		pch = strtok(NULL, delimiter);
		(lineitems+line_num)->l_discount = (double) atof(pch);

		pch = strtok(NULL, delimiter);
		(lineitems+line_num)->l_tax = (double) atof(pch);

		pch = strtok(NULL, delimiter);
		strncpy( (lineitems+line_num)->l_returnflag, pch, 1);

		pch = strtok(NULL, delimiter);
		strncpy( (lineitems+line_num)->l_linestatus, pch, 1);

		pch = strtok(NULL, delimiter);
		strncpy( (lineitems+line_num)->l_shipdate, pch, 10);

		pch = strtok(NULL, delimiter);
		strncpy( (lineitems+line_num)->l_commitdate, pch, 10);

		pch = strtok(NULL, delimiter);
		strncpy( (lineitems+line_num)->l_receiptdate, pch, 10);

		pch = strtok(NULL, delimiter);
		strncpy( (lineitems+line_num)->l_shipinstruct, pch, 25);

		pch = strtok(NULL, delimiter);
		strncpy( (lineitems+line_num)->l_shipmode, pch, 10);

		pch = strtok(NULL, delimiter);
		strncpy( (lineitems+line_num)->l_comment, pch, 44);

		line_num ++;
	}

	fclose(fp);
	assert(line_num == tuple_num);

	pmem_unmap(lineitems, tuple_num * LINEITEM_T_SIZE);
}




int main(int argc, char const *argv[]) {
	assert (argc == 2);
	int sf = atoi(argv[1]);

	char output_dir[CHAR_BUFFER_LEN];
	snprintf(output_dir, CHAR_BUFFER_LEN, "%s/sf%d", NVM_DIR_PATH, sf);
	mkdir(output_dir);

	get_cardinality(sf);

	printf("scaling factor: %d\n", sf);
	printf("part_num: %zu\n", part_num);
	printf("lineitem_num: %zu\n", lineitem_num);

	struct timespec begin, end; 
	clock_gettime(CLOCK_REALTIME, &begin);

	tbl2bin_part(sf, part_num);
	tbl2bin_lineitem(sf, lineitem_num);

	clock_gettime(CLOCK_REALTIME, &end);
	double elapsed = diff_sec(begin, end);
	printf("Time elapsed: %.9f seconds.\n", elapsed);

	return 0;
}