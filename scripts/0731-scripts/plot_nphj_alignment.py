import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import sys

from utils import *

LOG_PATH = "../../logs/0731-logs/"
FIG_PATH = "../../figs/0731-figs/"


x_axis_label_list = [
	"unaligned", 
	"aligned", 
	"xpline_aligned", 
	"xpline_aligned_bkt4", 
	"xpline_aligned_bkt8"
]



legend_list = [
	"store", 
	"clflush", "clwb", "ntstore", 
	"store + fence", 
	"clflush + fence", "clwb + fence", "ntstore + fence"
]


'''
legend_list = [
	"store",
	"clflush", "clwb", "ntstore"
]
'''



def plot_time(mem_type):
	regular_time_list = [[],[],[]]
	clflush_time_list = [[],[],[]]
	clflushopt_time_list = [[],[],[]]
	ntstore_time_list = [[],[],[]]
	regular_fence_time_list = [[],[],[]]
	clflush_fence_time_list = [[],[],[]]
	clflushopt_fence_time_list = [[],[],[]]
	ntstore_fence_time_list = [[],[],[]]


	for alignment in x_axis_label_list:
		regular_time, clflush_time, clflushopt_time, ntstore_time, \
		regular_fence_time, clflush_fence_time, clflushopt_fence_time, ntstore_fence_time = get_data_from_log(alignment, mem_type)

		regular_time_list[0].append(regular_time[0])
		regular_time_list[1].append(regular_time[1])
		regular_time_list[2].append(regular_time[2])
		clflush_time_list[0].append(clflush_time[0])
		clflush_time_list[1].append(clflush_time[1])
		clflush_time_list[2].append(clflush_time[2])
		clflushopt_time_list[0].append(clflushopt_time[0])
		clflushopt_time_list[1].append(clflushopt_time[1])
		clflushopt_time_list[2].append(clflushopt_time[2])
		regular_fence_time_list[0].append(regular_fence_time[0])
		regular_fence_time_list[1].append(regular_fence_time[1])
		regular_fence_time_list[2].append(regular_fence_time[2])
		clflush_fence_time_list[0].append(clflush_fence_time[0])
		clflush_fence_time_list[1].append(clflush_fence_time[1])
		clflush_fence_time_list[2].append(clflush_fence_time[2])
		clflushopt_fence_time_list[0].append(clflushopt_fence_time[0])
		clflushopt_fence_time_list[1].append(clflushopt_fence_time[1])
		clflushopt_fence_time_list[2].append(clflushopt_fence_time[2])

		if alignment == "unaligned":
			ntstore_time_list[0].append(0)
			ntstore_time_list[1].append(0)
			ntstore_time_list[2].append(0)
			ntstore_fence_time_list[0].append(0)
			ntstore_fence_time_list[1].append(0)
			ntstore_fence_time_list[2].append(0)
		else:
			ntstore_time_list[0].append(ntstore_time[0])
			ntstore_time_list[1].append(ntstore_time[1])
			ntstore_time_list[2].append(ntstore_time[2])
			ntstore_fence_time_list[0].append(ntstore_fence_time[0])
			ntstore_fence_time_list[1].append(ntstore_fence_time[1])
			ntstore_fence_time_list[2].append(ntstore_fence_time[2])

	# print(regular_time_list)
	# print(clflush_time_list)
	# print(clflushopt_time_list)
	# print(ntstore_time_list)
	# print(regular_fence_time_list)
	# print(clflush_fence_time_list)
	# print(clflushopt_fence_time_list)
	# print(ntstore_fence_time_list)


	general_time_list = [
		regular_time_list,
		clflush_time_list,
		clflushopt_time_list,
		ntstore_time_list,
		regular_fence_time_list,
		clflush_fence_time_list,
		clflushopt_fence_time_list,
		ntstore_fence_time_list
	]


	x_axis = np.arange(len(x_axis_label_list))
	width = 0.6/len(general_time_list)
	x_starting_idx_list = [ (-0.4 + i * 0.8/len(general_time_list)) for i in range(len(general_time_list)) ]
	fig = plt.figure(figsize=(6.5, 3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("NPHJ_SC elapsed time w.r.t. alignment. {}".format(mem_type), fontsize=14)
	for idx, time_list in enumerate(general_time_list):
		plt.bar(x_axis+x_starting_idx_list[idx], time_list[0], width=width, edgecolor="black",
			color=color_list[idx%(len(color_list))][0], hatch=hatch_list[idx], label=legend_list[idx])
		plt.bar(x_axis+x_starting_idx_list[idx], time_list[1], width=width, edgecolor="black",
			color=color_list[idx%(len(color_list))][1], hatch=hatch_list[idx], bottom=time_list[0])



	# plt.grid(True)
	# plt.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	plt.ylabel("elapsed time (s)", fontsize=14)
	# plt.xlabel("alignment", fontsize=14)
	# plt.xticks([index for index in x_axis], x_axis_label_list, fontsize=14)
	# plt.yticks([2, 4, 6, 8], fontsize=14)
	# plt.ylim(top=10)
	# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "nphj_alignment_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	plt.title("", fontsize=20)
	plt.savefig(os.path.join(FIG_PATH, "nphj_alignment_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	os.system("pdftops -eps {} {}".format(
			os.path.join(FIG_PATH, "nphj_alignment_{}.pdf".format(mem_type)),
			os.path.join(FIG_PATH, "nphj_alignment_{}.eps".format(mem_type))
		)
	)



if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	for mem_type in memtype_list:
		plot_time(mem_type)