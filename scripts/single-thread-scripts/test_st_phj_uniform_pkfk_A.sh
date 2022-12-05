date



DIR_PATH=../../logs/single-thread-logs
mkdir -p $DIR_PATH




ST_SHRll_SC_NVM_LOG=$DIR_PATH/st_shrll_sc_nvm.log
rm $ST_SHRll_SC_NVM_LOG
ST_SHRll_LP_NVM_LOG=$DIR_PATH/st_shrll_lp_nvm.log
rm $ST_SHRll_LP_NVM_LOG
ST_SHRll_HR_NVM_LOG=$DIR_PATH/st_shrll_hr_nvm.log
rm $ST_SHRll_HR_NVM_LOG
ST_SHRll_HRO_NVM_LOG=$DIR_PATH/st_shrll_hro_nvm.log
rm $ST_SHRll_HRO_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=1 -DBUILDPART_THREAD_NUM=1 -DINTERMEDIATEMEMSET_THREAD_NUM=1 -DPROBEJOIN_THREAD_NUM=1 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/single-thread-scripts
numactl --membind=0 ../../bin/main --algo=phj_shr_sc --workload=uniform --subtype=A --param=pkfk >> $ST_SHRll_SC_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_shr_lp --workload=uniform --subtype=A --param=pkfk >> $ST_SHRll_LP_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_shr_hr --workload=uniform --subtype=A --param=pkfk >> $ST_SHRll_HR_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_shr_hro --workload=uniform --subtype=A --param=pkfk >> $ST_SHRll_HRO_NVM_LOG






ST_SHRcm_BC_NVM_LOG=$DIR_PATH/st_shrcm_bc_nvm.log
rm $ST_SHRcm_BC_NVM_LOG
ST_SHRcm_SC_NVM_LOG=$DIR_PATH/st_shrcm_sc_nvm.log
rm $ST_SHRcm_SC_NVM_LOG
ST_SHRcm_LP_NVM_LOG=$DIR_PATH/st_shrcm_lp_nvm.log
rm $ST_SHRcm_LP_NVM_LOG
ST_SHRcm_HR_NVM_LOG=$DIR_PATH/st_shrcm_hr_nvm.log
rm $ST_SHRcm_HR_NVM_LOG
ST_SHRcm_HRO_NVM_LOG=$DIR_PATH/st_shrcm_hro_nvm.log
rm $ST_SHRcm_HRO_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=1 -DBUILDPART_THREAD_NUM=1 -DINTERMEDIATEMEMSET_THREAD_NUM=1 -DPROBEJOIN_THREAD_NUM=1 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/single-thread-scripts
numactl --membind=0 ../../bin/main --algo=phj_shr_uni_bc --workload=uniform --subtype=A --param=pkfk >> $ST_SHRcm_BC_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_shr_uni_sc --workload=uniform --subtype=A --param=pkfk >> $ST_SHRcm_SC_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_shr_uni_lp --workload=uniform --subtype=A --param=pkfk >> $ST_SHRcm_LP_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hr --workload=uniform --subtype=A --param=pkfk >> $ST_SHRcm_HR_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hro --workload=uniform --subtype=A --param=pkfk >> $ST_SHRcm_HRO_NVM_LOG






ST_INDll_SC_NVM_LOG=$DIR_PATH/st_indll_sc_nvm.log
rm $ST_INDll_SC_NVM_LOG
ST_INDll_LP_NVM_LOG=$DIR_PATH/st_indll_lp_nvm.log
rm $ST_INDll_LP_NVM_LOG
ST_INDll_HR_NVM_LOG=$DIR_PATH/st_indll_hr_nvm.log
rm $ST_INDll_HR_NVM_LOG
ST_INDll_HRO_NVM_LOG=$DIR_PATH/st_indll_hro_nvm.log
rm $ST_INDll_HRO_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=1 -DBUILDPART_THREAD_NUM=1 -DINTERMEDIATEMEMSET_THREAD_NUM=1 -DPROBEJOIN_THREAD_NUM=1 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/single-thread-scripts
numactl --membind=0 ../../bin/main --algo=phj_ind_sc --workload=uniform --subtype=A --param=pkfk >> $ST_INDll_SC_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_lp --workload=uniform --subtype=A --param=pkfk >> $ST_INDll_LP_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_hr --workload=uniform --subtype=A --param=pkfk >> $ST_INDll_HR_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_hro --workload=uniform --subtype=A --param=pkfk >> $ST_INDll_HRO_NVM_LOG






