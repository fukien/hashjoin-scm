cmake_minimum_required(VERSION 3.16.3)
project(hashjoin-scm)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "-O3 -march=native -Wall")
# set(CMAKE_C_FLAGS "-O3 -march=native -Wall -ggdb3")
# set(CMAKE_C_FLAGS "-O0 -march=native -Wall -ggdb3")
# add_definitions(-DIN_DEBUG)
#ifdef IN_DEBUG
#endif /* IN_DEBUG */



set(PAPI_INC_PATH "/work/papi/src/install/include")
set(PAPI_LIB_PATH "/work/papi/src/install/lib")
set(IPMWATCH_INC_PATH "/work/intel-pmwatch/src/inc")
add_definitions(-DCPU_CFG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config/mc/cpu.cfg")
add_definitions(-DCORE_NUM=40)
add_definitions(-DSINGLE_SOCKET_CORE_NUM=20)



add_definitions(-DWORK_PATH_PREFIX="/pmemfs0/hashjoin-scm/work")
add_definitions(-DDUMP_PATH_PREFIX="/pmemfs0/hashjoin-scm/dump")

if(RUN_BILLION)
	add_definitions(-DDATA_PATH_PREFIX="/pmemfs1/hashjoin-scm/billion-data")
elseif(RUN_FNDLY)
	add_definitions(-DDATA_PATH_PREFIX="/pmemfs1/hashjoin-scm/fndly-data")
else()
	add_definitions(-DDATA_PATH_PREFIX="/pmemfs0/hashjoin-scm/data")
endif()

set(SRC_PATH "src")
set(BIN_PATH "bin")
set(INC_PATH "src/inc")
set(PMW_PATH "src/pmw")
set(PAPI_PATH "src/papi")
set(ALGO_PATH "src/algo")
set(JOB_PATH "src/job")
set(SSB_PATH "src/ssb")
set(TPCH_PATH "src/tpch")

add_definitions(-DDATA_CFG_PATH_PREFIX="${CMAKE_CURRENT_SOURCE_DIR}/config")


if(USE_PMWATCH)
	add_definitions(-DUSE_PMWATCH)
endif()

if(USE_PAPI)
	add_definitions(-DUSE_PAPI)
endif()

add_definitions(-DCHAR_BUFFER_LEN=256)

if(TRIAL_NUM)
	add_definitions(-DTRIAL_NUM=${TRIAL_NUM})
else()
	add_definitions(-DTRIAL_NUM=4)
endif()

if(USE_NVM)
	add_definitions(-DUSE_NVM)
endif()

if(CLFLUSH)
	add_definitions(-DCLFLUSH)
endif()

if(CLFLUSHOPT)
	add_definitions(-DCLFLUSHOPT)
endif()

if(NT_STORE)
	add_definitions(-DNT_STORE)
endif()

if(ENABLE_FENCE)
	add_definitions(-DENABLE_FENCE)
endif()

if (WARMUP)
	add_definitions(-DWARMUP)
endif()

if (UN_PREPOPULATE)
	add_definitions(-DUN_PREPOPULATE)
endif()

if(USE_HUGE)
	add_definitions(-DUSE_HUGE)
endif()

if(USE_ALIGNMENT)
	add_definitions(-DUSE_ALIGNMENT)
endif()

if(USE_HYPERTHREADING)
	add_definitions(-DUSE_HYPERTHREADING)
endif()

if(SYNCSTATS)
	add_definitions(-DSYNCSTATS)
endif()

if(USE_NUMA)
	add_definitions(-DUSE_NUMA)
endif()

if(TUPLE_T_SIZE)
	add_definitions(-DTUPLE_T_SIZE=${TUPLE_T_SIZE})
endif()

if(THREAD_NUM)
	add_definitions(-DTHREAD_NUM=${THREAD_NUM})
else()
	add_definitions(-DTHREAD_NUM=20)
endif()

if(BUILDPART_THREAD_NUM)
	add_definitions(-DBUILDPART_THREAD_NUM=${BUILDPART_THREAD_NUM})
else()
	add_definitions(-DBUILDPART_THREAD_NUM=12)
	# add_definitions(-DBUILDPART_THREAD_NUM=${THREAD_NUM})
	# add_definitions(-DBUILDPART_THREAD_NUM=20)
	# add_definitions(-DBUILDPART_THREAD_NUM=10)
	# add_definitions(-DBUILDPART_THREAD_NUM=9)
	# add_definitions(-DBUILDPART_THREAD_NUM=1)
endif()

if(BUILDPART_WRITE_THREAD_NUM)
	add_definitions(-DBUILDPART_WRITE_THREAD_NUM=${BUILDPART_WRITE_THREAD_NUM})
else()
	add_definitions(-DBUILDPART_WRITE_THREAD_NUM=11)
	# add_definitions(-DBUILDPART_WRITE_THREAD_NUM=${THREAD_NUM})
	# add_definitions(-DBUILDPART_WRITE_THREAD_NUM=20)
	# add_definitions(-DBUILDPART_WRITE_THREAD_NUM=10)
	# add_definitions(-DBUILDPART_WRITE_THREAD_NUM=9)
	# add_definitions(-DBUILDPART_WRITE_THREAD_NUM=1)
endif()

if(INTERMEDIATEMEMSET_THREAD_NUM)
	add_definitions(-DINTERMEDIATEMEMSET_THREAD_NUM=${INTERMEDIATEMEMSET_THREAD_NUM})
else()
	add_definitions(-DINTERMEDIATEMEMSET_THREAD_NUM=7)
	# add_definitions(-DINTERMEDIATEMEMSET_THREAD_NUM=${BUILDPART_THREAD_NUM})
	# add_definitions(-DINTERMEDIATEMEMSET_THREAD_NUM=20)
	# add_definitions(-DINTERMEDIATEMEMSET_THREAD_NUM=16)
	# add_definitions(-DINTERMEDIATEMEMSET_THREAD_NUM=1)
endif()

if(PROBEJOIN_THREAD_NUM)
	add_definitions(-DPROBEJOIN_THREAD_NUM=${PROBEJOIN_THREAD_NUM})
else()
	add_definitions(-DPROBEJOIN_THREAD_NUM=20)
	# add_definitions(-DPROBEJOIN_THREAD_NUM=${THREAD_NUM})
	# add_definitions(-DPROBEJOIN_THREAD_NUM=16)
	# add_definitions(-DPROBEJOIN_THREAD_NUM=2)
	# add_definitions(-DPROBEJOIN_THREAD_NUM=1)
endif()

if(RETRIEVE_THREAD_NUM)
	add_definitions(-DRETRIEVE_THREAD_NUM=${RETRIEVE_THREAD_NUM})
else()
	add_definitions(-DRETRIEVE_THREAD_NUM=19)
endif()

if(XSOCKET_THREAD_NUM_DEFAULT)
	add_definitions(-DXSOCKET_THREAD_NUM_DEFAULT=${XSOCKET_THREAD_NUM_DEFAULT})
else()
	add_definitions(-DXSOCKET_THREAD_NUM_DEFAULT=4)
endif()

if(TEST_SELECTIVITY)
	add_definitions(-DTEST_SELECTIVITY)
endif()

if (RUN_BILLION)
	add_definitions(-DRUN_BILLION)
endif()

if (RUN_FNDLY)
	add_definitions(-DRUN_FNDLY)
if (FNDLY_R_CARDINALITY)
	add_definitions(-DFNDLY_R_CARDINALITY=${FNDLY_R_CARDINALITY})
else()
	add_definitions(-DFNDLY_R_CARDINALITY=1024)
endif()
if (FNDLY_S_CARDINALITY)
	add_definitions(-DFNDLY_S_CARDINALITY=${FNDLY_S_CARDINALITY})
else()
	add_definitions(-DFNDLY_S_CARDINALITY=16384)
endif()
endif()

if (RUN_MINI)
	add_definitions(-DRUN_MINI)
endif()

if (RUN_PAYLOAD)
	add_definitions(-DRUN_PAYLOAD)
endif()

# ptr
if (NONTEMP_STORE_MATER)
	add_definitions(-DNONTEMP_STORE_MATER)
endif()

if (NONTEMP_STORE_MATER_ON_NVM)
	add_definitions(-DNONTEMP_STORE_MATER_ON_NVM)
endif()


# NPHJ
if(PREFETCH_DISTANCE)
	add_definitions(-DPREFETCH_DISTANCE=${PREFETCH_DISTANCE})
# else()
	# add_definitions(-DPREFETCH_DISTANCE=8)
endif()

if(PREFETCH_CACHE)
	add_definitions(-DPREFETCH_CACHE)
endif()

if(PREFETCH_CACHE_NVM)
	add_definitions(-DPREFETCH_CACHE_NVM)
endif()


if(BUCKET_CAPACITY)
	add_definitions(-DBUCKET_CAPACITY=${BUCKET_CAPACITY})
else()
	add_definitions(-DBUCKET_CAPACITY=2)
endif()

if(BUCKET_CACHELINE_ALIGNED)
	add_definitions(-DBUCKET_CACHELINE_ALIGNED)
endif()

if(BUCKET_XPLINE_ALIGNED)
	add_definitions(-DBUCKET_XPLINE_ALIGNED)
endif()

if(OVERFLOW_BUF_SIZE)
	add_definitions(-DOVERFLOW_BUF_SIZE=${OVERFLOW_BUF_SIZE})
else()
	add_definitions(-DOVERFLOW_BUF_SIZE=1024)
endif()

if(LOCK_IN_BUCKET)
	add_definitions(-DLOCK_IN_BUCKET)
endif()


# PHJ
if(BW_REG)
	add_definitions(-DBW_REG)
endif()

if(NUM_RADIX_BITS)
	add_definitions(-DNUM_RADIX_BITS=${NUM_RADIX_BITS})
else()
	add_definitions(-DNUM_RADIX_BITS=10)
endif()

if(NUM_PASSES)
	add_definitions(-DNUM_PASSES=${NUM_PASSES})
else()
	add_definitions(-DNUM_PASSES=1)
endif()

if(SKEW_HANDLING)
	add_definitions(-DSKEW_HANDLING)
endif()

if(PHJ_SYNCSTATS)
	add_definitions(-DPHJ_SYNCSTATS)
endif()

if(OUTPUT_OFFSET_NVM)
	add_definitions(-DOUTPUT_OFFSET_NVM)
endif()

if(SWWCB_SIZE)
	add_definitions(-DSWWCB_SIZE=${SWWCB_SIZE})
endif()

if(USE_SWWCB_OPTIMIZED_PART)
	add_definitions(-DUSE_SWWCB_OPTIMIZED_PART)
endif()
if(USE_SWWCB_OPTIMIZED_PART_ON_NVM)
	add_definitions(-DUSE_SWWCB_OPTIMIZED_PART_ON_NVM)
endif()

if(USE_SWWCB_OPTIMIZED_RADIX_CLUSTER)
	add_definitions(-DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER)
endif()
if(USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM)
	add_definitions(-DUSE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM)
endif()

if(PTR_USE_SWWCB_OPTIMIZED_PART)
	add_definitions(-DPTR_USE_SWWCB_OPTIMIZED_PART)
endif()
if(PTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM)
	add_definitions(-DPTR_USE_SWWCB_OPTIMIZED_PART_ON_NVM)
endif()

if(PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER)
	add_definitions(-DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER)
endif()
if(PTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM)
	add_definitions(-DPTR_USE_SWWCB_OPTIMIZED_RADIX_CLUSTER_ON_NVM)
endif()


# memset, build, probe
if(PHJ_MBP)
	add_definitions(-DPHJ_MBP)
endif()

if(INTERMEDIATE_SCALING_FACTOR)
	add_definitions(-DINTERMEDIATE_SCALING_FACTOR=${INTERMEDIATE_SCALING_FACTOR})
else()
	add_definitions(-DINTERMEDIATE_SCALING_FACTOR=2)
endif()

if(PHJ_SC_BUCKET_CAPACITY)
	add_definitions(-DPHJ_SC_BUCKET_CAPACITY=${PHJ_SC_BUCKET_CAPACITY})
else()
	add_definitions(-DPHJ_SC_BUCKET_CAPACITY=2)
endif()

if(TEST_PARTITIONING)
	add_definitions(-DTEST_PARTITIONING)
endif()

# PHJ_ASYMM_RADIX
if(NUM_RADIX_BITS_PASS1)
	add_definitions(-DNUM_RADIX_BITS_PASS1=${NUM_RADIX_BITS_PASS1})
endif()


# PHJ_SHR
if(SHARE_LATCH_ON_NVM)
	add_definitions(-DSHARE_LATCH_ON_NVM)
endif()

if(PART_BUFFER_T_CAPACITY)
	add_definitions(-DPART_BUFFER_T_CAPACITY=${PART_BUFFER_T_CAPACITY})
endif()


# SSB
if(QUERYFLIGHT)
	add_definitions(-DQUERYFLIGHT=${QUERYFLIGHT})
else()
	add_definitions(-DQUERYFLIGHT=4)
endif()

# TPCH or SSB
if(QUERY)
	add_definitions(-DQUERY=${QUERY})
else()
	add_definitions(-DQUERY=14)
endif()



# FNDLY
if(FNDLY_R_CARDINALITY)
	add_definitions(-DFNDLY_R_CARDINALITY=${FNDLY_R_CARDINALITY})
endif()
if(FNDLY_S_CARDINALITY)
	add_definitions(-DFNDLY_S_CARDINALITY=${FNDLY_S_CARDINALITY})
endif()



add_subdirectory(${INC_PATH})
link_libraries(Inc)

add_subdirectory(${PMW_PATH})
link_libraries(Pmw)

include_directories(${PAPI_INC_PATH})
add_subdirectory(${PAPI_PATH})
link_directories(${PAPI_LIB_PATH})
link_libraries(Papi -lpapi)

add_subdirectory(${ALGO_PATH})


add_executable(gen ${SRC_PATH}/gen.c)
set_target_properties(gen PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})
target_link_libraries(gen -lpmem)

