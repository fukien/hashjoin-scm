date
start_time=$(date +%s)



bash run_fndly.sh > run_fndly.log

# Generate the dataset
cd ../..
./revitalize.sh
./bin/gen
cd ../scripts/epsilon-asymmetry-scripts

bash pkfk_b.sh > pkfk_b.log



end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date