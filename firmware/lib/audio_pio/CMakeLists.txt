add_library(audio_pio audio_pio.c)

pico_generate_pio_header(audio_pio ${CMAKE_CURRENT_LIST_DIR}/audio_pio.pio
                        OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_include_directories(audio_pio PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(audio_pio PUBLIC pico_stdlib hardware_i2c hardware_pio hardware_dma)
