import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import sys

from utils import *

FIG_PATH = "../../figs/nphj-prefetching-figs/"


MIN_POW = 0
MAX_POW = 14


x_axis_label_list = ["$2^{" + str(i) + "}$" for i in np.arange(MIN_POW+1, MAX_POW+1)]
x_axis_label_list = ["0"] + x_axis_label_list
x_axis_slice = slice(0,len(x_axis_label_list),2)

x_data_width = 0.8

def plot_time(mem_type):
	nphj_sc_list = []
	nphj_lp_list = []

	general_time_list = [
		nphj_sc_list, nphj_lp_list
	]

	for idx, algo in enumerate(nphj_algo):
		buildpart_time_list, probejoin_time_list, total_time_list = get_data_from_log(algo, mem_type)
		general_time_list[idx] = [buildpart_time_list, probejoin_time_list, total_time_list]


	x_axis = np.arange(len(x_axis_label_list))
	bar_width = x_data_width/len(general_time_list) * 3/4
	x_starting_idx_list = [ -x_data_width/2 + (i+1/2) * x_data_width/len(general_time_list) for i in range(len(general_time_list)) ]

	fig = plt.figure(figsize=(6.5, 3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("nphj elapsed time w.r.t. prefetching. {}".format(mem_type), fontsize=14)
	for idx, time_list in enumerate(general_time_list):
		# plt.bar(x_axis+x_starting_idx_list[idx], time_list[0], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch="", label=legend_list[idx])
		# plt.bar(x_axis+x_starting_idx_list[idx], time_list[1], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch="//", bottom=time_list[0])
		# plt.bar(x_axis+x_starting_idx_list[idx], time_list[2], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch=hatch_list[idx], label=legend_list[idx])
		plt.bar(x_axis+x_starting_idx_list[idx], time_list[2], width=bar_width, edgecolor=color_list[idx%(len(color_list))],
			color="none", hatch=hatch_list[idx], label=legend_list[idx])



	# plt.grid(True)
	plt.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	plt.ylabel("Elapsed Time (s)", fontsize=14)
	plt.xlabel("Prefetching Distance", fontsize=14)
	plt.xticks([index for index in x_axis][x_axis_slice], x_axis_label_list[x_axis_slice], fontsize=14)
	# plt.yticks([2, 4, 6, 8], fontsize=14)
	# plt.ylim(top=10)
	# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, mem_type, "nphj_prefetching_runtime_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	# plt.title("", fontsize=20)
	# plt.savefig(os.path.join(FIG_PATH, mem_type, "nphj_prefetching_runtime_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	# os.system("pdftops -eps {} {}".format(
	# 		os.path.join(FIG_PATH, mem_type, "nphj_prefetching_runtime_{}.pdf".format(mem_type)),
	# 		os.path.join(FIG_PATH, mem_type, "nphj_prefetching_runtime_{}.eps".format(mem_type))
	# 	)
	# )

	plt.close()

	return general_time_list



def plot_counter(mem_type, event, flag):
	nphj_sc_list = []
	nphj_lp_list = []

	general_counter_list = [
		nphj_sc_list, nphj_lp_list
	]

	for idx, algo in enumerate(nphj_algo):
		buildpart_counter_list, probejoin_counter_list, total_counter_list = get_counter_from_log(algo, mem_type, event, flag)
		general_counter_list[idx] = [buildpart_counter_list, probejoin_counter_list, total_counter_list]


	x_axis = np.arange(len(x_axis_label_list))
	bar_width = x_data_width/len(general_counter_list) * 3/4
	x_starting_idx_list = [ -x_data_width/2 + (i+1/2) * x_data_width/len(general_counter_list) for i in range(len(general_counter_list)) ]
	
	fig = plt.figure(figsize=(6.5, 3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("nphj {} w.r.t. prefetching. {}".format(event, mem_type), fontsize=14)
	for idx, counter_list in enumerate(general_counter_list):
		# plt.bar(x_axis+x_starting_idx_list[idx], counter_list[0], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch="", label=legend_list[idx])
		# plt.bar(x_axis+x_starting_idx_list[idx], counter_list[1], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch="//", bottom=counter_list[0])
		# plt.bar(x_axis+x_starting_idx_list[idx], counter_list[2], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch=hatch_list[idx], label=legend_list[idx])
		plt.bar(x_axis+x_starting_idx_list[idx], counter_list[2], width=bar_width, edgecolor=color_list[idx%(len(color_list))],
			color="none", hatch=hatch_list[idx], label=legend_list[idx])

	plt.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	plt.ylabel("Media Reads (MOps)", fontsize=14)
	plt.xlabel("Prefetching Distance", fontsize=14)
	plt.xticks([index for index in x_axis][x_axis_slice], x_axis_label_list[x_axis_slice], fontsize=14)
	# plt.yticks([2, 4, 6, 8], fontsize=14)
	# plt.ylim(top=10)
	# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, mem_type, "nphj_prefetching_{}_{}.png".format(event, mem_type)), bbox_inches="tight", format="png")
	# plt.title("", fontsize=20)
	# plt.savefig(os.path.join(FIG_PATH, mem_type, "nphj_prefetching_{}_{}.pdf".format(event, mem_type)), bbox_inches="tight", format="pdf")
	# os.system("pdftops -eps {} {}".format(
	# 		os.path.join(FIG_PATH, mem_type, "nphj_prefetching_{}_{}.pdf".format(event, mem_type)),
	# 		os.path.join(FIG_PATH, mem_type, "nphj_prefetching_{}_{}.eps".format(event, mem_type))
	# 	)
	# )

	plt.close()

	return general_counter_list





def share_x_axis_plot(mem_type, event, general_time_list, general_counter_list):
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
	# ax1.set_title("nphj elapsed time w.r.t. prefetching. {}".format(mem_type), fontsize=14)
	# ax2.set_title("nphj {} w.r.t. prefetching. {}".format(event, mem_type), fontsize=14)

	for idx, time_list in enumerate(general_time_list):
		# ax1.bar(x_axis+x_starting_idx_list[idx], time_list[0], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch="", label=legend_list[idx])
		# ax1.bar(x_axis+x_starting_idx_list[idx], time_list[1], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch="//", bottom=time_list[0])
		# ax1.bar(x_axis+x_starting_idx_list[idx], time_list[2], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch=hatch_list[idx], label=legend_list[idx])
		ax1.bar(x_axis+x_starting_idx_list[idx], time_list[2], width=bar_width, edgecolor=color_list[idx%(len(color_list))],
			color="none", hatch=hatch_list[idx], label=legend_list[idx])

	ax1.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	ax1.set_ylabel("Elapsed Time (s)", fontsize=14)
	ax1.tick_params(axis='y', labelsize=14)



	for idx, counter_list in enumerate(general_counter_list):
		# ax2.bar(x_axis+x_starting_idx_list[idx], counter_list[0], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch="", label=legend_list[idx])
		# ax2.bar(x_axis+x_starting_idx_list[idx], counter_list[1], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch="//", bottom=counter_list[0])
		# ax2.bar(x_axis+x_starting_idx_list[idx], counter_list[2], width=bar_width, edgecolor="black",
		# 	color=color_list[idx%(len(color_list))], hatch=hatch_list[idx], label=legend_list[idx])
		ax2.bar(x_axis+x_starting_idx_list[idx], counter_list[2], width=bar_width, edgecolor=color_list[idx%(len(color_list))],
			color="none", hatch=hatch_list[idx], label=legend_list[idx])

	# ax2.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	ax2.set_ylabel("Media Reads (MOps)", fontsize=14)
	ax2.tick_params(axis='y', labelsize=14)

	ax1.get_shared_x_axes().join(ax1, ax2)
	ax1.axes.get_xaxis().set_visible(False)
	ax2.set_xticks(x_axis[::2])
	ax2.set_xticklabels(x_axis_label_list[::2], fontsize=14)
	ax2.set_xlabel("Prefetching Distance", fontsize=14)

	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, mem_type, "nphj_prefetching_runtime_{}_{}_bar_chart_column.png".format(event, mem_type)), bbox_inches="tight", format="png")
	# plt.title("", fontsize=20)
	# plt.savefig(os.path.join(FIG_PATH, mem_type, "nphj_prefetching_runtime_{}_{}_bar_chart_column.pdf".format(event, mem_type)), bbox_inches="tight", format="pdf")
	# os.system("pdftops -eps {} {}".format(
	# 		os.path.join(FIG_PATH, mem_type, "nphj_prefetching_runtime_{}_{}_bar_chart_column.pdf".format(event, mem_type)),
	# 		os.path.join(FIG_PATH, mem_type, "nphj_prefetching_runtime_{}_{}_bar_chart_column.eps".format(event, mem_type))
	# 	)
	# )

	plt.close()





if __name__ == "__main__":
	if not os.path.exists(os.path.join(FIG_PATH, "nvm")):
		os.makedirs(os.path.join(FIG_PATH, "nvm"))

	general_time_list = plot_time("nvm")
	general_counter_list = plot_counter("nvm", "media_read_ops", "pmwatch")
	share_x_axis_plot("nvm", "media_read_ops", general_time_list, general_counter_list)



























