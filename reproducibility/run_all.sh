# Record the start time for benchmarking
start_time=$(date +%s)

# Reproduce the experiments of Figure 1
cd fig1-scripts
bash run_fig1.sh
cd ..

# Generate the dataset
cd ..
./revitalize.sh
./bin/gen
cd reproducibility/

# # Reproduce the experiments of Figure 3
# # Note: You may need "sudo" to access the counters
# cd fig3-scripts
# bash run_fig3.sh
# cd ..

# Reproduce the experiments of Figure 4a
cd fig4a-scripts
bash run_fig4a.sh
cd ..

# # Reproduce the experiments of Figure 4b 
# # Note: You may need "sudo" to access the counters
# cd fig4b-scripts
# sudo bash run_fig4b.sh
# cd ..

# Reproduce the experiments of Figure 5
cd fig5-scripts
bash run_fig5.sh
cd ..

# Reproduce the experiments of Figure 6
cd fig6-scripts
bash run_fig6.sh
cd ..

# Reproduce the experiments of Figure 7
cd fig7-scripts
bash run_fig7.sh
cd ..

# Reproduce the experiments of Figure 8(a)
cd fig8a-scripts
bash run_fig8a.sh
cd ..

# Reproduce the experiments of Figure 8(b)
cd fig8b-scripts
bash run_fig8b.sh
cd ..

# Reproduce the experiments of Figure 8(c)
cd fig8c-scripts
bash run_fig8c.sh
cd ..

# Free up the dataset space for billion-scale workloads
rm -rf /pmemfs0/hashjoin-scm/data

# Reproduce the experiments of Figure 8(d) (billion-scale workloads)
cd fig8d-scripts
bash run_fig8d.sh
cd ..


# Free up the dataset space for large-size-payload workloads
rm -rf /pmemfs0/hashjoin-scm/data
rm -rf /pmemfs1/hashjoin-scm/data

# Reproduce the experiments of Figure 9 (large-size payloads)
cd fig9-scripts
bash run_fig9.sh
cd ..


# Free up the dataset space for read/write asymmetry workloads
rm -rf /pmemfs0/hashjoin-scm/data
rm -rf /pmemfs1/hashjoin-scm/data

# # Reproduce the experiments of Figure 10(b) (read/write asymmetry)
# cd fig10-scripts
# bash run_fig10b.sh
# cd ..


# # Free up the dataset space for other workloads
# rm -rf /pmemfs0/hashjoin-scm/data
# rm -rf /pmemfs1/hashjoin-scm/data



# Record the end time for benchmarking
end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"

# Display the current date and time
date
