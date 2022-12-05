date



DIR_PATH=/work/huang/workspace/pm-join/logs/payload-logs
mkdir -p $DIR_PATH


MIN_POW_NUM=4
MAX_POW_NUM=9



NPHJ_SC_PAYLOAD_NVM_LOG=$DIR_PATH/nphj_sc_payload_nvm.log
rm $NPHJ_SC_PAYLOAD_NVM_LOG
NPHJ_LP_PAYLOAD_NVM_LOG=$DIR_PATH/nphj_lp_payload_nvm.log
rm $NPHJ_LP_PAYLOAD_NVM_LOG


for (( i = $MIN_POW_NUM; i <= $MAX_POW_NUM; i++ )); do
	cd ../../build
	rm -rf *
	cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=$((2**$i)) -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
	make -j 32
	cd ../scripts/payload-scripts/
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_sc --workload=payload --subtype=$((2**$i)) --param=pkfk >> $NPHJ_SC_PAYLOAD_NVM_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=nphj_lp --workload=payload --subtype=$((2**$i)) --param=pkfk >> $NPHJ_LP_PAYLOAD_NVM_LOG
done





PTR_NPHJ_SC_PAYLOAD_NVM_LOG=$DIR_PATH/ptr_nphj_sc_payload_nvm.log
rm $PTR_NPHJ_SC_PAYLOAD_NVM_LOG
PTR_NPHJ_LP_PAYLOAD_NVM_LOG=$DIR_PATH/ptr_nphj_lp_payload_nvm.log
rm $PTR_NPHJ_LP_PAYLOAD_NVM_LOG
for (( i = $MIN_POW_NUM; i <= $MAX_POW_NUM; i++ )); do
	cd ../../build
	rm -rf *
	cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=$((2**$i)) -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
	make -j 32
	cd ../scripts/payload-scripts/
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_nphj_sc --workload=payload --subtype=$((2**$i)) --param=pkfk >> $PTR_NPHJ_SC_PAYLOAD_NVM_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_nphj_lp --workload=payload --subtype=$((2**$i)) --param=pkfk >> $PTR_NPHJ_LP_PAYLOAD_NVM_LOG
done





RDX_BC_PAYLOAD_NVM_LOG=$DIR_PATH/rdx_bc_payload_nvm.log
rm $RDX_BC_PAYLOAD_NVM_LOG
RDX_SC_PAYLOAD_NVM_LOG=$DIR_PATH/rdx_sc_payload_nvm.log
rm $RDX_SC_PAYLOAD_NVM_LOG
RDX_LP_PAYLOAD_NVM_LOG=$DIR_PATH/rdx_lp_payload_nvm.log
rm $RDX_LP_PAYLOAD_NVM_LOG
RDX_HR_PAYLOAD_NVM_LOG=$DIR_PATH/rdx_hr_payload_nvm.log
rm $RDX_HR_PAYLOAD_NVM_LOG
# RDX_HRO_PAYLOAD_NVM_LOG=$DIR_PATH/rdx_hro_payload_nvm.log
# rm $RDX_HRO_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=12 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=payload --subtype=16 --param=pkfk >> $RDX_BC_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hr --workload=payload --subtype=16 --param=pkfk >> $RDX_HR_PAYLOAD_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=payload --subtype=16 --param=pkfk >> $RDX_HRO_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_lp --workload=payload --subtype=16 --param=pkfk >> $RDX_LP_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_sc --workload=payload --subtype=16 --param=pkfk >> $RDX_SC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=32 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=12 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=payload --subtype=32 --param=pkfk >> $RDX_BC_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hr --workload=payload --subtype=32 --param=pkfk >> $RDX_HR_PAYLOAD_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=payload --subtype=32 --param=pkfk >> $RDX_HRO_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_lp --workload=payload --subtype=32 --param=pkfk >> $RDX_LP_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=32 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_sc --workload=payload --subtype=32 --param=pkfk >> $RDX_SC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=64 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=payload --subtype=64 --param=pkfk >> $RDX_BC_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hr --workload=payload --subtype=64 --param=pkfk >> $RDX_HR_PAYLOAD_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=payload --subtype=64 --param=pkfk >> $RDX_HRO_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_lp --workload=payload --subtype=64 --param=pkfk >> $RDX_LP_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_sc --workload=payload --subtype=64 --param=pkfk >> $RDX_SC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=128 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=payload --subtype=128 --param=pkfk >> $RDX_BC_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hr --workload=payload --subtype=128 --param=pkfk >> $RDX_HR_PAYLOAD_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=payload --subtype=128 --param=pkfk >> $RDX_HRO_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_lp --workload=payload --subtype=128 --param=pkfk >> $RDX_LP_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=128 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_sc --workload=payload --subtype=128 --param=pkfk >> $RDX_SC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=256 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=payload --subtype=256 --param=pkfk >> $RDX_BC_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_hr --workload=payload --subtype=256 --param=pkfk >> $RDX_HR_PAYLOAD_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=payload --subtype=256 --param=pkfk >> $RDX_HRO_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_lp --workload=payload --subtype=256 --param=pkfk >> $RDX_LP_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_sc --workload=payload --subtype=256 --param=pkfk >> $RDX_SC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=512 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_bc --workload=payload --subtype=512 --param=pkfk >> $RDX_BC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=512 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_hr --workload=payload --subtype=512 --param=pkfk >> $RDX_HR_PAYLOAD_NVM_LOG
# numactl --membind=0 ../../bin/main --algo=phj_radix_hro --workload=payload --subtype=512 --param=pkfk >> $RDX_HRO_PAYLOAD_NVM_LOG
numactl --membind=0 ../../bin/main --algo=phj_radix_sc --workload=payload --subtype=512 --param=pkfk >> $RDX_SC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=512 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=17 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts
numactl --membind=0 ../../bin/main --algo=phj_radix_lp --workload=payload --subtype=512 --param=pkfk >> $RDX_LP_PAYLOAD_NVM_LOG






