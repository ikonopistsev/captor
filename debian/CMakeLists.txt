cmake_minimum_required(VERSION 3.0.2)

file(READ "../version.txt" FILE_VER)
file(READ "../revision.txt" FILE_REV)

# менять верисю пакета тут
project(captor VERSION ${FILE_VER} LANGUAGES CXX)

include_directories("/usr/include/mysql")
include_directories("../")

macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

use_cxx11()

set(sources
    ../src/refbuf.cpp
    ../src/netcat.cpp
)

add_library(captor SHARED ${sources})

add_definitions("-DHAVE_DLOPEN")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

target_link_libraries(captor event)

install(TARGETS captor LIBRARY DESTINATION lib/mysql/plugin)

#настройки cpack'a
set(CPACK_PACKAGE_VENDOR "LLC Bridge-Technology")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "mysql plugin is converting result to json objects and then send over tcp.")
set(CPACK_PACKAGE_CONTACT "ikonopistsev@jsuff.com")
set(CPACK_STRIP_FILES TRUE)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ikonopistsev")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SECTION main)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_DEBIAN_PACKAGE_RELEASE ${FILE_REV})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

IF(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    FIND_PROGRAM(DPKG_CMD dpkg)
    IF(NOT DPKG_CMD)
        MESSAGE(STATUS "Can not find dpkg in your path, default to i386.")
        SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
    ENDIF(NOT DPKG_CMD)
    EXECUTE_PROCESS(COMMAND "${DPKG_CMD}" --print-architecture
        OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
        OUTPUT_STRIP_TRAILING_WHITESPACE)
ENDIF(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)

set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})

include(CPack)
