date


DIR_PATH=../../logs/numa-tpch-logs
mkdir -p $DIR_PATH






PHJ_RADIX_BC_LOCAL_DRAM_LOG=$DIR_PATH/phj_radix_bc_local_dram.log
rm $PHJ_RADIX_BC_LOCAL_DRAM_LOG
NPHJ_SC_LOCAL_DRAM_LOG=$DIR_PATH/nphj_sc_local_dram.log
rm $NPHJ_SC_LOCAL_DRAM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=5 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=10 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/numa-tpch-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=phj_radix_bc >> $PHJ_RADIX_BC_LOCAL_DRAM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=nphj_sc >> $NPHJ_SC_LOCAL_DRAM_LOG



PHJ_RADIX_BC_REMOTE_DRAM_LOG=$DIR_PATH/phj_radix_bc_remote_dram.log
rm $PHJ_RADIX_BC_REMOTE_DRAM_LOG
NPHJ_SC_REMOTE_DRAM_LOG=$DIR_PATH/nphj_sc_remote_dram.log
rm $NPHJ_SC_REMOTE_DRAM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=5 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=true -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=10 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/numa-tpch-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=phj_radix_bc >> $PHJ_RADIX_BC_REMOTE_DRAM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=nphj_sc >> $NPHJ_SC_REMOTE_DRAM_LOG





PHJ_RADIX_BC_LOCAL_DRAM_HUGE_LOG=$DIR_PATH/phj_radix_bc_local_dram_huge.log
rm $PHJ_RADIX_BC_LOCAL_DRAM_HUGE_LOG
NPHJ_SC_LOCAL_DRAM_HUGE_LOG=$DIR_PATH/nphj_sc_local_dram_huge.log
rm $NPHJ_SC_LOCAL_DRAM_HUGE_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=5 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=true -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=10 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/numa-tpch-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=phj_radix_bc >> $PHJ_RADIX_BC_LOCAL_DRAM_HUGE_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=nphj_sc >> $NPHJ_SC_LOCAL_DRAM_HUGE_LOG



PHJ_RADIX_BC_REMOTE_DRAM_HUGE_LOG=$DIR_PATH/phj_radix_bc_remote_dram_huge.log
rm $PHJ_RADIX_BC_REMOTE_DRAM_HUGE_LOG
NPHJ_SC_REMOTE_DRAM_HUGE_LOG=$DIR_PATH/nphj_sc_remote_dram_huge.log
rm $NPHJ_SC_REMOTE_DRAM_HUGE_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=5 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=true -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=true -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=10 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/numa-tpch-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=phj_radix_bc >> $PHJ_RADIX_BC_REMOTE_DRAM_HUGE_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=nphj_sc >> $NPHJ_SC_REMOTE_DRAM_HUGE_LOG





PHJ_RADIX_BC_LOCAL_NVM_LOG=$DIR_PATH/phj_radix_bc_local_nvm.log
rm $PHJ_RADIX_BC_LOCAL_NVM_LOG
NPHJ_SC_LOCAL_NVM_LOG=$DIR_PATH/nphj_sc_local_nvm.log
rm $NPHJ_SC_LOCAL_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=5 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=12 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/numa-tpch-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=phj_radix_bc >> $PHJ_RADIX_BC_LOCAL_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=5 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/numa-tpch-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=nphj_sc >> $NPHJ_SC_LOCAL_NVM_LOG



PHJ_RADIX_BC_REMOTE_NVM_LOG=$DIR_PATH/phj_radix_bc_remote_nvm.log
rm $PHJ_RADIX_BC_REMOTE_NVM_LOG
NPHJ_SC_REMOTE_NVM_LOG=$DIR_PATH/nphj_sc_remote_nvm.log
rm $NPHJ_SC_REMOTE_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=5 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=8 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=4 -DUSE_NUMA=true -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/numa-tpch-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=phj_radix_bc >> $PHJ_RADIX_BC_REMOTE_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=5 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=2 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=true -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/numa-tpch-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --workload=uniform --subtype=A --param=pkfk --algo=nphj_sc >> $NPHJ_SC_REMOTE_NVM_LOG






cd ../../
./clean.sh

date