import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import sys

from utils import *

LOG_PATH = "../../logs/nphj-alignment-runtime-logs/"
FIG_PATH = "../../figs/nphj-alignment-runtime-figs/"
WARMUP_FLAG = True


x_axis_label_list = [
	"unaligned", 
	"64B", 
	"256B", 
	"bkt4", 
	"bkt4$\\blacktriangledown$"
	# "bkt4(7)"
]


log_file_list = [
	"nphj_sc_unaligned_nvm.log",
	"nphj_sc_aligned_nvm.log",
	"nphj_sc_xpline_aligned_nvm.log",
	"nphj_sc_xpline_aligned_bkt4_nvm.log",
	"nphj_sc_xpline_aligned_bkt4_7thr_nvm.log"
]

legend_list = [
	"unaligned", 
	"64B",
	"256B",
	"256B-Bkt4",
	"256B-Bkt4$\\blacktriangledown$"
]


def list_packing(buildpart_time, probejoin_time, total_time, idx):
	buildpart_time_list = [0] * len(x_axis_label_list)
	probejoin_time_list = [0] * len(x_axis_label_list)
	total_time_list = [0] * len(x_axis_label_list)

	buildpart_time_list[idx] = buildpart_time
	probejoin_time_list [idx] = probejoin_time
	total_time_list[idx] = total_time

	return buildpart_time_list, probejoin_time_list, total_time_list


if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	unanligned_list = []
	aligned_list = []
	xp_aligned_list = []
	bkt4_list = []
	bkt4_7thr_list = []

	general_time_list = [
		unanligned_list,
		aligned_list,
		xp_aligned_list,
		bkt4_list,
		bkt4_7thr_list
	]

	for idx, log_file in enumerate(log_file_list):
		buildpart_time, probejoin_time, total_time = get_data_from_log(os.path.join(LOG_PATH, log_file))
		general_time_list[idx] = list_packing(buildpart_time, probejoin_time, total_time, idx)


	x_axis = np.arange(len(x_axis_label_list))
	
	fig = plt.figure(figsize=(4.5, 3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("NPHJ runtime w.r.t. bucket alignment", fontsize=14)

	for idx, time_list in enumerate(general_time_list):
		plt.bar(x_axis, time_list[2], color="none", edgecolor=color_list[idx], hatch=hatch_list[idx], label=legend_list[idx])


	'''
	# draw hatch
	ax1.bar(range(1, 5), range(1, 5), color='none', edgecolor='red', hatch="/", lw=1., zorder = 0)
	# draw edge
	ax1.bar(range(1, 5), range(1, 5), color='none', edgecolor='k', zorder=1, lw=2.)
	'''


	# plt.grid(True)
	# plt.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	plt.legend(loc="upper center", bbox_to_anchor=(0.495, 1.445), 
		fancybox=True, shadow=True, frameon=False,
		ncol=2, fontsize=14, 
		labelspacing=0.3, columnspacing=1.6, 
		handlelength=1.5, handleheight=1
	)
	plt.ylabel("Elapsed Time (s)", fontsize=14)
	# plt.xlabel("Bucket Alignment Configuration", fontsize=14)
	plt.xticks([])
	# plt.xticks([index for index in x_axis], x_axis_label_list, fontsize=14)
	plt.yticks(fontsize=14)
	# plt.yticks([2, 4, 6, 8], fontsize=14)
	# plt.ylim(top=10)
	# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "nphj_ali_runtime.png"), bbox_inches="tight", format="png")
	# plt.title("", fontsize=20)
	# plt.savefig(os.path.join(FIG_PATH, "nphj_ali_runtime.pdf"), bbox_inches="tight", format="pdf")
	# os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
	# 		os.path.join(FIG_PATH, "nphj_ali_runtime.pdf"),
	# 		os.path.join(FIG_PATH, "nphj_ali_runtime.eps")
	# 	)
	# )









