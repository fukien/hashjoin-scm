date



DIR_PATH=../../logs/epsilon-asymmetry-logs
mkdir -p $DIR_PATH







rm -rf /pmemfs1/hashjoin-scm/fndly-data/uniform
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DRUN_FNDLY=true -DFNDLY_R_CARDINALITY=512 -DFNDLY_S_CARDINALITY=16384
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=20-39 --membind=0-1 ../../bin/fndly_gen
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_AR.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_LR.bin
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_AS.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_LS.bin

RDX_BC_FNDLY_L_NVM_LOG=$DIR_PATH/rdx_bc_fndly_l_nvm.log
rm $RDX_BC_FNDLY_L_NVM_LOG
RDX_HRO_FNDLY_L_NVM_LOG=$DIR_PATH/rdx_hro_fndly_l_nvm.log
rm $RDX_HRO_FNDLY_L_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=L --param=pkfk >> $RDX_BC_FNDLY_L_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=L --param=pkfk >> $RDX_HRO_FNDLY_L_NVM_LOG

ASYM_BC_FNDLY_L_NVM_LOG=$DIR_PATH/asym_bc_fndly_l_nvm.log
rm $ASYM_BC_FNDLY_L_NVM_LOG
ASYM_HRO_FNDLY_L_NVM_LOG=$DIR_PATH/asym_hro_fndly_l_nvm.log
rm $ASYM_HRO_FNDLY_L_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=13 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=L --param=pkfk >> $ASYM_BC_FNDLY_L_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=L --param=pkfk >> $ASYM_HRO_FNDLY_L_NVM_LOG

RDX_BW_REG_BC_FNDLY_L_NVM_LOG=$DIR_PATH/rdx_bw_reg_bc_fndly_l_nvm.log
rm $RDX_BW_REG_BC_FNDLY_L_NVM_LOG
RDX_BW_REG_HRO_FNDLY_L_NVM_LOG=$DIR_PATH/rdx_bw_reg_hro_fndly_l_nvm.log
rm $RDX_BW_REG_HRO_FNDLY_L_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=18 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_bc --workload=uniform --subtype=L --param=pkfk >> $RDX_BW_REG_BC_FNDLY_L_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_hro --workload=uniform --subtype=L --param=pkfk >> $RDX_BW_REG_HRO_FNDLY_L_NVM_LOG






rm -rf /pmemfs1/hashjoin-scm/fndly-data/uniform
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DRUN_FNDLY=true -DFNDLY_R_CARDINALITY=2048 -DFNDLY_S_CARDINALITY=16384
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=20-39 --membind=0-1 ../../bin/fndly_gen
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_AR.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_PR.bin
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_AS.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_PS.bin

RDX_BC_FNDLY_P_NVM_LOG=$DIR_PATH/rdx_bc_fndly_p_nvm.log
rm $RDX_BC_FNDLY_P_NVM_LOG
RDX_HRO_FNDLY_P_NVM_LOG=$DIR_PATH/rdx_hro_fndly_p_nvm.log
rm $RDX_HRO_FNDLY_P_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=P --param=pkfk >> $RDX_BC_FNDLY_P_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=P --param=pkfk >> $RDX_HRO_FNDLY_P_NVM_LOG

ASYM_BC_FNDLY_P_NVM_LOG=$DIR_PATH/asym_bc_fndly_p_nvm.log
rm $ASYM_BC_FNDLY_P_NVM_LOG
ASYM_HRO_FNDLY_P_NVM_LOG=$DIR_PATH/asym_hro_fndly_p_nvm.log
rm $ASYM_HRO_FNDLY_P_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=14 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=P --param=pkfk >> $ASYM_BC_FNDLY_P_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=P --param=pkfk >> $ASYM_HRO_FNDLY_P_NVM_LOG

