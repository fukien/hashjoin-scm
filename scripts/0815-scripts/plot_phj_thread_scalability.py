import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import sys

from utils import *

LOG_PATH = "../../logs/0815-logs/"
FIG_PATH = "../../figs/0815-figs/thread_scalability"

x_axis = np.arange(2, 41, 2)
x_axis_label_list = [str(i) for i in x_axis]

x_axis_slice = slice(1, len(x_axis_label_list), 2)
x_data_width = 0.8

start_intercept_idx = 0
end_intercept_idx = len(x_axis_label_list)

core_setting_list = [
	"hyperthreading",
	"samecore"
]


def plot_thread_scalability(mem_type, core_setting):
	phj_shr_list = []
	phj_shr_uni_list = []
	phj_ind_hro_list = []
	phj_ind_uni_hro_list = []
	phj_radix_hro_list = []
	phj_asymm_radix_hro_list = []

	general_time_list = [
		phj_shr_list,
		phj_shr_uni_list,
		phj_ind_hro_list,
		phj_ind_uni_hro_list,
		phj_radix_hro_list,
		phj_asymm_radix_hro_list
	]

	for idx, algo in enumerate(phj_algo):
		buildpart_time_list, probejoin_time_list, total_time_list = get_data_from_log_thread_scalablitlity(algo, core_setting, mem_type)
		general_time_list[idx] = [buildpart_time_list, probejoin_time_list, total_time_list]

	x_axis = np.arange(len(x_axis_label_list))
	# bar_width = x_data_width/len(general_time_list)
	# x_starting_idx_list = [ -x_data_width/2 + (i+1/2) * bar_width for i in range(len(general_time_list)) ]


	'''
	fig = plt.figure(figsize=(6.5, 3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("phj part time w.r.t. thread_scalability {}, {}".format(core_setting, mem_type), fontsize=14)
	for idx, time_list in enumerate(general_time_list):
		plt.plot(x_axis[start_intercept_idx:end_intercept_idx], time_list[0][start_intercept_idx:end_intercept_idx], 
		color=color_list[idx], marker=marker_list[idx], label=legend_list[idx])
	# plt.grid(True)
	# plt.yscale("symlog")
	plt.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	plt.ylabel("Partition Phase Time (s)", fontsize=14)
	plt.xlabel("#Thread", fontsize=14)
	plt.axvline(x=9, linewidth=1, color ="black", linestyle="--")
	plt.xticks([index for index in x_axis][start_intercept_idx:end_intercept_idx][x_axis_slice], 
		x_axis_label_list[start_intercept_idx:end_intercept_idx][x_axis_slice], fontsize=14)
	# plt.yticks([2, 4, 6, 8], fontsize=14)
	# plt.ylim(top=10)
	# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}.png".format(core_setting, mem_type)), bbox_inches="tight", format="png")
	plt.title("", fontsize=20)
	plt.savefig(os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}.pdf".format(core_setting, mem_type)), bbox_inches="tight", format="pdf")
	os.system("pdftops -eps {} {}".format(
			os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}.pdf".format(core_setting, mem_type)),
			os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}.eps".format(core_setting, mem_type))
		)
	)
	plt.close()
	'''


	fig=plt.figure(figsize=(6.5, 5))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	# plt.tight_layout()
	plt.subplots_adjust(hspace=0.03)

	ax1 = plt.subplot(211)
	ax2 = plt.subplot(212)
	# ax1.set_title("phj probe time w.r.t. thread. {}, {}".format(core_setting, mem_type), fontsize=14)
	# ax2.set_title("phj build time w.r.t. thread. {}, {}".format(core_setting, mem_type), fontsize=14)

	for idx, time_list in enumerate(general_time_list):
		ax1.plot(x_axis[start_intercept_idx:end_intercept_idx], time_list[0][start_intercept_idx:end_intercept_idx], 
			color=color_list[idx], marker=marker_list[idx], label=legend_list[idx])

	ax1.axvline(x=9, linewidth=1, color ="black", linestyle="--")
	ax1.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	ax1.set_ylabel("Elapsed Time (s)", fontsize=14)
	ax1.tick_params(axis='y', labelsize=14)


	general_time_list_zoom_in = general_time_list[2:]
	color_list_zoom_in = color_list[2:]
	marker_list_zoom_in = marker_list[2:]
	legend_list_zoom_in = legend_list[2:]
	for idx, time_list in enumerate(general_time_list_zoom_in):
		ax2.plot(x_axis[start_intercept_idx:end_intercept_idx], time_list[0][start_intercept_idx:end_intercept_idx], 
			color=color_list_zoom_in[idx], marker=marker_list_zoom_in[idx], label=legend_list_zoom_in[idx])

	ax2.axvline(x=9, linewidth=1, color ="black", linestyle="--")
	# ax2.legend(loc=0, ncol=2, fontsize=14, columnspacing=1)
	ax2.set_ylabel("Elapsed Time (s)", fontsize=14)
	ax2.tick_params(axis='y', labelsize=14)
	ax2.set_yticks(np.arange(0, 5, 0.5)[::2])
	# ax2.set_ylim(0, 2)

	ax1.get_shared_x_axes().join(ax1, ax2)
	ax2.set_xticks(x_axis[start_intercept_idx:end_intercept_idx][x_axis_slice])
	ax2.set_xticklabels(x_axis_label_list[start_intercept_idx:end_intercept_idx][x_axis_slice], fontsize=14)
	ax2.set_xlabel("#Thread", fontsize=14)


	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}.png".format(core_setting, mem_type)), bbox_inches="tight", format="png")
	plt.title("", fontsize=20)
	plt.savefig(os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}.pdf".format(core_setting, mem_type)), bbox_inches="tight", format="pdf")
	os.system("pdftops -eps {} {}".format(
			os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}.pdf".format(core_setting, mem_type)),
			os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}.eps".format(core_setting, mem_type))
		)
	)

	plt.close()



if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	for mem_type in ["dram", "nvm"]:
		for core_setting in core_setting_list:
			plot_thread_scalability(mem_type, core_setting)
	
	# plot_thread_scalability("nvm", "hyperthreading")





































