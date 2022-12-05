import copy
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import sys

from utils import *

LOG_PATH = "../../logs/phj-part-swwcbnt-logs/"
FIG_PATH = "../../figs/phj-part-swwcbnt-figs/"

MIN_POW=6
MAX_POW=11

x_axis = [i+1 for i in range(MAX_POW-MIN_POW+1)]
x_axis.insert(0, 0)
x_axis_label_list = ["REG", "64B", "128B", "256B", "512B", "1KB", "2KB"]

x_axis_slice = slice(0, len(x_axis_label_list), 2)
x_data_width = 0.8

intercept_idx = len(x_axis_label_list)
rotation_degree = 30



def plot_swwcb(mem_type, im_mem_type):
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

	for idx, algo in enumerate(part_algo):
		buildpart_time_list, probejoin_time_list, total_time_list = get_data_from_log(algo, mem_type, im_mem_type)
		general_time_list[idx] = [buildpart_time_list, probejoin_time_list, total_time_list]

	x_axis = np.arange(len(x_axis_label_list))
	# bar_width = x_data_width/len(general_time_list)
	# x_starting_idx_list = [ -x_data_width/2 + (i+1/2) * bar_width for i in range(len(general_time_list)) ]

	default_fontsize = 17

	fig = plt.figure(figsize=(6.5, 3))
	# plt.style.use("ggplot")
	# plt.style.use("seaborn-white")
	plt.title("phj part time w.r.t. swwcb size {}, im-{}".format(mem_type, im_mem_type), fontsize=default_fontsize)
	for idx, time_list in enumerate(general_time_list):
		plt.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
			color=color_list[idx], marker=marker_list[idx], label=legend_list[idx])
		print(im_mem_type, part_algo[idx], time_list[0])
	print()

	# plt.grid(True)
	# plt.yscale("symlog")
	# plt.legend(loc=0, ncol=2, fontsize=default_fontsize, columnspacing=1)
	plt.legend(loc="upper center", bbox_to_anchor=(0.495, 1.41),
		fancybox=True, shadow=True, ncol=3, fontsize=default_fontsize, columnspacing=1.15
	)
	plt.ylabel("Partition Phase Time (s)", fontsize=default_fontsize)
	plt.xlabel("SWWCB SIZE", fontsize=default_fontsize)
	# plt.xticks([index for index in x_axis][:intercept_idx][x_axis_slice], x_axis_label_list[:intercept_idx][x_axis_slice], fontsize=default_fontsize)
	plt.xticks([index for index in x_axis], x_axis_label_list, fontsize=default_fontsize, rotation=rotation_degree)
	plt.yticks(fontsize=default_fontsize)
	# plt.ylim(top=10)
	# plt.gca().yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2fs"))
	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "phj_part_swwcb_runtime_{}_im_{}.png".format(mem_type, im_mem_type)), bbox_inches="tight", format="png")
	plt.title("", fontsize=20)
	plt.savefig(os.path.join(FIG_PATH, "phj_part_swwcb_runtime_{}_im_{}.pdf".format(mem_type, im_mem_type)), bbox_inches="tight", format="pdf")
	os.system("pdftops -eps {} {}".format(
			os.path.join(FIG_PATH, "phj_part_swwcb_runtime_{}_im_{}.pdf".format(mem_type, im_mem_type)),
			os.path.join(FIG_PATH, "phj_part_swwcb_runtime_{}_im_{}.eps".format(mem_type, im_mem_type))
		)
	)

	plt.close()






