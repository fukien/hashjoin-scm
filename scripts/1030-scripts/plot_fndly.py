import collections
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from matplotlib.gridspec import GridSpec
import numpy as np
import os
import pandas as pd
import seaborn as sns
import sys


LOG_PATH = "../../logs/1030-logs"
FIG_PATH = "../../figs/1030-figs"


mem_type = "nvm"

algo_list = ["rdx", "rdx_bw_reg", "asym"]
algo_num = len(algo_list)

buildpart2color = {
	"rdx": "lightpink",
	"rdx_bw_reg": "paleturquoise",
	"asym": "yellow",
}

probejoin2color = {
	"rdx": "palevioletred",
	"rdx_bw_reg": "darkturquoise",
	"asym": "darkkhaki",
}

algo2hatch = {
	"rdx": "//",
	"rdx_bw_reg": "\\\\",
	"asym": "xx"
}

algo2legend = {
	"rdx": "RDX-1",
	"rdx_bw_reg": "RDX-2",
	"asym": "ASYM"
}

algo2logname = {
	"rdx": {
		"b": "rdx_bw_reg_bc_1_pass",
		"h": "rdx_bc",
		"l": "rdx_hro",
		"p": "rdx_hro",
		"q": "rdx_bc",
		"t": "rdx_bc",
		"u": "rdx_hro",
		"v": "rdx_bc",
		"n": "rdx_bc",
	},
	"rdx_bw_reg": {
		"b": "rdx_bw_reg_bc_2_pass",
		"h": "rdx_bw_reg_bc",
		"l": "rdx_bw_reg_bc",
		"p": "rdx_bw_reg_bc",
		"q": "rdx_bw_reg_bc",
		"t": "rdx_bw_reg_bc",
		"u": "rdx_bw_reg_bc",
		"v": "rdx_bw_reg_bc",
		"n": "rdx_bw_reg_bc",
	},
	"asym": {
		"b": "asym_bc",
		"h": "asym_hro",
		"l": "asym_hro",
		"p": "asym_hro",
		"q": "asym_bc",
		"t": "asym_bc",
		"u": "asym_hro",
		"v": "asym_bc",
		"n": "asym_hro",
	}
}

workload2title = {
	"q": "256M:1024M",
	"t": "512M:4096M",
	"n": "1024M:16384M",
	"p": "2048M:16384M",
	"u": "4096M:16384M",
	"b": "64M:256M",
	"v": "128M:1024M",
	"h": "256M:4096M",
	"l": "512M:16384M",
}

workload2ratio = {
	# "q": "1:4",
	# "t": "1:8",
	# "n": "1:16",
	# "p": "1:8",
	# "u": "1:4",
	# "b": "1:4",
	# "v": "1:8",
	# "h": "1:16",
	# "l": "1:32",

	"q": "$x=4$",
	"t": "$x=8$",
	"n": "$x=16$",
	"p": "$x=8$",
	"u": "$x=4$",
	"b": "$x=4$",
	"v": "$x=8$",
	"h": "$x=16$",
	"l": "$x=32$",

	# "q": "$x=4$, $k=2$",
	# "t": "$x=8$, $k=2$",
	# "n": "$x=16$, $k=2$",
	# "p": "$x=8$, $k=4$",
	# "u": "$x=4$, $k=4$",
	# "b": "$x=4$, $k=2$",
	# "v": "$x=8$, $k=2$",
	# "h": "$x=16$, $k=2$",
	# "l": "$x=32$, $k=2$",
}


workload_group_5_list = ["q", "t", "n", "p", "u"]
workload_group_7_list = ["b", "v", "h", "l", "n", "p", "u"]



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


def list_packing(buildpart_time, probejoin_time, total_time, idx, length):
	buildpart_time_list = [0] * length
	probejoin_time_list = [0] * length
	total_time_list = [0] * length

	buildpart_time_list[idx] = buildpart_time
	probejoin_time_list [idx] = probejoin_time
	total_time_list[idx] = total_time

	return buildpart_time_list, probejoin_time_list, total_time_list


