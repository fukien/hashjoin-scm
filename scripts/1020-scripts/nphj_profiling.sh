date



DIR_PATH=../../logs/1020-logs
mkdir -p $DIR_PATH




NPHJ_SC_PKFK_NVM_PROFILING_LOG=$DIR_PATH/nphj_sc_pkfk_nvm_profiling.log
rm $NPHJ_SC_PKFK_NVM_PROFILING_LOG
NPHJ_LP_PKFK_NVM_PROFILING_LOG=$DIR_PATH/nphj_lp_pkfk_nvm_profiling.log
rm $NPHJ_LP_PKFK_NVM_PROFILING_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=true -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/1020-scripts
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=uniform --subtype=A --param=pkfk >> $NPHJ_SC_PKFK_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=uniform --subtype=B --param=pkfk >> $NPHJ_SC_PKFK_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=uniform --subtype=C --param=pkfk >> $NPHJ_SC_PKFK_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_lp --workload=uniform --subtype=A --param=pkfk >> $NPHJ_LP_PKFK_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_lp --workload=uniform --subtype=B --param=pkfk >> $NPHJ_LP_PKFK_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_lp --workload=uniform --subtype=C --param=pkfk >> $NPHJ_LP_PKFK_NVM_PROFILING_LOG




NPHJ_SC_DUPFK_NVM_PROFILING_LOG=$DIR_PATH/nphj_sc_dupfk_nvm_profiling.log
rm $NPHJ_SC_DUPFK_NVM_PROFILING_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=true -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/1020-scripts
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=uniform --subtype=D --param=dupfk >> $NPHJ_SC_DUPFK_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=uniform --subtype=E --param=dupfk >> $NPHJ_SC_DUPFK_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=uniform --subtype=F --param=dupfk >> $NPHJ_SC_DUPFK_NVM_PROFILING_LOG





NPHJ_SC_ZIPF_NVM_PROFILING_LOG=$DIR_PATH/nphj_sc_zipf_nvm_profiling.log
rm $NPHJ_SC_ZIPF_NVM_PROFILING_LOG
NPHJ_LP_ZIPF_NVM_PROFILING_LOG=$DIR_PATH/nphj_lp_zipf_nvm_profiling.log
rm $NPHJ_LP_ZIPF_NVM_PROFILING_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=true -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/1020-scripts
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=zipfian --subtype=A --param=fk_1.050 >> $NPHJ_SC_ZIPF_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=zipfian --subtype=A --param=fk_1.250 >> $NPHJ_SC_ZIPF_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=zipfian --subtype=A --param=fk_1.500 >> $NPHJ_SC_ZIPF_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=zipfian --subtype=A --param=fk_1.750 >> $NPHJ_SC_ZIPF_NVM_PROFILING_LOG
# numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=zipfian --subtype=A --param=fk_1.900 >> $NPHJ_SC_ZIPF_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_lp --workload=zipfian --subtype=A --param=fk_1.050 >> $NPHJ_LP_ZIPF_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_lp --workload=zipfian --subtype=A --param=fk_1.250 >> $NPHJ_LP_ZIPF_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_lp --workload=zipfian --subtype=A --param=fk_1.500 >> $NPHJ_LP_ZIPF_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_lp --workload=zipfian --subtype=A --param=fk_1.750 >> $NPHJ_LP_ZIPF_NVM_PROFILING_LOG
# numactl --membind=0 ../../bin/main --algo=nphj_lp --workload=zipfian --subtype=A --param=fk_1.900 >> $NPHJ_LP_ZIPF_NVM_PROFILING_LOG




NPHJ_SC_SELECTIVITY_NVM_PROFILING_LOG=$DIR_PATH/nphj_sc_selectivity_nvm_profiling.log
rm $NPHJ_SC_SELECTIVITY_NVM_PROFILING_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=true -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/1020-scripts
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=selectivity --subtype=A --param=fk_0.200 >> $NPHJ_SC_SELECTIVITY_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=selectivity --subtype=A --param=fk_0.400 >> $NPHJ_SC_SELECTIVITY_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=selectivity --subtype=A --param=fk_0.600 >> $NPHJ_SC_SELECTIVITY_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=selectivity --subtype=A --param=fk_0.800 >> $NPHJ_SC_SELECTIVITY_NVM_PROFILING_LOG





NPHJ_SC_DENSITY_NVM_PROFILING_LOG=$DIR_PATH/nphj_sc_density_nvm_profiling.log
rm $NPHJ_SC_DENSITY_NVM_PROFILING_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=true -DWARMUP=true -DSYNCSTATS=true -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/1020-scripts
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=density --subtype=A --param=0.200 >> $NPHJ_SC_DENSITY_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=density --subtype=A --param=0.400 >> $NPHJ_SC_DENSITY_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=density --subtype=A --param=0.600 >> $NPHJ_SC_DENSITY_NVM_PROFILING_LOG
numactl --membind=0 ../../bin/main --algo=nphj_sc --workload=density --subtype=A --param=0.800 >> $NPHJ_SC_DENSITY_NVM_PROFILING_LOG





cd ../../
./clean.sh



date