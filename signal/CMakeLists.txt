cmake_minimum_required(VERSION 2.8)
add_executable(sig_thread sig_thread.c)
set_target_properties(sig_thread
    PROPERTIES
    COMPILE_FLAGS "-g -Wall"
    )
target_link_libraries(sig_thread "-lpthread")
