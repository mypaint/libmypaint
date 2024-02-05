cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{CMAKE_TOOLCHAIN_FILE})
   set(CMAKE_TOOLCHAIN_FILE $ENV{CMAKE_TOOLCHAIN_FILE})
endif()

# Name of the project
project(mypaintBuildAll)
# This is the name of the Exe change this and it will change everywhere
set(TargetName mypaint)
find_package(json-c CONFIG REQUIRED)
add_library(${TargetName} STATIC)
include_directories(${CMAKE_SOURCE_DIR}/ )

include_directories(${CMAKE_SOURCE_DIR}/include )
include_directories(${CMAKE_SOURCE_DIR}/glib)
include_directories(${CMAKE_SOURCE_DIR}/fastapprox)

set(C_FILES 
            ${CMAKE_SOURCE_DIR}/src/brushmodes.c                  
            ${CMAKE_SOURCE_DIR}/src/mypaint-mapping.c             
            ${CMAKE_SOURCE_DIR}/src/mypaint.c
            ${CMAKE_SOURCE_DIR}/src/fifo.c                        
            ${CMAKE_SOURCE_DIR}/src/mypaint-matrix.c              
            ${CMAKE_SOURCE_DIR}/src/operationqueue.c
            ${CMAKE_SOURCE_DIR}/src/helpers.c                     
            ${CMAKE_SOURCE_DIR}/src/mypaint-rectangle.c           
            ${CMAKE_SOURCE_DIR}/src/rng-double.c
            ${CMAKE_SOURCE_DIR}/src/mypaint-brush-settings.c      
            ${CMAKE_SOURCE_DIR}/src/mypaint-surface.c             
            ${CMAKE_SOURCE_DIR}/src/tilemap.c
            ${CMAKE_SOURCE_DIR}/src/mypaint-brush.c              
            ${CMAKE_SOURCE_DIR}/src/mypaint-symmetry.c
            ${CMAKE_SOURCE_DIR}/src/mypaint-fixed-tiled-surface.c 
            ${CMAKE_SOURCE_DIR}/src/mypaint-tiled-surface.c)
target_sources(${TargetName} PRIVATE ${C_FILES})
target_link_libraries(${TargetName} PRIVATE json-c::json-c)


# run python script to generate headers
add_custom_target(
 run ALL
 COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generate.py ${CMAKE_CURRENT_SOURCE_DIR}/include/mypaint-brush-settings-gen.h ${CMAKE_CURRENT_SOURCE_DIR}/include/brushsettings-gen.h
 COMMENT "Generating headers"
)

add_dependencies(${TargetName} run)
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
add_subdirectory(${CMAKE_SOURCE_DIR}/examples)