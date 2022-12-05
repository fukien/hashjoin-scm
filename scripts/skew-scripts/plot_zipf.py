import collections
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import pandas as pd
import seaborn as sns
import sys

from utils import *

PHJ_LOG_PATH = "../../logs/skew-logs/"
NPHJ_LOG_PATH = "../../logs/skew-logs/"
FIG_PATH = "../../figs/skew-figs/"

mem_type = "nvm"

algo_list = ["nphj", "shrll", "indll", "rdx", "asym"]
algo_num = len(algo_list)


if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	default_fontsize = 14

	x_axis_label_list = ["1.05", "1.25", "1.50", "1.75"]
	x_axis = np.arange(len(x_axis_label_list))

	x_data_width = 0.8
	bar_width = x_data_width/algo_num * 3/4

	x_starting_idx_list = [ 
		-x_data_width/2 + \
		(i+1/2) * x_data_width/algo_num \
		for i in range(algo_num) 
	]

	fig = plt.figure(figsize=(6.5, 3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("zipf {}".format(mem_type))
	for idx, algo in enumerate(algo_list):
		if algo.startswith("nphj"):
			filepath = os.path.join(
				NPHJ_LOG_PATH, 
				"{}_zipf_{}.log".format(algo2logname[algo], mem_type)
			)
		else:
			filepath = os.path.join(
				PHJ_LOG_PATH, 
				"{}_zipf_{}.log".format(algo2logname[algo], mem_type)
			)
		buildpart_time_list, probejoin_time_list, total_time_list = \
			get_data_from_log(filepath)

		plt.bar(x_axis+x_starting_idx_list[idx], buildpart_time_list,
			width=bar_width, edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], label=algo2legend[algo]
		)
		plt.bar(x_axis+x_starting_idx_list[idx], probejoin_time_list,
			width=bar_width, edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], bottom=buildpart_time_list
		)
		if algo == "shrll":
			for jdx in range(len(x_axis)):
				plt.text(
					x_axis[jdx]+x_starting_idx_list[idx]-0.2,
					4.1,
					str( round(total_time_list[jdx], 2) )
				)

	plt.ylim(0, 4.0)

	plt.grid(True)

	plt.legend(
		loc=0, fontsize=default_fontsize, 
		ncol=2, columnspacing=1,
		fancybox=False, shadow=False
	)

	plt.xlabel("Skew", fontsize=default_fontsize)
	plt.ylabel("Elapsed Time (s)", fontsize=default_fontsize)
	plt.xticks([index for index in x_axis], x_axis_label_list, fontsize=default_fontsize)
	plt.yticks(fontsize=default_fontsize)

	plt.savefig(os.path.join(FIG_PATH, "zipf_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	# plt.title("", fontsize=default_fontsize)
	# plt.savefig(os.path.join(FIG_PATH, "zipf_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	# os.system("pdftops -eps {} {}".format(
	# 		os.path.join(FIG_PATH, "zipf_{}.pdf".format(mem_type)),
	# 		os.path.join(FIG_PATH, "zipf_{}.eps".format(mem_type))
	# 	)
	# )

	plt.close()

























