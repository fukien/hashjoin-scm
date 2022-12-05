import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import sys

LOG_PATH = "../../logs/persist-cost-logs/"
FIG_PATH = "../../figs/persist-cost-figs/"
WARMUP_FLAG = True

color_list = [
	"coral", "chartreuse", "navajowhite", "lightskyblue",
	"coral", "chartreuse", "navajowhite", "lightskyblue"
]
hatch_list = [
	# "", "", "", "",
	"", "//", "--", "xx"
	# "xx", "xx", "xx", "xx"
]

x_axis_label_list = [
	"16", 
	"32", 
	"64", 
	"128", 
	"256"
]

'''
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

def round4decimals(value):
	return round(float(value), 4)

def get_average(timelist):
	return sum(timelist)/len(timelist)

def get_data_from_log(access_granularity):
	filepath = os.path.join(LOG_PATH, "persist_cost_{}.log".format(access_granularity))
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





if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	regular_time_list = []
	clflush_time_list = []
	clflushopt_time_list = []
	ntstore_time_list = []
	regular_fence_time_list = []
	clflush_fence_time_list = []
	clflushopt_fence_time_list = []
	ntstore_fence_time_list = []

	for access_granularity in x_axis_label_list:
		regular_time, clflush_time, clflushopt_time, ntstore_time, \
		regular_fence_time, clflush_fence_time, clflushopt_fence_time, ntstore_fence_time = get_data_from_log(access_granularity)
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

	x_axis = np.arange(len(x_axis_label_list))
	width = 0.6/len(general_time_list)
	x_starting_idx_list = [ (-0.4 + i * 0.8/len(general_time_list)) for i in range(len(general_time_list)) ]
	fig = plt.figure(figsize=(6, 2.2))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("write instruction w.r.t. persist overhead", fontsize=14)
	for idx, time_list in enumerate(general_time_list):
		plt.bar(x_axis+x_starting_idx_list[idx], time_list, width=width, edgecolor="black",
			color=color_list[idx], hatch=hatch_list[idx], label=legend_list[idx])

	# plt.grid(True)
	# plt.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)

	plt.legend( 
		# loc="upper right", 
		bbox_to_anchor=(0.375, 0.605),
		# fancybox=True, shadow=True, frameon=True,
		ncol=2, fontsize=14, 
		columnspacing=1, labelspacing=0.04
	)

	plt.ylabel("Elapsed Time (s)", fontsize=14)
	plt.xlabel("Access Size [Byte]", fontsize=14)
	plt.xticks([index for index in x_axis], x_axis_label_list, fontsize=14)
	plt.yticks([2, 4, 6, 8], fontsize=14)
	plt.ylim(top=10)
	# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "elapsed-time.png"), bbox_inches="tight", format="png")
	# plt.title("", fontsize=20)
	# plt.savefig(os.path.join(FIG_PATH, "elapsed-time.pdf"), bbox_inches="tight", format="pdf")
	# os.system("pdftops -eps {} {}".format(
	# 		os.path.join(FIG_PATH, "elapsed-time.pdf"),
	# 		os.path.join(FIG_PATH, "elapsed-time.eps")
	# 	)
	# )









