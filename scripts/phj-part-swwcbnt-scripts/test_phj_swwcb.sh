date


DIR_PATH=../../logs/phj-part-swwcbnt-logs
mkdir -p $DIR_PATH


MIN_POW=6
MAX_POW=11





SHRll_SWWCB_NVM_IM_DRAM_LOG=$DIR_PATH/phj_shr_swwcb_nvm_im_dram.log
rm $SHRll_SWWCB_NVM_IM_DRAM_LOG
SHRcm_SWWCB_NVM_IM_DRAM_LOG=$DIR_PATH/phj_shr_uni_swwcb_nvm_im_dram.log
rm $SHRcm_SWWCB_NVM_IM_DRAM_LOG
INDll_SWWCB_NVM_IM_DRAM_LOG=$DIR_PATH/phj_ind_swwcb_nvm_im_dram.log
rm $INDll_SWWCB_NVM_IM_DRAM_LOG
INDcm_SWWCB_NVM_IM_DRAM_LOG=$DIR_PATH/phj_ind_uni_swwcb_nvm_im_dram.log
rm $INDcm_SWWCB_NVM_IM_DRAM_LOG
RDX_SWWCB_NVM_IM_DRAM_LOG=$DIR_PATH/phj_radix_swwcb_nvm_im_dram.log
rm $RDX_SWWCB_NVM_IM_DRAM_LOG
ASYM_SWWCB_NVM_IM_DRAM_LOG=$DIR_PATH/phj_asymm_radix_swwcb_nvm_im_dram.log
rm $ASYM_SWWCB_NVM_IM_DRAM_LOG



cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=8 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=false -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/phj-part-swwcbnt-scripts
numactl --membind=0 ../../bin/main --algo=phj_shr_hro --workload=uniform --subtype=A --param=pkfk >> $SHRll_SWWCB_NVM_IM_DRAM_LOG
numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hro --workload=uniform --subtype=A --param=pkfk >> $SHRcm_SWWCB_NVM_IM_DRAM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_hro --workload=uniform --subtype=A --param=pkfk >> $INDll_SWWCB_NVM_IM_DRAM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hro --workload=uniform --subtype=A --param=pkfk >> $INDcm_SWWCB_NVM_IM_DRAM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_SWWCB_NVM_IM_DRAM_LOG
numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $ASYM_SWWCB_NVM_IM_DRAM_LOG
for (( pow =$MIN_POW ; pow <=$MAX_POW ; pow++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DSWWCB_SIZE=$((2**$pow)) -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=8 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-part-swwcbnt-scripts
	numactl --membind=0 ../../bin/main --algo=phj_shr_hro --workload=uniform --subtype=A --param=pkfk >> $SHRll_SWWCB_NVM_IM_DRAM_LOG
	numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hro --workload=uniform --subtype=A --param=pkfk >> $SHRcm_SWWCB_NVM_IM_DRAM_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_hro --workload=uniform --subtype=A --param=pkfk >> $INDll_SWWCB_NVM_IM_DRAM_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hro --workload=uniform --subtype=A --param=pkfk >> $INDcm_SWWCB_NVM_IM_DRAM_LOG
	numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_SWWCB_NVM_IM_DRAM_LOG
	numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $ASYM_SWWCB_NVM_IM_DRAM_LOG
done






SHRll_SWWCB_NVM_IM_NVM_LOG=$DIR_PATH/phj_shr_swwcb_nvm_im_nvm.log
rm $SHRll_SWWCB_NVM_IM_NVM_LOG
SHRcm_SWWCB_NVM_IM_NVM_LOG=$DIR_PATH/phj_shr_uni_swwcb_nvm_im_nvm.log
rm $SHRcm_SWWCB_NVM_IM_NVM_LOG
INDll_SWWCB_NVM_IM_NVM_LOG=$DIR_PATH/phj_ind_swwcb_nvm_im_nvm.log
rm $INDll_SWWCB_NVM_IM_NVM_LOG
INDcm_SWWCB_NVM_IM_NVM_LOG=$DIR_PATH/phj_ind_uni_swwcb_nvm_im_nvm.log
rm $INDcm_SWWCB_NVM_IM_NVM_LOG
RDX_SWWCB_NVM_IM_NVM_LOG=$DIR_PATH/phj_radix_swwcb_nvm_im_nvm.log
rm $RDX_SWWCB_NVM_IM_NVM_LOG
ASYM_SWWCB_NVM_IM_NVM_LOG=$DIR_PATH/phj_asymm_radix_swwcb_nvm_im_nvm.log
rm $ASYM_SWWCB_NVM_IM_NVM_LOG



cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=8 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=false -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/phj-part-swwcbnt-scripts
numactl --membind=0 ../../bin/main --algo=phj_shr_hro --workload=uniform --subtype=A --param=pkfk >> $SHRll_SWWCB_NVM_IM_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hro --workload=uniform --subtype=A --param=pkfk >> $SHRcm_SWWCB_NVM_IM_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_hro --workload=uniform --subtype=A --param=pkfk >> $INDll_SWWCB_NVM_IM_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hro --workload=uniform --subtype=A --param=pkfk >> $INDcm_SWWCB_NVM_IM_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_SWWCB_NVM_IM_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $ASYM_SWWCB_NVM_IM_NVM_LOG
for (( pow =$MIN_POW ; pow <=$MAX_POW ; pow++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DSWWCB_SIZE=$((2**$pow)) -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=8 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=true  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=true
	make -j 32
	cd ../scripts/phj-part-swwcbnt-scripts
	numactl --membind=0 ../../bin/main --algo=phj_shr_hro --workload=uniform --subtype=A --param=pkfk >> $SHRll_SWWCB_NVM_IM_NVM_LOG
	numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hro --workload=uniform --subtype=A --param=pkfk >> $SHRcm_SWWCB_NVM_IM_NVM_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_hro --workload=uniform --subtype=A --param=pkfk >> $INDll_SWWCB_NVM_IM_NVM_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hro --workload=uniform --subtype=A --param=pkfk >> $INDcm_SWWCB_NVM_IM_NVM_LOG
	numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_SWWCB_NVM_IM_NVM_LOG
	numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $ASYM_SWWCB_NVM_IM_NVM_LOG
done






cd ../../
./clean.sh



date