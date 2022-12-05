date


DIR_PATH=../../logs/phj-join-thread-scalability-logs
mkdir -p $DIR_PATH



SHRll_SC_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/shrll_sc_nvm_hyperthreading.log
rm $SHRll_SC_FANOUT_NVM_HYPERTHREADING_LOG
SHRll_LP_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/shrll_lp_nvm_hyperthreading.log
rm $SHRll_LP_FANOUT_NVM_HYPERTHREADING_LOG
SHRll_HR_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/shrll_hr_nvm_hyperthreading.log
rm $SHRll_HR_FANOUT_NVM_HYPERTHREADING_LOG
SHRll_HRO_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/shrll_hro_nvm_hyperthreading.log
rm $SHRll_HRO_FANOUT_NVM_HYPERTHREADING_LOG

for (( thr = 1; thr <= 20; thr++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DSWWCB_SIZE=256 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=40 -DPROBEJOIN_THREAD_NUM=$((2*$thr)) -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=8 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-join-thread-scalability-scripts
	numactl --membind=0 ../../bin/main --algo=phj_shr_sc --workload=uniform --subtype=A --param=pkfk >> $SHRll_SC_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_shr_lp --workload=uniform --subtype=A --param=pkfk >> $SHRll_LP_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_shr_hr --workload=uniform --subtype=A --param=pkfk >> $SHRll_HR_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_shr_hro --workload=uniform --subtype=A --param=pkfk >> $SHRll_HRO_FANOUT_NVM_HYPERTHREADING_LOG
done



SHRcm_BC_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/shrcm_bc_nvm_hyperthreading.log
rm $SHRcm_BC_FANOUT_NVM_HYPERTHREADING_LOG
SHRcm_SC_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/shrcm_sc_nvm_hyperthreading.log
rm $SHRcm_SC_FANOUT_NVM_HYPERTHREADING_LOG
SHRcm_LP_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/shrcm_lp_nvm_hyperthreading.log
rm $SHRcm_LP_FANOUT_NVM_HYPERTHREADING_LOG
SHRcm_HR_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/shrcm_hr_nvm_hyperthreading.log
rm $SHRcm_HR_FANOUT_NVM_HYPERTHREADING_LOG
SHRcm_HRO_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/shrcm_hro_nvm_hyperthreading.log
rm $SHRcm_HRO_FANOUT_NVM_HYPERTHREADING_LOG

for (( thr = 1; thr <= 20; thr++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DSWWCB_SIZE=256 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=40 -DPROBEJOIN_THREAD_NUM=$((2*$thr)) -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=8 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-join-thread-scalability-scripts
	numactl --membind=0 ../../bin/main --algo=phj_shr_uni_bc --workload=uniform --subtype=A --param=pkfk >> $SHRcm_BC_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_shr_uni_sc --workload=uniform --subtype=A --param=pkfk >> $SHRcm_SC_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_shr_uni_lp --workload=uniform --subtype=A --param=pkfk >> $SHRcm_LP_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hr --workload=uniform --subtype=A --param=pkfk >> $SHRcm_HR_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hro --workload=uniform --subtype=A --param=pkfk >> $SHRcm_HRO_FANOUT_NVM_HYPERTHREADING_LOG
done




INDll_SC_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/indll_sc_nvm_hyperthreading.log
rm $INDll_SC_FANOUT_NVM_HYPERTHREADING_LOG
INDll_LP_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/indll_lp_nvm_hyperthreading.log
rm $INDll_LP_FANOUT_NVM_HYPERTHREADING_LOG
INDll_HR_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/indll_hr_nvm_hyperthreading.log
rm $INDll_HR_FANOUT_NVM_HYPERTHREADING_LOG
INDll_HRO_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/indll_hro_nvm_hyperthreading.log
rm $INDll_HRO_FANOUT_NVM_HYPERTHREADING_LOG

for (( thr = 1; thr <= 20; thr++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DSWWCB_SIZE=256 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=$((2*$thr)) -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=8 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-join-thread-scalability-scripts
	numactl --membind=0 ../../bin/main --algo=phj_ind_sc --workload=uniform --subtype=A --param=pkfk >> $INDll_SC_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_lp --workload=uniform --subtype=A --param=pkfk >> $INDll_LP_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_hr --workload=uniform --subtype=A --param=pkfk >> $INDll_HR_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_hro --workload=uniform --subtype=A --param=pkfk >> $INDll_HRO_FANOUT_NVM_HYPERTHREADING_LOG
done




INDcm_BC_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/indcm_bc_nvm_hyperthreading.log
rm $INDcm_BC_FANOUT_NVM_HYPERTHREADING_LOG
INDcm_SC_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/indcm_sc_nvm_hyperthreading.log
rm $INDcm_SC_FANOUT_NVM_HYPERTHREADING_LOG
INDcm_LP_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/indcm_lp_nvm_hyperthreading.log
rm $INDcm_LP_FANOUT_NVM_HYPERTHREADING_LOG
INDcm_HR_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/indcm_hr_nvm_hyperthreading.log
rm $INDcm_HR_FANOUT_NVM_HYPERTHREADING_LOG
INDcm_HRO_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/indcm_hro_nvm_hyperthreading.log
rm $INDcm_HRO_FANOUT_NVM_HYPERTHREADING_LOG

for (( thr = 1; thr <= 20; thr++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DSWWCB_SIZE=256 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=$((2*$thr)) -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=8 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-join-thread-scalability-scripts
	numactl --membind=0 ../../bin/main --algo=phj_ind_uni_bc --workload=uniform --subtype=A --param=pkfk >> $INDcm_BC_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_uni_sc --workload=uniform --subtype=A --param=pkfk >> $INDcm_SC_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_uni_lp --workload=uniform --subtype=A --param=pkfk >> $INDcm_LP_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hr --workload=uniform --subtype=A --param=pkfk >> $INDcm_HR_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hro --workload=uniform --subtype=A --param=pkfk >> $INDcm_HRO_FANOUT_NVM_HYPERTHREADING_LOG
done




RDX_BC_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/rdx_bc_nvm_hyperthreading.log
rm $RDX_BC_FANOUT_NVM_HYPERTHREADING_LOG
RDX_SC_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/rdx_sc_nvm_hyperthreading.log
rm $RDX_SC_FANOUT_NVM_HYPERTHREADING_LOG
RDX_LP_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/rdx_lp_nvm_hyperthreading.log
rm $RDX_LP_FANOUT_NVM_HYPERTHREADING_LOG
RDX_HR_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/rdx_hr_nvm_hyperthreading.log
rm $RDX_HR_FANOUT_NVM_HYPERTHREADING_LOG
RDX_HRO_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/rdx_hro_nvm_hyperthreading.log
rm $RDX_HRO_FANOUT_NVM_HYPERTHREADING_LOG

for (( thr = 1; thr <= 20; thr++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DSWWCB_SIZE=256 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=$((2*$thr)) -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=8 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-join-thread-scalability-scripts
	numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=A --param=pkfk >> $RDX_BC_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_radix_sc --workload=uniform --subtype=A --param=pkfk >> $RDX_SC_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_radix_lp --workload=uniform --subtype=A --param=pkfk >> $RDX_LP_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_radix_hr --workload=uniform --subtype=A --param=pkfk >> $RDX_HR_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_HRO_FANOUT_NVM_HYPERTHREADING_LOG
done




ASYM_BC_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/asym_bc_nvm_hyperthreading.log
rm $ASYM_BC_FANOUT_NVM_HYPERTHREADING_LOG
ASYM_SC_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/asym_sc_nvm_hyperthreading.log
rm $ASYM_SC_FANOUT_NVM_HYPERTHREADING_LOG
ASYM_HR_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/asym_hr_nvm_hyperthreading.log
rm $ASYM_HR_FANOUT_NVM_HYPERTHREADING_LOG
ASYM_HRO_FANOUT_NVM_HYPERTHREADING_LOG=$DIR_PATH/asym_hro_nvm_hyperthreading.log
rm $ASYM_HRO_FANOUT_NVM_HYPERTHREADING_LOG

for (( thr = 1; thr <= 20; thr++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DSWWCB_SIZE=256 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=$((2*$thr)) -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=12 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-join-thread-scalability-scripts
	numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=A --param=pkfk >> $ASYM_BC_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_sc --workload=uniform --subtype=A --param=pkfk >> $ASYM_SC_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_hr --workload=uniform --subtype=A --param=pkfk >> $ASYM_HR_FANOUT_NVM_HYPERTHREADING_LOG
	numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $ASYM_HRO_FANOUT_NVM_HYPERTHREADING_LOG
done






cd ../../
./clean.sh


date