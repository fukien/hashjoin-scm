import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from matplotlib import gridspec
import numpy as np
import os
import sys

from utils import *

LOG_PATH = "../../logs/0815-logs/"
FIG_PATH = "../../figs/0816-figs/thread_scalability"

x_axis = np.arange(2, 41, 2)
x_axis_label_list = [str(i) for i in x_axis]
x_axis = [i for i in range(20)]

# x_data_width = 0.8
# x_axis_slice = slice(1, len(x_axis_label_list), 2)

ax1_x_axis_slice = slice(1, len(x_axis_label_list), 2)
ax2_x_axis_slice = slice(0, len(x_axis_label_list), 2)


start_intercept_idx = 3
end_intercept_idx = len(x_axis_label_list)

rotation_degree = 30

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


	default_fontsize = 14

	fig = plt.figure(figsize=(9.66, 3))
	plt.subplots_adjust(wspace=0.15)
	gs = gridspec.GridSpec(1, 2, 
		width_ratios=[end_intercept_idx, 
		end_intercept_idx - start_intercept_idx]
	) 
	# ax1 = plt.subplot(121)
	# ax2 = plt.subplot(122)
	ax1 = plt.subplot(gs[0])
	ax2 = plt.subplot(gs[1])

	ax1.set_title("", fontsize=default_fontsize, fontweight="bold")
	for idx, time_list in enumerate(general_time_list):
		ax1.plot(x_axis[0:end_intercept_idx], time_list[0][0:end_intercept_idx], 
			color=color_list[idx], marker=marker_list[idx], label=legend_list[idx])

	ax1.axvline(x=9, linewidth=1, color ="black", linestyle="--")
	ax1.legend(loc=0, ncol=2, fontsize=default_fontsize, columnspacing=1, 
		# fancybox=False, shadow=False, edgecolor="none", facecolor="none"
	)
	ax1.set_ylabel("Partition Phase Time (s)", fontsize=default_fontsize)
	ax1.tick_params(axis='y', labelsize=default_fontsize)
	# ax1.set_yticks(np.arange(0, 1.75, 0.25))
	ax1.set_xticks(np.arange(0, len(x_axis), 1)[ax1_x_axis_slice])
	ax1.set_xticklabels(x_axis_label_list[ax1_x_axis_slice], fontsize=default_fontsize, rotation=rotation_degree)
	ax1.set_xlabel("#Thread", fontsize=default_fontsize)


	general_time_list_zoom_in = general_time_list[2:]
	color_list_zoom_in = color_list[2:]
	marker_list_zoom_in = marker_list[2:]
	legend_list_zoom_in = legend_list[2:]

	# ax2.set_title("", fontsize=default_fontsize, fontweight="bold")
	for idx, time_list in enumerate(general_time_list_zoom_in):
		ax2.plot(x_axis[start_intercept_idx:end_intercept_idx], time_list[0][start_intercept_idx:end_intercept_idx], 
			color=color_list_zoom_in[idx], marker=marker_list_zoom_in[idx], label=legend_list_zoom_in[idx])
	ax2.axvline(x=9, linewidth=1, color ="black", linestyle="--")
	ax2.tick_params(axis='y', labelsize=default_fontsize)
	# ax2.set_yticks(np.arange(0.1, 0.6, 0.1)[1:])
	ax2.set_xticks(np.arange(start_intercept_idx, end_intercept_idx, 2))
	ax2.set_xticklabels(x_axis_label_list[start_intercept_idx:end_intercept_idx][ax2_x_axis_slice], 
		fontsize=default_fontsize, rotation=rotation_degree)
	ax2.set_xlabel("#Thread", fontsize=default_fontsize)



	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}_h.png".format(core_setting, mem_type)), bbox_inches="tight", format="png")
	plt.title("", fontsize=20)
	plt.savefig(os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}_h.pdf".format(core_setting, mem_type)), bbox_inches="tight", format="pdf")
	os.system("pdftops -eps {} {}".format(
			os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}_h.pdf".format(core_setting, mem_type)),
			os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}_h.eps".format(core_setting, mem_type))
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





