ASYM_BC_PAYLOAD_NVM_LOG=$DIR_PATH/asym_bc_payload_nvm.log
rm $ASYM_BC_PAYLOAD_NVM_LOG
ASYM_SC_PAYLOAD_NVM_LOG=$DIR_PATH/asym_sc_payload_nvm.log
rm $ASYM_SC_PAYLOAD_NVM_LOG
ASYM_HR_PAYLOAD_NVM_LOG=$DIR_PATH/asym_hr_payload_nvm.log
rm $ASYM_HR_PAYLOAD_NVM_LOG
# ASYM_HRO_PAYLOAD_NVM_LOG=$DIR_PATH/asym_hro_payload_nvm.log
# rm $ASYM_HRO_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/payload-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=payload --subtype=16 --param=pkfk >> $ASYM_BC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_sc --workload=payload --subtype=16 --param=pkfk >> $ASYM_SC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hr --workload=payload --subtype=16 --param=pkfk >> $ASYM_HR_PAYLOAD_NVM_LOG
# numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=payload --subtype=16 --param=pkfk >> $ASYM_HRO_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=32 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=13 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/payload-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=payload --subtype=32 --param=pkfk >> $ASYM_BC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=32 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=12 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/payload-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_sc --workload=payload --subtype=32 --param=pkfk >> $ASYM_SC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hr --workload=payload --subtype=32 --param=pkfk >> $ASYM_HR_PAYLOAD_NVM_LOG
# numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=payload --subtype=32 --param=pkfk >> $ASYM_HRO_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=64 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=13 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/payload-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=payload --subtype=64 --param=pkfk >> $ASYM_BC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_sc --workload=payload --subtype=64 --param=pkfk >> $ASYM_SC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hr --workload=payload --subtype=64 --param=pkfk >> $ASYM_HR_PAYLOAD_NVM_LOG
# numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=payload --subtype=64 --param=pkfk >> $ASYM_HRO_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=128 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=13 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/payload-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=payload --subtype=128 --param=pkfk >> $ASYM_BC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=128 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=14 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/payload-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hr --workload=payload --subtype=128 --param=pkfk >> $ASYM_HR_PAYLOAD_NVM_LOG
# numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=payload --subtype=128 --param=pkfk >> $ASYM_HRO_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=128 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=15 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/payload-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_sc --workload=payload --subtype=128 --param=pkfk >> $ASYM_SC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=256 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=14 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/payload-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=payload --subtype=256 --param=pkfk >> $ASYM_BC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_sc --workload=payload --subtype=256 --param=pkfk >> $ASYM_SC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hr --workload=payload --subtype=256 --param=pkfk >> $ASYM_HR_PAYLOAD_NVM_LOG
# numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=payload --subtype=256 --param=pkfk >> $ASYM_HRO_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=512 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=13 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/payload-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_bc --workload=payload --subtype=512 --param=pkfk >> $ASYM_BC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hr --workload=payload --subtype=512 --param=pkfk >> $ASYM_HR_PAYLOAD_NVM_LOG
# numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=payload --subtype=512 --param=pkfk >> $ASYM_HRO_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=512 -DRUN_BILLION=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=15 -DNUM_RADIX_BITS_PASS1=14 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=false
make -j 32
cd ../scripts/payload-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_sc --workload=payload --subtype=512 --param=pkfk >> $ASYM_SC_PAYLOAD_NVM_LOG








