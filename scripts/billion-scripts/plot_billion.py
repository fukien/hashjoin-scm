import collections
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import pandas as pd
import seaborn as sns
import sys

from utils import *

LOG_PATH = "../../logs/billion-logs/"
FIG_PATH = "../../figs/billion-figs/"

mem_type = "nvm"

algo_list = [
	"nphj", "shrll", "shrcm", "indll", "indcm", "rdx", "asym"
]
algo_num = len(algo_list)

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

	x_axis_label_list = [algo2legend[algo] for algo in algo_list]
	x_axis = np.arange(len(x_axis_label_list))

	'''
	x_data_width = 0.8
	bar_width = x_data_width/algo_num * 3/4

	x_starting_idx_list = [ 
		-x_data_width/2 + \
		(i+1/2) * x_data_width/algo_num \
		for i in range(algo_num) 
	]
	'''

	general_buildpart_time_list = [[]] * algo_num
	general_probejoin_time_list = [[]] * algo_num

	for idx, algo in enumerate(algo_list):
		filepath = os.path.join(
			LOG_PATH, 
			"{}_billion_{}.log".format(billion_algo2logname[algo], mem_type)
		)
		if algo == "rdx":
			filepath = os.path.join("../../logs/billion-logs/rdx_bc_billion_{}.log".format(mem_type))
		if algo == "asym":
			filepath = os.path.join("../../logs/billion-logs/asym_hro_billion_{}.log".format(mem_type))
		buildpart_time_list, probejoin_time_list, total_time_list= \
			get_data_from_log(filepath)
		general_buildpart_time_list[idx], general_probejoin_time_list[idx], _ = \
			list_packing(
				buildpart_time_list[0], probejoin_time_list[0], total_time_list[0], 
				idx, algo_num
			)

	fig = plt.figure(figsize=(6.5, 3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("billion {}".format(mem_type))
	for idx, algo in enumerate(algo_list):
		plt.bar(
			x_axis,
			# x_starting_idx_list[idx], 
			general_buildpart_time_list[idx],
			# width=bar_width, 
			edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], label=algo2legend[algo]
		)
		plt.bar(
			x_axis,
			# x_starting_idx_list[idx], 
			general_probejoin_time_list[idx],
			# width=bar_width, 
			edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], bottom=general_buildpart_time_list[idx]
		)

	plt.grid(True)

	# plt.legend(
	# 	loc=0, fontsize=default_fontsize, 
	# 	ncol=2, columnspacing=1,
	# 	fancybox=False, shadow=False
	# )

	# plt.xlabel("", fontsize=default_fontsize)
	plt.ylabel("Elapsed Time (s)", fontsize=default_fontsize)
	plt.xticks([index for index in x_axis], x_axis_label_list, fontsize=default_fontsize)
	plt.yticks(fontsize=default_fontsize)

	plt.savefig(os.path.join(FIG_PATH, "billion_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	# plt.title("", fontsize=default_fontsize)
	# plt.savefig(os.path.join(FIG_PATH, "billion_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	# os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
	# 		os.path.join(FIG_PATH, "billion_{}.pdf".format(mem_type)),
	# 		os.path.join(FIG_PATH, "billion_{}.eps".format(mem_type))
	# 	)
	# )

	plt.close()
