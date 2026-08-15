aux_source_directory(. DIR_qspi_pio_SRCS)

add_library(qspi_pio ${DIR_qspi_pio_SRCS})

pico_generate_pio_header(qspi_pio ${CMAKE_CURRENT_LIST_DIR}/qspi.pio
                        OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_include_directories(qspi_pio PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(qspi_pio PUBLIC pico_stdlib hardware_pio hardware_gpio)
