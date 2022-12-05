date


DIR_PATH=../../logs/phj-part-fanout-logs
mkdir -p $DIR_PATH


MIN_BIT=4
MAX_BIT=16





SHRll_FANOUT_NVM_1_PASS_LOG=$DIR_PATH/phj_shr_fanout_nvm_1_pass.log
rm $SHRll_FANOUT_NVM_1_PASS_LOG
SHRcm_FANOUT_NVM_1_PASS_LOG=$DIR_PATH/phj_shr_uni_fanout_nvm_1_pass.log
rm $SHRcm_FANOUT_NVM_1_PASS_LOG
INDll_FANOUT_NVM_1_PASS_LOG=$DIR_PATH/phj_ind_fanout_nvm_1_pass.log
rm $INDll_FANOUT_NVM_1_PASS_LOG
INDcm_FANOUT_NVM_1_PASS_LOG=$DIR_PATH/phj_ind_uni_fanout_nvm_1_pass.log
rm $INDcm_FANOUT_NVM_1_PASS_LOG
RDX_FANOUT_NVM_1_PASS_LOG=$DIR_PATH/phj_radix_fanout_nvm_1_pass.log
rm $RDX_FANOUT_NVM_1_PASS_LOG



for (( bit =$MIN_BIT ; bit <=$MAX_BIT ; bit++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DSWWCB_SIZE=256 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=$bit -DNUM_RADIX_BITS_PASS1=8 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-part-fanout-scripts
	numactl --membind=0 ../../bin/main --algo=phj_shr_hro --workload=uniform --subtype=A --param=pkfk >> $SHRll_FANOUT_NVM_1_PASS_LOG
	numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hro --workload=uniform --subtype=A --param=pkfk >> $SHRcm_FANOUT_NVM_1_PASS_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_hro --workload=uniform --subtype=A --param=pkfk >> $INDll_FANOUT_NVM_1_PASS_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hro --workload=uniform --subtype=A --param=pkfk >> $INDcm_FANOUT_NVM_1_PASS_LOG
	numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_FANOUT_NVM_1_PASS_LOG
done






SHRll_FANOUT_NVM_2_PASS_LOG=$DIR_PATH/phj_shr_fanout_nvm_2_pass.log
rm $SHRll_FANOUT_NVM_2_PASS_LOG
SHRcm_FANOUT_NVM_2_PASS_LOG=$DIR_PATH/phj_shr_uni_fanout_nvm_2_pass.log
rm $SHRcm_FANOUT_NVM_2_PASS_LOG
INDll_FANOUT_NVM_2_PASS_LOG=$DIR_PATH/phj_ind_fanout_nvm_2_pass.log
rm $INDll_FANOUT_NVM_2_PASS_LOG
INDcm_FANOUT_NVM_2_PASS_LOG=$DIR_PATH/phj_ind_uni_fanout_nvm_2_pass.log
rm $INDcm_FANOUT_NVM_2_PASS_LOG
RDX_FANOUT_NVM_2_PASS_LOG=$DIR_PATH/phj_radix_fanout_nvm_2_pass.log
rm $RDX_FANOUT_NVM_2_PASS_LOG
ASYM_FANOUT_NVM_2_PASS_LOG=$DIR_PATH/phj_asymm_radix_fanout_nvm_2_pass.log
rm $ASYM_FANOUT_NVM_2_PASS_LOG

for (( bit =$MIN_BIT ; bit <=$MAX_BIT ; bit++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DSWWCB_SIZE=256 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=$bit -DNUM_RADIX_BITS_PASS1=$(($bit/2)) -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-part-fanout-scripts
	numactl --membind=0 ../../bin/main --algo=phj_shr_hro --workload=uniform --subtype=A --param=pkfk >> $SHRll_FANOUT_NVM_2_PASS_LOG
	numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hro --workload=uniform --subtype=A --param=pkfk >> $SHRcm_FANOUT_NVM_2_PASS_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_hro --workload=uniform --subtype=A --param=pkfk >> $INDll_FANOUT_NVM_2_PASS_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hro --workload=uniform --subtype=A --param=pkfk >> $INDcm_FANOUT_NVM_2_PASS_LOG
	numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_FANOUT_NVM_2_PASS_LOG
	numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $ASYM_FANOUT_NVM_2_PASS_LOG
done





cd ../../
./clean.sh



date