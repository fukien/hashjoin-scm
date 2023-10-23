date
start_time=$(date +%s)

bash test_nphj_uniform_pkfk_A.sh > test_nphj_uniform_pkfk_A.log
bash test_phj_uniform_pkfk_A.sh > test_phj_uniform_pkfk_A.log

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date