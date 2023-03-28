import copy
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import sys

from utils import *

LOG_PATH = "../../logs/phj-part-fanout-logs/"
FIG_PATH = "../../figs/phj-part-fanout-figs/"

MIN_BIT=4
MAX_BIT=16

x_axis = [i for i in range(MIN_BIT, MAX_BIT+1)]
x_axis_label_list = ["$2^{" + str(i) + "}$" for i in x_axis]


x_axis_slice = slice(0, len(x_axis_label_list), 2)
x_data_width = 0.8

intercept_idx = len(x_axis_label_list)



def plot_fanout(mem_type):
	phj_shr_hro_list_1_pass = []
	phj_shr_uni_hro_list_1_pass = []
	phj_ind_hro_list_1_pass = []
	phj_ind_uni_hro_list_1_pass = []
	phj_radix_hro_list_1_pass = []

	general_time_list_1_pass = [
		phj_shr_hro_list_1_pass,
		phj_shr_uni_hro_list_1_pass,
		phj_ind_hro_list_1_pass,
		phj_ind_uni_hro_list_1_pass,
		phj_radix_hro_list_1_pass,
	]

	for idx, algo in enumerate(part_algo[:-1]):
		buildpart_time_list, probejoin_time_list, total_time_list = get_data_from_log(algo, mem_type, "1_pass")
		general_time_list_1_pass[idx] = [buildpart_time_list, probejoin_time_list, total_time_list]


	phj_shr_list_2_pass = []
	phj_shr_uni_hro_list_2_pass = []
	phj_ind_hro_list_2_pass = []
	phj_ind_uni_hro_list_2_pass = []
	phj_radix_hro_list_2_pass = []
	phj_asymm_radix_hro_list_2_pass = []

	general_time_list_2_pass = [
		phj_shr_list_2_pass,
		phj_shr_uni_hro_list_2_pass,
		phj_ind_hro_list_2_pass,
		phj_ind_uni_hro_list_2_pass,
		phj_radix_hro_list_2_pass,
		phj_asymm_radix_hro_list_2_pass
	]

	for idx, algo in enumerate(part_algo):
		buildpart_time_list, probejoin_time_list, total_time_list = get_data_from_log(algo, mem_type, "2_pass")
		general_time_list_2_pass[idx] = [buildpart_time_list, probejoin_time_list, total_time_list]

	x_axis = np.arange(len(x_axis_label_list))
	# bar_width = x_data_width/len(general_time_list)
	# x_starting_idx_list = [ -x_data_width/2 + (i+1/2) * bar_width for i in range(len(general_time_list)) ]


	'''
	fig = plt.figure(figsize=(6.5, 3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("phj part time w.r.t. fanout {}".format(mem_type), fontsize=14)
	for idx, time_list in enumerate(general_time_list_1_pass):
		plt.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
			color=color_list[idx], marker=marker_list[idx], label=legend_list[idx] + "-1", linestyle="--")

	for idx, time_list in enumerate(general_time_list_2_pass):
		if legend_list[idx] == "ASYM":
			plt.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
				color=color_list[idx], marker=marker_list[idx], label=legend_list[idx] + "-2-1")
		else:
			plt.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
				color=color_list[idx], marker=marker_list[idx], label=legend_list[idx] + "-2")

	# plt.grid(True)
	# plt.yscale("symlog")
	plt.legend(loc=0, ncol=3, fontsize=14, columnspacing=1)
	plt.ylabel("Partition Phase Time (s)", fontsize=14)
	plt.xlabel("#Partition", fontsize=14)
	plt.xticks([index for index in x_axis][:intercept_idx][x_axis_slice], x_axis_label_list[:intercept_idx][x_axis_slice], fontsize=14)
	# plt.yticks([2, 4, 6, 8], fontsize=14)
	# plt.ylim(top=10)
	# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "phj_part_fanout_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	plt.title("", fontsize=20)
	plt.savefig(os.path.join(FIG_PATH, "phj_part_fanout_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
			os.path.join(FIG_PATH, "phj_part_fanout_{}.pdf".format(mem_type)),
			os.path.join(FIG_PATH, "phj_part_fanout_{}.eps".format(mem_type))
		)
	)

	plt.close()
	'''


	fig = plt.figure(figsize=(6.5, 5))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	# plt.tight_layout()
	plt.subplots_adjust(hspace=0.03)

	ax1 = plt.subplot(211)
	ax2 = plt.subplot(212)
	# ax1.set_title("phj_part_fanout_{}".format(fanout), fontsize=14)
	# ax2.set_title("phj_part_fanout_{}".format(fanout), fontsize=14)


	for idx, time_list in enumerate(general_time_list_1_pass):
		ax1.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
			color=color_list[idx], marker=marker_list[idx], label=legend_list[idx] + "-1", linestyle="--")
	for idx, time_list in enumerate(general_time_list_2_pass):
		if legend_list[idx] == "ASYM":
			ax1.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
				color=color_list[idx], marker=marker_list[idx], label=legend_list[idx] + "-2-1")
		else:
			ax1.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
				color=color_list[idx], marker=marker_list[idx], label=legend_list[idx] + "-2")

	# ax1.axvline(x=9, linewidth=1, color ="black", linestyle="--")
	ax1.legend(loc="upper right", bbox_to_anchor=(1.03, 1.07), ncol=2, fontsize=14, 
		fancybox=False, shadow=False, edgecolor="none", facecolor="none",
		labelspacing=0.01, columnspacing=1, handlelength=2.5
	)
	# ax1.legend(loc="upper center", bbox_to_anchor=(0.495, 1.50), 
	# 	fancybox=True, shadow=True, ncol=4, fontsize=14, 
	# 	labelspacing=0.15, columnspacing=1.2, handlelength=0.1
	# )
	ax1.set_ylabel("Elapsed Time (s)", fontsize=14)
	ax1.tick_params(axis='y', labelsize=14)
	ax1.set_yticks(np.arange(0, 60, 10))
	# ax1.set_yscale("log")

	general_time_list_1_pass_zoom_in = copy.deepcopy(general_time_list_1_pass)
	general_time_list_2_pass_zoom_in = copy.deepcopy(general_time_list_2_pass)
	color_list_zoom_in = copy.deepcopy(color_list)
	marker_list_zoom_in = copy.deepcopy(marker_list)
	legend_list_zoom_in = copy.deepcopy(legend_list)

	# general_time_list_1_pass_zoom_in.pop(0)
	# general_time_list_2_pass_zoom_in.pop(0)
	# color_list_zoom_in.pop(0)
	# marker_list_zoom_in.pop(0)
	# legend_list_zoom_in.pop(0)

	zoom_in_intercept = 2
	general_time_list_1_pass_zoom_in = general_time_list_1_pass_zoom_in[zoom_in_intercept:]
	general_time_list_2_pass_zoom_in = general_time_list_2_pass_zoom_in[zoom_in_intercept:]
	color_list_zoom_in = color_list_zoom_in[zoom_in_intercept:]
	marker_list_zoom_in = marker_list_zoom_in[zoom_in_intercept:]
	legend_list_zoom_in = legend_list_zoom_in[zoom_in_intercept:]

	for idx, time_list in enumerate(general_time_list_1_pass_zoom_in):
		ax2.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
			color=color_list_zoom_in[idx], marker=marker_list_zoom_in[idx], label=legend_list_zoom_in[idx] + "-1", linestyle="--")
	for idx, time_list in enumerate(general_time_list_2_pass_zoom_in):
		if legend_list_zoom_in[idx] == "ASYM":
			ax2.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
				color=color_list_zoom_in[idx], marker=marker_list_zoom_in[idx], label=legend_list_zoom_in[idx] + "-2-1")
		else:
			ax2.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
				color=color_list_zoom_in[idx], marker=marker_list_zoom_in[idx], label=legend_list_zoom_in[idx] + "-2")


	# ax2.axvline(x=9, linewidth=1, color ="black", linestyle="--")
	# ax2.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	ax2.set_ylabel("Elapsed Time (s)", fontsize=14)
	ax2.tick_params(axis='y', labelsize=14)
	ax2.set_yticks(np.arange(0, 3, 0.5))
	ax2.set_ylim(0, 3)

	ax1.get_shared_x_axes().join(ax1, ax2)
	ax1.axes.get_xaxis().set_visible(False)
	# ax2.set_xticks(x_axis[1::2])
	# ax2.set_xticklabels(x_axis_label_list[1::2], fontsize=14)
	ax2.set_xticks(x_axis)
	ax2.set_xticklabels(x_axis_label_list, fontsize=14, rotation=45)
	ax2.set_xlabel("#Fanout", fontsize=14)

	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "phj_part_fanout_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	# plt.title("", fontsize=20)
	# plt.savefig(os.path.join(FIG_PATH, "phj_part_fanout_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	# os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
	# 		os.path.join(FIG_PATH, "phj_part_fanout_{}.pdf".format(mem_type)),
	# 		os.path.join(FIG_PATH, "phj_part_fanout_{}.eps".format(mem_type))
	# 	)
	# )

	plt.close()








