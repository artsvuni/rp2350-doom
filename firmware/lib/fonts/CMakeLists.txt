aux_source_directory(. DIR_fonts_SRCS)

add_library(fonts ${DIR_fonts_SRCS})
target_include_directories(fonts PUBLIC ${CMAKE_CURRENT_LIST_DIR})
