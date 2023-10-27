import collections
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from matplotlib.gridspec import GridSpec
from matplotlib.ticker import FormatStrFormatter
import numpy as np
import os
import pandas as pd
import seaborn as sns
import sys

# from utils import *

FIG_PATH = "../../figs/"
mem_type = "nvm"
WARMUP_FLAG = True


def round4decimals(value):
	return round(float(value), 4)

def get_average(timelist):
	return sum(timelist)/len(timelist)

def get_persist_data_from_log(access_granularity):
	filepath = os.path.join("../../logs/persist-cost-logs/", "persist_cost_{}.log".format(access_granularity))
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
			
			if line.startswith("elapsed"):
				line = line.strip().split(" ")
				tmp_list.append(float(line[-1]))

	# print(regular_list)
	# print(clflush_list)
	# print(clflushopt_list)
	# print(ntstore_list)
	# print(regular_fence_list)
	# print(clflush_fence_list)
	# print(clflushopt_fence_list)
	# print(ntstore_fence_list)

	if WARMUP_FLAG:
		del regular_list[0]
		del clflush_list[0]
		del clflushopt_list[0]
		del ntstore_list[0]
		del regular_fence_list[0]
		del clflush_fence_list[0]
		del clflushopt_fence_list[0]
		del ntstore_fence_list[0]
	return get_average(regular_list), \
		get_average(clflush_list), \
		get_average(clflushopt_list), \
		get_average(ntstore_list), \
		get_average(regular_fence_list), \
		get_average(clflush_fence_list), \
		get_average(clflushopt_fence_list), \
		get_average(ntstore_fence_list)

def get_pagefault_data_from_log(filepath):
	filepath = os.path.join(filepath)
	mgmt_time_percentage_list = []
	write_time_percentage_list = []
	read_time_percentage_list = []
	other_time_percentage_list = []

	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break
			if line.startswith("total_time"):
				line = line.strip().split("\t")
				total_time = float(line[0].split()[-1])
				mgmt_time = float(line[1].split()[-1])
				write_time = float(line[2].split()[-1])
				read_time = float(line[3].split()[-1])
				other_time = total_time - mgmt_time - write_time - read_time;
				mgmt_time_percentage_list.append(mgmt_time/total_time)
				write_time_percentage_list.append(write_time/total_time)
				read_time_percentage_list.append(read_time/total_time)
				other_time_percentage_list.append(other_time/total_time)

	# return mgmt_time_percentage_list, write_time_percentage_list, read_time_percentage_list, other_time_percentage_list

	if WARMUP_FLAG:
		del mgmt_time_percentage_list[0]
		del write_time_percentage_list[0]
		del read_time_percentage_list[0]
		del other_time_percentage_list[0]
	return sum(mgmt_time_percentage_list)/len(mgmt_time_percentage_list), \
		sum(write_time_percentage_list)/len(write_time_percentage_list),  \
		sum(read_time_percentage_list)/len(read_time_percentage_list), \
		sum(other_time_percentage_list)/len(other_time_percentage_list)

def get_tpch_ssb_data_log(filepath, size_dict):
	outer_size = 0
	outer_no = 0
	inner_size = 0
	inner_no = 0
	with open(filepath, "r") as f:
		line = f.readline()		# skip the first line		
		while True:
			line = f.readline()
			if not line:
				break
			line = line.strip().split(",")
			outer_size = int(line[2])
			outer_no = int(line[3])
			inner_size = int(line[4])
			inner_no = int(line[5])

			if outer_size in size_dict:
				size_dict[outer_size] += outer_no
			else:
				size_dict.setdefault(outer_size, outer_no)
			if inner_size in size_dict:
				size_dict[inner_size] += inner_no
			else:
				size_dict.setdefault(inner_size, inner_no)





