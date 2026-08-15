add_library(pwr_button pwr_button.c)
target_include_directories(pwr_button PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(pwr_button PUBLIC config)
