date



DIR_PATH=../../logs/billion-logs
mkdir -p $DIR_PATH





NPHJ_SC_BILLION_NVM_LOG=$DIR_PATH/nphj_sc_billion_nvm.log
rm $NPHJ_SC_BILLION_NVM_LOG
# NPHJ_LP_BILLION_NVM_LOG=$DIR_PATH/nphj_lp_billion_nvm.log
# rm $NPHJ_LP_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DRUN_PAYLOAD=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/billion-scripts
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=uniform --subtype=A --param=pkfk >> $NPHJ_SC_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=nphj_lp --workload=uniform --subtype=A --param=pkfk >> $NPHJ_LP_BILLION_NVM_LOG





# SHRll_SC_BILLION_NVM_LOG=$DIR_PATH/shrll_sc_billion_nvm.log
# rm $SHRll_SC_BILLION_NVM_LOG
# SHRll_LP_BILLION_NVM_LOG=$DIR_PATH/shrll_lp_billion_nvm.log
# rm $SHRll_LP_BILLION_NVM_LOG
# SHRll_HR_BILLION_NVM_LOG=$DIR_PATH/shrll_hr_billion_nvm.log
# rm $SHRll_HR_BILLION_NVM_LOG
SHRll_HRO_BILLION_NVM_LOG=$DIR_PATH/shrll_hro_billion_nvm.log
rm $SHRll_HRO_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=40 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/billion-scripts
# numactl --membind=0 ../../bin/main --algo=phj_shr_sc --workload=uniform --subtype=A --param=pkfk >> $SHRll_SC_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_shr_lp --workload=uniform --subtype=A --param=pkfk >> $SHRll_LP_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_shr_hr --workload=uniform --subtype=A --param=pkfk >> $SHRll_HR_BILLION_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_shr_hro --workload=uniform --subtype=A --param=pkfk >> $SHRll_HRO_BILLION_NVM_LOG





SHRcm_BC_BILLION_NVM_LOG=$DIR_PATH/shrcm_bc_billion_nvm.log
rm $SHRcm_BC_BILLION_NVM_LOG
# SHRcm_SC_BILLION_NVM_LOG=$DIR_PATH/shrcm_sc_billion_nvm.log
# rm $SHRcm_SC_BILLION_NVM_LOG
# SHRcm_LP_BILLION_NVM_LOG=$DIR_PATH/shrcm_lp_billion_nvm.log
# rm $SHRcm_LP_BILLION_NVM_LOG
# SHRcm_HR_BILLION_NVM_LOG=$DIR_PATH/shrcm_hr_billion_nvm.log
# rm $SHRcm_HR_BILLION_NVM_LOG
# SHRcm_HRO_BILLION_NVM_LOG=$DIR_PATH/shrcm_hro_billion_nvm.log
# rm $SHRcm_HRO_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=40 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/billion-scripts
numactl --membind=0 ../../bin/main --algo=phj_shr_uni_bc --workload=uniform --subtype=A --param=pkfk >> $SHRcm_BC_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_shr_uni_sc --workload=uniform --subtype=A --param=pkfk >> $SHRcm_SC_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hr --workload=uniform --subtype=A --param=pkfk >> $SHRcm_HR_BILLION_NVM_LOG
# cd ../../
# ./clean.sh
# mkdir build
# cd build
# cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=40 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
# make -j 32
# cd ../scripts/billion-scripts
# numactl --membind=0 ../../bin/main --algo=phj_shr_uni_lp --workload=uniform --subtype=A --param=pkfk >> $SHRcm_LP_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_shr_uni_hro --workload=uniform --subtype=A --param=pkfk >> $SHRcm_HRO_BILLION_NVM_LOG








# INDll_SC_BILLION_NVM_LOG=$DIR_PATH/indll_sc_billion_nvm.log
# rm $INDll_SC_BILLION_NVM_LOG
# INDll_LP_BILLION_NVM_LOG=$DIR_PATH/indll_lp_billion_nvm.log
# rm $INDll_LP_BILLION_NVM_LOG
# INDll_HR_BILLION_NVM_LOG=$DIR_PATH/indll_hr_billion_nvm.log
# rm $INDll_HR_BILLION_NVM_LOG
INDll_HRO_BILLION_NVM_LOG=$DIR_PATH/indll_hro_billion_nvm.log
rm $INDll_HRO_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=12 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/billion-scripts
# numactl --membind=0 ../../bin/main --algo=phj_ind_sc --workload=uniform --subtype=A --param=pkfk >> $INDll_SC_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_ind_lp --workload=uniform --subtype=A --param=pkfk >> $INDll_LP_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_ind_hr --workload=uniform --subtype=A --param=pkfk >> $INDll_HR_BILLION_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_ind_hro --workload=uniform --subtype=A --param=pkfk >> $INDll_HRO_BILLION_NVM_LOG







