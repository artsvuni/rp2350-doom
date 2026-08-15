aux_source_directory(. DIR_amoled_SRCS)

add_library(amoled ${DIR_amoled_SRCS})
target_include_directories(amoled PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(amoled PUBLIC qspi_pio config pico_stdlib hardware_gpio hardware_dma)
