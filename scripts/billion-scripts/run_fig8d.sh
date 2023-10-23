date
start_time=$(date +%s)

bash datagen.sh > datagen.log
bash billion.sh > billion.log

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date