INDcm_BC_BILLION_NVM_LOG=$DIR_PATH/indcm_bc_billion_nvm.log
rm $INDcm_BC_BILLION_NVM_LOG
# INDcm_SC_BILLION_NVM_LOG=$DIR_PATH/indcm_sc_billion_nvm.log
# rm $INDcm_SC_BILLION_NVM_LOG
# INDcm_LP_BILLION_NVM_LOG=$DIR_PATH/indcm_lp_billion_nvm.log
# rm $INDcm_LP_BILLION_NVM_LOG
# INDcm_HR_BILLION_NVM_LOG=$DIR_PATH/indcm_hr_billion_nvm.log
# rm $INDcm_HR_BILLION_NVM_LOG
# INDcm_HRO_BILLION_NVM_LOG=$DIR_PATH/indcm_hro_billion_nvm.log
# rm $INDcm_HRO_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/billion-scripts
numactl --membind=0 ../../bin/main --algo=phj_ind_uni_bc --workload=uniform --subtype=A --param=pkfk >> $INDcm_BC_BILLION_NVM_LOG
# cd ../../
# ./clean.sh
# mkdir build
# cd build
# cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
# make -j 32
# cd ../scripts/billion-scripts
# numactl --membind=0 ../../bin/main --algo=phj_ind_uni_sc --workload=uniform --subtype=A --param=pkfk >> $INDcm_SC_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_ind_uni_lp --workload=uniform --subtype=A --param=pkfk >> $INDcm_LP_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hr --workload=uniform --subtype=A --param=pkfk >> $INDcm_HR_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_ind_uni_hro --workload=uniform --subtype=A --param=pkfk >> $INDcm_HRO_BILLION_NVM_LOG




<<COMMENT
RDX_BC_BILLION_NVM_LOG=$DIR_PATH/rdx_bc_billion_nvm.log
rm $RDX_BC_BILLION_NVM_LOG
RDX_SC_BILLION_NVM_LOG=$DIR_PATH/rdx_sc_billion_nvm.log
rm $RDX_SC_BILLION_NVM_LOG
RDX_LP_BILLION_NVM_LOG=$DIR_PATH/rdx_lp_billion_nvm.log
rm $RDX_LP_BILLION_NVM_LOG
RDX_HR_BILLION_NVM_LOG=$DIR_PATH/rdx_hr_billion_nvm.log
rm $RDX_HR_BILLION_NVM_LOG
RDX_HRO_BILLION_NVM_LOG=$DIR_PATH/rdx_hro_billion_nvm.log
rm $RDX_HRO_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=18 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/billion-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=A --param=pkfk >> $RDX_BC_BILLION_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hr --workload=uniform --subtype=A --param=pkfk >> $RDX_HR_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/billion-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_sc --workload=uniform --subtype=A --param=pkfk >> $RDX_SC_BILLION_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_lp --workload=uniform --subtype=A --param=pkfk >> $RDX_LP_BILLION_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_HRO_BILLION_NVM_LOG







ASYM_BC_BILLION_NVM_LOG=$DIR_PATH/asym_bc_billion_nvm.log
rm $ASYM_BC_BILLION_NVM_LOG
ASYM_SC_BILLION_NVM_LOG=$DIR_PATH/asym_sc_billion_nvm.log
rm $ASYM_SC_BILLION_NVM_LOG
ASYM_HR_BILLION_NVM_LOG=$DIR_PATH/asym_hr_billion_nvm.log
rm $ASYM_HR_BILLION_NVM_LOG
ASYM_HRO_BILLION_NVM_LOG=$DIR_PATH/asym_hro_billion_nvm.log
rm $ASYM_HRO_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=14 -DNUM_PASSES=2 -DPHJ_MBP=true -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/billion-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=A --param=pkfk >> $ASYM_BC_BILLION_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_sc --workload=uniform --subtype=A --param=pkfk >> $ASYM_SC_BILLION_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hr --workload=uniform --subtype=A --param=pkfk >> $ASYM_HR_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=13 -DNUM_PASSES=2 -DPHJ_MBP=true -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/billion-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $ASYM_HRO_BILLION_NVM_LOG
COMMENT






RDX_BW_REG_BC_BILLION_NVM_LOG=$DIR_PATH/rdx_bw_reg_bc_billion_nvm.log
rm $RDX_BW_REG_BC_BILLION_NVM_LOG
# RDX_BW_REG_SC_BILLION_NVM_LOG=$DIR_PATH/rdx_bw_reg_sc_billion_nvm.log
# rm $RDX_BW_REG_SC_BILLION_NVM_LOG
# RDX_BW_REG_LP_BILLION_NVM_LOG=$DIR_PATH/rdx_bw_reg_lp_billion_nvm.log
# rm $RDX_BW_REG_LP_BILLION_NVM_LOG
# RDX_BW_REG_HR_BILLION_NVM_LOG=$DIR_PATH/rdx_bw_reg_hr_billion_nvm.log
# rm $RDX_BW_REG_HR_BILLION_NVM_LOG
# RDX_BW_REG_HRO_BILLION_NVM_LOG=$DIR_PATH/rdx_bw_reg_hro_billion_nvm.log
# rm $RDX_BW_REG_HRO_BILLION_NVM_LOG
# cd ../../
# ./clean.sh
# mkdir build
# cd build
# cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DBW_REG=true
# make -j 32
# cd ../scripts/billion-scripts
# numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_lp --workload=uniform --subtype=A --param=pkfk >> $RDX_BW_REG_LP_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=18 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DBW_REG=true
make -j 32
cd ../scripts/billion-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_bc --workload=uniform --subtype=A --param=pkfk >> $RDX_BW_REG_BC_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_sc --workload=uniform --subtype=A --param=pkfk >> $RDX_BW_REG_SC_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_hr --workload=uniform --subtype=A --param=pkfk >> $RDX_BW_REG_HR_BILLION_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_BW_REG_HRO_BILLION_NVM_LOG









