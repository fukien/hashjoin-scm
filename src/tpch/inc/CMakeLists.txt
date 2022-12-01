link_libraries(-lm)
aux_source_directory(. DIR_LIB_SRCS)
add_library(TPCHInc SHARED ${DIR_LIB_SRCS})
# set_target_properties(TPCHInc PROPERTIES LINK_FLAGS "-Wl,-rpath=/usr/local/lib/pmdk_debug")