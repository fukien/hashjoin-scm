include_directories(${IPMWATCH_INC_PATH})
link_libraries(-lipmctl -lpmwcollect -lpmwapi)
aux_source_directory(. DIR_LIB_SRCS)
add_library(Pmw SHARED ${DIR_LIB_SRCS})