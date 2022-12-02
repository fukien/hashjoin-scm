import numpy as np
import os

LOG_PATH = "../../logs/0801-logs/"


color_list = [
	# "coral", "chartreuse", "navajowhite", "lightskyblue",
	# "coral", "chartreuse", "navajowhite", "lightskyblue"

	["moccasin", "orange"],
	["coral", "orangered"],
	["lightskyblue", "deepskyblue"],
	["lime", "seagreen"]
]

hatch_list = [
	# "", "//", "--", "xx"
	"", "", "", "",
	"xx", "xx", "xx", "xx"
]


memtype_list = [
	"dram", 
	"nvm"
]

hw_list = {
	"hw_on",
	"hw_off"
}

subtype_list = [
	"pkfk_a", 
	# "pkfk_b", 
	# "pkfk_c",
	"dupfk_d", 
	# "dupfk_e", 
	# "dupfk_f",
	# "zipfian_1.050", "zipfian_1.250", "zipfian_1.500", "zipfian_1.750", "zipfian_1.900",
	# "selectivity_0.200", "selectivity_0.400", "selectivity_0.600", "selectivity_0.800"
]

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


nodetype_list = [
	"local", 
	"remote"
]

nphj_algo = ["nphj_sc", "nphj_lp_atom", "nphj_lp_lock"]
phj_algo = ["phj_radix_bc", "phj_radix_lp", "phj_radix_hr", "phj_radix_hro"]

algo2buildpart = {
	"nphj_sc": "build",
	"nphj_lp_atom": "build",
	"nphj_lp_lock": "build",
	"phj_radix_bc": "part",
	"phj_radix_lp": "part",
	"phj_radix_hr": "part",
	"phj_radix_hro": "part",
}

algo2probejoin = {
	"nphj_sc": "probe",
	"nphj_lp_atom": "probe",
	"nphj_lp_lock": "probe",
	"phj_radix_bc": "join",
	"phj_radix_lp": "join",
	"phj_radix_hr": "join",
	"phj_radix_hro": "join",
}

algo2color = {
	"nphj_sc": "tab:blue",
	"nphj_lp_atom": "tab:orange",
	"nphj_lp_lock": "tab:brown",
	"phj_radix_bc": "tab:red",
	"phj_radix_lp": "tab:purple",
	"phj_radix_hr": "tab:pink",
	"phj_radix_hro": "tab:green",
}

def round4decimals(value):
	return round(float(value), 4)

def get_data_from_log(alignment, mem_type):
	filepath = os.path.join(LOG_PATH, "nphj_sc_{}_{}.log".format(alignment, mem_type))

	regular_list = []
	clflush_list = []
	clflushopt_list = []
	ntstore_list = []
	regular_fence_list = []
	clflush_fence_list = []
	clflushopt_fence_list = []
	ntstore_fence_list = []

	tmp_list = []

	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break

			if line.startswith("REGULAR"):
				tmp_list = regular_list
				# print("REGULAR")
			if line.startswith("CLFLUSH"):
				tmp_list = clflush_list
				# print("CLFLUSH")
			if line.startswith("CLFLUSHOPT"):
				tmp_list = clflushopt_list
				# print("CLFLUSHOPT")
			if line.startswith("NT_STORE"):
				tmp_list = ntstore_list
				# print("NT_STORE")
			if line.startswith("REGULAR + FENCE"):
				tmp_list = regular_fence_list
				# print("REGULAR + FENCE")
			if line.startswith("CLFLUSH + FENCE"):
				tmp_list = clflush_fence_list
				# print("CLFLUSH + FENCE")
			if line.startswith("CLFLUSHOPT + FENCE"):
				tmp_list = clflushopt_fence_list
				# print("CLFLUSHOPT + FENCE")
			if line.startswith("NT_STORE + FENCE"):
				tmp_list = ntstore_fence_list
				# print("NT_STORE + FENCE")

			if line.startswith("num_trials"):
				line = line.strip().split("\t")
				tmp_list.append(round4decimals(line[-3].split()[-1]))	# buildpart
				tmp_list.append(round4decimals(line[-2].split()[-1]))	# probejoin
				tmp_list.append(round4decimals(line[-1].split()[-1]))	# total


		# print(regular_list)
		# print(clflush_list)
		# print(clflushopt_list)
		# print(ntstore_list)
		# print(regular_fence_list)
		# print(clflush_fence_list)
		# print(clflushopt_fence_list)
		# print(ntstore_fence_list)


	return regular_list, \
		clflush_list, \
		clflushopt_list, \
		ntstore_list, \
		regular_fence_list, \
		clflush_fence_list, \
		clflushopt_fence_list, \
		ntstore_fence_list





def get_counter_from_log(alignment, mem_type, event, flag):
	filepath = os.path.join(LOG_PATH, "nphj_sc_{}_{}.log".format(alignment, mem_type))

	regular_list = []
	clflush_list = []
	clflushopt_list = []
	ntstore_list = []
	regular_fence_list = []
	clflush_fence_list = []
	clflushopt_fence_list = []
	ntstore_fence_list = []

	tmp_list = []

	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break

			if line.startswith("REGULAR"):
				tmp_list = regular_list
				# print("REGULAR")
			if line.startswith("CLFLUSH"):
				tmp_list = clflush_list
				# print("CLFLUSH")
			if line.startswith("CLFLUSHOPT"):
				tmp_list = clflushopt_list
				# print("CLFLUSHOPT")
			if line.startswith("NT_STORE"):
				tmp_list = ntstore_list
				# print("NT_STORE")
			if line.startswith("REGULAR + FENCE"):
				tmp_list = regular_fence_list
				# print("REGULAR + FENCE")
			if line.startswith("CLFLUSH + FENCE"):
				tmp_list = clflush_fence_list
				# print("CLFLUSH + FENCE")
			if line.startswith("CLFLUSHOPT + FENCE"):
				tmp_list = clflushopt_fence_list
				# print("CLFLUSHOPT + FENCE")
			if line.startswith("NT_STORE + FENCE"):
				tmp_list = ntstore_fence_list
				# print("NT_STORE + FENCE")

			if flag == "papi":
				if line.startswith("[agg_PAPI]") and event in line:
					line = line.strip().split("\t")
					tmp_list.append(round4decimals(line[-2].split()[-1]))	# buildpart
					tmp_list.append(round4decimals(line[-1].split()[-1]))	# probejoin
					tmp_list.append(round4decimals(line[-3].split()[-1]))	# total
			else:	# pmwatch
				if line.startswith("[agg_PMWatch]") and "DIMM: 0" in line and event in line:
					line = line.strip().split("\t")
					tmp_list.append(round4decimals(line[-2].split()[-1]))	# buildpart
					tmp_list.append(round4decimals(line[-1].split()[-1]))	# probejoin
					tmp_list.append(round4decimals(line[-3].split()[-1]))	# total



		# print(regular_list)
		# print(clflush_list)
		# print(clflushopt_list)
		# print(ntstore_list)
		# print(regular_fence_list)
		# print(clflush_fence_list)
		# print(clflushopt_fence_list)
		# print(ntstore_fence_list)


	return regular_list, \
		clflush_list, \
		clflushopt_list, \
		ntstore_list, \
		regular_fence_list, \
		clflush_fence_list, \
		clflushopt_fence_list, \
		ntstore_fence_list























