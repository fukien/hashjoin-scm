import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import sys

from utils import *

LOG_PATH = "../../logs/0803-logs/"
FIG_PATH = "../../figs/0803-figs/"
WARMUP_FLAG = True


x_axis_label_list = [
	"unaligned", 
	# "64B", 
	# "256B", 
	"bkt4", 
	"bkt4$\\blacktriangledown$"
	# "bkt4(7)"
]

log_file_list = [
	"nphj_sc_unaligned_nvm.log",
	# "nphj_sc_aligned_nvm.log",
	# "nphj_sc_xpline_aligned_nvm.log",
	"nphj_sc_xpline_aligned_bkt4_nvm.log",
	"nphj_sc_xpline_aligned_bkt4_7thr_nvm.log"
]

def list_packing(buildpart_counter, probejoin_counter, total_counter, idx):
	buildpart_counter_list = [0] * len(x_axis_label_list)
	probejoin_counter_list = [0] * len(x_axis_label_list)
	total_counter_list = [0] * len(x_axis_label_list)

	buildpart_counter_list[idx] = buildpart_counter
	probejoin_counter_list [idx] = probejoin_counter
	total_counter_list[idx] = total_counter

	return buildpart_counter_list, probejoin_counter_list, total_counter_list



def get_counter_list(event, flag):
	unanligned_list = []
	# aligned_list = []
	# xp_aligned_list = []
	bkt4_list = []
	bkt4_7thr_list = []

	general_counter_list = [
		unanligned_list,
		# aligned_list,
		# xp_aligned_list,
		bkt4_list,
		bkt4_7thr_list
	]

	for idx, log_file in enumerate(log_file_list):
		buildpart_counter, probejoin_counter, total_counter = get_counter_from_log(os.path.join(LOG_PATH, log_file), event, flag)
		general_counter_list[idx] = list_packing(buildpart_counter, probejoin_counter, total_counter, idx)

	return general_counter_list



if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	# print( os.path.join(FIG_PATH, "nphj_ali_counter_{}.png".format(mem_type)) )

	x_axis = np.arange(len(x_axis_label_list))


	bar_width = 0.45

	plt.figure(figsize=(2.25, 4.3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	# plt.tight_layout()
	plt.subplots_adjust(hspace=0.15)


	ax1 = plt.subplot(211)
	ax2 = plt.subplot(212)
	# ax1.set_title("L3-MISS w.r.t. alignment. {}".format(mem_type), fontsize=14)
	# ax2.set_title("Media Reads w.r.t. alignment. {}".format(mem_type), fontsize=14)

	general_counter_list = get_counter_list("PAPI_L3_TCM", "papi")
	for idx, counter_list in enumerate(general_counter_list):
		ax1.bar(x_axis, counter_list[2], edgecolor=color_list[idx], color="none", hatch=hatch_list[idx])

	# ax1.legend(edgecolor="white")

	ax1.set_ylabel("LLC Missess\n(MOps)", fontsize=14)
	ax1.tick_params(axis='y', labelsize=14)
	ax1.set_xticks([])

	general_counter_list = get_counter_list("media_read_ops", "pmwatch")
	for idx, counter_list in enumerate(general_counter_list):
		ax2.bar(x_axis, counter_list[2], edgecolor=color_list[idx], color="none", hatch=hatch_list[idx])

	ax2.set_ylabel("Media Reads\n(MOps)", fontsize=14)
	ax2.tick_params(axis='y', labelsize=14)

	ax1.get_shared_x_axes().join(ax1, ax2)
	ax2.set_xticks([])
	# ax2.set_xticks(x_axis)
	# ax2.set_xticklabels(x_axis_label_list, fontsize=14)
	# ax2.set_xlabel("Bucket Alignment Configuration", fontsize=14)


	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "nphj_ali_counter.png"), bbox_inches="tight", format="png")
	plt.title("", fontsize=20)
	plt.savefig(os.path.join(FIG_PATH, "nphj_ali_counter.pdf"), bbox_inches="tight", format="pdf")
	os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
			os.path.join(FIG_PATH, "nphj_ali_counter.pdf"),
			os.path.join(FIG_PATH, "nphj_ali_counter.eps")
		)
	)


	plt.close()

