cmake_minimum_required (VERSION 2.6)

project (emf2svg)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES external-build)

option(USE_CLANG "build application with clang" OFF)

if(USE_CLANG)
    SET(CMAKE_CXX_COMPILER "clang++")
    SET(CMAKE_CC_COMPILER    "clang")
endif(USE_CLANG)

option(DEBUG "compile with debug symbol" OFF)

option(STATIC "compile statically" OFF)

if(DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
endif(DEBUG)

if(STATIC)
    set(SHARED "")
else(STATIC)
    set(SHARED "SHARED")
endif(STATIC)



IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   SET(MEMSTREAMLIB "memstream")
   add_definitions(-DDARWIN)

   include(ExternalProject)

   SET(LIBICONVBUILDDIR "${CMAKE_CURRENT_SOURCE_DIR}/external-build/libiconv/")
   SET(LIBICONVHEADERDIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/libiconv/")
   SET(LIBICONVLINKDIR  "${LIBICONVBUILDDIR}/src/iconv-standalone-build/lib/")
   SET(LIBICONVLIB  "iconv")

   ExternalProject_Add( iconv-standalone
        PREFIX ${LIBICONVBUILDDIR}
        DOWNLOAD_DIR ${LIBICONVBUILDDIR}/sources/
        DOWNLOAD_COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/deps/libiconv/ ./
        SOURCE_DIR ${LIBICONVBUILDDIR}/sources/
        CONFIGURE_COMMAND ${LIBICONVBUILDDIR}/sources/configure
        BUILD_COMMAND ${MAKE}
   )

   SET(ARGPHEADERDIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/argp-standalone/")
   SET(ARGPBUILDDIR "${CMAKE_CURRENT_SOURCE_DIR}/external-build/argp/")
   SET(ARGPLINKDIR  "${ARGPBUILDDIR}/src/argp-standalone-build/")
   SET(ARGPLIB  "argp")

   ExternalProject_Add( argp-standalone
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
    /sw/lib/
    /usr/lib/
    ${ARGPLINKDIR}
    ${LIBICONVLINKDIR}
)

include_directories(
    ./inc/
    /usr/include/ 
    /sw/include/
    ${ARGPHEADERDIR}
    ${LIBICONVHEADERDIR}
)

add_library(EMFSVG ${SHARED} src/lib/EMFSVG.c)
add_library(EMFSVG_print ${SHARED} src/lib/EMFSVG_print.c)
add_library(PMFSVG ${SHARED} src/lib/PMFSVG.c)
add_library(uemf_utf ${SHARED} src/lib/uemf_utf.c)
add_library(uemf_endian ${SHARED} src/lib/uemf_endian.c)
add_library(uemf ${SHARED} src/lib/uemf.c)
add_library(upmf ${SHARED} src/lib/upmf.c)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   add_library(memstream ${SHARED} src/lib/memstream.c)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


add_executable(emf2svg src/conv/emf2svg.cpp)

target_link_libraries(emf2svg EMFSVG EMFSVG_print PMFSVG upmf uemf uemf_utf uemf_endian ${MEMSTREAMLIB} ${ARGPLIB} ${LIBICONVLIB}) 

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

INSTALL(TARGETS emf2svg EMFSVG EMFSVG_print PMFSVG upmf uemf uemf_utf uemf_endian ${MEMSTREAMLIB} ${ARGPLIB} ${LIBICONVLIB}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
