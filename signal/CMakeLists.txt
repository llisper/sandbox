cmake_minimum_required(VERSION 2.8)
set(target worker_test)
#add_executable(${target} sig_thread.c)
add_executable(${target}
    worker.c
    worker_test.c
    )
set_target_properties(${target}
    PROPERTIES
    COMPILE_FLAGS "-g -Wall"
    )
target_link_libraries(${target} "-lpthread")
