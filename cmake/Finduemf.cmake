# FindUEMF.cmake - Find UEMF library
#
# This module defines:
#  UEMF_FOUND - True if UEMF library is found
#  UEMF_INCLUDE_DIRS - UEMF include directories
#  UEMF_LIBRARIES - UEMF libraries to link
#  UEMF_VERSION - Version of UEMF library (if available)

find_path(UEMF_INCLUDE_DIR
  NAMES uemf.h
  PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
)

find_library(UEMF_LIBRARY
  NAMES uemf libuemf
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    /usr/lib/x86_64-linux-gnu
    /usr/lib/aarch64-linux-gnu
    /usr/lib/arm-linux-gnueabihf
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(uemf
  REQUIRED_VARS UEMF_LIBRARY UEMF_INCLUDE_DIR
)

if(UEMF_FOUND)
  set(UEMF_LIBRARIES ${UEMF_LIBRARY})
  set(UEMF_INCLUDE_DIRS ${UEMF_INCLUDE_DIR})
  mark_as_advanced(UEMF_INCLUDE_DIR UEMF_LIBRARY)
  
  if(NOT TARGET uemf::uemf)
    add_library(uemf::uemf UNKNOWN IMPORTED)
    set_target_properties(uemf::uemf PROPERTIES
      IMPORTED_LOCATION "${UEMF_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${UEMF_INCLUDE_DIR}"
    )
  endif()
endif()

