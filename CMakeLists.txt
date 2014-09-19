cmake_minimum_required (VERSION 2.6)

project (emf2svg)

LINK_DIRECTORIES(
    ${CMAKE_BINARY_DIR}/lib
    /usr/local/lib
    /usr/lib/
)

include_directories(
    ./inc/
    /usr/include/ 
)

add_library(EMFSVG src/lib/EMFSVG.cpp)
add_library(PMFSVG src/lib/PMFSVG.cpp)
add_library(uemf_utf src/lib/uemf_utf.c)
add_library(uemf_endian src/lib/uemf_endian.c)
add_library(uemf src/lib/uemf.c)
add_library(upmf src/lib/upmf.c)

add_executable(emf2svg src/conv/emf2svg.cpp)

target_link_libraries(emf2svg EMFSVG PMFSVG upmf uemf uemf_utf uemf_endian)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

