add_library(bootlog bootlog.c)
target_include_directories(bootlog PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(bootlog PUBLIC config qspi_pio amoled gui fonts)
