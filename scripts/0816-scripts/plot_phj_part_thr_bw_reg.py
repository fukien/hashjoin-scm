import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from matplotlib import gridspec
from matplotlib.gridspec import GridSpec
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

ax0_x_axis_slice = slice(1, len(x_axis_label_list), 2)
ax1_x_axis_slice = slice(0, len(x_axis_label_list), 2)


start_intercept_idx = 3
end_intercept_idx = len(x_axis_label_list)

rotation_degree = 30

core_setting_list = [
	"hyperthreading",
	# "samecore"
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

	fig = plt.figure(figsize=(6., 5), constrained_layout=True)
	# plt.subplots_adjust(wspace=0.5)
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	gs = GridSpec(2, 5, figure=fig)

	ax0 = fig.add_subplot(gs[0, :])
	ax1 = fig.add_subplot(gs[1, 0:-2])
	ax2 = fig.add_subplot(gs[1, -2:])

	# ax2.axes.get_xaxis().set_visible(False)

	ax0.tick_params(axis='y', labelsize=default_fontsize)
	ax1.tick_params(axis='y', labelsize=default_fontsize)
	ax2.tick_params(axis='y', labelsize=default_fontsize)

	ax0.set_ylabel("Partition\nPhase Time (s)", fontsize=default_fontsize)
	ax1.set_ylabel("Partition\nPhase Time (s)", fontsize=default_fontsize)


	# ax0.set_title("", fontsize=default_fontsize, fontweight="bold")
	for idx, time_list in enumerate(general_time_list):
		ax0.plot(x_axis[0:end_intercept_idx], time_list[0][0:end_intercept_idx], 
			color=color_list[idx], marker=marker_list[idx], label=legend_list[idx])

	ax0.axvline(x=9, linewidth=1, color ="black", linestyle="--")
	ax0.legend(loc=0, ncol=2, fontsize=default_fontsize, columnspacing=1, 
		# fancybox=False, shadow=False, edgecolor="none", facecolor="none"
	)
	# ax0.set_yticks(np.arange(0, 1.75, 0.25))
	ax0.set_xticks(np.arange(0, len(x_axis), 1)[ax0_x_axis_slice])
	ax0.set_xticklabels(x_axis_label_list[ax0_x_axis_slice], fontsize=default_fontsize, rotation=rotation_degree)
	ax0.set_xlabel("#Thread", fontsize=default_fontsize)


	general_time_list_zoom_in = general_time_list[2:]
	color_list_zoom_in = color_list[2:]
	marker_list_zoom_in = marker_list[2:]
	legend_list_zoom_in = legend_list[2:]

	# ax1.set_title("", fontsize=default_fontsize, fontweight="bold")
	for idx, time_list in enumerate(general_time_list_zoom_in):
		ax1.plot(x_axis[start_intercept_idx:end_intercept_idx], time_list[0][start_intercept_idx:end_intercept_idx], 
			color=color_list_zoom_in[idx], marker=marker_list_zoom_in[idx], label=legend_list_zoom_in[idx])
	ax1.axvline(x=9, linewidth=1, color ="black", linestyle="--")
	# ax1.set_yticks(np.arange(0.1, 0.6, 0.1)[1:])
	ax1.set_xticks(np.arange(start_intercept_idx, end_intercept_idx, 2))
	ax1.set_xticklabels(x_axis_label_list[start_intercept_idx:end_intercept_idx][ax1_x_axis_slice], 
		fontsize=default_fontsize, rotation=rotation_degree)
	ax1.set_xlabel("#Thread", fontsize=default_fontsize)


	bw_reg_x_axis_label_list = ["THR-20", "BW-REG"]
	x_axis = np.arange(len(bw_reg_x_axis_label_list))
	bar_width = 0.6

	thr_20_buildpart_time_list = []
	bw_reg_buildpart_time_list = []

	tmp_buildpart_time, _, _ = \
		bw_reg_get_data_from_log_thread_scalablitlity(
			os.path.join("../../logs/0815-logs/", "rdx_hro_thr_20_nvm.log")
		)
	thr_20_buildpart_time_list.append(tmp_buildpart_time)
	thr_20_buildpart_time_list.append(0)

	bw_reg_buildpart_time_list.append(0)
	tmp_buildpart_time, _, _ = \
		bw_reg_get_data_from_log_thread_scalablitlity(
			os.path.join("../../logs/0815-logs/", "rdx_bw_reg_11_hro_nvm.log")
		)
	bw_reg_buildpart_time_list.append(tmp_buildpart_time)


	ax2.bar(
		x_axis, thr_20_buildpart_time_list,
		width=bar_width, edgecolor="salmon", color="none",
		hatch="//", 
		# label=algo2legend[algo]
	)
	ax2.text(
		x_axis[0]-0.2,
		thr_20_buildpart_time_list[0]+0.06,
		str( round(thr_20_buildpart_time_list[0], 2) ),
		fontsize=default_fontsize-2
	)

	ax2.bar(
		x_axis, bw_reg_buildpart_time_list,
		width=bar_width, edgecolor="lightseagreen", color="none",
		hatch="\\", 
		# label=algo2legend[algo]
	)
	ax2.text(
		x_axis[1]-0.2,
		bw_reg_buildpart_time_list[1]+0.06,
		str( round(bw_reg_buildpart_time_list[1], 2) ),
		fontsize=default_fontsize-2
	)

	ax2.set_ylim(0, 1.63)
	ax2.set_yticks(np.arange(0, 1.6, 0.25))
	ax2.set_xticks(np.arange(0, len(x_axis), 1))
	ax2.set_xticklabels(bw_reg_x_axis_label_list, fontsize=default_fontsize)

	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}_h_bw_reg.png".format(core_setting, mem_type)), bbox_inches="tight", format="png")
	plt.title("", fontsize=20)
	plt.savefig(os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}_h_bw_reg.pdf".format(core_setting, mem_type)), bbox_inches="tight", format="pdf")
	os.system("pdftops -eps {} {}".format(
			os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}_h_bw_reg.pdf".format(core_setting, mem_type)),
			os.path.join(FIG_PATH, "phj_part_thread_scalability_runtime_{}_{}_h_bw_reg.eps".format(core_setting, mem_type))
		)
	)

	plt.close()



if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	# for mem_type in ["dram", "nvm"]:
	for mem_type in ["nvm"]:
		for core_setting in core_setting_list:
			plot_thread_scalability(mem_type, core_setting)
	
	# plot_thread_scalability("nvm", "hyperthreading")





































