add_library(display display.c)
target_link_libraries(display PUBLIC amoled gui fonts qspi_pio)
