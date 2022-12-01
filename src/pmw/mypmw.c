#include <stdint.h>
#include <stdio.h>

#include "mypmw.h"


LIFETIME_INFO_COUNTER_NODE before[MaxNVMNum];
LIFETIME_INFO_COUNTER_NODE after[MaxNVMNum];
LIFETIME_INFO_COUNTER_NODE diff[MaxNVMNum];

int page_num = 1;
unsigned int NVMCount;

PMSTAT_STATUS pmwInit() {
	int ret_code = PMW_COMM_Init();
	if (ret_code != NVM_SUCCESS) {
		if (ret_code != NVM_ERR_INVALID_PERMISSIONS) {
			PMW_COMM_Print_Warning_Message();
		}
		if (ret_code != PMW_ERR_DLLLOAD && ret_code != PMW_ERR_DLLSYM) {
			PMW_COMM_Cleanup();
		}
		return ret_code;
	}
	PMW_COMM_Get_Number_of_DIMM(&NVMCount);
	return PMSTAT_OK;
}

PMSTAT_STATUS pmwStart() {
	for (int i = 0; i < NVMCount; ++i) {
		int ret_code = PMW_COMM_Get_Memory_Info_Page(i, &before[i], page_num);
		if ( ret_code != NVM_SUCCESS) {
			fprintf(stderr, "\nSomething went wrong while collecting metrics.\n");
			return PMSTAT_ERR;
		}
	}
	return PMSTAT_OK;
}

PMSTAT_STATUS pmwEnd(MY_PMWATCH_OP_BUF_NODE* output) {
	uint64_t total_bytes_read, total_bytes_written, media_read, media_write;
	double read_hit_ratio, write_hit_ratio;

	for (int j = 0; j < NVMCount; j++){
		int ret_code = PMW_COMM_Get_Memory_Info_Page(j, &after[j], page_num);
		if ( ret_code != NVM_SUCCESS) {
			fprintf(stderr, "\nSomething went wrong while collecting metrics.\n");
			return PMSTAT_ERR;
		}
		// get the difference of before and after sleep count
		PMW_UTILS_Cal_Diff(&diff[j], &before[j], &after[j]);
			// calculate derived output metrics

			// bytes_read less than bytes_written results in negative value for total_bytes_read/media_read
			// providing "0" value to avoid displaying bogus negative value in this scenario
			if (diff[j].bytes_read > diff[j].bytes_written) {
				total_bytes_read = (diff[j].bytes_read - diff[j].bytes_written) * 64;
				media_read		 = (diff[j].bytes_read - diff[j].bytes_written) / 4;
			} else {
				total_bytes_read = 0ULL;
				media_read	   = 0ULL;
			}
			total_bytes_written = diff[j].bytes_written * 64;
			media_write		   = diff[j].bytes_written / 4;

			// calculating read and write hit ratio
			if (diff[j].host_reads > media_read) {
				double opr_01 = (double) diff[j].host_reads - media_read;
				read_hit_ratio = (double) opr_01 / diff[j].host_reads;
			} else {
				read_hit_ratio = 0;
			}
			if (diff[j].host_writes > media_write) {
				double opr_02 = (double) diff[j].host_writes - media_write;
				write_hit_ratio = (double) opr_02 / diff[j].host_writes;
			} else {
				write_hit_ratio = 0;
			}
			output[j].total_bytes_read				= total_bytes_read;
			output[j].total_bytes_written			= total_bytes_written;

			output[j].media_read_ops				= media_read;
			output[j].media_write_ops				= media_write;
			output[j].read_64B_ops_received			= diff[j].bytes_read;
			output[j].write_64B_ops_received		= diff[j].bytes_written;
			
			output[j].host_reads					= diff[j].host_reads;
			output[j].host_writes					= diff[j].host_writes;
			output[j].read_hit_ratio				= read_hit_ratio;
			output[j].write_hit_ratio				= write_hit_ratio;

			if (diff[j].host_reads != 0) {
				output[j].read_amplification = (double) total_bytes_read / ( diff[j].host_reads * 64 );
			}
			if (diff[j].host_writes != 0) {
				output[j].write_amplification = (double) total_bytes_written / ( diff[j].host_writes * 64 );
			}
	}

	return PMSTAT_OK;
}


PMSTAT_STATUS pmwClear() {
	PMW_COMM_Cleanup();
	return PMSTAT_OK;
}