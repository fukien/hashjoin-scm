// #pragma once

#ifndef INTEL_PMWATCH_PMWATCH_H
#define INTEL_PMWATCH_PMWATCH_H

#include "pmw_utils.h"
#include "pmw_collect.h"

#ifndef MaxNVMNum
#define MaxNVMNum 12
// #define MaxNVMNum 16
#endif /* MaxNVMNum */

struct MY_PMWATCH_OP_BUF_NODE_S {
// 					variable					description in pdf
	uint64_t		epoch;
	uint64_t		timestamp;
	uint64_t		total_bytes_read;			// bytes_read
	uint64_t		total_bytes_written;		// bytes_written
	double			read_hit_ratio;
	double			write_hit_ratio;

	uint64_t		media_read_ops;				// media_read_ops
	uint64_t		media_write_ops;			// media_write_ops
	uint64_t		read_64B_ops_received;		// read_64B_ops_received
	uint64_t		write_64B_ops_received;		// write_64B_ops_received

	uint64_t		host_reads;					// cpu_read_ops
	uint64_t		host_writes;				// cpu_write_ops
	unsigned char 	health_status;
	unsigned char 	percentage_remaining;
	unsigned char 	percentage_used;
	uint64_t		power_on_time;
	uint64_t		uptime;
	uint64_t		last_shutdown_time;
	double			media_temp;
	double			controller_temp;
	double			max_media_temp;
	double			max_controller_temp;
	
	double			read_amplification;
	double			write_amplification;
};

typedef struct MY_PMWATCH_OP_BUF_NODE_S MY_PMWATCH_OP_BUF_NODE;

typedef enum PMSTAT_STATUS{
	PMSTAT_OK,
	PMSTAT_ERR
}PMSTAT_STATUS;

PMSTAT_STATUS pmwInit();
PMSTAT_STATUS pmwStart();
PMSTAT_STATUS pmwEnd(MY_PMWATCH_OP_BUF_NODE* output);
PMSTAT_STATUS pmwClear();

#endif // INTEL_PMWATCH_PMWATCH_H