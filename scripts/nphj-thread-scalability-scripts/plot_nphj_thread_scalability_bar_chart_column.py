import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import sys

from utils import *

LOG_PATH = "../../logs/nphj-thread-scalability-logs/"
FIG_PATH = "../../figs/nphj-thread-scalability-figs/thread_scalability"

x_axis = np.arange(2, 41, 2)
x_axis_label_list = [str(i) for i in x_axis]

x_axis_slice = slice(0, len(x_axis_label_list),2)
x_data_width = 0.8


core_setting_list = [
	"hyperthreading",
	"samecore"
]


def plot_thread_scalability(mem_type, core_setting):
	nphj_sc_list = []
	nphj_lp_list = []

	general_time_list = [
		nphj_sc_list, nphj_lp_list
	]

	for idx, algo in enumerate(nphj_algo):
		buildpart_time_list, probejoin_time_list, total_time_list = get_data_from_log_thread_scalablitlity(algo, core_setting, mem_type)
		general_time_list[idx] = [buildpart_time_list, probejoin_time_list, total_time_list]

	'''
		x_axis = np.arange(len(x_axis_label_list))
		bar_width = x_data_width/len(general_time_list)
		x_starting_idx_list = [ -x_data_width/2 + (i+1/2) * bar_width for i in range(len(general_time_list)) ]

		fig = plt.figure(figsize=(6.5, 3))
		# plt.style.use("ggplot")
		# plt.style.use("seaborn-white")
		plt.title("nphj elapsed time w.r.t. thread_scalability {}, {}".format(core_setting, mem_type), fontsize=14)
		for idx, time_list in enumerate(general_time_list):
			plt.bar(x_axis+x_starting_idx_list[idx], time_list[0], width=bar_width, edgecolor="black",
				color=color_list[idx%(len(color_list))][0], hatch=hatch_list[idx], label=legend_list[idx])
			plt.bar(x_axis+x_starting_idx_list[idx], time_list[1], width=bar_width, edgecolor="black",
				color=color_list[idx%(len(color_list))][1], hatch=hatch_list[idx], bottom=time_list[0])

		# plt.grid(True)
		plt.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
		plt.ylabel("Elapsed Time (s)", fontsize=14)
		plt.xlabel("Thread", fontsize=14)
		plt.xticks([index for index in x_axis], x_axis_label_list, fontsize=14)
		# plt.yticks([2, 4, 6, 8], fontsize=14)
		# plt.ylim(top=10)
		# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
		# plt.show()
		plt.savefig(os.path.join(FIG_PATH, "nphj_thread_scalability_{}_{}.png".format(core_setting, mem_type)), bbox_inches="tight", format="png")
		plt.title("", fontsize=20)
		plt.savefig(os.path.join(FIG_PATH, "nphj_thread_scalability_{}_{}.pdf".format(core_setting, mem_type)), bbox_inches="tight", format="pdf")
		os.system("pdftops -eps {} {}".format(
				os.path.join(FIG_PATH, "nphj_thread_scalability_{}_{}.pdf".format(core_setting, mem_type)),
				os.path.join(FIG_PATH, "nphj_thread_scalability_{}_{}.eps".format(core_setting, mem_type))
			)
		)

		plt.close()
	'''

	x_axis = np.arange(len(x_axis_label_list))
	bar_width = x_data_width/len(general_time_list) * 3/4
	x_starting_idx_list = [ -x_data_width/2 + (i+1/2) * x_data_width/len(general_time_list) for i in range(len(general_time_list)) ]


	fig=plt.figure(figsize=(6.5, 5.4))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	# plt.tight_layout()
	plt.subplots_adjust(hspace=0.03)

	ax1 = plt.subplot(211)
	ax2 = plt.subplot(212)
	# ax1.set_title("nphj probe time w.r.t. thread. {}".format(mem_type), fontsize=14)
	# ax2.set_title("nphj build time w.r.t. thread. {}".format(mem_type), fontsize=14)

	for idx, time_list in enumerate(general_time_list):
		# ax1.bar(x_axis+x_starting_idx_list[idx], time_list[1], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch=hatch_list[idx], label=legend_list[idx])
		ax1.bar(x_axis+x_starting_idx_list[idx], time_list[1], width=bar_width, color="none",
			edgecolor=color_list[idx%(len(color_list))], hatch=hatch_list[idx], label=legend_list[idx])

	ax1.axvline(x=9, linewidth=1, color ="black", linestyle="--")
	ax1.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	ax1.set_ylabel("Probe Phase Time (s)", fontsize=14)
	ax1.tick_params(axis='y', labelsize=14)


	for idx, time_list in enumerate(general_time_list):
		# ax2.bar(x_axis+x_starting_idx_list[idx], time_list[0], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch=hatch_list[idx], label=legend_list[idx])
		ax2.bar(x_axis+x_starting_idx_list[idx], time_list[0], width=bar_width, color="none",
			edgecolor=color_list[idx%(len(color_list))], hatch=hatch_list[idx], label=legend_list[idx])

	ax2.axvline(x=9, linewidth=1, color ="black", linestyle="--")
	# ax2.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	ax2.set_ylabel("Build Phase Time (s)", fontsize=14)
	ax2.tick_params(axis='y', labelsize=14)


	ax1.get_shared_x_axes().join(ax1, ax2)
	ax1.axes.get_xaxis().set_visible(False)
	ax2.set_xticks(x_axis[1::2])
	ax2.set_xticklabels(x_axis_label_list[1::2], fontsize=14)
	ax2.set_xlabel("#Thread", fontsize=14)

	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "nphj_thread_scalability_runtime_{}_bar_chart_column.png".format(mem_type)), bbox_inches="tight", format="png")
	# plt.title("", fontsize=20)
	# plt.savefig(os.path.join(FIG_PATH, "nphj_thread_scalability_runtime_{}_bar_chart_column.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	# os.system("pdftops -eps {} {}".format(
	# 		os.path.join(FIG_PATH, "nphj_thread_scalability_runtime_{}_bar_chart_column.pdf".format(mem_type)),
	# 		os.path.join(FIG_PATH, "nphj_thread_scalability_runtime_{}_bar_chart_column.eps".format(mem_type))
	# 	)
	# )

	plt.close()




if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	plot_thread_scalability("nvm", "hyperthreading")










