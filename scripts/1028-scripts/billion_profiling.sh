date



DIR_PATH=../../logs/1028-logs
mkdir -p $DIR_PATH



RDX_BC_BILLION_NVM_PROFILING_LOG=$DIR_PATH/rdx_bc_billion_nvm_profiling.log
rm $RDX_BC_BILLION_NVM_PROFILING_LOG
RDX_SC_BILLION_NVM_PROFILING_LOG=$DIR_PATH/rdx_sc_billion_nvm_profiling.log
rm $RDX_SC_BILLION_NVM_PROFILING_LOG
RDX_LP_BILLION_NVM_PROFILING_LOG=$DIR_PATH/rdx_lp_billion_nvm_profiling.log
rm $RDX_LP_BILLION_NVM_PROFILING_LOG
RDX_HR_BILLION_NVM_PROFILING_LOG=$DIR_PATH/rdx_hr_billion_nvm_profiling.log
rm $RDX_HR_BILLION_NVM_PROFILING_LOG
RDX_HRO_BILLION_NVM_PROFILING_LOG=$DIR_PATH/rdx_hro_billion_nvm_profiling.log
rm $RDX_HRO_BILLION_NVM_PROFILING_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=true -DPHJ_SYNCSTATS=true -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=true -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/1028-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_lp --workload=uniform --subtype=A --param=pkfk >> $RDX_LP_BILLION_NVM_PROFILING_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=true -DPHJ_SYNCSTATS=true -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=true -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/1028-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=A --param=pkfk >> $RDX_BC_BILLION_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hr --workload=uniform --subtype=A --param=pkfk >> $RDX_HR_BILLION_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_sc --workload=uniform --subtype=A --param=pkfk >> $RDX_SC_BILLION_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_HRO_BILLION_NVM_PROFILING_LOG







ASYM_BC_BILLION_NVM_PROFILING_LOG=$DIR_PATH/asym_bc_billion_nvm_profiling.log
rm $ASYM_BC_BILLION_NVM_PROFILING_LOG
ASYM_SC_BILLION_NVM_PROFILING_LOG=$DIR_PATH/asym_sc_billion_nvm_profiling.log
rm $ASYM_SC_BILLION_NVM_PROFILING_LOG
ASYM_HR_BILLION_NVM_PROFILING_LOG=$DIR_PATH/asym_hr_billion_nvm_profiling.log
rm $ASYM_HR_BILLION_NVM_PROFILING_LOG
ASYM_HRO_BILLION_NVM_PROFILING_LOG=$DIR_PATH/asym_hro_billion_nvm_profiling.log
rm $ASYM_HRO_BILLION_NVM_PROFILING_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=true -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=true -DPHJ_SYNCSTATS=true -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=14 -DNUM_PASSES=2 -DPHJ_MBP=true -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/1028-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=A --param=pkfk >> $ASYM_BC_BILLION_NVM_PROFILING_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_sc --workload=uniform --subtype=A --param=pkfk >> $ASYM_SC_BILLION_NVM_PROFILING_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hr --workload=uniform --subtype=A --param=pkfk >> $ASYM_HR_BILLION_NVM_PROFILING_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $ASYM_HRO_BILLION_NVM_PROFILING_LOG






cd ../../
./clean.sh



date