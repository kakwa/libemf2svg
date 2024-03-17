cmake_minimum_required (VERSION 3.12)
include(CheckFunctionExists)
include(ExternalProject)

# Comments re vcpkg
# - We use vcpkg to install the following libraries
#   and their dependencies:
#   -- libpng
#   -- freetype
#   -- fontconfig
#   -- iconv
# - vcpkg is configured as git submodule. Some experts consider it is the best practice
#   (though other experts do not)
# - The list of vcpkg modules is specified at vcpkg.json
# - With this setup and when called with -DCMAKE_TOOLCHAIN_FILE={project root}/vcpkg/scripts/buildsystems/vcpkg.cmake
#   CMake automatically runs vcpkg install for all listed packages before the project directive is executed

project (emf2svg)

set(emf2svg_VERSION_MAJOR 1)
set(emf2svg_VERSION_MINOR 7)
set(emf2svg_VERSION_PATCH 3)
set(emf2svg_VERSION ${emf2svg_VERSION_MAJOR}.${emf2svg_VERSION_MINOR}.${emf2svg_VERSION_PATCH})

if(VCPKG_TARGET_TRIPLET)
  set(PLATFORM_TOOLCHAIN ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${VCPKG_TARGET_TRIPLET}.cmake)
  message(STATUS "Configured with platform toolchain = '${PLATFORM_TOOLCHAIN}'")
  if(EXISTS ${PLATFORM_TOOLCHAIN})
    set(PLATFORM_TOOLCHAIN_OPTION -DCMAKE_TOOLCHAIN_FILE=${PLATFORM_TOOLCHAIN})
    include(${PLATFORM_TOOLCHAIN})
  endif(EXISTS ${PLATFORM_TOOLCHAIN})
endif(VCPKG_TARGET_TRIPLET)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

option(LONLY     "build library only"           OFF)
option(GCOV      "compile with gcov support"    OFF)
option(UNITTEST  "compile unit tests"           OFF)
option(INDEX     "print record indexes"         OFF)
option(STATIC    "compile statically"           OFF)
option(FORCELE   "force little endian architecture"   OFF)

if(STATIC)
    set(SHARED "SHARED")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBRARIES OFF)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
else(STATIC)
    set(SHARED "SHARED")
endif(STATIC)

if(GCOV)
  SET(UNITTEST ON)
  SET(CMAKE_BUILD_TYPE "Debug")
endif(GCOV)

if(UNITTEST)
  SET(LONLY OFF)
  message(WARNING "LONLY cannot be used with GCOV and/or UNITTEST; ignoring")
endif(UNITTEST)

