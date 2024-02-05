cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{CMAKE_TOOLCHAIN_FILE})
   set(CMAKE_TOOLCHAIN_FILE $ENV{CMAKE_TOOLCHAIN_FILE})
endif()

# Name of the project
project(mypaintExamplesBuildAll)

add_executable(minimal)
target_sources(minimal PRIVATE 
                            ${CMAKE_CURRENT_SOURCE_DIR}/minimal.c 
                            ${CMAKE_CURRENT_SOURCE_DIR}/write_ppm.c
                            )
target_link_directories(minimal PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(minimal PRIVATE mypaint)


