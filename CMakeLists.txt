cmake_minimum_required (VERSION 2.6)

project (emf2svg)


IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   SET(MEMSTREAMLIB "memstream")
   SET(ARGPHEADERDIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/argp-standalone/)
   SET(ARGPBUILDDIR "${CMAKE_CURRENT_SOURCE_DIR}/external-build/argp/")
   add_definitions(-DDARWIN)

   include(ExternalProject)

   ExternalProject_Add(argp-standalone
        PREFIX ${ARGPBUILDDIR}
        DOWNLOAD_DIR ${ARGPBUILDDIR}/sources/
        DOWNLOAD_COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/deps/argp-standalone/ ${ARGPBUILDDIR}/sources/
        SOURCE_DIR ${ARGPBUILDDIR}/sources/
        PATCH_COMMAND patch -b argp-fmtstream.h < ${CMAKE_CURRENT_SOURCE_DIR}/deps/patch/patch-argp-fmtstream.h
        CONFIGURE_COMMAND ${ARGPBUILDDIR}/sources/configure
        BUILD_COMMAND ${MAKE}
   )

ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

LINK_DIRECTORIES(
    ${CMAKE_BINARY_DIR}/
    /usr/local/lib
    /usr/lib/
)

include_directories(
    ./inc/
    /usr/include/ 
    ${ARGPHEADERDIR}
)

add_library(EMFSVG src/lib/EMFSVG.cpp)
add_library(PMFSVG src/lib/PMFSVG.cpp)
add_library(uemf_utf src/lib/uemf_utf.c)
add_library(uemf_endian src/lib/uemf_endian.c)
add_library(uemf src/lib/uemf.c)
add_library(upmf src/lib/upmf.c)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   add_library(memstream src/lib/memstream.c)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


add_executable(emf2svg src/conv/emf2svg.cpp)

target_link_libraries(emf2svg EMFSVG PMFSVG upmf uemf uemf_utf uemf_endian ${MEMSTREAMLIB})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