if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)
	default_fontsize = 14

	fig = plt.figure(figsize=(15, 2.75), constrained_layout=True)
	# plt.subplots_adjust(wspace=0.5)
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")

	# gs = GridSpec(2, 23, height_ratios=[4, 1], figure=fig)
	gs = GridSpec(10, 23, figure=fig,)

	ax0 = fig.add_subplot(gs[:, 0:10])
	ax1 = fig.add_subplot(gs[:8, 11:15])
	ax2 = fig.add_subplot(gs[3:, 16:])
	ax3 = fig.add_subplot(gs[8:, 11:15])

	# hide the spines between ax and ax2
	ax1.spines['bottom'].set_visible(False)
	ax3.spines['top'].set_visible(False)
	ax3.xaxis.tick_bottom()
	ax1.axes.get_xaxis().set_visible(False)
	plt.subplots_adjust(hspace=0.0001, wspace=5)


	ax0.set_ylabel("Elapsed Time (s)", fontsize=default_fontsize)
	ax1.set_ylabel("Frequency", fontsize=default_fontsize)

	ax0.set_xlabel("Access Size [Byte]", fontsize=default_fontsize)
	ax2.set_xlabel("Nomarlized Elapsed Time (s)", fontsize=default_fontsize)
	ax3.set_xlabel("Tuple Size [Byte]", fontsize=default_fontsize)


	ax0.tick_params(axis='y', labelsize=default_fontsize)
	ax1.tick_params(axis='y', labelsize=default_fontsize)
	ax2.tick_params(axis='y', labelsize=default_fontsize)
	ax3.tick_params(axis='y', labelsize=default_fontsize)

	ax0.tick_params(axis='x', labelsize=default_fontsize)
	ax1.tick_params(axis='x', labelsize=default_fontsize)
	ax2.tick_params(axis='x', labelsize=default_fontsize)
	ax3.tick_params(axis='x', labelsize=default_fontsize)

	ax0.set_title("(a) Runtime w.r.t. Access Size", fontsize=default_fontsize, fontweight="bold", 
		y=-0.4
	)
	ax2.set_title("(c) Execution Time Breakdown", fontsize=default_fontsize, fontweight="bold", 
		y=-0.56
	)
	ax3.set_title("(b) Tuple Size Distribution", fontsize=default_fontsize, fontweight="bold", 
		y=-2.
	)

	access_granularity_list = [
		"16", 
		"32", 
		"64", 
		"128", 
		"256"
	]

	regular_time_list = []
	clflush_time_list = []
	clflushopt_time_list = []
	ntstore_time_list = []
	regular_fence_time_list = []
	clflush_fence_time_list = []
	clflushopt_fence_time_list = []
	ntstore_fence_time_list = []

	for access_granularity in access_granularity_list:
		regular_time, clflush_time, clflushopt_time, ntstore_time, \
		regular_fence_time, clflush_fence_time, clflushopt_fence_time, ntstore_fence_time = get_persist_data_from_log(access_granularity)
		regular_time_list.append(regular_time)
		clflush_time_list.append(clflush_time)
		clflushopt_time_list.append(clflushopt_time)
		ntstore_time_list.append(ntstore_time)
		regular_fence_time_list.append(regular_fence_time)
		clflush_fence_time_list.append(clflush_fence_time)
		clflushopt_fence_time_list.append(clflushopt_fence_time)
		ntstore_fence_time_list.append(ntstore_fence_time)

	general_time_list = [
		regular_time_list,
		# clflush_time_list,
		# clflushopt_time_list,
		# ntstore_time_list,
		# regular_fence_time_list,
		clflush_fence_time_list,
		clflushopt_fence_time_list,
		ntstore_fence_time_list
	]

	access_color_list = [
		"coral", "chartreuse", "navajowhite", "lightskyblue",
		# "coral", "chartreuse", "navajowhite", "lightskyblue"
	]
	access_hatch_list = [
		# "", "", "", "",
		"", "//", "--", "xx"
		# "xx", "xx", "xx", "xx"
	]
	access_legend_list = [
		"store",
		"clflush", "clwb", "ntstore"
	]

	x_axis = np.arange(len(access_granularity_list))
	width = 0.6/len(general_time_list)
	x_starting_idx_list = [ (-0.4 + i * 0.8/len(general_time_list)) for i in range(len(general_time_list)) ]

	for idx, time_list in enumerate(general_time_list):
		ax0.bar(x_axis+x_starting_idx_list[idx], time_list, width=width, edgecolor="black",
			color=access_color_list[idx], hatch=access_hatch_list[idx], label=access_legend_list[idx])

	ax0.set_xticks([i for i in range(len(access_granularity_list))])
	ax0.set_xticklabels(access_granularity_list)
	ax0.legend( 
		loc=0, 
		# bbox_to_anchor=(0.38, 0.755),
		fancybox=True, shadow=True, frameon=True,
		ncol=2, fontsize=default_fontsize, handletextpad=0.5, borderpad=0.2, 
		columnspacing=1, labelspacing=0.04,
	)


	ssb_size_dict = {}
	get_tpch_ssb_data_log(os.path.join("../../logs/tup-dist-logs/ssb-join.csv"), ssb_size_dict)
	ssb_od = collections.OrderedDict(sorted(ssb_size_dict.items()))
	ssb_size_list = np.array(list(ssb_od.keys()))
	ssb_freq_list = np.array(list(ssb_od.values()) ) / sum(list(ssb_od.values()))

	tpch_size_dict = {}
	get_tpch_ssb_data_log(os.path.join("../../logs/tup-dist-logs/tpch-join.csv"), tpch_size_dict)
	tpch_od = collections.OrderedDict(sorted(tpch_size_dict.items()))
	tpch_size_list = np.array(list(tpch_od.keys()))
	tpch_freq_list = np.array(list(tpch_od.values()) ) / sum(list(tpch_od.values()))

	print("SSB:", ssb_size_list.shape, "TPC-H:", tpch_size_list.shape)
	print("SSB:", ssb_freq_list.shape, "TPC-H:", tpch_freq_list.shape)
	print("SSB:", sum(list(ssb_freq_list)), "TPC-H:", sum(list(tpch_freq_list)))


	ax1.bar(ssb_size_list, ssb_freq_list, edgecolor="black", color="aqua", width=10, label="SSB", hatch="//")
	ax1.bar(tpch_size_list, tpch_freq_list, edgecolor="black", color="hotpink", width=10, label="TPC-H")
	ax3.bar(ssb_size_list, ssb_freq_list, edgecolor="black", color="aqua", width=10, label="SSB", hatch="//")
	ax3.bar(tpch_size_list, tpch_freq_list, edgecolor="black", color="hotpink", width=10, label="TPC-H")
	ax1.set_ylim(0.015, 0.84)
	ax3.set_ylim(0, 0.0099)

	d = .025  # how big to make the diagonal lines in axes coordinates
	# arguments to pass to plot, just so we don't keep repeating them
	kwargs = dict(transform=ax1.transAxes, color='k', clip_on=False)
	# ax.plot((-d, +d), (-d, +d), **kwargs)        # top-left diagonal
	# ax.plot((1 - d, 1 + d), (-d, +d), **kwargs)  # top-right diagonal
	ax1.plot((-d, +d), (0, 0), **kwargs)        # top-left horizontal
	ax1.plot((1 - d, 1 + d), (0, 0), **kwargs)  # top-right horizontal

	kwargs.update(transform=ax3.transAxes)  # switch to the bottom axes
	# ax3.plot((-d, +d), (1 - d, 1 + d), **kwargs)  # bottom-left diagonal
	# ax3.plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)  # bottom-right diagonal
	ax3.plot((-d, +d), (1, 1), **kwargs)  # bottom-left diagonal
	ax3.plot((1 - d, 1 + d), (1, 1), **kwargs)  # bottom-right diagonal

	ax1.set_yticks([0.10, 0.20, 0.40, 0.60, 0.80])
	ax1.yaxis.set_major_formatter(FormatStrFormatter('%.2f'))
	ax1.yaxis.set_tick_params(labelsize=default_fontsize)
	# ax3.set_yticks([0, 0.005, 0.010])
	ax3.set_yticks([0, 0.01])
	ax3.axhline(y=0.01, color="black", linewidth=2, linestyle="--")
	ax3.set_xticks([0, 64, 128, 256])
	ax3.yaxis.set_major_formatter(FormatStrFormatter('%.2f'))
	ax3.yaxis.set_tick_params(labelsize=default_fontsize)
	ax3.xaxis.set_tick_params(labelsize=default_fontsize)

	ax1.yaxis.set_label_coords(-0.34, 0.3)

	ax1.legend(
		loc=0,
		fontsize=default_fontsize, handletextpad=0.5, borderpad=0.2, 
		fancybox=True, shadow=True, frameon=True, labelspacing=0.04,
	)



	ax2.set_ylim(0, 0.8)
	y_axis_list = [0.15, 0.4, 0.65]
	ax2.set_yticks(y_axis_list)
	ax2.set_yticklabels([
		"DRAM",
		# "DRAM (ALIGNMENT)",
		"DRAM \n (HUGE)", "SCM"
	])
	ax2.set_xticks([0, 0.25, 0.5, 0.75, 1.0])
	ax2.grid(True)

	dram_list = get_pagefault_data_from_log( "../../logs/pagefault-cost-logs/page_fault_cost_1_dram.log")
	dram_huge_list = get_pagefault_data_from_log("../../logs/pagefault-cost-logs/page_fault_cost_1_dram_huge.log")
	nvm_list = get_pagefault_data_from_log("../../logs/pagefault-cost-logs/page_fault_cost_1_nvm.log")

	mgmt_time_percentage_list = [ dram_list[0], 
		# dram_alignment_list[0],
		dram_huge_list[0], nvm_list[0] ]
	write_time_percentage_list = [ dram_list[1], 
		# dram_alignment_list[1],
		dram_huge_list[1], nvm_list[1] ]
	read_time_percentage_list = [ dram_list[2], 
		# dram_alignment_list[2],
		dram_huge_list[2], nvm_list[2] ]
	other_time_percentage_list = [ dram_list[3], 
		# dram_alignment_list[3],
		dram_huge_list[3], nvm_list[3] ]

	plt.rcParams["hatch.linewidth"] = 2

	height = 0.2

	ax2.barh(y_axis_list, mgmt_time_percentage_list, 
		height=height, 
		color="darkorange",
		edgecolor="none", 
		hatch="--", label="page fault",
		# left=np.array(other_time_percentage_list)
	)
	ax2.barh(y_axis_list, write_time_percentage_list, 
		height=height, 
		color="deepskyblue",
		edgecolor="none", 
		hatch="xx", label="write", 
		left=np.array(mgmt_time_percentage_list)
	)
	ax2.barh(y_axis_list, read_time_percentage_list, 
		height=height,
		color="orangered",
		edgecolor="none", 
		hatch="++", label="read", 
		left=np.array(mgmt_time_percentage_list)+np.array(write_time_percentage_list)
	)
	ax2.barh(y_axis_list, other_time_percentage_list, 
		height=height, 
		color="limegreen",
		edgecolor="none", 
		hatch="//", label="others", 
		left=np.array(mgmt_time_percentage_list)+np.array(write_time_percentage_list)+np.array(read_time_percentage_list)
	)

	ax2.legend(
		# loc=0, 
		bbox_to_anchor=(0.98, 1.45),
		fancybox=True, shadow=True, frameon=True,
		ncol=2, fontsize=default_fontsize, borderpad=0.2,
		columnspacing=1, labelspacing=0.04,
	)


	plt.savefig(os.path.join(FIG_PATH, "fig_1.png"), bbox_inches="tight", format="png")

	plt.close()