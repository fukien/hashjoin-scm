import copy
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from matplotlib import gridspec
import numpy as np
import os
import sys

from utils import *

LOG_PATH = "../../logs/phj-join-fanout-logs/"
FIG_PATH = "../../figs/phj-join-fanout-figs/"

MIN_BIT=4
MAX_BIT=16
ASYM_MIN_BIT=12

x_axis = [i for i in range(MIN_BIT, MAX_BIT+1)]
x_axis_label_list = ["$2^{" + str(i) + "}$" for i in x_axis]
x_axis = [i+1 for i in range(MAX_BIT-MIN_BIT+1)]
x_axis.insert(0, 0)

x_axis_slice = slice(1, len(x_axis_label_list), 2)
x_data_width = 0.8

start_intercept_idx = 5
end_intercept_idx = len(x_axis_label_list)

rotation_degree = 30



def plot_row_figs(mem_type):
	phj_shrll_sc_list = []
	phj_shrll_lp_list = []
	phj_shrll_hro_list = []
	phj_shrcm_bc_list = []
	phj_shrcm_sc_list = []
	phj_shrcm_lp_list = []
	phj_shrcm_hro_list = []
	phj_radix_bc_list = []
	phj_radix_sc_list = []
	phj_radix_lp_list = []
	phj_radix_hro_list = []

	general_time_list = {
		"shrll_sc": phj_shrll_sc_list,
		"shrll_lp": phj_shrll_lp_list,
		"shrll_hro": phj_shrll_hro_list,
		"shrcm_bc": phj_shrcm_bc_list,
		"shrcm_sc": phj_shrcm_sc_list,
		"shrcm_lp": phj_shrcm_lp_list,
		"shrcm_hro": phj_shrcm_hro_list,
		"rdx_bc": phj_radix_bc_list,
		"rdx_sc": phj_radix_sc_list,
		"rdx_lp": phj_radix_lp_list,
		"rdx_hro": phj_radix_hro_list,
	}

	for algo_part in part_algo:
		for algo_join in join_algo[algo_part]:
			algo = "{}_{}".format(algo_part, algo_join)
			buildpart_time_list, probejoin_time_list, total_time_list = get_data_from_log(algo, mem_type)
			general_time_list[algo] = [buildpart_time_list, probejoin_time_list, total_time_list]


	default_fontsize = 14

	fig = plt.figure(figsize=(9.66, 3))
	plt.subplots_adjust(wspace=0.14)
	gs = gridspec.GridSpec(1, 2, 
		width_ratios=[end_intercept_idx, 
		end_intercept_idx - start_intercept_idx]
	) 
	# ax1 = plt.subplot(121)
	# ax2 = plt.subplot(122)
	ax1 = plt.subplot(gs[0])
	ax2 = plt.subplot(gs[1])


	ax1.set_title("", fontsize=default_fontsize, fontweight="bold")
	for algo_part in part_algo:
		for algo_join in join_algo[algo_part]:
			algo = "{}_{}".format(algo_part, algo_join)
			ax1.plot(x_axis[:end_intercept_idx], 
				general_time_list[algo][1][:end_intercept_idx], 
				color=part2color[algo_part], 
				marker=join2marker[algo_join], fillstyle=join2fillstyle[algo_join], 
				markeredgecolor="black", markerfacecolor=join2markerfacecolor[algo_join],
				label="{}_{}".format(part2legend[algo_part], join2legend[algo_join]),
				linestyle=part2linestyle[algo_part]
			)

	ax1.legend(loc="upper right", bbox_to_anchor=(1.03, 1.05),
		ncol=2, fontsize=default_fontsize, columnspacing=0.5, 
		fancybox=False, shadow=False, edgecolor="none", facecolor="none"
	)
	ax1.set_ylabel("Join Phase Time (s)", fontsize=default_fontsize)
	ax1.tick_params(axis='y', labelsize=default_fontsize)
	ax1.set_yticks(np.arange(0, 6, 1))
	ax1.set_xticks(np.arange(0, len(x_axis), 1))
	ax1.set_xticklabels(x_axis_label_list, fontsize=default_fontsize, rotation=rotation_degree)
	ax1.set_xlabel("#Fanout", fontsize=default_fontsize)



	# ax2.set_title("", fontsize=default_fontsize, fontweight="bold")
	for algo_part in part_algo:
		for algo_join in join_algo[algo_part]:
			algo = "{}_{}".format(algo_part, algo_join)
			ax2.plot(x_axis[start_intercept_idx:end_intercept_idx], 
				general_time_list[algo][1][start_intercept_idx:end_intercept_idx], 
				color=part2color[algo_part], 
				marker=join2marker[algo_join], fillstyle=join2fillstyle[algo_join], 
				markeredgecolor="black", markerfacecolor=join2markerfacecolor[algo_join],
				label="{}_{}".format(part2legend[algo_part], join2legend[algo_join]),
				linestyle=part2linestyle[algo_part]
			)

	ax2.tick_params(axis='y', labelsize=default_fontsize)
	ax2.set_yticks(np.arange(0.1, 0.6, 0.1)[1:])
	ax2.set_xticks(np.arange(start_intercept_idx, end_intercept_idx, 1))
	ax2.set_xticklabels(x_axis_label_list[start_intercept_idx:end_intercept_idx], 
		fontsize=default_fontsize, rotation=rotation_degree)
	ax2.set_xlabel("#Fanout", fontsize=default_fontsize)


	# plt.legend(loc="lower center", bbox_to_anchor=(-0.475, -0.625),
	# 	fancybox=True, shadow=True, ncol=6, fontsize=default_fontsize, columnspacing=0.5
	# )
	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "phj_joinphase_fanout_row_figures_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	# plt.savefig(os.path.join(FIG_PATH, "phj_joinphase_fanout_row_figures_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	# os.system("/Applications/Inkscape.app/Contents/MacOS/inkscape {} --export-eps={}".format(
	# 		os.path.join(FIG_PATH, "phj_joinphase_fanout_row_figures_{}.pdf".format(mem_type)),
	# 		os.path.join(FIG_PATH, "phj_joinphase_fanout_row_figures_{}.eps".format(mem_type))
	# 	)
	# )

	plt.close()





if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	plot_row_figs("nvm")

