RDX_BW_REG_BC_FNDLY_P_NVM_LOG=$DIR_PATH/rdx_bw_reg_bc_fndly_p_nvm.log
rm $RDX_BW_REG_BC_FNDLY_P_NVM_LOG
RDX_BW_REG_HRO_FNDLY_P_NVM_LOG=$DIR_PATH/rdx_bw_reg_hro_fndly_p_nvm.log
rm $RDX_BW_REG_HRO_FNDLY_P_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=18 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_bc --workload=uniform --subtype=P --param=pkfk >> $RDX_BW_REG_BC_FNDLY_P_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_hro --workload=uniform --subtype=P --param=pkfk >> $RDX_BW_REG_HRO_FNDLY_P_NVM_LOG






rm -rf /pmemfs1/hashjoin-scm/fndly-data/uniform
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DRUN_FNDLY=true -DFNDLY_R_CARDINALITY=256 -DFNDLY_S_CARDINALITY=1024
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=20-39 --membind=0-1 ../../bin/fndly_gen
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_AR.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_QR.bin
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_AS.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_QS.bin

RDX_BC_FNDLY_Q_NVM_LOG=$DIR_PATH/rdx_bc_fndly_q_nvm.log
rm $RDX_BC_FNDLY_Q_NVM_LOG
RDX_HRO_FNDLY_Q_NVM_LOG=$DIR_PATH/rdx_hro_fndly_q_nvm.log
rm $RDX_HRO_FNDLY_Q_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=Q --param=pkfk >> $RDX_BC_FNDLY_Q_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=Q --param=pkfk >> $RDX_HRO_FNDLY_Q_NVM_LOG

ASYM_BC_FNDLY_Q_NVM_LOG=$DIR_PATH/asym_bc_fndly_q_nvm.log
rm $ASYM_BC_FNDLY_Q_NVM_LOG
ASYM_HRO_FNDLY_Q_NVM_LOG=$DIR_PATH/asym_hro_fndly_q_nvm.log
rm $ASYM_HRO_FNDLY_Q_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=13 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=Q --param=pkfk >> $ASYM_BC_FNDLY_Q_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=Q --param=pkfk >> $ASYM_HRO_FNDLY_Q_NVM_LOG

RDX_BW_REG_BC_FNDLY_Q_NVM_LOG=$DIR_PATH/rdx_bw_reg_bc_fndly_q_nvm.log
rm $RDX_BW_REG_BC_FNDLY_Q_NVM_LOG
RDX_BW_REG_HRO_FNDLY_Q_NVM_LOG=$DIR_PATH/rdx_bw_reg_hro_fndly_q_nvm.log
rm $RDX_BW_REG_HRO_FNDLY_Q_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=18 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_bc --workload=uniform --subtype=Q --param=pkfk >> $RDX_BW_REG_BC_FNDLY_Q_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_hro --workload=uniform --subtype=Q --param=pkfk >> $RDX_BW_REG_HRO_FNDLY_Q_NVM_LOG







rm -rf /pmemfs1/hashjoin-scm/fndly-data/uniform
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DRUN_FNDLY=true -DFNDLY_R_CARDINALITY=4096 -DFNDLY_S_CARDINALITY=16384
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=20-39 --membind=0-1 ../../bin/fndly_gen
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_AR.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_UR.bin
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_AS.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_US.bin

RDX_BC_FNDLY_U_NVM_LOG=$DIR_PATH/rdx_bc_fndly_u_nvm.log
rm $RDX_BC_FNDLY_U_NVM_LOG
RDX_HRO_FNDLY_U_NVM_LOG=$DIR_PATH/rdx_hro_fndly_u_nvm.log
rm $RDX_HRO_FNDLY_U_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=U --param=pkfk >> $RDX_BC_FNDLY_U_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=U --param=pkfk >> $RDX_HRO_FNDLY_U_NVM_LOG

