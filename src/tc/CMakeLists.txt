cmake_minimum_required (VERSION 3.0)
include_directories (${CMAKE_SOURCE_DIR}
                     ${CMAKE_SOURCE_DIR}/src
                     ${CMAKE_SOURCE_DIR}/src/libshared/src
                     ${TASK_INCLUDE_DIRS})

set (tc_SRCS
  ffi.h
  util.cpp util.h
  Replica.cpp Replica.h
  Task.cpp Task.h)

add_library (tc STATIC ${tc_SRCS})
