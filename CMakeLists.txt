cmake_minimum_required (VERSION 2.6)

project (emf2svg)

set(EMFSVG_VERSION_MAJOR 0)
set(EMFSVG_VERSION_MINOR 0)
set(EMFSVG_VERSION_PATCH 1)
set(EMFSVG_VERSION ${EMFSVG_VERSION_MAJOR}.${EMFSVG_VERSION_MINOR}.${EMFSVG_VERSION_PATCH})

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
    set(CMAKE_BUILD_TYPE Debug)
endif(DEBUG)

if(STATIC)
    set(SHARED "")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    SET(BUILD_SHARED_LIBRARIES OFF)
    SET(CMAKE_EXE_LINKER_FLAGS "-static")
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

   SET(LIBCLIPPERBUILDDIR "${CMAKE_CURRENT_SOURCE_DIR}/external-build/clipper/")
   SET(LIBCLIPPERHEADERDIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/clipper/")
   SET(LIBCLIPPERLINKDIR  "${LIBCLIPPERBUILDDIR}/src/iconv-standalone-build/lib/")
   SET(LIBCLIPPERLIB  "clipper")

   ExternalProject_Add( clipper-standalone
        PREFIX ${LIBCLIPPERBUILDDIR}
        DOWNLOAD_DIR ${LIBCLIPPERBUILDDIR}/sources/
        DOWNLOAD_COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/deps/libpolyclipping/ ./
        SOURCE_DIR ${LIBCLIPPERBUILDDIR}/sources/
        CONFIGURE_COMMAND cmake ${LIBCLIPPERBUILDDIR}/sources/configure/
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

add_library(EMFSVG ${SHARED} src/lib/EMFSVG.c src/lib/EMFSVG_print.c src/lib/PMFSVG.c src/lib/PMFSVG_print.c src/lib/uemf_utf.c src/lib/uemf_endian.c src/lib/uemf.c src/lib/upmf.c)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   add_library(memstream ${SHARED} src/lib/memstream.c)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set_target_properties(EMFSVG PROPERTIES VERSION ${EMFSVG_VERSION} SOVERSION ${EMFSVG_VERSION_MAJOR})


add_executable(emf2svg src/conv/emf2svg.cpp)

target_link_libraries(emf2svg EMFSVG ${MEMSTREAMLIB} ${ARGPLIB} ${LIBICONVLIB}) 

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

INSTALL(TARGETS emf2svg EMFSVG ${MEMSTREAMLIB} ${ARGPLIB} ${LIBICONVLIB}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
INSTALL(FILES inc/EMFSVG.h DESTINATION "include")
