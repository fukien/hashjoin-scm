import numpy as np
import os

LOG_PATH = "../../logs/phj-part-thread-scalability-logs/"

legend_list = [
	"SHRll", 
	"SHRcm",
	"INDll",
	"INDcm",
	"RDX",
	"ASYM"
]

marker_list = [
	"^",
	"d",
	"o",
	"s",
	"P",
	"*"
]

color_list = [
	# "coral", "chartreuse", "navajowhite", "lightskyblue",
	# "coral", "chartreuse", "navajowhite", "lightskyblue"

	# ["moccasin", "orange"],
	# ["coral", "orangered"],
	# ["lightskyblue", "deepskyblue"],
	# ["lime", "seagreen"]

	# "moccasin", "coral"
	# "lightskyblue", "lime"
	# "crimson", "royalblue"
	
	"orange", 
	"orangered", 
	"seagreen",
	"dodgerblue",
	"rebeccapurple", 
	"deeppink" 
]


# nphj_algo = ["nphj_sc", "nphj_lp"]
# nphj_algo = ["nphj_sc", "nphj_lp_atom", "nphj_lp_lock"]
# phj_algo = ["phj_radix_bc", "phj_radix_lp", "phj_radix_hr", "phj_radix_hro"]

phj_algo = [
	"phj_shr_hro",
	"phj_shr_uni_hro",
	"phj_ind_hro",
	"phj_ind_uni_hro",
	"phj_radix_hro",
	"phj_asymm_radix_hro"
]


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


def get_data_from_log_thread_scalablitlity(algo, core_setting, mem_type):
	filepath = os.path.join(LOG_PATH, "{}_thread_scalability_{}_{}.log".format(algo, core_setting, mem_type))

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



def bw_reg_get_data_from_log_thread_scalablitlity(filepath):
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

















