cmake_minimum_required(VERSION 2.8)

project(libhamster)

option(build_unittests "build unittests of libhamster" ON)

if (build_unittests)
  enable_testing()
endif (build_unittests)

include(CPack)

set(cflags "-g -Wall")
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
set(build_version "0.0.4")
set(so_version "1")

file(GLOB SOURCES 
  RELATIVE ${CMAKE_SOURCE_DIR}
  *.c
  )

add_library(hamster SHARED ${SOURCES})
set_target_properties(hamster PROPERTIES
  VERSION ${build_version}
  SOVERSION ${so_version}
  )

install(TARGETS hamster LIBRARY DESTINATION lib)
install(FILES hamster.h DESTINATION include)

# tests
if (build_unittests)
  add_library(hamster_unittest STATIC ${SOURCES}) 
  set_target_properties(hamster_unittest PROPERTIES
    COMPILE_DEFINITIONS "UNITTEST"
    COMPILE_FLAGS ${cflags}
    )
  add_subdirectory(tests)
endif (build_unittests)
