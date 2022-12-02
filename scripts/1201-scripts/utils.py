import numpy as np
import os

LOG_PATH = "../../logs/0829-logs/"


part_algo = [
	"shrll",
	"shrcm",
	"indll",
	"indcm",
	"rdx",
	"asym"
]

join_algo = {
	# "shrll": ["sc", "lp", "hr", "hro"],
	# "shrcm": ["bc", "sc", "lp", "hr", "hro"],
	# "indll": ["sc", "lp", "hr", "hro"],
	# "indcm": ["bc", "sc", "lp", "hr", "hro"],
	# "rdx": ["bc", "sc", "lp", "hr", "hro"],
	# "asym": ["bc", "sc", "hr", "hro"]

	"shrll": ["sc", "lp", "hro"],
	"shrcm": ["bc", "sc", "lp", "hro"],
	"indll": ["sc", "lp", "hro"],
	"indcm": ["bc", "sc", "lp", "hro"],
	"rdx": ["bc", "sc", "lp", "hro"],
	"asym": ["bc", "sc", "hro"]
}

# part2color = {
# 	# "shrll": "orange", 
# 	# "shrcm": "orangered", 
# 	# "indll":"seagreen",
# 	# "indcm": "dodgerblue",
# 	# "rdx": "rebeccapurple", 
# 	# "asym": "deeppink" 

# 	"shrll": "deepskyblue", 
# 	"shrcm": "crimson", 
# 	"rdx": "yellowgreen"
# }

part2linestyle = {
	# "bc": "solid",
	# "sc": "dotted",
	# "lp": "dashed",
	# # "hr": "dashdot",
	# "hro": (0, (3, 5, 1, 5, 1, 5))	# dashdotdotted

	"shrll": "dashed", 
	"shrcm": "dotted",
	"rdx": "solid", 
}

join2color = {
	"bc": ["moccasin", "orange"],
	"sc": ["coral", "orangered"],
	"lp": ["lightskyblue", "deepskyblue"],
	"hro": ["lime", "seagreen"]
}

join2hatch = {
	"bc": "",
	"sc": "\\\\",
	"lp": "//",
	"hro": "++"
}

join2marker = {
	"bc": "o",
	"sc": "^",
	"lp": "*",
	"hro": "."
}

join2markerfacecolor = {
	"bc": "white",
	"sc": "black",
	"lp": "white",
	"hro": "black"
}

join2fillstyle = {
	"bc": "none",
	"sc": "full",
	"lp": "none",
	"hro": "full"	
}

part2legend = {
	# "shrll": "SHRll", 
	# "shrcm": "SHRcm",
	# "indll": "INDll",
	# "indcm": "INDcm",
	# "rdx": "RDX",
	# "asym": "ASYM"
	"shrll": "*ll", 
	"shrcm": "*cm",
	"rdx": "RDX",
}

join2legend = {
	"bc": "BC",
	"sc": "SC",
	"lp": "LP",
	# "hr": "HR",
	# "hro": "HRO"
	"hro": "HR"
}



hatch_list = [
	# "", "//", "--", "xx"
	# "", "", "", "",
	# "xx", "xx", "xx", "xx"
	"//", "\\\\"
]


memtype_list = [
	"dram", 
	"dram_huge",
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


def get_data_from_log(algo, mem_type):
	filepath = os.path.join(LOG_PATH, "{}_{}_{}.log".format(algo, mem_type))

	buildpart_time_list = []
	probejoin_time_list = []
	total_time_list = []

	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break

			if line.startswith("num_trials"):
				line = line.strip().split("\t")
				buildpart_time_list.append( round4decimals(line[-3].split()[-1]) )	# buildpart
				probejoin_time_list.append( round4decimals(line[-2].split()[-1]) )	# probejoin
				total_time_list.append( round4decimals(line[-1].split()[-1]) )	# total

	return buildpart_time_list, probejoin_time_list, total_time_list








def get_counter_from_log(algo, mem_type, event, event_flag):
	filepath = os.path.join(LOG_PATH, "{}_fanout_{}_counter.log".format(algo, mem_type))
	buildpart_time_list = []
	probejoin_time_list = []
	total_time_list = []

	total_counter_list = []
	buildpart_counter_list = []
	probejoin_counter_list = []

	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break

			if line.startswith("num_trials"):
				line = line.strip().split("\t")
				buildpart_time_list.append( round4decimals(line[-3].split()[-1]) )	# buildpart
				probejoin_time_list.append( round4decimals(line[-2].split()[-1]) )	# probejoin
				total_time_list.append( round4decimals(line[-1].split()[-1]) )	# total
				continue
			
			if event_flag == "papi":
				if line.startswith("[agg_PAPI]") and event in line:
					line = line.strip().split("\t")
					buildpart_counter_list.append(round4decimals(line[-2].split()[-1]))	# buildpart
					probejoin_counter_list.append(round4decimals(line[-1].split()[-1]))	# probejoin
					total_counter_list.append(round4decimals(line[-3].split()[-1]))	# total
					continue
			else:	# pmwatch
				if line.startswith("[agg_PMWatch]") and "DIMM: 0" in line and event in line:
					line = line.strip().split("\t")
					buildpart_counter_list.append(round4decimals(line[-2].split()[-1]))	# buildpart
					probejoin_counter_list.append(round4decimals(line[-1].split()[-1]))	# probejoin
					total_counter_list.append(round4decimals(line[-3].split()[-1]))	# total
					continue

	return buildpart_time_list, probejoin_time_list, total_time_list, \
		total_counter_list, buildpart_counter_list, probejoin_counter_list


