ST_INDcm_BC_NVM_LOG=$DIR_PATH/st_indcm_bc_nvm.log
rm $ST_INDcm_BC_NVM_LOG
ST_INDcm_SC_NVM_LOG=$DIR_PATH/st_indcm_sc_nvm.log
rm $ST_INDcm_SC_NVM_LOG
ST_INDcm_LP_NVM_LOG=$DIR_PATH/st_indcm_lp_nvm.log
rm $ST_INDcm_LP_NVM_LOG
ST_INDcm_HR_NVM_LOG=$DIR_PATH/st_indcm_hr_nvm.log
rm $ST_INDcm_HR_NVM_LOG
ST_INDcm_HRO_NVM_LOG=$DIR_PATH/st_indcm_hro_nvm.log
rm $ST_INDcm_HRO_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=1 -DBUILDPART_THREAD_NUM=1 -DINTERMEDIATEMEMSET_THREAD_NUM=1 -DPROBEJOIN_THREAD_NUM=1 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/single-thread-scripts
numactl --membind=0 ../../bin/main --algo=phj_ind_uni_bc --workload=uniform --subtype=A --param=pkfk >> $ST_INDcm_BC_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_uni_sc --workload=uniform --subtype=A --param=pkfk >> $ST_INDcm_SC_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_uni_lp --workload=uniform --subtype=A --param=pkfk >> $ST_INDcm_LP_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hr --workload=uniform --subtype=A --param=pkfk >> $ST_INDcm_HR_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hro --workload=uniform --subtype=A --param=pkfk >> $ST_INDcm_HRO_NVM_LOG





ST_RDX_BC_NVM_LOG=$DIR_PATH/st_rdx_bc_nvm.log
rm $ST_RDX_BC_NVM_LOG
ST_RDX_SC_NVM_LOG=$DIR_PATH/st_rdx_sc_nvm.log
rm $ST_RDX_SC_NVM_LOG
ST_RDX_LP_NVM_LOG=$DIR_PATH/st_rdx_lp_nvm.log
rm $ST_RDX_LP_NVM_LOG
ST_RDX_HR_NVM_LOG=$DIR_PATH/st_rdx_hr_nvm.log
rm $ST_RDX_HR_NVM_LOG
ST_RDX_HRO_NVM_LOG=$DIR_PATH/st_rdx_hro_nvm.log
rm $ST_RDX_HRO_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=1 -DBUILDPART_THREAD_NUM=1 -DINTERMEDIATEMEMSET_THREAD_NUM=1 -DPROBEJOIN_THREAD_NUM=1 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/single-thread-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=A --param=pkfk >> $ST_RDX_BC_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_sc --workload=uniform --subtype=A --param=pkfk >> $ST_RDX_SC_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_lp --workload=uniform --subtype=A --param=pkfk >> $ST_RDX_LP_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hr --workload=uniform --subtype=A --param=pkfk >> $ST_RDX_HR_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $ST_RDX_HRO_NVM_LOG





ST_ASYM_BC_NVM_LOG=$DIR_PATH/st_asym_bc_nvm.log
rm $ST_ASYM_BC_NVM_LOG
ST_ASYM_SC_NVM_LOG=$DIR_PATH/st_asym_sc_nvm.log
rm $ST_ASYM_SC_NVM_LOG
ST_ASYM_HR_NVM_LOG=$DIR_PATH/st_asym_hr_nvm.log
rm $ST_ASYM_HR_NVM_LOG
ST_ASYM_HRO_NVM_LOG=$DIR_PATH/st_asym_hro_nvm.log
rm $ST_ASYM_HRO_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=1 -DBUILDPART_THREAD_NUM=1 -DINTERMEDIATEMEMSET_THREAD_NUM=1 -DPROBEJOIN_THREAD_NUM=1 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=12 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/single-thread-scripts
numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=A --param=pkfk >> $ST_ASYM_BC_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_sc --workload=uniform --subtype=A --param=pkfk >> $ST_ASYM_SC_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_hr --workload=uniform --subtype=A --param=pkfk >> $ST_ASYM_HR_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $ST_ASYM_HRO_NVM_LOG




cd ../../
./clean.sh


date