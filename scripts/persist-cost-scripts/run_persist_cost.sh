date

DIR_PATH=../../logs/persist-cost-logs
mkdir -p $DIR_PATH


for byte in {16,32,64,128,256}
do

	PERSIST_COST_LOG=$DIR_PATH/persist_cost_$byte.log
	rm $PERSIST_COST_LOG


	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DUSE_NVM=true -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DTUPLE_T_SIZE=$byte
	make -j 32
	cd ../scripts/persist-cost-scripts

	echo REGULAR >> $PERSIST_COST_LOG
	for (( i = 0; i < 11; i++ )); do
		numactl --physcpubind=0 --membind=0 ../../bin/microbench/persist_cost >> $PERSIST_COST_LOG
	done
	echo >> $PERSIST_COST_LOG


	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DUSE_NVM=true -DCLFLUSH=true -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DTUPLE_T_SIZE=$byte
	make -j 32
	cd ../scripts/persist-cost-scripts

	echo CLFLUSH >> $PERSIST_COST_LOG
	for (( i = 0; i < 11; i++ )); do
		numactl --physcpubind=0 --membind=0 ../../bin/microbench/persist_cost >> $PERSIST_COST_LOG
	done
	echo >> $PERSIST_COST_LOG


	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DUSE_NVM=true -DCLFLUSH=false -DCLFLUSHOPT=true -DNT_STORE=false -DENABLE_FENCE=false -DTUPLE_T_SIZE=$byte
	make -j 32
	cd ../scripts/persist-cost-scripts

	echo CLFLUSHOPT >> $PERSIST_COST_LOG
	for (( i = 0; i < 11; i++ )); do
		numactl --physcpubind=0 --membind=0 ../../bin/microbench/persist_cost >> $PERSIST_COST_LOG
	done
	echo >> $PERSIST_COST_LOG


	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DUSE_NVM=true -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=true -DENABLE_FENCE=false -DTUPLE_T_SIZE=$byte
	make -j 32
	cd ../scripts/persist-cost-scripts

	echo NT_STORE >> $PERSIST_COST_LOG
	for (( i = 0; i < 11; i++ )); do
		numactl --physcpubind=0 --membind=0 ../../bin/microbench/persist_cost >> $PERSIST_COST_LOG
	done
	echo >> $PERSIST_COST_LOG


	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DUSE_NVM=true -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=true -DTUPLE_T_SIZE=$byte
	make -j 32
	cd ../scripts/persist-cost-scripts

	echo REGULAR + FENCE >> $PERSIST_COST_LOG
	for (( i = 0; i < 11; i++ )); do
		numactl --physcpubind=0 --membind=0 ../../bin/microbench/persist_cost >> $PERSIST_COST_LOG
	done
	echo >> $PERSIST_COST_LOG


	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DUSE_NVM=true -DCLFLUSH=true -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=true -DTUPLE_T_SIZE=$byte
	make -j 32
	cd ../scripts/persist-cost-scripts

	echo CLFLUSH + FENCE >> $PERSIST_COST_LOG
	for (( i = 0; i < 11; i++ )); do
		numactl --physcpubind=0 --membind=0 ../../bin/microbench/persist_cost >> $PERSIST_COST_LOG
	done
	echo >> $PERSIST_COST_LOG


	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DUSE_NVM=true -DCLFLUSH=false -DCLFLUSHOPT=true -DNT_STORE=false -DENABLE_FENCE=true -DTUPLE_T_SIZE=$byte
	make -j 32
	cd ../scripts/persist-cost-scripts

	echo CLFLUSHOPT + FENCE >> $PERSIST_COST_LOG
	for (( i = 0; i < 11; i++ )); do
		numactl --physcpubind=0 --membind=0 ../../bin/microbench/persist_cost >> $PERSIST_COST_LOG
	done
	echo >> $PERSIST_COST_LOG


	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DUSE_NVM=true -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=true -DENABLE_FENCE=true -DTUPLE_T_SIZE=$byte
	make -j 32
	cd ../scripts/persist-cost-scripts

	echo NT_STORE + FENCE >> $PERSIST_COST_LOG
	for (( i = 0; i < 11; i++ )); do
		numactl --physcpubind=0 --membind=0 ../../bin/microbench/persist_cost >> $PERSIST_COST_LOG
	done
	echo >> $PERSIST_COST_LOG


done


date