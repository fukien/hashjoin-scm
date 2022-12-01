link_libraries(-lm -lpmem -lpthread -lconfig)
aux_source_directory(. DIR_LIB_SRCS)
add_library(Inc SHARED ${DIR_LIB_SRCS})
# set_target_properties(Inc PROPERTIES LINK_FLAGS "-Wl,-rpath=/usr/local/lib/pmdk_debug")