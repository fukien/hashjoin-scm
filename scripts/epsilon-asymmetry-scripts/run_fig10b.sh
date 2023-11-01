date
start_time=$(date +%s)


# Free up the dataset space for other workloads
rm -rf /pmemfs0/hashjoin-scm/data
rm -rf /pmemfs1/hashjoin-scm/data

bash run_fndly.sh > run_fndly.log

# Generate the dataset
cd ../..
./revitalize.sh
./bin/gen
cd ../scripts/epsilon-asymmetry-scripts

bash pkfk_b.sh > pkfk_b.log


# Free up the dataset space for other workloads
rm -rf /pmemfs0/hashjoin-scm/data
rm -rf /pmemfs1/hashjoin-scm/data


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date