# ASYM_BW_REG_BC_BILLION_NVM_LOG=$DIR_PATH/asym_bw_reg_bc_billion_nvm.log
# rm $ASYM_BW_REG_BC_BILLION_NVM_LOG
# ASYM_BW_REG_SC_BILLION_NVM_LOG=$DIR_PATH/asym_bw_reg_sc_billion_nvm.log
# rm $ASYM_BW_REG_SC_BILLION_NVM_LOG
# ASYM_BW_REG_HR_BILLION_NVM_LOG=$DIR_PATH/asym_bw_reg_hr_billion_nvm.log
# rm $ASYM_BW_REG_HR_BILLION_NVM_LOG
ASYM_BW_REG_HRO_BILLION_NVM_LOG=$DIR_PATH/asym_bw_reg_hro_billion_nvm.log
rm $ASYM_BW_REG_HRO_BILLION_NVM_LOG
# cd ../../
# ./clean.sh
# mkdir build
# cd build
# cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=14 -DNUM_PASSES=2 -DPHJ_MBP=true -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
# make -j 32
# cd ../scripts/billion-scripts
# numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bw_reg_bc --workload=uniform --subtype=A --param=pkfk >> $ASYM_BW_REG_BC_BILLION_NVM_LOG
# numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bw_reg_sc --workload=uniform --subtype=A --param=pkfk >> $ASYM_BW_REG_SC_BILLION_NVM_LOG
# numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bw_reg_hr --workload=uniform --subtype=A --param=pkfk >> $ASYM_BW_REG_HR_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=13 -DNUM_PASSES=2 -DPHJ_MBP=true -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/billion-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bw_reg_hro --workload=uniform --subtype=A --param=pkfk >> $ASYM_BW_REG_HRO_BILLION_NVM_LOG




<<COMMENT
RDX_BC_BILLION_NVM_LOG=$DIR_PATH/rdx_bc_billion_nvm.log
rm $RDX_BC_BILLION_NVM_LOG
RDX_SC_BILLION_NVM_LOG=$DIR_PATH/rdx_sc_billion_nvm.log
rm $RDX_SC_BILLION_NVM_LOG
RDX_LP_BILLION_NVM_LOG=$DIR_PATH/rdx_lp_billion_nvm.log
rm $RDX_LP_BILLION_NVM_LOG
RDX_HR_BILLION_NVM_LOG=$DIR_PATH/rdx_hr_billion_nvm.log
rm $RDX_HR_BILLION_NVM_LOG
RDX_HRO_BILLION_NVM_LOG=$DIR_PATH/rdx_hro_billion_nvm.log
rm $RDX_HRO_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/billion-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_lp --workload=uniform --subtype=A --param=pkfk >> $RDX_LP_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/billion-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=A --param=pkfk >> $RDX_BC_BILLION_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hr --workload=uniform --subtype=A --param=pkfk >> $RDX_HR_BILLION_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_sc --workload=uniform --subtype=A --param=pkfk >> $RDX_SC_BILLION_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_HRO_BILLION_NVM_LOG





ASYM_BC_BILLION_NVM_LOG=$DIR_PATH/asym_bc_billion_nvm.log
rm $ASYM_BC_BILLION_NVM_LOG
ASYM_SC_BILLION_NVM_LOG=$DIR_PATH/asym_sc_billion_nvm.log
rm $ASYM_SC_BILLION_NVM_LOG
ASYM_HR_BILLION_NVM_LOG=$DIR_PATH/asym_hr_billion_nvm.log
rm $ASYM_HR_BILLION_NVM_LOG
ASYM_HRO_BILLION_NVM_LOG=$DIR_PATH/asym_hro_billion_nvm.log
rm $ASYM_HRO_BILLION_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=14 -DNUM_PASSES=2 -DPHJ_MBP=true -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/billion-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=A --param=pkfk >> $ASYM_BC_BILLION_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_sc --workload=uniform --subtype=A --param=pkfk >> $ASYM_SC_BILLION_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hr --workload=uniform --subtype=A --param=pkfk >> $ASYM_HR_BILLION_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $ASYM_HRO_BILLION_NVM_LOG
COMMENT




cd ../../
./clean.sh



date