date



DIR_PATH=../../logs/0829-logs
mkdir -p $DIR_PATH




ST_NPHJ_SC_NVM_LOG=$DIR_PATH/st_nphj_sc_nvm.log
rm $ST_NPHJ_SC_NVM_LOG
ST_NPHJ_LP_NVM_LOG=$DIR_PATH/st_nphj_lp_nvm.log
rm $ST_NPHJ_LP_NVM_LOG


cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=1 -DBUILDPART_THREAD_NUM=1 -DINTERMEDIATEMEMSET_THREAD_NUM=1 -DPROBEJOIN_THREAD_NUM=1 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/0829-scripts
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=uniform --subtype=A --param=pkfk >> $ST_NPHJ_SC_NVM_LOG
numactl --membind=0 ../../bin/main --algo=nphj_lp --workload=uniform --subtype=A --param=pkfk >> $ST_NPHJ_LP_NVM_LOG



cd ../../
./clean.sh


date