ASYM_BC_FNDLY_U_NVM_LOG=$DIR_PATH/asym_bc_fndly_u_nvm.log
rm $ASYM_BC_FNDLY_U_NVM_LOG
ASYM_HRO_FNDLY_U_NVM_LOG=$DIR_PATH/asym_hro_fndly_u_nvm.log
rm $ASYM_HRO_FNDLY_U_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=17 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=U --param=pkfk >> $ASYM_BC_FNDLY_U_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=U --param=pkfk >> $ASYM_HRO_FNDLY_U_NVM_LOG

RDX_BW_REG_BC_FNDLY_U_NVM_LOG=$DIR_PATH/rdx_bw_reg_bc_fndly_u_nvm.log
rm $RDX_BW_REG_BC_FNDLY_U_NVM_LOG
RDX_BW_REG_HRO_FNDLY_U_NVM_LOG=$DIR_PATH/rdx_bw_reg_hro_fndly_u_nvm.log
rm $RDX_BW_REG_HRO_FNDLY_U_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=18 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_bc --workload=uniform --subtype=U --param=pkfk >> $RDX_BW_REG_BC_FNDLY_U_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_hro --workload=uniform --subtype=U --param=pkfk >> $RDX_BW_REG_HRO_FNDLY_U_NVM_LOG





rm -rf /pmemfs1/hashjoin-scm/fndly-data/uniform
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DRUN_FNDLY=true -DFNDLY_R_CARDINALITY=512 -DFNDLY_S_CARDINALITY=4096
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=20-39 --membind=0-1 ../../bin/fndly_gen
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_AR.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_TR.bin
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_AS.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_TS.bin


RDX_BC_FNDLY_T_NVM_LOG=$DIR_PATH/rdx_bc_fndly_t_nvm.log
rm $RDX_BC_FNDLY_T_NVM_LOG
RDX_HRO_FNDLY_T_NVM_LOG=$DIR_PATH/rdx_hro_fndly_t_nvm.log
rm $RDX_HRO_FNDLY_T_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=T --param=pkfk >> $RDX_BC_FNDLY_T_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=T --param=pkfk >> $RDX_HRO_FNDLY_T_NVM_LOG

ASYM_BC_FNDLY_T_NVM_LOG=$DIR_PATH/asym_bc_fndly_t_nvm.log
rm $ASYM_BC_FNDLY_T_NVM_LOG
ASYM_HRO_FNDLY_T_NVM_LOG=$DIR_PATH/asym_hro_fndly_t_nvm.log
rm $ASYM_HRO_FNDLY_T_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=13 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=T --param=pkfk >> $ASYM_BC_FNDLY_T_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=T --param=pkfk >> $ASYM_HRO_FNDLY_T_NVM_LOG

RDX_BW_REG_BC_FNDLY_T_NVM_LOG=$DIR_PATH/rdx_bw_reg_bc_fndly_t_nvm.log
rm $RDX_BW_REG_BC_FNDLY_T_NVM_LOG
RDX_BW_REG_HRO_FNDLY_T_NVM_LOG=$DIR_PATH/rdx_bw_reg_hro_fndly_t_nvm.log
rm $RDX_BW_REG_HRO_FNDLY_T_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=18 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_bc --workload=uniform --subtype=T --param=pkfk >> $RDX_BW_REG_BC_FNDLY_T_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_hro --workload=uniform --subtype=T --param=pkfk >> $RDX_BW_REG_HRO_FNDLY_T_NVM_LOG





rm -rf /pmemfs1/hashjoin-scm/fndly-data/uniform
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DRUN_FNDLY=true -DFNDLY_R_CARDINALITY=128 -DFNDLY_S_CARDINALITY=1024
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=20-39 --membind=0-1 ../../bin/fndly_gen
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_AR.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_VR.bin
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_AS.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_VS.bin

RDX_BC_FNDLY_V_NVM_LOG=$DIR_PATH/rdx_bc_fndly_v_nvm.log
rm $RDX_BC_FNDLY_V_NVM_LOG
RDX_HRO_FNDLY_V_NVM_LOG=$DIR_PATH/rdx_hro_fndly_v_nvm.log
rm $RDX_HRO_FNDLY_V_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=12 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=V --param=pkfk >> $RDX_BC_FNDLY_V_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=V --param=pkfk >> $RDX_HRO_FNDLY_V_NVM_LOG