def plot_counter(part, passnum, mem_type, 
	event, event_flag, plot_time_flag):
	buildpart_time_list, probejoin_time_list, total_time_list,\
		total_counter_list, buildpart_counter_list, probejoin_counter_list\
		 = get_counter_from_log(part, mem_type, passnum, event, event_flag)

	if plot_time_flag:
		fig = plt.figure(figsize=(6.5, 3))
		# plt.style.use("ggplot")
		# plt.style.use("seaborn-white")
		plt.title("{} {} part time w.r.t. fanout {}".format(part, passnum, mem_type), fontsize=14)
		plt.plot(x_axis[:intercept_idx], buildpart_time_list[:intercept_idx])

		# plt.grid(True)
		# plt.yscale("symlog")
		plt.legend(loc=0, ncol=3, fontsize=14, columnspacing=1)
		plt.ylabel("Partition Phase Time (s)", fontsize=14)
		plt.xlabel("#Part", fontsize=14)
		plt.xticks([index for index in x_axis][:intercept_idx][x_axis_slice], x_axis_label_list[:intercept_idx][x_axis_slice], fontsize=14)
		# plt.yticks([2, 4, 6, 8], fontsize=14)
		# plt.ylim(top=10)
		# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
		# plt.show()
		plt.savefig(os.path.join(FIG_PATH, part, "{}_{}_part_fanout_{}.png".format(part, passnum, mem_type)), bbox_inches="tight", format="png")
		# plt.title("", fontsize=20)
		# plt.savefig(os.path.join(FIG_PATH, part, "{}_{}_part_fanout_{}.pdf".format(part, passnum, mem_type)), bbox_inches="tight", format="pdf")
		# os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
		# 		os.path.join(FIG_PATH, part, "{}_{}_part_fanout_{}.pdf".format(part, passnum, mem_type)),
		# 		os.path.join(FIG_PATH, part, "{}_{}_part_fanout_{}.eps".format(part, passnum, mem_type))
		# 	)
		# )

		plt.close()


	fig = plt.figure(figsize=(6.5, 3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("{} {} part {} w.r.t. fanout {}".format(part, passnum, event, mem_type), fontsize=14)
	plt.plot(x_axis[:intercept_idx], buildpart_counter_list[:intercept_idx])

	# plt.grid(True)
	# plt.yscale("symlog")
	plt.legend(loc=0, ncol=3, fontsize=14, columnspacing=1)
	plt.ylabel("Counter", fontsize=14)
	plt.xlabel("#Part", fontsize=14)
	plt.xticks([index for index in x_axis][:intercept_idx][x_axis_slice], x_axis_label_list[:intercept_idx][x_axis_slice], fontsize=14)
	# plt.yticks([2, 4, 6, 8], fontsize=14)
	# plt.ylim(top=10)
	# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, part, "{}_{}_part_{}_fanout_{}.png".format(part, passnum, event, mem_type)), bbox_inches="tight", format="png")
	# plt.title("", fontsize=20)
	# plt.savefig(os.path.join(FIG_PATH, part, "{}_{}_part_{}_fanout_{}.pdf".format(part, passnum, event, mem_type)), bbox_inches="tight", format="pdf")
	# os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
	# 		os.path.join(FIG_PATH, part, "{}_{}_part_{}_fanout_{}.pdf".format(part, passnum, event, mem_type)),
	# 		os.path.join(FIG_PATH, part, "{}_{}_part_{}_fanout_{}.eps".format(part, passnum, event, mem_type))
	# 	)
	# )

	plt.close()











if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	plot_fanout("nvm")

	'''
	for part in part_algo:
		for passnum in ["1_pass", "2_pass"]:
			if passnum == "1_pass" and part == "phj_asymm_radix":
				continue
			if not os.path.exists(os.path.join(FIG_PATH, part)):
				os.makedirs(os.path.join(FIG_PATH, part))
			for idx, event in enumerate(papi_event_list):
				if idx == 0:
					plot_counter(part, passnum, "nvm", event, "papi", True)
				else:
					plot_counter(part, passnum, "nvm", event, "papi", False)
			for event in pmwatch_event_list:
				plot_counter(part, passnum, "nvm", event, "pmwatch", False)
	'''





































