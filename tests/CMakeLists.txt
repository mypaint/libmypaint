cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{CMAKE_TOOLCHAIN_FILE})
   set(CMAKE_TOOLCHAIN_FILE $ENV{CMAKE_TOOLCHAIN_FILE})
endif()

# Name of the project
project(mypainttestsBuildAll)

add_executable(test-brush-load)
target_sources(test-brush-load PRIVATE 
                                        ${CMAKE_CURRENT_SOURCE_DIR}/test-brush-load.c
                                        ${CMAKE_CURRENT_SOURCE_DIR}/testutils.c)
target_link_directories(test-brush-load PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(test-brush-load PRIVATE mypaint)


add_executable(test-details)
target_sources(test-details PRIVATE 
                                        ${CMAKE_CURRENT_SOURCE_DIR}/test-details.c
                                        ${CMAKE_CURRENT_SOURCE_DIR}/testutils.c
                                        ${CMAKE_CURRENT_SOURCE_DIR}/mypaint-benchmark.c)

target_link_directories(test-details PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(test-details PRIVATE mypaint)


add_executable(test-brush-persistence)
target_sources(test-brush-persistence PRIVATE 
                                        ${CMAKE_CURRENT_SOURCE_DIR}/test-brush-persistence.c
                                        ${CMAKE_CURRENT_SOURCE_DIR}/testutils.c
                                        ${CMAKE_CURRENT_SOURCE_DIR}/mypaint-benchmark.c)

target_link_directories(test-brush-persistence PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(test-brush-persistence PRIVATE mypaint)


add_executable(test-rng)
target_sources(test-rng PRIVATE 
                                        ${CMAKE_CURRENT_SOURCE_DIR}/test-rng.c
                                        ${CMAKE_CURRENT_SOURCE_DIR}/testutils.c
                                        ${CMAKE_CURRENT_SOURCE_DIR}/mypaint-benchmark.c)

target_link_directories(test-rng PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(test-rng PRIVATE mypaint)



add_executable(test-fixed-tiled-surface)
target_sources(test-fixed-tiled-surface PRIVATE 
                                        ${CMAKE_CURRENT_SOURCE_DIR}/test-fixed-tiled-surface.c
                                        ${CMAKE_CURRENT_SOURCE_DIR}/testutils.c
                                        ${CMAKE_CURRENT_SOURCE_DIR}/mypaint-test-surface.c
                                        ${CMAKE_CURRENT_SOURCE_DIR}/mypaint-utils-stroke-player.c
                                        ${CMAKE_CURRENT_SOURCE_DIR}/mypaint-benchmark.c)
target_link_directories(test-fixed-tiled-surface PRIVATE ${CMAKE_BINARY_DIR}/..)
target_link_libraries(test-fixed-tiled-surface PRIVATE mypaint)







add_custom_command(
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/tests/brushes
            ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/tests/brushes
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT  ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/tests/brushes/
    COMMENT "copying test files"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/tests/events
    ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/tests/events
WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
OUTPUT  ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/tests/events/
COMMENT "copying test files"
    )
# Now a custom target to copy the files which check to see if the files exist
add_custom_target(
    CopyTestfiles ALL
    DEPENDS ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/tests/brushes/)
# now add this a dependency for NGLTests so it gets run.
add_dependencies(test-brush-load CopyTestfiles )



