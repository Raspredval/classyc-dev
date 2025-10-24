macro(assert TEST_CONDITION MESSAGE)
  if(NOT ${TEST_CONDITION})
    message(FATAL_ERROR "${MESSAGE}")
  endif()
endmacro()

assert(
    "${CMAKE_ARGV1}" STREQUAL "-P" AND
    "${CMAKE_ARGV2}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/create_symlimk.cmake",
    "Invalid create_symlink arguments!")

set(LINK_SRC    ${CMAKE_ARGV3})
set(LINK_DEST   ${CMAKE_ARGV4})

file(CREATE_LINK ${LINK_SRC} ${LINK_DEST} SYMBOLIC COPY_ON_ERROR)