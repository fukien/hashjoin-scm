import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import sys

LOG_PATH = "../../logs/pagefault-cost-logs/"
FIG_PATH = "../../figs/pagefault-cost-figs/"
WARMUP_FLAG = True

y_axis_label_list = ["DRAM", 
	# "DRAM (ALIGNMENT)",
	"DRAM \n (HUGE)", "SCM"]

def round4decimals(value):
	return round(float(value), 4)

def get_data_from_log(filepath):
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


if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	dram_list = get_data_from_log(os.path.join(LOG_PATH, "page_fault_cost_1_dram.log"))
	# dram_alignment_list = get_data_from_log(os.path.join(LOG_PATH, "page_fault_cost_1_dram_alignment.log"))
	dram_huge_list = get_data_from_log(os.path.join(LOG_PATH, "page_fault_cost_1_dram_huge.log"))
	nvm_list = get_data_from_log(os.path.join(LOG_PATH, "page_fault_cost_1_nvm.log"))

	print(dram_list)
	print(dram_huge_list)
	print(nvm_list)

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

	fig = plt.figure(figsize=(5.8, 3.5))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("execution time breakdown", fontsize=14)

	height = 0.75

	plt.barh(y_axis_label_list, mgmt_time_percentage_list, 
		height=height, 
		color="darkorange",
		edgecolor="none", 
		hatch="-", label="page fault",
		# left=np.array(other_time_percentage_list)
	)
	plt.barh(y_axis_label_list, write_time_percentage_list, 
		height=height, 
		color="deepskyblue",
		edgecolor="none", 
		hatch="x", label="write", 
		left=np.array(mgmt_time_percentage_list)
	)
	plt.barh(y_axis_label_list, read_time_percentage_list, 
		height=height,
		color="orangered",
		edgecolor="none", 
		hatch="+", label="read", 
		left=np.array(mgmt_time_percentage_list)+np.array(write_time_percentage_list)
	)
	plt.barh(y_axis_label_list, other_time_percentage_list, 
		height=height, 
		color="limegreen",
		edgecolor="none", 
		hatch="/", label="others", 
		left=np.array(mgmt_time_percentage_list)+np.array(write_time_percentage_list)+np.array(read_time_percentage_list)
	)


	plt.grid(True)
	# plt.legend(loc=0, ncol=2, fontsize=19, columnspacing=0.5)
	plt.legend(loc="upper center", bbox_to_anchor=(0.495, 1.39), 
		fancybox=True, shadow=True, frameon=True,
		ncol=2, fontsize=21, 
		labelspacing=0.01, handlelength=1.5, columnspacing=1.5
	)
	plt.xlabel("Nomarlized Elapsed Time (s)", fontsize=21)
	
	plt.yticks(fontsize=21)
	plt.xticks([0, 0.25, 0.50, 0.75, 1.0], fontsize=21)
	# plt.xticks([0, 0.25, 0.50, 0.75, 0.85, 1.0], fontsize=13)
	# plt.axvline(x=0.85, color="r", linestyle="--")

	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "pagefault-timebreakdown-h.png"), bbox_inches="tight", format="png")
	# plt.title("", fontsize=20)
	# plt.savefig(os.path.join(FIG_PATH, "pagefault-timebreakdown-h.pdf"), bbox_inches="tight", format="pdf")
	# os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
	# 		os.path.join(FIG_PATH, "pagefault-timebreakdown-h.pdf"),
	# 		os.path.join(FIG_PATH, "pagefault-timebreakdown-h.eps")
	# 	)
	# )


















