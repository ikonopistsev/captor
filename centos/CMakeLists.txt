cmake_minimum_required(VERSION 3.0.2)

set(RELATIVE_SOURCE_DIR "../")

include(../CMakeLists.txt)

# версию сборки тут
# дополнительная версия пакета
set(CUSTOM_BUILD_VERSION ${FILE_REV})

install(TARGETS captor LIBRARY DESTINATION lib64/mysql/plugin)

#настройки cpack'a
set(CPACK_PACKAGE_VENDOR "LLC Bridge-Technology")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "mysql plugin send data over tcp.")
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
