date


MIN_POW_NUM=4
MAX_POW_NUM=9


for (( i = $MIN_POW_NUM; i <= $MAX_POW_NUM; i++ )); do
	mkdir ../../build
	cd ../../build
	rm -rf *
	cmake .. -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=$((2**$i))
	make -j 32
	cd ../scripts/payload-scripts/
	numactl --physcpubind=0-19 --membind=0-1 ../../bin/gen
done



date