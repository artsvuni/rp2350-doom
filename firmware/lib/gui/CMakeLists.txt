aux_source_directory(. DIR_gui_SRCS)

add_library(gui ${DIR_gui_SRCS})
target_include_directories(gui PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(gui PUBLIC config)
