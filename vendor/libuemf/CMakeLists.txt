cmake_minimum_required (VERSION 3.0)
project(libUEMF VERSION 0.2.7 LANGUAGES C)
# Note - library version is 0.0.3 for now, even for small bug fixes to
# the project which increment the final version number.


# Default installation is to /usr/local
# Commented out line below installs to current directory
# Pick one or the other or edit to set target.
#SET(CMAKE_INSTALL_PREFIX          /usr/local                  )
SET(CMAKE_INSTALL_PREFIX           ..                          )

SET(FS8 -std=c99 -pedantic -Wall -fPIC -c)
SET(FS9 -Wall -std=c99 -pedantic -O3)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(uemf SHARED
    uemf.c
    uemf_print.c
    uemf_endian.c
    uemf_safe.c
    uemf_utf.c
    uwmf.c
    uwmf_print.c
    uwmf_endian.c
    upmf.c
    upmf_print.c
)

set_target_properties(uemf PROPERTIES VERSION 0.0.3)
set_target_properties(uemf PROPERTIES SOVERSION 0)
target_compile_options(uemf PRIVATE ${FS8})

add_executable(cutemf            cutemf.c            )
add_executable(pmfdual2single    pmfdual2single.c    )
add_executable(reademf           reademf.c           )
add_executable(readwmf           readwmf.c           )
add_executable(testbed_emf       testbed_emf.c       )
add_executable(testbed_pmf       testbed_pmf.c       )
add_executable(testbed_wmf       testbed_wmf.c       )
add_executable(test_mapmodes_emf test_mapmodes_emf.c )
###

target_compile_options(cutemf            PRIVATE ${FS9} )
target_compile_options(pmfdual2single    PRIVATE ${FS9} )
target_compile_options(reademf           PRIVATE ${FS9} )
target_compile_options(readwmf           PRIVATE ${FS9} )
target_compile_options(testbed_emf       PRIVATE ${FS9} )
target_compile_options(testbed_pmf       PRIVATE ${FS9} )
target_compile_options(testbed_wmf       PRIVATE ${FS9} )
target_compile_options(test_mapmodes_emf PRIVATE ${FS9} )
###
target_link_libraries(cutemf            PRIVATE  uemf m )
target_link_libraries(pmfdual2single    PRIVATE  uemf m )
target_link_libraries(reademf           PRIVATE  uemf m )
target_link_libraries(readwmf           PRIVATE  uemf m )
target_link_libraries(testbed_emf       PRIVATE  uemf m )
target_link_libraries(testbed_pmf       PRIVATE  uemf m )
target_link_libraries(testbed_wmf       PRIVATE  uemf m )
target_link_libraries(test_mapmodes_emf PRIVATE  uemf m )

INSTALL(TARGETS uemf 
                cutemf  pmfdual2single reademf readwmf 
                testbed_emf testbed_pmf testbed_wmf test_mapmodes_emf
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

FILE(GLOB hfiles "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
INSTALL(FILES ${hfiles} DESTINATION include)


#####################################################
#
#install man pages
#FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.1")
#INSTALL(FILES ${files} DESTINATION "${CMAKE_INSTALL_PREFIX}/man/man1" )
##install html pages
#FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.html")
#INSTALL(FILES ${files} DESTINATION "${CMAKE_INSTALL_PREFIX}/doc/html" )
