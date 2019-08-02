cmake_minimum_required(VERSION 3.0.2)

file(READ "version.txt" FILE_VER)
file(READ "revision.txt" FILE_REV)

# менять верисю пакета тут
project(captor VERSION ${FILE_VER} LANGUAGES CXX)
# версию сборки тут
# дополнительная версия пакета
set(CUSTOM_BUILD_VERSION ${FILE_REV})

include_directories("/usr/include/mysql")
include_directories("./")

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
    src/refbuf.cpp
    src/netcat.cpp
    src/journal.cpp
)

add_library(captor SHARED ${sources})

if (WIN32)
    target_link_libraries(captor event ws2_32)
else()
    add_definitions("-DHAVE_DLOPEN")

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

    target_link_libraries(captor event)
endif()

install(TARGETS captor LIBRARY DESTINATION lib64/mysql/plugin)
