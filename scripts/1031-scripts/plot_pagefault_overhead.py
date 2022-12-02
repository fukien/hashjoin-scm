import collections
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import pandas as pd
import seaborn as sns
import sys

from utils import *


mem_type = "nvm"

LOG_PATH = "../../logs/1031-logs/"
FIG_PATH = "../../figs/1031-figs/"

algo_list = ["nphj", "rdx"]
algo_num = len(algo_list)


if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	default_fontsize = 15

	x_axis_label_list = [
		# "pre-fault", 
		"w/ pre-fault",
		# "pre-populate", 
		# "pre-zero", 

		# "on-the-fly",
		"w/o pre-fault"
	]
	x_axis = np.arange(len(x_axis_label_list))

	x_data_width = 0.8
	bar_width = x_data_width/algo_num * 3/4

	x_starting_idx_list = [ 
		-x_data_width/2 + \
		(i+1/2) * x_data_width/algo_num \
		for i in range(algo_num) 
	]


	fig = plt.figure(figsize=(3.5, 3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("pagefault {}".format(mem_type))

	for idx, algo in enumerate(algo_list):
		buildpart_time_list = []
		probejoin_time_list = []

		filepath = os.path.join(
			LOG_PATH, 
			"{}_prepopulate_{}.log".format(algo2logname[algo], mem_type)
		)
		buildpart_time, probejoin_time, _ =  get_data_from_log(filepath)
		buildpart_time_list.append(buildpart_time)
		probejoin_time_list.append(probejoin_time)

		filepath =  os.path.join(
			LOG_PATH, 
			"{}_unprepopulate_{}.log".format(algo2logname[algo], mem_type)
		)
		buildpart_time, probejoin_time, _ =  get_data_from_log(filepath)
		buildpart_time_list.append(buildpart_time)
		probejoin_time_list.append(probejoin_time)

		buildpart_time_list = np.array(buildpart_time_list)
		probejoin_time_list = np.array(probejoin_time_list)

		plt.bar(
			x_axis+x_starting_idx_list[idx], buildpart_time_list,
			width=bar_width, 
			edgecolor="black", color=buildpart2color[algo],
			hatch=algo2hatch[algo], label=algo2legend[algo]
		)
		plt.bar(x_axis+x_starting_idx_list[idx], probejoin_time_list,
			width=bar_width, 
			edgecolor="black", color=probejoin2color[algo],
			hatch=algo2hatch[algo], bottom=buildpart_time_list
		)

	plt.grid()
	# plt.ylim(0, 16)

	plt.legend(
		loc="upper left", bbox_to_anchor=(0, 1.18),
		fontsize=default_fontsize, 
		ncol=2, columnspacing=1,
		fancybox=False, shadow=False, frameon=False
	)
	# plt.legend(loc=0, ncol=2, columnspacing=0.8)

	plt.ylabel("Elapsed Time (s)", fontsize=default_fontsize)

	plt.xticks([index for index in x_axis], x_axis_label_list, fontsize=default_fontsize)

	plt.yticks(fontsize=default_fontsize)

	plt.savefig(os.path.join(FIG_PATH, "pagefault_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	plt.title("", fontsize=default_fontsize)
	plt.savefig(os.path.join(FIG_PATH, "pagefault_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	os.system("pdftops -eps {} {}".format(
			os.path.join(FIG_PATH, "pagefault_{}.pdf".format(mem_type)),
			os.path.join(FIG_PATH, "pagefault_{}.eps".format(mem_type))
		)
	)

	plt.close()
