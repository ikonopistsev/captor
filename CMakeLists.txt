cmake_minimum_required(VERSION 3.0.2)

# менять верисю пакета тут
project(captor VERSION 1.6.0 LANGUAGES CXX)
# версию сборки тут
# дополнительная версия пакета
set(CUSTOM_BUILD_VERSION 1)

include_directories("/usr/include/mysql")
include_directories("./btdef")
include_directories("./btpro")

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

#настройки cpack'a
set(CPACK_PACKAGE_VENDOR "LLC Bridge-Technology")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "mysql plugin is converting result to json objects and then send over tcp.")
set(CPACK_PACKAGE_CONTACT "ikonopistsev@roboforex.com")
set(CPACK_STRIP_FILES TRUE)

# указываем версию пакета (прописывается в пакете)
# рабоает для RPM
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

#указываем генератор
set(CPACK_GENERATOR "RPM")

# условие для DEB и RPM
set(CUSTOM_SUFFIX ".el7")

#для RPM
# необходимо выпилить автодиректории
# указывать надо именно полные пути, хз почему
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST /usr /usr/lib64 /usr/lib64/mysql /usr/lib64/mysql/plugin)

#добавить зависимость от субд
set(CPACK_RPM_PACKAGE_REQUIRES mariadb-server)

# говорим чтобы он проверил зависимости
set(CPACK_RPM_PACKAGE_AUTOREQ ON)

# прописываем релиз пакета (внутри RPM)
set(CPACK_RPM_PACKAGE_RELEASE ${CUSTOM_BUILD_VERSION})

# формируем релиз пакета с указанием суфикса
set(CUSTOM_FULL_BUILD_VERSION ${CUSTOM_BUILD_VERSION}${CUSTOM_SUFFIX})
# прописываем имя выходного файла
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${CUSTOM_FULL_BUILD_VERSION}.${CMAKE_SYSTEM_PROCESSOR}")
# стартуем
include(CPack)

#set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ikonopistsev")
#set(CPACK_GENERATOR "DEB")
#set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
#set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
#set(CPACK_DEBIAN_PACKAGE_SECTION main)

#IF(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
#    FIND_PROGRAM(DPKG_CMD dpkg)
#    IF(NOT DPKG_CMD)
#        MESSAGE(STATUS "Can not find dpkg in your path, default to i386.")
#        SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
#    ENDIF(NOT DPKG_CMD)
#    EXECUTE_PROCESS(COMMAND "${DPKG_CMD}" --print-architecture
#        OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
#        OUTPUT_STRIP_TRAILING_WHITESPACE)
#ENDIF(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)

#set(CPACK_SYSTEM_NAME ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})

#include(CPack)
