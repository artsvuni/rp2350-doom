aux_source_directory(. DIR_Config_SRCS)

add_library(config ${DIR_Config_SRCS})
target_include_directories(config PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(config PUBLIC pico_stdlib hardware_spi hardware_i2c hardware_pwm hardware_adc hardware_dma hardware_irq)
