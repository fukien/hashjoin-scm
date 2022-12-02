date




CUR_DIR=$(pwd)
UNALIGNED_HEADER=$CUR_DIR/nphj_sc_unaligned.h
ALGO_DIR=../../src/algo/
DEFAULT_HEADER=$ALGO_DIR/nphj.h


DIR_PATH=../../logs/0803-logs
mkdir -p $DIR_PATH





NPHJ_SC_UNALIGNED_NVM_LOG=$DIR_PATH/nphj_sc_unaligned_nvm.log
rm $NPHJ_SC_UNALIGNED_NVM_LOG

cp $DEFAULT_HEADER $CUR_DIR/nphj_sc_aligned.h
cp $UNALIGNED_HEADER $ALGO_DIR/nphj.h

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=8 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/0803-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_sc  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_SC_UNALIGNED_NVM_LOG






NPHJ_SC_ALIGNED_NVM_LOG=$DIR_PATH/nphj_sc_aligned_nvm.log
rm $NPHJ_SC_ALIGNED_NVM_LOG

cp $CUR_DIR/nphj_sc_aligned.h $DEFAULT_HEADER

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=8 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=true -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/0803-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_sc  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_SC_ALIGNED_NVM_LOG






NPHJ_SC_XPLINE_ALIGNED_NVM_LOG=$DIR_PATH/nphj_sc_xpline_aligned_nvm.log
rm $NPHJ_SC_XPLINE_ALIGNED_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=8 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=true -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/0803-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_sc  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_SC_XPLINE_ALIGNED_NVM_LOG






NPHJ_SC_XPLINE_ALIGNED_BKT4_NVM_LOG=$DIR_PATH/nphj_sc_xpline_aligned_bkt4_nvm.log
rm $NPHJ_SC_XPLINE_ALIGNED_BKT4_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=8 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=4 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=true -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/0803-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_sc  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_SC_XPLINE_ALIGNED_BKT4_NVM_LOG






NPHJ_SC_XPLINE_ALIGNED_BKT4_7THR_NVM_LOG=$DIR_PATH/nphj_sc_xpline_aligned_bkt4_7thr_nvm.log
rm $NPHJ_SC_XPLINE_ALIGNED_BKT4_7THR_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=8 -DPROBEJOIN_THREAD_NUM=7 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=4 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=true -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/0803-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_sc  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_SC_XPLINE_ALIGNED_BKT4_7THR_NVM_LOG













cd ../../
./clean.sh




date