add_library(es8311 es8311.c)
target_include_directories(es8311 PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(es8311 PUBLIC config audio_pio pico_stdlib hardware_i2c)
