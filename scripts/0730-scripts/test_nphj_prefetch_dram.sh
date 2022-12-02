date


DIR_PATH=../../logs/0730-logs
mkdir -p $DIR_PATH



NPHJ_SC_PREFETCH_DRAM_LOG=$DIR_PATH/nphj_sc_prefetch_dram.log
rm $NPHJ_SC_PREFETCH_DRAM_LOG
NPHJ_LP_PREFETCH_DRAM_LOG=$DIR_PATH/nphj_lp_prefetch_dram.log
rm $NPHJ_LP_PREFETCH_DRAM_LOG



cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=false -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/0730-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_sc  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_SC_PREFETCH_DRAM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_lp  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_LP_PREFETCH_DRAM_LOG

for (( dist = 1; dist <= 14; dist++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=$((2**$dist)) -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/0730-scripts
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_sc  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_SC_PREFETCH_DRAM_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_lp  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_LP_PREFETCH_DRAM_LOG
done





NPHJ_SC_PREFETCH_DRAM_HUGE_LOG=$DIR_PATH/nphj_sc_prefetch_dram_huge.log
rm $NPHJ_SC_PREFETCH_DRAM_HUGE_LOG
NPHJ_LP_PREFETCH_DRAM_HUGE_LOG=$DIR_PATH/nphj_lp_prefetch_dram_huge.log
rm $NPHJ_LP_PREFETCH_DRAM_HUGE_LOG



cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=true -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=false -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/0730-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_sc  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_SC_PREFETCH_DRAM_HUGE_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_lp  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_LP_PREFETCH_DRAM_HUGE_LOG

for (( dist = 1; dist <= 14; dist++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=true -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=true -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=$((2**$dist)) -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DCLFLUSH=false -DCLFLUSHOPT=false -DNT_STORE=false -DENABLE_FENCE=false -DBUCKET_CAPACITY=2 -DBUCKET_CACHELINE_ALIGNED=false -DBUCKET_XPLINE_ALIGNED=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/0730-scripts
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_sc  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_SC_PREFETCH_DRAM_HUGE_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_lp  --workload=uniform --subtype=A --param=pkfk >> $NPHJ_LP_PREFETCH_DRAM_HUGE_LOG
done




cd ../../
./clean.sh


date