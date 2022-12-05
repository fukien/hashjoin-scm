import collections
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from matplotlib.gridspec import GridSpec
import numpy as np
import os
import pandas as pd
import seaborn as sns
import sys

from utils import *

NUMA_LOG_PATH = "../../logs/numa-tpch-logs/"
TPCH_LOG_PATH = "../../logs/numa-tpch-logs/"
FIG_PATH = "../../figs/numa-tpch-figs/"

mem_type = "nvm"

algo_list = ["nphj", "rdx"]
algo_num = len(algo_list)


if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	default_fontsize = 14

	legend_bars = []
	legend_names = ["NPHJ", "RDX"]

	fig = plt.figure(figsize=(7.5, 2.5), constrained_layout=True)
	# plt.subplots_adjust(wspace=0.5)
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	gs = GridSpec(1, 4, figure=fig)

	ax0 = fig.add_subplot(gs[0, 0])
	ax1 = fig.add_subplot(gs[0, 1])
	ax2 = fig.add_subplot(gs[0, 2])
	ax3 = fig.add_subplot(gs[0, 3])

	ax3.axes.get_xaxis().set_visible(False)

	title_y_coord = -0.48
	ax0.set_title("(a) Elapsed\nTime (s)", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
	ax1.set_title("(b) SCM Reads\n(MOps)", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
	ax2.set_title("(c) SCM Writes\n(MOps)", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
	ax3.set_title("(d) TPC-H Q14\nElapsed Time (s)", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)


	ax0.tick_params(axis='y', labelsize=default_fontsize)
	ax1.tick_params(axis='y', labelsize=default_fontsize)
	ax2.tick_params(axis='y', labelsize=default_fontsize)
	ax3.tick_params(axis='y', labelsize=default_fontsize)

	ax0.grid()
	ax1.grid()
	ax2.grid()
	ax3.grid()

	# x_axis_label_list = ["LOCAL", "REMOTE"]
	x_axis_label_list = ["local", "remote"]
	x_axis = np.arange(len(x_axis_label_list))

	x_data_width = 0.8
	bar_width = x_data_width/algo_num * 3/4

	x_starting_idx_list = [ 
		-x_data_width/2 + \
		(i+1/2) * x_data_width/algo_num \
		for i in range(algo_num) 
	]

	general_buildpart_time_list = [ [] for i in range(algo_num) ]
	general_probejoin_time_list = [ [] for i in range(algo_num) ]

	for idx, algo in enumerate(algo_list):
		buildpart_time, probejoin_time, _ = \
			get_data_from_log(
				os.path.join(NUMA_LOG_PATH, 
				"{}_local_{}.log".format(algo2logname[algo], mem_type)
				)
			)
		general_buildpart_time_list[idx].append(buildpart_time)
		general_probejoin_time_list[idx].append(probejoin_time)

		buildpart_time, probejoin_time, _ = \
			get_data_from_log(
				os.path.join(NUMA_LOG_PATH, 
				"{}_remote_{}.log".format(algo2logname[algo], mem_type)
				)
			)
		general_buildpart_time_list[idx].append(buildpart_time)
		general_probejoin_time_list[idx].append(probejoin_time)

		tmp_bar = ax0.bar(x_axis+x_starting_idx_list[idx], 
			general_buildpart_time_list[idx],
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
		)
		ax0.bar(x_axis+x_starting_idx_list[idx], 
			general_probejoin_time_list[idx],
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], 
			bottom=general_buildpart_time_list[idx]
		)
		legend_bars.append(tmp_bar)

	# ax0.set_ylim(0, 4.2)
	ax0.set_yticks(np.arange(0, 5, 1))
	ax0.set_xticks(np.arange(0, len(x_axis), 1))
	ax0.set_xticklabels(x_axis_label_list, fontsize=default_fontsize)

	general_buildpart_counter_list = [ [] for i in range(algo_num) ]
	general_probejoin_counter_list = [ [] for i in range(algo_num) ]
	for idx, algo in enumerate(algo_list):
		_, _, _, buildpart_counter, probejoin_counter, _ = \
			get_counter_from_log(
				os.path.join(NUMA_LOG_PATH, 
					"{}_local_{}_profiling.log".format(algo2logname[algo], mem_type)
				),
				"host_reads", "pmwatch"
			)
		general_buildpart_counter_list[idx].append(buildpart_counter)
		general_probejoin_counter_list[idx].append(probejoin_counter)

		_, _, _, buildpart_counter, probejoin_counter, _ = \
			get_counter_from_log(
				os.path.join(NUMA_LOG_PATH, 
					"{}_remote_{}_profiling.log".format(algo2logname[algo], mem_type)
				),
				"host_reads", "pmwatch"
			)
		general_buildpart_counter_list[idx].append(buildpart_counter)
		general_probejoin_counter_list[idx].append(probejoin_counter)

		ax1.bar(x_axis+x_starting_idx_list[idx], 
			general_buildpart_counter_list[idx],
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
		)
		ax1.bar(x_axis+x_starting_idx_list[idx], 
			general_probejoin_counter_list[idx],
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], 
			bottom=general_buildpart_counter_list[idx]
		)

	ax1.set_xticks(np.arange(0, len(x_axis), 1))
	ax1.set_xticklabels(x_axis_label_list, fontsize=default_fontsize)

	general_buildpart_counter_list = [ [] for i in range(algo_num) ]
	general_probejoin_counter_list = [ [] for i in range(algo_num) ]
	for idx, algo in enumerate(algo_list):
		_, _, _, buildpart_counter, probejoin_counter, _ = \
			get_counter_from_log(
				os.path.join(NUMA_LOG_PATH, 
					"{}_local_{}_profiling.log".format(algo2logname[algo], mem_type)
				),
				"host_writes", "pmwatch"
			)
		general_buildpart_counter_list[idx].append(buildpart_counter)
		general_probejoin_counter_list[idx].append(probejoin_counter)

		_, _, _, buildpart_counter, probejoin_counter, _ = \
			get_counter_from_log(
				os.path.join(NUMA_LOG_PATH, 
					"{}_remote_{}_profiling.log".format(algo2logname[algo], mem_type)
				),
				"host_writes", "pmwatch"
			)
		general_buildpart_counter_list[idx].append(buildpart_counter)
		general_probejoin_counter_list[idx].append(probejoin_counter)

		ax2.bar(x_axis+x_starting_idx_list[idx], 
			general_buildpart_counter_list[idx],
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
		)
		ax2.bar(x_axis+x_starting_idx_list[idx], 
			general_probejoin_counter_list[idx],
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], 
			bottom=general_buildpart_counter_list[idx]
		)
	ax2.set_xticks(np.arange(0, len(x_axis), 1))
	ax2.set_xticklabels(x_axis_label_list, fontsize=default_fontsize)


	x_axis_label_list = [algo2legend[algo] for algo in algo_list]
	x_axis = np.arange(len(x_axis_label_list))

	# x_data_width = 0.8
	# bar_width = x_data_width/algo_num * 3/4
	bar_width = 0.6

	# x_starting_idx_list = [ 
	# 	-x_data_width/2 + \
	# 	(i+1/2) * x_data_width/algo_num \
	# 	for i in range(algo_num) 
	# ]

	general_pushdown_time_list = [[]] * algo_num
	general_buildpart_time_list = [[]] * algo_num
	general_probejoin_time_list = [[]] * algo_num

	tpch_algo2logname = {
		"nphj": "nphj_sc",
		"shrll": "shrll_hro",
		"shrcm": "shrcm_bc",
		"indll": "indll_hro",
		"indcm": "indcm_bc",
		# "rdx": "phj_radix_bc",
		"rdx": "rdx_bc",
		"asym": "asym_bc"
	}

	for idx, algo in enumerate(algo_list):
		filepath = os.path.join(
			TPCH_LOG_PATH, 
			"tpch_{}_{}.log".format(tpch_algo2logname[algo], mem_type)
		)
		pushdown_time, buildpart_time, probejoin_time, total_time = \
			tpch_get_data_from_log(filepath)
		general_pushdown_time_list[idx], \
		general_buildpart_time_list[idx], \
		general_probejoin_time_list[idx], _ = \
			tpch_list_packing(pushdown_time, buildpart_time, probejoin_time, total_time, idx, algo_num)

		ax3.bar(
			x_axis, general_pushdown_time_list[idx],
			width=bar_width, edgecolor="black", color="gainsboro",
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
		)
		ax3.bar(
			x_axis, general_buildpart_time_list[idx],
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
			bottom=general_pushdown_time_list[idx]
		)
		ax3.bar(
			x_axis, general_probejoin_time_list[idx],
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], 
			bottom=np.array(general_pushdown_time_list[idx])+np.array(general_buildpart_time_list[idx])
		)
		ax3.text(
			x_axis[algo_list.index(algo)]-0.15,
			total_time+0.125,
			str( round(total_time, 1) ),
			fontsize=default_fontsize-2
		)

	ax3.set_ylim(0, 5.2)
	ax3.set_yticks(np.arange(0, 6, 1))
	ax3.set_xticks(np.arange(0, len(x_axis), 1))

	fig.legend(legend_bars, legend_names, 
		loc="upper center", bbox_to_anchor=(0.5, 1.15),
		fancybox=False, shadow=False, frameon=False,
		ncol=algo_num, fontsize=default_fontsize, columnspacing=1.15
	)

	plt.savefig(os.path.join(FIG_PATH, "numa_tpch_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	# plt.title("", fontsize=default_fontsize)
	plt.savefig(os.path.join(FIG_PATH, "numa_tpch_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	# os.system("pdftops -eps {} {}".format(
	# 		os.path.join(FIG_PATH, "numa_tpch_{}.pdf".format(mem_type)),
	# 		os.path.join(FIG_PATH, "numa_tpch_{}.eps".format(mem_type))
	# 	)
	# )
	# os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
	# 		os.path.join(FIG_PATH, "numa_tpch_{}.pdf".format(mem_type)),
	# 		os.path.join(FIG_PATH, "numa_tpch_{}.eps".format(mem_type))
	# 	)
	# )

	plt.close()





















