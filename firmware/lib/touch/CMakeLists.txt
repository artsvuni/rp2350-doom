add_library(touch FT3168.c)
target_include_directories(touch PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(touch PUBLIC config)