add_executable(fndly_gen ${SRC_PATH}/fndly_gen.c)
set_target_properties(fndly_gen PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})
target_link_libraries(fndly_gen -lpmem)


add_executable(main ${SRC_PATH}/main.c)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})
# set_target_properties(main PROPERTIES LINK_FLAGS "-Wl,-rpath=/work/huang/workspace/debug_pmdk/src/debug")
# set_target_properties(main PROPERTIES LINK_FLAGS "-Wl,-rpath=/usr/local/lib/pmdk_debug")
target_link_libraries(main -lpthread -lnuma Algo)
target_include_directories(main PRIVATE ${IPMWATCH_INC_PATH})
target_include_directories(main PRIVATE ${PAPI_INC_PATH})


add_executable(persist_cost ${SRC_PATH}/microbench/persist_cost.c)
set_target_properties(persist_cost PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH}/microbench)
target_link_libraries(persist_cost -lpmem)
target_include_directories(persist_cost PRIVATE ${IPMWATCH_INC_PATH})
target_include_directories(persist_cost PRIVATE ${PAPI_INC_PATH})

add_executable(page_fault_cost_1 ${SRC_PATH}/microbench/page_fault_cost_1.c)
set_target_properties(page_fault_cost_1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH}/microbench)
target_link_libraries(page_fault_cost_1 -lpmem)
target_include_directories(page_fault_cost_1 PRIVATE ${IPMWATCH_INC_PATH})
target_include_directories(page_fault_cost_1 PRIVATE ${PAPI_INC_PATH})


add_subdirectory(${TPCH_PATH}/inc)
add_subdirectory(${TPCH_PATH}/algo)


add_executable(tpch_toy ${TPCH_PATH}/tpch_toy.c)
set_target_properties(tpch_toy PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH}/tpch)
target_link_libraries(tpch_toy TPCHInc)

add_executable(tpch_tbl2bin ${TPCH_PATH}/tpch_tbl2bin.c)
set_target_properties(tpch_tbl2bin PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH}/tpch)
target_link_libraries(tpch_tbl2bin -lpmem TPCHInc)

add_executable(tpch_query ${TPCH_PATH}/tpch_query.c)
set_target_properties(tpch_query PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH}/tpch)
target_link_libraries(tpch_query -lpmem TPCHInc TPCHAlgo)
target_include_directories(tpch_query PRIVATE ${IPMWATCH_INC_PATH})
target_include_directories(tpch_query PRIVATE ${PAPI_INC_PATH})