def plot_workload_group(workload_list):
	# print("_".join(workload_list))
	workload_num = len(workload_list)

	x_axis = np.arange(algo_num)
	default_fontsize = 12
	title_y_coord = -0.32
	x_data_width = 2
	bar_width = x_data_width/algo_num * 9/10
	x_starting_idx_list = [ 
		-x_data_width/2 + \
		(i+1/2) * x_data_width/algo_num \
		for i in range(algo_num) 
	]

	legend_bars = []
	legend_names = []

	fig = plt.figure(figsize=(12, 2), constrained_layout=True)
	# plt.subplots_adjust(wspace=0.5)
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	gs = GridSpec(1, workload_num, figure=fig)

	for idx, workload in enumerate(workload_list):
		general_buildpart_time_list = [[]] * algo_num
		general_probejoin_time_list = [[]] * algo_num
		ax = fig.add_subplot(gs[0, idx])
		ax.axes.get_xaxis().set_visible(False)
		ax.set_title(workload2title[workload]+"\n({})".format(workload2ratio[workload]), fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
		ax.tick_params(axis='y', labelsize=default_fontsize)
		ax.grid()

		for jdx, algo in enumerate(algo_list):
			filepath = os.path.join(
				LOG_PATH, "{}_fndly_{}_{}.log".format(algo2logname[algo][workload], workload, mem_type)
			)
			if workload == "n":
				filepath = os.path.join(
					"../../logs/1028-logs/{}_billion_{}.log".format(algo2logname[algo][workload], mem_type)
				)
				if algo == "rdx_bw_reg":
					filepath = os.path.join(
						"../../logs/1021-logs/{}_billion_{}.log".format(algo2logname[algo][workload], mem_type)
					)
			buildpart_time, probejoin_time, total_time = get_data_from_log(filepath)
			general_buildpart_time_list[jdx], \
			general_probejoin_time_list[jdx], _ = \
				list_packing(buildpart_time, probejoin_time, total_time, jdx, algo_num);
			if idx == 0:
				tmp_bar = ax.bar(
						x_axis, general_buildpart_time_list[jdx],
						width=bar_width, edgecolor="black", color=buildpart2color[algo],
						hatch=algo2hatch[algo], 
						# label=algo2legend[algo]
					)
				ax.bar(
					x_axis, general_probejoin_time_list[jdx],
					width=bar_width, edgecolor="black", color=probejoin2color[algo],
					hatch=algo2hatch[algo], 
					# label=algo2legend[algo]
					bottom=general_buildpart_time_list[jdx]
				)
				legend_bars.append(tmp_bar)
				legend_names.append(algo2legend[algo])
			else:
				ax.bar(
					x_axis, general_buildpart_time_list[jdx],
					width=bar_width, edgecolor="black", color=buildpart2color[algo],
					hatch=algo2hatch[algo], 
					# label=algo2legend[algo]
				)
				ax.bar(
					x_axis, general_probejoin_time_list[jdx],
					width=bar_width, edgecolor="black", color=probejoin2color[algo],
					hatch=algo2hatch[algo], 
					# label=algo2legend[algo]
					bottom=general_buildpart_time_list[jdx]
				)
		if idx == 0:
			ax.set_ylabel("Elapsed Time (s)", fontsize=default_fontsize)

		if workload == "b":
			ax.set_yticks(np.arange(0, 2.1, 0.5))
		if workload == "t" or workload == "h":
			ax.set_yticks(np.arange(0, 25, 5))
		if workload == "p":
			ax.set_yticks(np.arange(0, 125, 25))

	fig.legend(legend_bars, legend_names, 
		loc="upper center", bbox_to_anchor=(0.5, 1.18),
		fancybox=False, shadow=False, frameon=False,
		ncol=algo_num, fontsize=default_fontsize, columnspacing=1.15
	)

	plt.savefig(os.path.join(FIG_PATH, "fndly_{}_{}.png".format("_".join(workload_list), mem_type)), bbox_inches="tight", format="png")
	# plt.title("", fontsize=default_fontsize)
	plt.savefig(os.path.join(FIG_PATH, "fndly_{}_{}.pdf".format("_".join(workload_list), mem_type)), bbox_inches="tight", format="pdf")
	os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
			os.path.join(FIG_PATH, "fndly_{}_{}.pdf".format("_".join(workload_list), mem_type)),
			os.path.join(FIG_PATH, "fndly_{}_{}.eps".format("_".join(workload_list), mem_type))
		)
	)

	plt.close()




if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)
	plot_workload_group(workload_group_5_list)
	plot_workload_group(workload_group_7_list)



