PTR_PHJ_RADIX_BC_PAYLOAD_NVM_LOG=$DIR_PATH/ptr_rdx_bc_payload_nvm.log
rm $PTR_PHJ_RADIX_BC_PAYLOAD_NVM_LOG
PTR_PHJ_RADIX_SC_PAYLOAD_NVM_LOG=$DIR_PATH/ptr_rdx_sc_payload_nvm.log
rm $PTR_PHJ_RADIX_SC_PAYLOAD_NVM_LOG
PTR_PHJ_RADIX_LP_PAYLOAD_NVM_LOG=$DIR_PATH/ptr_rdx_lp_payload_nvm.log
rm $PTR_PHJ_RADIX_LP_PAYLOAD_NVM_LOG
PTR_PHJ_RADIX_HR_PAYLOAD_NVM_LOG=$DIR_PATH/ptr_rdx_hr_payload_nvm.log
rm $PTR_PHJ_RADIX_HR_PAYLOAD_NVM_LOG
cd ../../build
rm -rf *
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=16 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=12 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_PART=true -DPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts/
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_bc --workload=payload --subtype=16 --param=pkfk >> $PTR_PHJ_RADIX_BC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_sc --workload=payload --subtype=16 --param=pkfk >> $PTR_PHJ_RADIX_SC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_lp --workload=payload --subtype=16 --param=pkfk >> $PTR_PHJ_RADIX_LP_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_hr --workload=payload --subtype=16 --param=pkfk >> $PTR_PHJ_RADIX_HR_PAYLOAD_NVM_LOG
cd ../../build
rm -rf *
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=32 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=12 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_PART=true -DPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts/
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_bc --workload=payload --subtype=32 --param=pkfk >> $PTR_PHJ_RADIX_BC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_lp --workload=payload --subtype=32 --param=pkfk >> $PTR_PHJ_RADIX_LP_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_hr --workload=payload --subtype=32 --param=pkfk >> $PTR_PHJ_RADIX_HR_PAYLOAD_NVM_LOG
cd ../../build
rm -rf *
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=32 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_PART=true -DPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts/
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_sc --workload=payload --subtype=32 --param=pkfk >> $PTR_PHJ_RADIX_SC_PAYLOAD_NVM_LOG
cd ../../build
rm -rf *
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=64 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=12 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_PART=true -DPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts/
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_bc --workload=payload --subtype=64 --param=pkfk >> $PTR_PHJ_RADIX_BC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_lp --workload=payload --subtype=64 --param=pkfk >> $PTR_PHJ_RADIX_LP_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_hr --workload=payload --subtype=64 --param=pkfk >> $PTR_PHJ_RADIX_HR_PAYLOAD_NVM_LOG
cd ../../build
rm -rf *
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=64 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=11 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_PART=true -DPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts/
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_sc --workload=payload --subtype=64 --param=pkfk >> $PTR_PHJ_RADIX_SC_PAYLOAD_NVM_LOG
cd ../../build
rm -rf *
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=128 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=12 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_PART=true -DPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts/
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_bc --workload=payload --subtype=128 --param=pkfk >> $PTR_PHJ_RADIX_BC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_sc --workload=payload --subtype=128 --param=pkfk >> $PTR_PHJ_RADIX_SC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_lp --workload=payload --subtype=128 --param=pkfk >> $PTR_PHJ_RADIX_LP_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_hr --workload=payload --subtype=128 --param=pkfk >> $PTR_PHJ_RADIX_HR_PAYLOAD_NVM_LOG
cd ../../build
rm -rf *
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=256 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_PART=true -DPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts/
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_bc --workload=payload --subtype=256 --param=pkfk >> $PTR_PHJ_RADIX_BC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_lp --workload=payload --subtype=256 --param=pkfk >> $PTR_PHJ_RADIX_LP_PAYLOAD_NVM_LOG
cd ../../build
rm -rf *
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=256 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_PART=true -DPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts/
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_hr --workload=payload --subtype=256 --param=pkfk >> $PTR_PHJ_RADIX_HR_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_sc --workload=payload --subtype=256 --param=pkfk >> $PTR_PHJ_RADIX_SC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=512 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=13 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_PART=true -DPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts/
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_bc --workload=payload --subtype=512 --param=pkfk >> $PTR_PHJ_RADIX_BC_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_sc --workload=payload --subtype=512 --param=pkfk >> $PTR_PHJ_RADIX_SC_PAYLOAD_NVM_LOG
cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_PAYLOAD=true -DTUPLE_T_SIZE=512 -DRUN_BILLION=false -DRUN_FNDLY=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DPHJ_SYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=12 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=14 -DNUM_RADIX_BITS_PASS1=17 -DNUM_PASSES=1 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_PART=true -DPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM=false -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DSKEW_HANDLING=false -DBW_REG=true -DNONTEMP_STORE_MATER=true -DNONTEMP_STORE_MATER_ON_NVM=false
make -j 32
cd ../scripts/payload-scripts/
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_lp --workload=payload --subtype=512 --param=pkfk >> $PTR_PHJ_RADIX_LP_PAYLOAD_NVM_LOG
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=ptr_phj_radix_hr --workload=payload --subtype=512 --param=pkfk >> $PTR_PHJ_RADIX_HR_PAYLOAD_NVM_LOG






cd ../../
./clean.sh




date