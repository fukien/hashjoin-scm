import collections
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os
import pandas as pd
import seaborn as sns
import sys

from utils import *


mem_type = "nvm"

LOG_PATH = "../../logs/1022-logs/"
FIG_PATH = "../../figs/1022-figs/"

algo_list = ["ptr_nphj_sc", "ptr_rdx_bc"]
algo_num = len(algo_list)


if __name__ == "__main__":
	event_flag = sys.argv[1]
	event = sys.argv[2]
	for algo in algo_list:
		filepath = os.path.join(LOG_PATH, "{}_payload_nvm_profiling.log".format(algo))
		buildpart_time_list, probejoin_time_list, total_time_list, \
			buildpart_counter_list, probejoin_counter_list, total_counter_list \
			= get_counter_from_log(filepath, event, event_flag)
		retrieve_time_list = np.array(total_time_list) - np.array(buildpart_time_list) - np.array(probejoin_time_list)
		retrieve_time_list = [round4decimals(i) for i in retrieve_time_list]
		retrieve_counter_list = np.array(total_counter_list) - np.array(buildpart_counter_list) - np.array(probejoin_counter_list)
		retrieve_counter_list = [round4decimals(i) for i in retrieve_counter_list]
		print(algo)
		print("\tbuildpart", buildpart_time_list, "\t", buildpart_counter_list)
		print("\tprobejoin", probejoin_time_list, "\t", probejoin_counter_list)
		print("\tretrieve", retrieve_time_list, "\t", retrieve_counter_list)
		print("\ttotal", total_time_list, "\t", total_counter_list)