find_package(PNG REQUIRED)
find_package(Freetype REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(LibXml2 REQUIRED)

set(DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps)
set(EXTERNAL_INCLUDE_DIR ${DEPS}/include)
set(EXTERNAL_LIB_DIR ${DEPS}/lib)
set(PATCHES ${CMAKE_CURRENT_SOURCE_DIR}/patches)

# Comments re fmem setup
# The patch fixes https://github.com/Snaipe/fmem/issues/4
# https://gitlab.kitware.com/cmake/cmake/-/issues/21086
# Basically, you cannot predict when PATCH_COMMAND is executed so we always roll back
# the patch that may be applied during the previous build in order to ensure that
# the same patch may be applied correctly again

# And finally a small helper for building native Windows and WSL from single source tree
set(FMEM_NAME fm${EXTERNAL_LIB_DIR_SUFFIX})

ExternalProject_Add(${FMEM_NAME}
   PREFIX ${DEPS}
   GIT_REPOSITORY https://github.com/tamatebako/fmem.git
   GIT_TAG bdce2760f0190253600f11984220fc0a007742c8
   CMAKE_ARGS -DBUILD_TESTING=FALSE
              -DCMAKE_INSTALL_PREFIX=${DEPS}
              -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
              ${PLATFORM_TOOLCHAIN_OPTION}
              ${CMAKE_OSX_ARCHITECTURES_OPTION}
)
set(EXTERNAL_FMEM "fmem")

set(DEPS_UEMF libUEMF-0.2.5)

add_custom_target(tag
    COMMAND git tag -a ${emf2svg_VERSION} -m "tagging version ${emf2svg_VERSION}"
    COMMAND git push origin ${emf2svg_VERSION}
)

# set version as a definition
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DE2S_VERSION='\"${emf2svg_VERSION}\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DE2S_VERSION=${emf2svg_VERSION}")

if(NOT LONLY)
  message(STATUS "Configuring argp")
  list(APPEND CMAKE_MESSAGE_INDENT "   ")
  message(STATUS "Checking if argp is provided by system libraries")
  CHECK_FUNCTION_EXISTS(argp_parse HAVE_BUNDLED_ARGP_PARSE_FUNCTION)

  message(STATUS "Looking for standalone argp library")
  set(CMAKE_REQUIRED_LIBRARIES argp)
  CHECK_FUNCTION_EXISTS(argp_parse HAVE_EXTERNAL_ARGP_PARSE_FUNCTION)
  list(POP_BACK CMAKE_MESSAGE_INDENT)
endif(NOT LONLY)

if(INDEX)
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -DRECORD_INDEX='true'")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRECORD_INDEX='true'")
endif(INDEX)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Od -Zi")
  else(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
    set(CMAKE_EXE_LINKER_FLAGS "-INCREMENTAL:NO")
  endif(MSVC)
endif()

if(UNIX)
  link_libraries(m)
  add_compile_options(-fPIC)
endif(UNIX)

if(NOT FORCELE)
  include(TestBigEndian)
  TEST_BIG_ENDIAN(BIGENDIAN)
  IF(${BIGENDIAN})
      add_definitions(-DWORDS_BIGENDIAN)
  ENDIF(${BIGENDIAN})
endif(NOT FORCELE)

# Build external dependancies if we are on OSX
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  execute_process(
    COMMAND brew --prefix
      RESULT_VARIABLE BREW_PREFIX_RES
      OUTPUT_VARIABLE BREW_PREFIX
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(NOT (BREW_PREFIX_RES EQUAL 0))
      message(FATAL_ERROR "Could not find brew setup")
  endif()
  set(BREW_LIB_DIR ${BREW_PREFIX}/lib)
  set(EXTERNAL_ICONV "iconv")
  add_definitions(-DDARWIN)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Find/build external dependencies if it is Microsoft Visual Studio build
if(MSVC)
  find_package(Iconv REQUIRED)
  set(EXTERNAL_ICONV ${Iconv_LIBRARY})
endif(MSVC)

if(NOT LONLY)
  if(HAVE_BUNDLED_ARGP_PARSE_FUNCTION)
    message(STATUS "Using bundled argp")
  elseif(HAVE_EXTERNAL_ARGP_PARSE_FUNCTION)
    message(STATUS "Using stand-alone argp")
    set(EXTERNAL_ARGP "argp")
  else()
    message(STATUS "Building argp")
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # https://www.gnu.org/software/gnulib/manual/html_node/argp_005fprogram_005fversion_005fhook.html
      set(EXTERNAL_ARGP "-Wl,-force_load,${DEPS}/lib/libargp-standalone.a")
    else(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(EXTERNAL_ARGP "argp-standalone")
    endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    set(ARGP_NAME argp${EXTERNAL_LIB_DIR_SUFFIX})
    ExternalProject_Add(${ARGP_NAME}
      PREFIX ${DEPS}
      GIT_REPOSITORY https://github.com/tom42/argp-standalone.git
      GIT_TAG 238d83d6fb4fbdbb3e0893f51698d8d54696bfb0
      CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${DEPS}
                  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                  ${PLATFORM_TOOLCHAIN_OPTION}
                  ${CMAKE_OSX_ARCHITECTURES_OPTION}
      PATCH_COMMAND cd ${DEPS}/src/${ARGP_NAME} && git restore CMakeLists.txt && git apply ${PATCHES}/argp/CMakeLists.txt.patch
    )
  endif(HAVE_BUNDLED_ARGP_PARSE_FUNCTION)
endif(NOT LONLY)

if(MSVC)
  include_directories(
    ./inc
    ${PNG_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIRS}
    ${EXTERNAL_INCLUDE_DIR}
    ${DEPS_UEMF}
  )
  link_directories(
    ${CMAKE_BINARY_DIR}
    ${EXTERNAL_LIB_DIR}
  )
else(MSVC)
  include_directories(
    ./inc
    ${PNG_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIRS}
    ${EXTERNAL_INCLUDE_DIR}
    ${DEPS_UEMF}
    /usr/local/include
    /usr/include
    /sw/include
  )
  link_directories(
    ${CMAKE_BINARY_DIR}
    ${EXTERNAL_LIB_DIR}
    ${BREW_LIB_DIR}
  )
endif(MSVC)

add_library(emf2svg
  ${SHARED}
  src/lib/pmf2svg.c
  src/lib/pmf2svg_print.c
  ${DEPS_UEMF}/uemf_utf.c
  ${DEPS_UEMF}/uemf_endian.c
  ${DEPS_UEMF}/uemf.c
  ${DEPS_UEMF}/upmf.c
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
)

set_target_properties(emf2svg
  PROPERTIES
  VERSION ${emf2svg_VERSION}
  SOVERSION ${emf2svg_VERSION_MAJOR}
)

target_link_libraries(emf2svg
  ${PNG_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${EXTERNAL_ICONV}
  ${FREETYPE_LIBRARIES}
  ${FONTCONFIG_LIBRARIES}
  ${EXPAT_LIBRARY_RELEASE}
  ${EXTERNAL_FMEM}
)

add_dependencies(emf2svg ${FMEM_NAME})

if(NOT LONLY)
  add_executable(emf2svg-conv src/conv/emf2svg.cpp)

  target_link_libraries(emf2svg-conv
    emf2svg
    ${EXTERNAL_ARGP}
    ${PNG_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${EXTERNAL_ICONV}
    ${FREETYPE_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${EXPAT_LIBRARY_RELEASE}
    ${EXTERNAL_FMEM}
  )

  if(ARGP_NAME)
    add_dependencies(emf2svg-conv ${ARGP_NAME})
  endif(ARGP_NAME)

  if(GCOV)
    Set(COVERAGE_EXCLUDES '*conv*' '*uemf*' '*upmf*' '*tests*' '*c++*')
    include(CodeCoverage)
    setup_target_for_coverage(NAME coverage
      EXECUTABLE emf2svg-test ./tests/resources/emf*/*
      DEPENDENCIES emf2svg-test
    )
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
  endif(GCOV)

  if(UNITTEST)
    add_executable(emf2svg-test tests/test.c)

    target_link_libraries(emf2svg-test
      emf2svg
    )
  endif(UNITTEST)
endif(NOT LONLY)

if (MSVC)
# x64: suppress mostly harmless warnings about 64 to 32 bit conversion (4244, 4267, 4305).
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std:c++14 -W3 -D_CRT_SECURE_NO_WARNINGS -wd4244 -wd4267 -wd4305")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std:c11 -W3 -D_CRT_SECURE_NO_WARNINGS -wd4244 -wd4267 -wd4305")
else(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall")
endif (MSVC)

if (NOT LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR lib)
endif ()

if (NOT BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR bin)
endif ()

if (NOT INCLUDE_INSTALL_DIR)
    set(INCLUDE_INSTALL_DIR include)
endif ()

list(APPEND BINTARGETS emf2svg)
if(NOT LONLY)
  list(APPEND BINTARGETS emf2svg-conv)
endif(NOT LONLY)

INSTALL(TARGETS ${BINTARGETS}
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

INSTALL(FILES inc/emf2svg.h DESTINATION ${INCLUDE_INSTALL_DIR})
