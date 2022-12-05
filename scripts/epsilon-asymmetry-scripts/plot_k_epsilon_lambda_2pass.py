import matplotlib.pyplot as plt
import numpy as np
import os

FIG_PATH = "../../figs/epsilon-asymmetry-figs/"

def k_func(x, labd, k):
	return (1+x) * (5 + 2*labd) / ( 5 + (2+k)*x + (2+x)*labd )

def k_func_ara(x, labd, k_ara):
	return (1+x) * (5 + 2*labd) / ( 5 + (2+k_ara)*x + (2+x)*labd )



if __name__ == "__main__":
	if not os.path.exists(FIG_PATH):
		os.makedirs(FIG_PATH)
	k_list = [pow(2, i) for i in range(1, 5)]
	k_array = np.array(k_list)

	default_fontsize = 12
	fontsize_increment = 9
	fontsize_minor_increment = 3

	'''
	print(k_func_ara(16, labd, k_array))
	print()
	for k in k_list:
		print(k_func(16, labd, k))
	'''
	x_times_list = [ pow(2, i) for i in range(1, 6) ]

	x_axis = [i for i in range(len(k_list))]
	x_axis_label_list = [ str(i) for i in k_list ]
	y_axis = [i for i in range(len(x_times_list))]
	y_axis_label_list = [ str(i) for i in x_times_list ]

	# boundaries = [0.2*i for i in range(10)]

	labd = 4.36
	grid = []
	for x_times in x_times_list:
		grid.append( k_func_ara(x_times, labd, k_array) )
	grid = np.array(grid)

	fig = plt.figure(figsize=(3.5, 3))
	plt.rcParams.update({
		"text.usetex": True,
	# 	"font.family": "sans-serif",
	# 	# "font.sans-serif": "Helvetica",
		"font.weight": "bold"
	})
	plt.title("$\lambda$={}".format(labd), fontsize=default_fontsize+fontsize_increment+fontsize_minor_increment, fontweight="bold")
	im = plt.imshow(grid, interpolation="bilinear",
		cmap="viridis"
	)
	plt.ylabel("$x$", fontsize=default_fontsize+fontsize_increment+fontsize_minor_increment, 
		fontweight="bold", labelpad=-8)
	plt.xlabel("$k$", fontsize=default_fontsize+fontsize_increment+fontsize_minor_increment, 
		fontweight="bold", labelpad=-8)
	plt.yticks(ticks=y_axis, labels=y_axis_label_list, fontsize=default_fontsize+fontsize_increment)
	plt.xticks(ticks=x_axis, labels=x_axis_label_list, fontsize=default_fontsize+fontsize_increment)
	# plt.show()
	# plt.colorbar(boundaries=boundaries)
	cbar = plt.colorbar(im)
	cbar.set_ticks(np.arange(0.75, 1.55, 0.25))
	# cbar.set_ticklabels(["hello", "world", "neumann", "shannon"])
	cbar.ax.tick_params(labelsize=default_fontsize+fontsize_increment)
	plt.savefig(os.path.join(FIG_PATH, "2-pass-rdx-asymm-lambda-{}.png".format(labd) ), bbox_inches="tight", format="png")
	# # plt.title("", fontsize=default_fontsize, fontweight="bold")
	# plt.savefig(os.path.join(FIG_PATH, "2-pass-rdx-asymm-lambda-{}.pdf".format(labd) ), bbox_inches="tight", format="pdf")
	# os.system("pdftops -eps {} {}".format(
	# 		os.path.join(FIG_PATH, "2-pass-rdx-asymm-lambda-{}.pdf".format(labd) ),
	# 		os.path.join(FIG_PATH, "2-pass-rdx-asymm-lambda-{}.eps".format(labd) )
	# 	)
	# )
	plt.close()
