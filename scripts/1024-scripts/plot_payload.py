import collections
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import pandas as pd
import seaborn as sns
import sys

from utils import *

LOG_PATH = "../../logs/1022-logs/"
FIG_PATH = "../../figs/1024-figs/"

mem_type = "nvm"

algo_list = [
	"nphj", 
	# "shrll", "shrcm", "indll", "indcm",
	"rdx", "asym",
	"ptr_nphj", "ptr_rdx"
]
algo_num = len(algo_list)

payload_algo2logname = {
	"nphj": "nphj_sc",
	# "shrll": "shrll_hro",
	# "shrcm": "shrcm_bc",
	# "indll": "indll_hro",
	# "indcm": "indcm_bc",
	"rdx": "rdx_bc",
	"asym": "asym_bc",
	"ptr_nphj": "ptr_nphj_sc",
	"ptr_rdx": "ptr_rdx_bc"
}

payload_algo2legend = {
	"nphj": "NPHJ",
	"shrll": "SHRll",
	"shrcm": "SHRcm",
	"indll": "INDll",
	"indcm": "INDcm",
	"rdx": "RDX",
	"asym": "ASYM",
	"ptr_nphj": "NPHJ${_p}$",
	"ptr_rdx": "RDX${_p}$"
}

payload_buildpart2color = {
	"nphj": "yellow",
	"shrll": "mediumslateblue",
	"shrcm": "lightcoral",
	"indll": "peru",
	"indcm": "navajowhite",
	"rdx": "lightpink",
	"asym": "lightgreen",
	"ptr_nphj": "paleturquoise",
	"ptr_rdx": "orchid"
}

payload_probejoin2color = {
	"nphj": "darkkhaki",
	"shrll": "slateblue",
	"shrcm": "indianred",
	"indll": "saddlebrown",
	"indcm": "tan",
	"rdx": "palevioletred",
	"asym": "darkseagreen",
	"ptr_nphj": "darkturquoise",
	"ptr_rdx": "darkorchid"
}

payload_algo2hatch = {
	"nphj": "",
	"shrll": "//",
	"shrcm": "\\\\",
	"indll": "//|",
	"indcm": "\\\\|",
	"rdx": "||",
	"asym": "xx",
	"ptr_nphj": "..",
	"ptr_rdx": "**"
}


if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	default_fontsize = 14

	x_axis_label_list = [
		"16", "32", "64", "128", "256", "512"
	]
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
	plt.title("payload {}".format(mem_type))
	for idx, algo in enumerate(algo_list):
		filepath = os.path.join(
			LOG_PATH, 
			"{}_payload_{}.log".format(payload_algo2logname[algo], mem_type)
		)
		buildpart_time_list, probejoin_time_list, total_time_list = \
			get_data_from_log(filepath)

		plt.bar(x_axis+x_starting_idx_list[idx], buildpart_time_list,
			width=bar_width, 
			edgecolor="black", color=payload_buildpart2color[algo],
			hatch=payload_algo2hatch[algo], label=payload_algo2legend[algo]
		)
		plt.bar(x_axis+x_starting_idx_list[idx], probejoin_time_list,
			width=bar_width, 
			edgecolor="black", color=payload_probejoin2color[algo],
			hatch=payload_algo2hatch[algo], bottom=buildpart_time_list
		)

		if algo.startswith("ptr"):
			retrieve_time_list = np.array(total_time_list) \
				- np.array(buildpart_time_list) - np.array(probejoin_time_list)
			plt.bar(x_axis+x_starting_idx_list[idx], retrieve_time_list,
				width=bar_width, edgecolor="black", color="gray",
				hatch=payload_algo2hatch[algo], 
				bottom=np.array(buildpart_time_list) + np.array(probejoin_time_list)
			)

			print(algo, 
				np.array(total_time_list) - np.array(buildpart_time_list) - np.array(probejoin_time_list)
			)

		if algo == "rdx":
			plt.text(
				x_axis[-1]+x_starting_idx_list[idx]-0.32,
				16.15,
				str ( round(total_time_list[-1], 1) ),
				fontsize=default_fontsize-4
			)
		if algo == "asym":
			plt.text(
				x_axis[-1]+x_starting_idx_list[idx]-0.05,
				16.15,
				str ( round(total_time_list[-1], 1) ),
				fontsize=default_fontsize-3
			)

	plt.grid(True)
	plt.ylim(0, 16)

	plt.legend(
		loc="upper left", bbox_to_anchor=(0, 1.03),
		fontsize=default_fontsize, 
		ncol=1, columnspacing=1,
		fancybox=False, shadow=False
	)

	plt.xlabel("Tuple Size [Byte]", fontsize=default_fontsize)
	plt.ylabel("Elapsed Time (s)", fontsize=default_fontsize)
	plt.xticks([index for index in x_axis], x_axis_label_list, fontsize=default_fontsize)
	plt.yticks(fontsize=default_fontsize)

	plt.savefig(os.path.join(FIG_PATH, "payload_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	plt.title("", fontsize=default_fontsize)
	plt.savefig(os.path.join(FIG_PATH, "payload_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	os.system("pdftops -eps {} {}".format(
			os.path.join(FIG_PATH, "payload_{}.pdf".format(mem_type)),
			os.path.join(FIG_PATH, "payload_{}.eps".format(mem_type))
		)
	)

	plt.close()

























