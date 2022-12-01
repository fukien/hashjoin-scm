#include <stdlib.h>
#include <stdio.h>

#include "inc/schema.h"
#include "inc/store.h"


int main(int argc, char** argv) {

	printf("size of part_t: %zu\n", PART_T_SIZE);
	printf("size of lineitem_t: %zu\n", LINEITEM_T_SIZE);

	printf("size of fltrq_part_t: %zu\n", FLTRQ_PART_T_SIZE);
	printf("size of fltrq_lineitem_t: %zu\n", FLTRQ_LINEITEM_T_SIZE);

#if QUERY == Q19
	printf("size of fltrq_match_t: %zu\n", FLTRQ_MATCH_T_SIZE);
#else /* QUERY == Q19 */
#endif /* QUERY == Q19 */

	return 0;
}