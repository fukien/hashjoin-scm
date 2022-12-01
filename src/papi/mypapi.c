#include "mypapi.h"

char* PAPI_event_names[NUM_PAPI_EVENT] = {
	"PAPI_TOT_CYC",
	// "PAPI_TOT_INS",
	// "PAPI_BR_MSP",
	"PAPI_L1_TCM",
	"PAPI_L2_TCM",
	"PAPI_L3_TCM",
	"perf::DTLB-STORE-MISSES",
	"perf::DTLB-LOAD-MISSES",
	"perf::ITLB-LOAD-MISSES"
};