cmake_minimum_required(VERSION 2.8)
set(cflags "-g -Wall -std=c99")

add_executable(ts thread_specific.c)
set_target_properties(ts PROPERTIES
    COMPILE_FLAGS ${cflags}
    )
target_link_libraries(ts pthread)

add_library(t_scope STATIC t_scope.c)
set_target_properties(t_scope PROPERTIES
    COMPILE_FLAGS ${cflags}
    )

add_executable(test test.c)
target_link_libraries(test 
    t_scope
    pthread
    )
set_target_properties(test PROPERTIES
    COMPILE_FLAGS ${cflags}
    )
