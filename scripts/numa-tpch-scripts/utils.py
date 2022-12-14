import numpy as np
import os

DIMM_USING_NUM = 6

buildpart2color = {
	"nphj": "yellow",
	"shrll": "mediumslateblue",
	"shrcm": "lightcoral",
	"indll": "peru",
	"indcm": "navajowhite",
	"rdx": "lightpink",
	"asym": "lightgreen",
}

probejoin2color = {
	"nphj": "darkkhaki",
	"shrll": "slateblue",
	"shrcm": "indianred",
	"indll": "saddlebrown",
	"indcm": "tan",
	"rdx": "palevioletred",
	"asym": "darkseagreen",
}

algo2hatch = {
	"nphj": "",
	"shrll": "//",
	"shrcm": "\\\\",
	"indll": "//|",
	"indcm": "\\\\|",
	"rdx": "||",
	"asym": "xx"
}

algo2legend = {
	"nphj": "NPHJ",
	"shrll": "SHRll",
	"shrcm": "SHRcm",
	"indll": "INDll",
	"indcm": "INDcm",
	"rdx": "RDX",
	"asym": "ASYM"
}

algo2logname = {
	"nphj": "nphj_sc",
	"shrll": "shrll_hro",
	"shrcm": "shrcm_bc",
	"indll": "indll_hro",
	"indcm": "indcm_bc",
	"rdx": "phj_radix_bc",
	"asym": "asym_bc"
}

papi_event_list = [
	"PAPI_TOT_CYC",
	"PAPI_L1_TCM",
	"PAPI_L2_TCM",
	"PAPI_L3_TCM",
	"DTLB-STORE-MISSES",
	"DTLB-LOAD-MISSES",
	"ITLB-LOAD-MISSES"
]

pmwatch_event_list = [
	"total_bytes_read",
	"total_bytes_written",
	"media_read_ops",
	"media_write_ops",
	"read_64B_ops_received",
	"write_64B_ops_received",
	"host_reads",
	"host_writes",
	"read_hit_ratio",
	"write_hit_ratio",
	"read_amplification",
	"write_amplification"
]

workload_list = [
	"pkfk",
	"dupfk",
	"zipf",
	"selectivity",
	"density"
]

workload_param = {
	"pkfk": ["uniform_pkfk_A", "uniform_pkfk_B", "uniform_pkfk_C"],
	"dupfk": ["uniform_dupfk_D", "uniform_dupfk_E", "uniform_dupfk_F"],
	"zipf": ["zipfian_1.050_A", "zipfian_1.250_A", "zipfian_1.500_A", "zipfian_1.750_A"], #, "zipfian_1.900_A"],
	"selectivity": ["selectivity_0.200_A", "selectivity_0.400_A", "selectivity_0.600_A", "selectivity_0.800_A"],
	"density": ["density_0.200_A", "density_0.400_A", "density_0.600_A", "density_0.800_A"]
}


def round4decimals(value):
	return round(float(value), 4)


def get_data_from_log(filepath):
	buildpart_time = None
	probejoin_time = None
	total_time = None

	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break

			if line.startswith("num_trials"):
				line = line.strip().split("\t")
				buildpart_time = round4decimals(line[-3].split()[-1])	# buildpart
				probejoin_time = round4decimals(line[-2].split()[-1])	# probejoin
				total_time = round4decimals(line[-1].split()[-1])	# total

	return buildpart_time, probejoin_time, total_time


def get_counter_from_log(filepath, event, event_flag):
	buildpart_time = None
	probejoin_time = None
	total_time = None

	total_counter = None
	buildpart_counter = None
	probejoin_counter = None

	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break

			if line.startswith("num_trials"):
				line = line.strip().split("\t")
				buildpart_time = round4decimals(line[-3].split()[-1])	# buildpart
				probejoin_time = round4decimals(line[-2].split()[-1])	# probejoin
				total_time = round4decimals(line[-1].split()[-1])	# total
				continue
			
			if event_flag == "papi":
				if line.startswith("[agg_PAPI]") and event in line:
					line = line.strip().split("\t")
					buildpart_counter = round4decimals(line[-2].split()[-1])/(10**6)	# buildpart
					probejoin_counter = round4decimals(line[-1].split()[-1])/(10**6)	# probejoin
					total_counter = round4decimals(line[-3].split()[-1])/(10**6)	# total
					continue
			else:	# pmwatch
				if line.startswith("[agg_PMWatch]") and "DIMM: 0" in line and event in line:
					line = line.strip().split("\t")
					buildpart_counter = round4decimals(line[-2].split()[-1])/(10**6)	# buildpart
					probejoin_counter = round4decimals(line[-1].split()[-1])/(10**6)	# probejoin
					total_counter = round4decimals(line[-3].split()[-1])/(10**6)	# total
					buildpart_counter *= DIMM_USING_NUM
					probejoin_counter *= DIMM_USING_NUM
					total_counter *= DIMM_USING_NUM
					continue

	return buildpart_time, probejoin_time, total_time, \
		buildpart_counter, probejoin_counter, total_counter



def tpch_get_data_from_log(filepath):
	pushdown_time = None
	buildpart_time = None
	probejoin_time = None
	total_time = None

	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break

			if line.startswith("num_trials"):
				line = line.strip().split("\t")
				pushdown_time = round4decimals(line[-5].split()[-1])	# pushdown
				buildpart_time = round4decimals(line[-4].split()[-1])	# buildpart
				probejoin_time = round4decimals(line[-3].split()[-1])	# probejoin
				total_time = round4decimals(line[-1].split()[-1])	# total

	return pushdown_time, buildpart_time, probejoin_time, total_time



def tpch_list_packing(pushdown_time, buildpart_time, probejoin_time, total_time, idx, length):
	pushdown_time_list = [0] * length
	buildpart_time_list = [0] * length
	probejoin_time_list = [0] * length
	total_time_list = [0] * length

	pushdown_time_list[idx] = pushdown_time
	buildpart_time_list[idx] = buildpart_time
	probejoin_time_list [idx] = probejoin_time
	total_time_list[idx] = total_time

	return pushdown_time_list, buildpart_time_list, probejoin_time_list, total_time_list