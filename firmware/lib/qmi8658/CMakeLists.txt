add_library(qmi8658 qmi8658.c)
target_include_directories(qmi8658 PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(qmi8658 PUBLIC config pico_stdlib hardware_i2c)