ASYM_BC_FNDLY_V_NVM_LOG=$DIR_PATH/asym_bc_fndly_v_nvm.log
rm $ASYM_BC_FNDLY_V_NVM_LOG
ASYM_HRO_FNDLY_V_NVM_LOG=$DIR_PATH/asym_hro_fndly_v_nvm.log
rm $ASYM_HRO_FNDLY_V_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=V --param=pkfk >> $ASYM_BC_FNDLY_V_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=V --param=pkfk >> $ASYM_HRO_FNDLY_V_NVM_LOG

RDX_BW_REG_BC_FNDLY_V_NVM_LOG=$DIR_PATH/rdx_bw_reg_bc_fndly_v_nvm.log
rm $RDX_BW_REG_BC_FNDLY_V_NVM_LOG
RDX_BW_REG_HRO_FNDLY_V_NVM_LOG=$DIR_PATH/rdx_bw_reg_hro_fndly_v_nvm.log
rm $RDX_BW_REG_HRO_FNDLY_V_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=18 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_bc --workload=uniform --subtype=V --param=pkfk >> $RDX_BW_REG_BC_FNDLY_V_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_hro --workload=uniform --subtype=V --param=pkfk >> $RDX_BW_REG_HRO_FNDLY_V_NVM_LOG





rm -rf /pmemfs1/hashjoin-scm/fndly-data/uniform
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DRUN_FNDLY=true -DFNDLY_R_CARDINALITY=256 -DFNDLY_S_CARDINALITY=4096
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=20-39 --membind=0-1 ../../bin/fndly_gen
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_AR.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/pk_HR.bin
mv /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_AS.bin /pmemfs1/hashjoin-scm/fndly-data/uniform/fk_HS.bin

RDX_BC_FNDLY_H_NVM_LOG=$DIR_PATH/rdx_bc_fndly_h_nvm.log
rm $RDX_BC_FNDLY_H_NVM_LOG
RDX_HRO_FNDLY_H_NVM_LOG=$DIR_PATH/rdx_hro_fndly_h_nvm.log
rm $RDX_HRO_FNDLY_H_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=uniform --subtype=H --param=pkfk >> $RDX_BC_FNDLY_H_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=H --param=pkfk >> $RDX_HRO_FNDLY_H_NVM_LOG

ASYM_BC_FNDLY_H_NVM_LOG=$DIR_PATH/asym_bc_fndly_h_nvm.log
rm $ASYM_BC_FNDLY_H_NVM_LOG
ASYM_HRO_FNDLY_H_NVM_LOG=$DIR_PATH/asym_hro_fndly_h_nvm.log
rm $ASYM_HRO_FNDLY_H_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=uniform --subtype=H --param=pkfk >> $ASYM_BC_FNDLY_H_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=H --param=pkfk >> $ASYM_HRO_FNDLY_H_NVM_LOG

RDX_BW_REG_BC_FNDLY_H_NVM_LOG=$DIR_PATH/rdx_bw_reg_bc_fndly_h_nvm.log
rm $RDX_BW_REG_BC_FNDLY_H_NVM_LOG
RDX_BW_REG_HRO_FNDLY_H_NVM_LOG=$DIR_PATH/rdx_bw_reg_hro_fndly_h_nvm.log
rm $RDX_BW_REG_HRO_FNDLY_H_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=true -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=18 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/epsilon-asymmetry-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_bc --workload=uniform --subtype=H --param=pkfk >> $RDX_BW_REG_BC_FNDLY_H_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_bw_reg_hro --workload=uniform --subtype=H --param=pkfk >> $RDX_BW_REG_HRO_FNDLY_H_NVM_LOG








rm -rf /pmemfs1/hashjoin-scm/fndly-data/uniform



cd ../../
./clean.sh



date