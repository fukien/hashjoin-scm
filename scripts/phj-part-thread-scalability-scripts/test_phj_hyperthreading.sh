date


DIR_PATH=../../logs/phj-part-thread-scalability-logs
mkdir -p $DIR_PATH


<<COMMENT
PHJ_SHR_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG=$DIR_PATH/phj_shr_hro_thread_scalability_hyperthreading_dram.log
rm $PHJ_SHR_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG
PHJ_IND_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG=$DIR_PATH/phj_ind_hro_thread_scalability_hyperthreading_dram.log
rm $PHJ_IND_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG
PHJ_SHR_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG=$DIR_PATH/phj_shr_uni_hro_thread_scalability_hyperthreading_dram.log
rm $PHJ_SHR_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG
PHJ_IND_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG=$DIR_PATH/phj_ind_uni_hro_thread_scalability_hyperthreading_dram.log
rm $PHJ_IND_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG
PHJ_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG=$DIR_PATH/phj_radix_hro_thread_scalability_hyperthreading_dram.log
rm $PHJ_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG
PHJ_ASYMM_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG=$DIR_PATH/phj_asymm_radix_hro_thread_scalability_hyperthreading_dram.log
rm $PHJ_ASYMM_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG



for (( thr = 1; thr <= 20; thr++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=$((2*$thr)) -DPROBEJOIN_THREAD_NUM=$((2*$thr)) -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-part-thread-scalability-scripts
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_shr_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_SHR_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_ind_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_IND_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_shr_uni_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_SHR_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_ind_uni_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_IND_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=false -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=$((2*$thr)) -DPROBEJOIN_THREAD_NUM=$((2*$thr)) -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-part-thread-scalability-scripts
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_ASYMM_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_DRAM_LOG
done
COMMENT



PHJ_SHR_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG=$DIR_PATH/phj_shr_hro_thread_scalability_hyperthreading_nvm.log
rm $PHJ_SHR_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG
PHJ_IND_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG=$DIR_PATH/phj_ind_hro_thread_scalability_hyperthreading_nvm.log
rm $PHJ_IND_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG
PHJ_SHR_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG=$DIR_PATH/phj_shr_uni_hro_thread_scalability_hyperthreading_nvm.log
rm $PHJ_SHR_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG
PHJ_IND_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG=$DIR_PATH/phj_ind_uni_hro_thread_scalability_hyperthreading_nvm.log
rm $PHJ_IND_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG
PHJ_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG=$DIR_PATH/phj_radix_hro_thread_scalability_hyperthreading_nvm.log
rm $PHJ_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG
PHJ_ASYMM_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG=$DIR_PATH/phj_asymm_radix_hro_thread_scalability_hyperthreading_nvm.log
rm $PHJ_ASYMM_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG


for (( thr = 1; thr <= 20; thr++ )); do
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=$((2*$thr)) -DPROBEJOIN_THREAD_NUM=$((2*$thr)) -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-part-thread-scalability-scripts
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_shr_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_SHR_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_ind_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_IND_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_shr_uni_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_SHR_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_ind_uni_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_IND_UNI_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG
	cd ../../
	./clean.sh
	mkdir build
	cd build
	cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=$((2*$thr)) -DPROBEJOIN_THREAD_NUM=$((2*$thr)) -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
	make -j 32
	cd ../scripts/phj-part-thread-scalability-scripts
	numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_asymm_radix_hro --workload=uniform --subtype=A --param=pkfk >> $PHJ_ASYMM_RADIX_HRO_THREAD_SCALABILITY_HYPERTHREADING_NVM_LOG
done




RDX_HRO_THR_20_NVM_LOG=$DIR_PATH/rdx_hro_thr_20_nvm.log
rm $RDX_HRO_THR_20_NVM_LOG
RDX_BW_REG_11_HRO_NVM_LOG=$DIR_PATH/rdx_bw_reg_11_hro_nvm.log
rm $RDX_BW_REG_11_HRO_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false
make -j 32
cd ../scripts/phj-part-thread-scalability-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_radix_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_HRO_THR_20_NVM_LOG

cd ../../
./clean.sh
mkdir build
cd build
cmake .. -DTRIAL_NUM=6 -DRUN_BILLION=false -DRUN_PAYLOAD=false -DUSE_PAPI=false -DUSE_PMWATCH=false -DWARMUP=true -DSYNCSTATS=false -DUSE_HYPERTHREADING=true -DUSE_NVM=true -DUSE_HUGE=false -DTHREAD_NUM=20 -DBUILDPART_THREAD_NUM=20 -DBUILDPART_WRITE_THREAD_NUM=11 -DPROBEJOIN_THREAD_NUM=20 -DUSE_NUMA=false -DPREFETCH_DISTANCE=32 -DPREFETCH_CACHE=false -DPREFETCH_CACHE_NVM=false -DLOCK_IN_BUCKET=false -DNUM_RADIX_BITS=16 -DNUM_RADIX_BITS_PASS1=11 -DNUM_PASSES=2 -DPHJ_MBP=false -DTEST_PARTITIONING=false -DOUTPUT_OFFSET_NVM=false -DUSE_SWWCB_OPTIMIZED_PART=true -DUSE_SWWCB_OPTIMIZED_PART_ON_NVM=false  -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER=true -DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM=false -DBW_REG=true
make -j 32
cd ../scripts/phj-part-thread-scalability-scripts
numactl --physcpubind=0-19 --membind=0 ../../bin/main --algo=phj_radix_bw_reg_hro --workload=uniform --subtype=A --param=pkfk >> $RDX_BW_REG_11_HRO_NVM_LOG





cd ../../
./clean.sh


date