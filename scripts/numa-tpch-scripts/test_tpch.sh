date


DIR_PATH=../../logs/numa-tpch-logs
mkdir -p $DIR_PATH





TPCH_NPHJ_SC_NVM_LOG=$DIR_PATH/tpch_nphj_sc_nvm.log
rm $TPCH_NPHJ_SC_NVM_LOG
TPCH_RDX_BC_NVM_LOG=$DIR_PATH/tpch_rdx_bc_nvm.log
rm $TPCH_RDX_BC_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/numa-tpch-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/tpch/tpch_query --algo=tpch_nphj_sc --sf=100 >> $TPCH_NPHJ_SC_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/tpch/tpch_query --algo=tpch_rdx_bc --sf=100 >> $TPCH_RDX_BC_NVM_LOG





TPCH_NPHJ_SC_DRAM_LOG=$DIR_PATH/tpch_nphj_sc_dram.log
rm $TPCH_NPHJ_SC_DRAM_LOG
TPCH_RDX_BC_DRAM_LOG=$DIR_PATH/tpch_rdx_bc_dram.log
rm $TPCH_RDX_BC_DRAM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/numa-tpch-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/tpch/tpch_query --algo=tpch_nphj_sc --sf=100 >> $TPCH_NPHJ_SC_DRAM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/tpch/tpch_query --algo=tpch_rdx_bc --sf=100 >> $TPCH_RDX_BC_DRAM_LOG





TPCH_NPHJ_SC_DRAM_HUGE_LOG=$DIR_PATH/tpch_nphj_sc_dram_huge.log
rm $TPCH_NPHJ_SC_DRAM_HUGE_LOG
TPCH_RDX_BC_DRAM_HUGE_LOG=$DIR_PATH/tpch_rdx_bc_dram_huge.log
rm $TPCH_RDX_BC_DRAM_HUGE_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=false -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=true -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true
make -j 32
cd ../scripts/numa-tpch-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/tpch/tpch_query --algo=tpch_nphj_sc --sf=100 >> $TPCH_NPHJ_SC_DRAM_HUGE_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/tpch/tpch_query --algo=tpch_rdx_bc --sf=100 >> $TPCH_RDX_BC_DRAM_HUGE_LOG





cd ../../
./clean.sh


date