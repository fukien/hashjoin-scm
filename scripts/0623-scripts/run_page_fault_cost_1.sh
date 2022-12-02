date



DIR_PATH=../../logs/0623-logs
mkdir -p $DIR_PATH

PAGE_FAULT_COST_1_DRAM_LOG=$DIR_PATH/page_fault_cost_1_dram.log
rm $PAGE_FAULT_COST_1_DRAM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DUSE_NVM=false -DUSE_HUGE=false -DUSE_ALIGNMENT=false
make -j 32
cd ../scripts/0623-scripts
for (( i = 0; i < 11; i++ )); do
	numactl --physcpubind=0 --membind=0 ../../bin/microbench/page_fault_cost_1 >> $PAGE_FAULT_COST_1_DRAM_LOG
done


PAGE_FAULT_COST_1_DRAM_ALIGNMENT_LOG=$DIR_PATH/page_fault_cost_1_dram_alignment.log
rm $PAGE_FAULT_COST_1_DRAM_ALIGNMENT_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DUSE_NVM=false -DUSE_HUGE=false -DUSE_ALIGNMENT=true
make -j 32
cd ../scripts/0623-scripts
for (( i = 0; i < 11; i++ )); do
	numactl --physcpubind=0 --membind=0 ../../bin/microbench/page_fault_cost_1 >> $PAGE_FAULT_COST_1_DRAM_ALIGNMENT_LOG
done


PAGE_FAULT_COST_1_DRAM_HUGE_LOG=$DIR_PATH/page_fault_cost_1_dram_huge.log
rm $PAGE_FAULT_COST_1_DRAM_HUGE_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DUSE_NVM=false -DUSE_HUGE=true -DUSE_ALIGNMENT=false
make -j 32
cd ../scripts/0623-scripts
for (( i = 0; i < 11; i++ )); do
	numactl --physcpubind=0 --membind=0 ../../bin/microbench/page_fault_cost_1 >> $PAGE_FAULT_COST_1_DRAM_HUGE_LOG
done


PAGE_FAULT_COST_1_NVM_LOG=$DIR_PATH/page_fault_cost_1_nvm.log
rm $PAGE_FAULT_COST_1_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DUSE_NVM=true -DUSE_HUGE=false -DUSE_ALIGNMENT=false
make -j 32
cd ../scripts/0623-scripts
for (( i = 0; i < 11; i++ )); do
	numactl --physcpubind=0 --membind=0 ../../bin/microbench/page_fault_cost_1 >> $PAGE_FAULT_COST_1_NVM_LOG
done



date