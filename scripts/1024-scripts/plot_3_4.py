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

FIG_PATH = "../../figs/1024-figs/"

mem_type = "nvm"

algo_list = ["nphj", "shrll", "shrcm", "indll", "indcm", "rdx", "asym"]
algo_num = len(algo_list)
zipf_algo_list = ["nphj", "shrll", "indll", "rdx", "asym"]
zipf_algo_num = len(zipf_algo_list)
general_buildpart_time_list = [[]] * algo_num
general_probejoin_time_list = [[]] * algo_num

billion_algo2logname = {
	"nphj": "nphj_sc",
	"shrll": "shrll_hro",
	"shrcm": "shrcm_bc",
	"indll": "indll_hro",
	"indcm": "indcm_bc",
	"rdx": "rdx_bw_reg_bc",
	"asym": "asym_hro"
}


if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	default_fontsize = 14

	legend_bars = []
	legend_names = ["NPHJ", "SHRll", "SHRcm", "INDll", "INDcm", "RDX", "ASYM"]

	fig = plt.figure(figsize=(14, 5), constrained_layout=True)
	# plt.subplots_adjust(wspace=0.5)
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	gs = GridSpec(2, 9, figure=fig)

	ax0 = fig.add_subplot(gs[0, 0:3])
	ax1 = fig.add_subplot(gs[0, 3:6])
	ax2 = fig.add_subplot(gs[0, 6:])
	ax3 = fig.add_subplot(gs[1, 0:2])
	ax4= fig.add_subplot(gs[1:, 2:5])
	ax5 = fig.add_subplot(gs[1, 5:7])
	ax6 = fig.add_subplot(gs[1, 7:])
	
	ax3.axes.get_xaxis().set_visible(False)
	ax5.axes.get_xaxis().set_visible(False)
	ax6.axes.get_xaxis().set_visible(False)

	title_y_coord = -0.55
	ax0.set_title("(a) Performance w.r.t. Size Difference", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
	ax1.set_title("(b) Performance w.r.t. Skewness", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
	ax2.set_title("(c) Performance w.r.t. Selectivity", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
	ax4.set_title("(e) Performance w.r.t. Density", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
	title_y_coord = -0.54
	ax3.set_title("(d) Performance on \n Duplicate Workload", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
	ax5.set_title("(f) Single-Thread \n Performance", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
	ax6.set_title("(g) Performance on \n Billion-Scale Workload", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)

	ax0.set_ylabel("Elapsed Time (s)", fontsize=default_fontsize)
	ax3.set_ylabel("Elapsed Time (s)", fontsize=default_fontsize)

	ax0.tick_params(axis='y', labelsize=default_fontsize)
	ax1.tick_params(axis='y', labelsize=default_fontsize)
	ax2.tick_params(axis='y', labelsize=default_fontsize)
	ax3.tick_params(axis='y', labelsize=default_fontsize)
	ax4.tick_params(axis='y', labelsize=default_fontsize)
	ax5.tick_params(axis='y', labelsize=default_fontsize)
	ax6.tick_params(axis='y', labelsize=default_fontsize)

	ax0.set_xlabel("Size Ratio (|R|/|S|) ", fontsize=default_fontsize)
	ax1.set_xlabel("Zipf Factor $\\theta$", fontsize=default_fontsize)
	ax2.set_xlabel("Selectivity", fontsize=default_fontsize)
	ax4.set_xlabel("Density", fontsize=default_fontsize)

	ax0.grid()
	ax1.grid()
	ax2.grid()
	ax3.grid()
	ax4.grid()
	ax5.grid()
	ax6.grid()


	# PKFK SIZE RATIO
	x_axis_label_list = ["1:16", "1:4", "1:1"]
	x_axis = np.arange(len(x_axis_label_list))

	x_data_width = 0.8
	bar_width = x_data_width/algo_num * 3/4

	x_starting_idx_list = [ 
		-x_data_width/2 + \
		(i+1/2) * x_data_width/algo_num \
		for i in range(algo_num) 
	]

	for idx, algo in enumerate(algo_list):
		filepath = os.path.join(
			"../../logs/1019-logs/", 
			"{}_pkfk_{}.log".format(algo2logname[algo], mem_type)
		)
		if algo == "nphj":
			filepath = os.path.join(
				"../../logs/1020-logs/", 
				"{}_pkfk_{}.log".format(algo2logname[algo], mem_type)
			)
		buildpart_time_list, probejoin_time_list, total_time_list = \
			get_data_from_log(filepath)

		tmp_bar = ax0.bar(x_axis+x_starting_idx_list[idx], buildpart_time_list,
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
		)
		ax0.bar(x_axis+x_starting_idx_list[idx], probejoin_time_list,
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], bottom=buildpart_time_list
		)
		legend_bars.append(tmp_bar)
		# if algo == "nphj":
		# 	print(buildpart_time_list)
		# 	print(probejoin_time_list)

	# ax0.set_ylim(0, 9.0)
	ax0.set_yticks(np.arange(0, 10, 2))
	ax0.set_xticks(np.arange(0, len(x_axis), 1))
	ax0.set_xticklabels(x_axis_label_list, fontsize=default_fontsize)



	# ZIPF
	x_axis_label_list = ["1.05", "1.25", "1.50", "1.75"]
	x_axis = np.arange(len(x_axis_label_list))

	x_data_width = 0.8
	bar_width = x_data_width/algo_num * 3/4

	x_starting_idx_list = [ 
		-x_data_width/2 + \
		(i+1/2) * x_data_width/algo_num \
		for i in range(algo_num) 
	]
	for idx, algo in enumerate(zipf_algo_list):
		filepath = os.path.join(
			"../../logs/1019-logs/", 
			"{}_zipf_{}.log".format(algo2logname[algo], mem_type)
		)
		if algo == "nphj":
			filepath = os.path.join(
				"../../logs/1020-logs/", 
				"{}_zipf_{}.log".format(algo2logname[algo], mem_type)
			)
		buildpart_time_list, probejoin_time_list, total_time_list = \
			get_data_from_log(filepath)

		ax1.bar(x_axis+x_starting_idx_list[idx], buildpart_time_list,
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
		)
		ax1.bar(x_axis+x_starting_idx_list[idx], probejoin_time_list,
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], bottom=buildpart_time_list
		)

		if algo == "shrll":
			for jdx in range(len(x_axis)):
				ax1.text(
					x_axis[jdx]+x_starting_idx_list[idx]-0.175,
					3.9,
					str( round(total_time_list[jdx], 1) )
				)

	ax1.set_ylim(0, 3.8)
	ax1.set_yticks(np.arange(0, 4, 1))
	ax1.set_xticks(np.arange(0, len(x_axis), 1))
	ax1.set_xticklabels(x_axis_label_list, fontsize=default_fontsize)


	# SELECTIVITY
	x_axis_label_list = ["0.20", "0.40", "0.60", "0.80"]
	x_axis = np.arange(len(x_axis_label_list))

	x_data_width = 0.8
	bar_width = x_data_width/algo_num * 3/4

	x_starting_idx_list = [ 
		-x_data_width/2 + \
		(i+1/2) * x_data_width/algo_num \
		for i in range(algo_num) 
	]
	for idx, algo in enumerate(algo_list):
		filepath = os.path.join(
			"../../logs/1019-logs/", 
			"{}_selectivity_{}.log".format(algo2logname[algo], mem_type)
		)
		if algo == "nphj":
			filepath = os.path.join(
				"../../logs/1020-logs/", 
				"{}_selectivity_{}.log".format(algo2logname[algo], mem_type)
			)
		buildpart_time_list, probejoin_time_list, total_time_list = \
			get_data_from_log(filepath)

		ax2.bar(x_axis+x_starting_idx_list[idx], buildpart_time_list,
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
		)
		ax2.bar(x_axis+x_starting_idx_list[idx], probejoin_time_list,
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], bottom=buildpart_time_list
		)
	ax2.set_ylim(0, 2.9)
	ax2.set_yticks(np.arange(0, 3, 0.5))
	ax2.set_xticks(np.arange(0, len(x_axis), 1))
	ax2.set_xticklabels(x_axis_label_list, fontsize=default_fontsize)


	# DUPFK
	x_axis_label_list = [algo2legend[algo] for algo in algo_list]
	x_axis = np.arange(len(x_axis_label_list))

	general_buildpart_time_list = [[]] * algo_num
	general_probejoin_time_list = [[]] * algo_num

	bar_width = 0.8

	for idx, algo in enumerate(algo_list):
		filepath = os.path.join(
			"../../logs/1019-logs/", 
			"{}_dupfk_{}.log".format(algo2logname[algo], mem_type)
		)
		if algo == "nphj":
			filepath = os.path.join(
				"../../logs/1020-logs/", 
				"{}_dupfk_{}.log".format(algo2logname[algo], mem_type)
			)
		buildpart_time_list, probejoin_time_list, total_time_list = \
			get_data_from_log(filepath)
		general_buildpart_time_list[idx], general_probejoin_time_list[idx], _ = \
			list_packing(
				buildpart_time_list[0], probejoin_time_list[0], total_time_list[0], 
				idx, algo_num
			)

		ax3.bar(
			x_axis, general_buildpart_time_list[idx],
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
		)
		ax3.bar(
			x_axis, general_probejoin_time_list[idx],
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], bottom=general_buildpart_time_list[idx]
		)

		if algo == "nphj":
			ax3.text(
				x_axis[algo_list.index(algo)]+x_starting_idx_list[idx],
				3.9,
				str( round(total_time_list[0], 1) )
			)

	ax3.set_ylim(0, 3.8)
	ax3.set_yticks(np.arange(0, 4, 1))


	# DENSITY
	x_axis_label_list = ["0.20", "0.40", "0.60", "0.80"]
	x_axis = np.arange(len(x_axis_label_list))

	x_data_width = 0.8
	bar_width = x_data_width/algo_num * 3/4

	x_starting_idx_list = [ 
		-x_data_width/2 + \
		(i+1/2) * x_data_width/algo_num \
		for i in range(algo_num) 
	]
	for idx, algo in enumerate(algo_list):
		filepath = os.path.join(
			"../../logs/1019-logs/", 
			"{}_density_{}.log".format(algo2logname[algo], mem_type)
		)
		if algo == "nphj":
			filepath = os.path.join(
				"../../logs/1020-logs/", 
				"{}_density_{}.log".format(algo2logname[algo], mem_type)
			)
		buildpart_time_list, probejoin_time_list, total_time_list = \
			get_data_from_log(filepath)

		ax4.bar(x_axis+x_starting_idx_list[idx], buildpart_time_list,
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
		)
		ax4.bar(x_axis+x_starting_idx_list[idx], probejoin_time_list,
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], bottom=buildpart_time_list
		)

		if algo == "nphj":
			ax4.text(
				x_axis[0]+x_starting_idx_list[idx]-0.1,
				5.1,
				str( round(total_time_list[0], 1) )
			)
	ax4.set_ylim(0, 5.0)
	ax4.set_yticks(np.arange(0, 5, 1))
	ax4.set_xticks(np.arange(0, len(x_axis), 1))
	ax4.set_xticklabels(x_axis_label_list, fontsize=default_fontsize)


	# Single-Thread
	x_axis_label_list = [algo2legend[algo] for algo in algo_list]
	x_axis = np.arange(len(x_axis_label_list))

	general_buildpart_time_list = [[]] * algo_num
	general_probejoin_time_list = [[]] * algo_num

	bar_width = 0.8

	for idx, algo in enumerate(algo_list):
		filepath = os.path.join(
			"../../logs/0829-logs/", 
			"st_{}_{}.log".format(algo2logname[algo], mem_type)
		)
		buildpart_time_list, probejoin_time_list, total_time_list = \
			get_data_from_log(filepath)
		general_buildpart_time_list[idx], general_probejoin_time_list[idx], _ = \
			list_packing(
				buildpart_time_list[0], probejoin_time_list[0], total_time_list[0], 
				idx, algo_num
			)

		ax5.bar(
			x_axis, general_buildpart_time_list[idx],
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
		)
		ax5.bar(
			x_axis, general_probejoin_time_list[idx],
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], bottom=general_buildpart_time_list[idx]
		)
	ax5.set_yticks(np.arange(0, 40, 10))



	# BILLION
	x_axis_label_list = [algo2legend[algo] for algo in algo_list]
	x_axis = np.arange(len(x_axis_label_list))

	general_buildpart_time_list = [[]] * algo_num
	general_probejoin_time_list = [[]] * algo_num

	for idx, algo in enumerate(algo_list):
		filepath = os.path.join(
			"../../logs/1021-logs/", 
			"{}_billion_{}.log".format(billion_algo2logname[algo], mem_type)
		)
		if algo == "rdx":
			filepath = os.path.join("../../logs/1028-logs/rdx_bc_billion_{}.log".format(mem_type))
		if algo == "asym":
			filepath = os.path.join("../../logs/1028-logs/asym_hro_billion_{}.log".format(mem_type))
		buildpart_time_list, probejoin_time_list, total_time_list = \
			get_data_from_log(filepath)
		general_buildpart_time_list[idx], general_probejoin_time_list[idx], _ = \
			list_packing(
				buildpart_time_list[0], probejoin_time_list[0], total_time_list[0], 
				idx, algo_num
			)

		ax6.bar(
			x_axis, general_buildpart_time_list[idx],
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], 
			# label=algo2legend[algo]
		)
		ax6.bar(
			x_axis, general_probejoin_time_list[idx],
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], bottom=general_buildpart_time_list[idx]
		)

		'''
		if algo == "indcm" or algo == "rdx" or algo == "asym":
			ax6.text(
				x_axis[algo_list.index(algo)]+x_starting_idx_list[idx]-0.625,
				total_time_list[0]+10,
				str( round(total_time_list[0], 1) )
			)
			# print(total_time_list[0])
		'''
	ax6.set_yticks(np.arange(0, 201, 50))


	fig.legend(legend_bars, legend_names, 
		loc="upper center", bbox_to_anchor=(0.5, 1.075),
		fancybox=True, shadow=True, frameon=False,
		ncol=algo_num, fontsize=default_fontsize, columnspacing=1.15
	)


	plt.savefig(os.path.join(FIG_PATH, "3_4_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	# plt.title("", fontsize=default_fontsize)
	plt.savefig(os.path.join(FIG_PATH, "3_4_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
			os.path.join(FIG_PATH, "3_4_{}.pdf".format(mem_type)),
			os.path.join(FIG_PATH, "3_4_{}.eps".format(mem_type))
		)
	)

	plt.close()

























