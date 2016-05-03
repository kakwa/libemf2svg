cmake_minimum_required (VERSION 2.6)

project (emf2svg)

# Project version (sematic versionning)
set(emf2svg_VERSION_MAJOR 0)
set(emf2svg_VERSION_MINOR 5)
set(emf2svg_VERSION_PATCH 1)

set(emf2svg_VERSION ${emf2svg_VERSION_MAJOR}.${emf2svg_VERSION_MINOR}.${emf2svg_VERSION_PATCH})

add_custom_target(tag
    COMMAND git tag -a ${emf2svg_VERSION} -m "tagging version ${emf2svg_VERSION}"
    COMMAND git push origin ${emf2svg_VERSION}
)


# set version as a definition
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -DE2S_VERSION='\"${emf2svg_VERSION}\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DE2S_VERSION='\"${emf2svg_VERSION}\"'")

# Options
option(DEBUG     "compile with debug symbol"    OFF)
option(INDEX     "print record indexes"         OFF)
option(STATIC    "compile statically"           OFF)
option(USE_CLANG "build application with clang" OFF)
option(USE_GCC   "build application with gcc"   OFF)
option(FORCELE   "force little endian architecture"   OFF)

if(INDEX)
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -DRECORD_INDEX='true'")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRECORD_INDEX='true'")
endif(INDEX)

if(USE_CLANG)
    set(CMAKE_CXX_COMPILER "clang++")
    set(CMAKE_CC_COMPILER  "clang"  )
endif(USE_CLANG)

if(USE_GCC)
    set(CMAKE_CXX_COMPILER "g++")
    set(CMAKE_CC_COMPILER  "gcc")
endif(USE_GCC)

if(DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
    set(CMAKE_BUILD_TYPE Debug)
endif(DEBUG)

if(STATIC)
    set(SHARED "")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBRARIES OFF)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
else(STATIC)
    set(SHARED "SHARED")
endif(STATIC)

if(UNIX)
  link_libraries(m)
endif(UNIX)

IF(NOT FORCELE)
    include(TestBigEndian)
    TEST_BIG_ENDIAN(BIGENDIAN)
    IF(${BIGENDIAN})
        add_definitions(-DWORDS_BIGENDIAN)
    ENDIF(${BIGENDIAN})
endif(NOT FORCELE)

# Build external dependancies if we are on OSX
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   set(EXTERNAL_MEMSTREAM "memstream")
   set(EXTERNAL_ICONV "iconv")
   set(EXTERNAL_ARGP "argp")
   add_definitions(-DDARWIN)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(${CMAKE_SYSTEM_NAME} MATCHES "MinGW")
   # Mac OS X specific code
   set(EXTERNAL_MEMSTREAM "memstream")
   set(EXTERNAL_ICONV "iconv")
   set(EXTERNAL_ARGP "argp")
   add_definitions(-DMINGW)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "MinGW")


# linking directories
LINK_DIRECTORIES(
    ${CMAKE_BINARY_DIR}/
    /usr/local/lib
    /sw/lib/
    /usr/lib/
    c:/MinGW/lib
)

# headers directories

include_directories(
    ./inc/
    /usr/include/ 
    /sw/include/
    ./deps/mingw
    c:/MinGW/include
)    
#ELSE(${CMAKE_SYSTEM_NAME} MATCHES "MinGW Makefiles")
#    include_directories(
#        ./inc/
#        /usr/include/ 
#        /sw/include/
#    )
#ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "MinGW Makefiles")

# Compile the library
add_library(emf2svg
    ${SHARED}
    src/lib/pmf2svg.c
    src/lib/pmf2svg_print.c
    src/lib/uemf_utf.c
    src/lib/uemf_endian.c
    src/lib/uemf.c
    src/lib/upmf.c
    src/lib/emf2svg_utils.c
    src/lib/emf2svg_img_utils.c
    src/lib/emf2svg_clip_utils.c
    src/lib/emf2svg_rec_control.c
    src/lib/emf2svg_rec_object_creation.c
    src/lib/emf2svg_rec_path.c
    src/lib/emf2svg_rec_clipping.c
    src/lib/emf2svg_rec_drawing.c
    src/lib/emf2svg_rec_bitmap.c
    src/lib/emf2svg_rec_object_manipulation.c
    src/lib/emf2svg_rec_comment.c
    src/lib/emf2svg_rec_transform.c
    src/lib/emf2svg_rec_state_record.c
    src/lib/emf2svg_print.c
    src/lib/emf2svg.c
    deps/mingw/mingw_posix2.c
)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   add_library(memstream 
       ${SHARED} 
       src/lib/memstream.c
)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

set_target_properties(emf2svg 
    PROPERTIES 
    VERSION ${emf2svg_VERSION} 
    SOVERSION ${emf2svg_VERSION_MAJOR}
)

# Compile the executable
add_executable(emf2svg-conv src/conv/emf2svg.cpp)
target_link_libraries(emf2svg
    png
    ${EXTERNAL_MEMSTREAM}
    ${EXTERNAL_ICONV}
    ${EXTERNAL_ARGP}
) 


target_link_libraries(emf2svg-conv
    emf2svg
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall")

# install binaries and library
INSTALL(TARGETS emf2svg emf2svg-conv ${EXTERNAL_MEMSTREAM}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

# install header file
INSTALL(FILES inc/emf2svg.h DESTINATION "include")
