import copy
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from matplotlib import gridspec
import numpy as np
import os
import sys

from utils import *

LOG_PATH = "../../logs/0829-logs/"
FIG_PATH = "../../figs/1201-figs/"

x_axis_label_list = [
	"NPHJ",
	"SHRll",
	"SHRcm",
	"INDll",
	"INDcm",
	"RDX",
	"ASYM"
]

x_data_width = 0.8

join_algo_num = 4



def plot_uniform_pkfk_A(mem_type, log_prefix):
	join2runtimelist = {
		"lp": [ [], [], [] ],
		"hro": [ [], [], [] ],
		"sc": [ [], [], [] ],
		"bc": [ [], [], [] ],
	}

	for algo_part in part_algo:
		for algo_join in join_algo[algo_part]:
			filepath = os.path.join(LOG_PATH, "{}{}_{}_{}.log".format(log_prefix, 
				algo_part, algo_join, mem_type) )
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
			join2runtimelist[algo_join][0].append(buildpart_time)
			join2runtimelist[algo_join][1].append(probejoin_time)
			join2runtimelist[algo_join][2].append(total_time)

		if algo_part == "shrll" or algo_part == "indll":
			join2runtimelist["bc"][0].append(0)
			join2runtimelist["bc"][1].append(0)
			join2runtimelist["bc"][2].append(0)

		if algo_part == "asym":
			join2runtimelist["lp"][0].append(0)
			join2runtimelist["lp"][1].append(0)			
			join2runtimelist["lp"][2].append(0)


	default_fontsize = 14

	x_axis = np.arange(len(x_axis_label_list))

	bar_width = x_data_width/join_algo_num * 3/4
	x_starting_idx_list = [ -x_data_width/2 + \
		(i+1/2) * x_data_width/join_algo_num \
		for i in range(join_algo_num) 
	]


	fig = plt.figure(figsize=(6.5, 2.3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("pkfk_A {}".format(mem_type))
	

	for idx, algo_join in enumerate(join2runtimelist.keys()):
		plt.bar(x_axis[1:]+x_starting_idx_list[idx], join2runtimelist[algo_join][0],
			width=bar_width, edgecolor="black", color=join2color[algo_join][0],
			hatch=join2hatch[algo_join], label=join2legend[algo_join]
		)
		plt.bar(x_axis[1:]+x_starting_idx_list[idx], join2runtimelist[algo_join][1],
			width=bar_width, edgecolor="black", color=join2color[algo_join][1],
			hatch=join2hatch[algo_join], bottom=join2runtimelist[algo_join][0]
		)




	nphj_sc_build_time_list = []
	nphj_sc_probe_time_list = []
	filepath = os.path.join(LOG_PATH, "{}{}_{}_{}.log".format(log_prefix, "nphj", "sc", mem_type) )
	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break
			if line.startswith("num_trials"):
				line = line.strip().split("\t")
				buildpart_time = round4decimals(line[-3].split()[-1])	# buildpart
				probejoin_time = round4decimals(line[-2].split()[-1])	# probejoin
				nphj_sc_build_time_list.append(buildpart_time)
				nphj_sc_probe_time_list.append(probejoin_time)

	nphj_sc_build_time_list.append(0.0)
	nphj_sc_probe_time_list.append(0.0)


	nphj_lp_build_time_list = []
	nphj_lp_probe_time_list = []
	nphj_lp_build_time_list.append(0.0)
	nphj_lp_probe_time_list.append(0.0)
	filepath = os.path.join(LOG_PATH, "{}{}_{}_{}.log".format(log_prefix, "nphj", "lp", mem_type) )
	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break
			if line.startswith("num_trials"):
				line = line.strip().split("\t")
				buildpart_time = round4decimals(line[-3].split()[-1])	# buildpart
				probejoin_time = round4decimals(line[-2].split()[-1])	# probejoin
				nphj_lp_build_time_list.append(buildpart_time)
				nphj_lp_probe_time_list.append(probejoin_time)


	nphj_sc_build_time_list = np.array(nphj_sc_build_time_list)
	nphj_sc_probe_time_list = np.array(nphj_sc_probe_time_list)
	nphj_lp_build_time_list = np.array(nphj_lp_build_time_list)
	nphj_lp_probe_time_list = np.array(nphj_lp_probe_time_list)


	print(x_starting_idx_list)
	print(x_data_width/join_algo_num)

	x_starting_idx_list = [ 
		-(bar_width)/2-0.1/4, 
		(bar_width)/2+0.1/4
	]

	plt.bar(x_axis[:1]+x_starting_idx_list[0], nphj_sc_build_time_list,
		width=bar_width, edgecolor="black",
		 color=join2color["sc"][0],
		hatch=join2hatch["sc"], 
		# label=join2legend["sc"]
	)

	plt.bar(x_axis[:1]+x_starting_idx_list[0], nphj_sc_probe_time_list,
		width=bar_width, edgecolor="black", 
		color=join2color["sc"][1],
		hatch=join2hatch["sc"], 
		bottom=nphj_sc_build_time_list
	)


	plt.bar(x_axis[0:1]+x_starting_idx_list[1], nphj_lp_build_time_list,
		width=bar_width, edgecolor="black",
		 color=join2color["lp"][0],
		hatch=join2hatch["lp"], 
		# label=join2legend["lp"]
	)

	plt.bar(x_axis[0:1]+x_starting_idx_list[1], nphj_lp_probe_time_list,
		width=bar_width, edgecolor="black", 
		color=join2color["lp"][1],
		hatch=join2hatch["lp"], 
		bottom=nphj_lp_build_time_list
	)



	# plt.grid(True)
	plt.legend(loc=0, ncol=2, fontsize=default_fontsize, columnspacing=1,
		fancybox=False, shadow=False
	)
	plt.ylabel("Elapsed Time (s)", fontsize=default_fontsize)
	plt.xticks([index for index in x_axis], x_axis_label_list, fontsize=default_fontsize, 
		# rotation=15
	)
	plt.yticks(fontsize=default_fontsize)
	# plt.ylim(top=10)
	# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
	# plt.show()

	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "pkfk_A_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	plt.title("", fontsize=default_fontsize)
	plt.savefig(os.path.join(FIG_PATH, "pkfk_A_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
			os.path.join(FIG_PATH, "pkfk_A_{}.pdf".format(mem_type)),
			os.path.join(FIG_PATH, "pkfk_A_{}.eps".format(mem_type))
		)
	)

	# plt.close()





if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)
	plot_uniform_pkfk_A("nvm", "")
























