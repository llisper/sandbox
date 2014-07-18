include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/gtest/include
  )
link_directories(${CMAKE_SOURCE_DIR}/gtest/lib)

function(unittest_case test_name)
  set(target unittest_${test_name})
  set(libs 
    hamster_unittest
    gtest
    gtest_main
    pthread
    )

  if(${ARGC} GREATER 1 AND "${ARGV1}" STREQUAL "NO_GTEST_MAIN")
    list(REMOVE_ITEM libs gtest_main)
  endif(${ARGC} GREATER 1 AND "${ARGV1}" STREQUAL "NO_GTEST_MAIN")

  message(STATUS "add_test: ${target}")

  add_executable(${target} ${target}.cc)
  target_link_libraries(${target} ${libs}) 
  set_target_properties(${target} PROPERTIES
    COMPILE_FLAGS ${cflags}
    )
  add_test(NAME ${target} COMMAND ${target})

endfunction(unittest_case test_name)

unittest_case(shm_segments)
unittest_case(shm_rb_tree)
unittest_case(hamster)
