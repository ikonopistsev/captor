cmake_minimum_required(VERSION 3.10)

file(READ "${RELATIVE_SOURCE_DIR}version.txt" FILE_VER)
file(READ "${RELATIVE_SOURCE_DIR}revision.txt" FILE_REV)

# менять верисю пакета тут
project(captor VERSION ${FILE_VER} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(libevent)

option(CAPTOR_HAVE_MY_BOOL "mysql has my_bool type" OFF)
if (CAPTOR_HAVE_MY_BOOL)
  add_definitions(-DHAVE_TYPE_MY_BOOL)
endif()

set(sources
    ${RELATIVE_SOURCE_DIR}src/refbuf.cpp
    ${RELATIVE_SOURCE_DIR}src/netcat.cpp
    ${RELATIVE_SOURCE_DIR}src/journal.cpp
)

include_directories("/usr/include/mysql")
include_directories(${RELATIVE_SOURCE_DIR})
include_directories("./")

add_definitions("-DHAVE_DLOPEN")

add_library(captor SHARED ${sources})

option(CAPTOR_STATIC_STDCPP "static linking of libstdc++" ON)
if (STATIC_STDCPP)
    target_link_libraries(captor LINK_PRIVATE -static-libgcc -static-libstdc++ -l:libstdc++.a event)
else()
    target_link_libraries(captor event)
endif()

