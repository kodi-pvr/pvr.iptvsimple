cmake_minimum_required(VERSION 3.5)
project(xz-utils)

include(ExternalProject)
externalproject_add(xz-utils
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${OUTPUT_DIR}
                      --enable-static
                      --disable-shared
                      --disable-doc
                      --with-pic
                      ${EXTRA_CFLAGS}
                    INSTALL_COMMAND ""
                    BUILD_IN_SOURCE 1)

install(CODE "execute_process(COMMAND make install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
