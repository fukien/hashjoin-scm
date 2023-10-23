date
start_time=$(date +%s)

bash run_persist_cost.sh > run_persist_cost.log
bash run_page_fault_cost_1.sh > run_page_fault_cost_1.log

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date