def plot_row_figs(mem_type):
	phj_shr_im_dram_list = []
	phj_shr_uni_im_dram_list = []
	phj_ind_hro_im_dram_list = []
	phj_ind_uni_hro_im_dram_list = []
	phj_radix_hro_im_dram_list = []
	phj_asymm_radix_hro_im_dram_list = []

	general_time_im_dram_list = [
		phj_shr_im_dram_list,
		phj_shr_uni_im_dram_list,
		phj_ind_hro_im_dram_list,
		phj_ind_uni_hro_im_dram_list,
		phj_radix_hro_im_dram_list,
		phj_asymm_radix_hro_im_dram_list
	]

	for idx, algo in enumerate(part_algo):
		buildpart_time_list, probejoin_time_list, total_time_list = get_data_from_log(algo, mem_type, "dram")
		general_time_im_dram_list[idx] = [buildpart_time_list, probejoin_time_list, total_time_list]

	phj_shr_im_nvm_list = []
	phj_shr_uni_im_nvm_list = []
	phj_ind_hro_im_nvm_list = []
	phj_ind_uni_hro_im_nvm_list = []
	phj_radix_hro_im_nvm_list = []
	phj_asymm_radix_hro_im_nvm_list = []

	general_time_im_nvm_list = [
		phj_shr_im_nvm_list,
		phj_shr_uni_im_nvm_list,
		phj_ind_hro_im_nvm_list,
		phj_ind_uni_hro_im_nvm_list,
		phj_radix_hro_im_nvm_list,
		phj_asymm_radix_hro_im_nvm_list
	]

	for idx, algo in enumerate(part_algo):
		buildpart_time_list, probejoin_time_list, total_time_list = get_data_from_log(algo, mem_type, "nvm")
		general_time_im_nvm_list[idx] = [buildpart_time_list, probejoin_time_list, total_time_list]


	default_fontsize = 14
	title_y_coord = -0.46

	fig = plt.figure(figsize=(9.6, 3))
	plt.subplots_adjust(wspace=0.1)
	ax1 = plt.subplot(121)
	ax2 = plt.subplot(122)

	ax1.set_title("(a) In-DRAM SWWCBs", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
	for idx, time_list in enumerate(general_time_im_dram_list):
		ax1.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
			color=color_list[idx], marker=marker_list[idx], label=legend_list[idx])

	# ax1.legend(loc="upper center", bbox_to_anchor=(0.495, 1.715), 
	# 	fancybox=True, shadow=True, ncol=3, fontsize=default_fontsize, columnspacing=1.2
	# )
	# ax1.set_yscale("log")
	ax1.set_ylabel("Partition Phase Time (s)", fontsize=default_fontsize)
	ax1.tick_params(axis='y', labelsize=default_fontsize)
	ax1.set_yticks(np.arange(0, 8, 1))
	ax1.set_xticks(np.arange(0, len(x_axis), 1))
	ax1.set_xticklabels(x_axis_label_list, fontsize=default_fontsize, rotation=rotation_degree)
	ax1.set_xlabel("SWWCB Size", fontsize=default_fontsize)


	ax2.set_title("(b) In-SCM SWWCBs", fontsize=default_fontsize, fontweight="bold", y=title_y_coord)
	for idx, time_list in enumerate(general_time_im_nvm_list):
		ax2.plot(x_axis[:intercept_idx], time_list[0][:intercept_idx], 
			color=color_list[idx], marker=marker_list[idx], label=legend_list[idx])

	ax2.tick_params(axis='y', labelsize=default_fontsize)
	ax2.set_yticks(np.arange(0, 8, 1))
	ax2.set_xticks(np.arange(0, len(x_axis), 1))
	ax2.set_xticklabels(x_axis_label_list, fontsize=default_fontsize, rotation=rotation_degree)
	ax2.set_xlabel("SWWCB Size", fontsize=default_fontsize)


	# plt.legend(loc=0, ncol=2, fontsize=default_fontsize, columnspacing=1)
	plt.legend(loc="upper center", bbox_to_anchor=(-0.1, 1.2),
		fancybox=True, shadow=True, frameon=False,
		ncol=6, fontsize=default_fontsize, columnspacing=1.15
	)
	# plt.show()
	plt.savefig(os.path.join(FIG_PATH, "phj_part_swwcb_runtime_row_figures_{}.png".format(mem_type)), bbox_inches="tight", format="png")
	# plt.savefig(os.path.join(FIG_PATH, "phj_part_swwcb_runtime_row_figures_{}.pdf".format(mem_type)), bbox_inches="tight", format="pdf")
	# os.system("pdftops -eps {} {}".format(
	# 		os.path.join(FIG_PATH, "phj_part_swwcb_runtime_row_figures_{}.pdf".format(mem_type)),
	# 		os.path.join(FIG_PATH, "phj_part_swwcb_runtime_row_figures_{}.eps".format(mem_type))
	# 	)
	# )

	plt.close()





if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)

	for im_mem_type in ["dram", "nvm"]:
		plot_swwcb("nvm", im_mem_type)

	plot_row_figs("nvm")




































