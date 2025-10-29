if (NOT ("${CMAKE_ARGV1}" STREQUAL "-P"))
    message(FATAL_ERROR "invalid first argument: expected \"-P\", got \"${CMAKE_ARGV1}\"")
endif()

get_filename_component(THIS_FILE "${CMAKE_ARGV2}" NAME)
if (NOT ("${THIS_FILE}" STREQUAL "create_symlink.cmake"))
    message(FATAL_ERROR "invalid second argument: expected a path to \"create_symlink.cmake\", got \"${CMAKE_ARGV2}\"")
endif()

# ^^^^^ Sanity Checks ^^^^^

set(LINK_SRC    ${CMAKE_ARGV3})
set(LINK_DEST   ${CMAKE_ARGV4})

file(CREATE_LINK ${LINK_SRC} ${LINK_DEST} SYMBOLIC COPY_ON_ERROR)