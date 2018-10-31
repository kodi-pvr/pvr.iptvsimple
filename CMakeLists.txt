project(pvr.iptvsimple)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(Rapidxml REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    ${RAPIDXML_INCLUDE_DIRS}
                    ${ZLIB_INCLUDE_DIRS})

set(DEPLIBS ${p8-platform_LIBRARIES}
            ${ZLIB_LIBRARIES})

message(STATUS "ZLIB_LIBRARIES: ${ZLIB_LIBRARIES}")

set(IPTV_SOURCES src/client.cpp
                 src/PVRIptvData.cpp)

set(IPTV_HEADERS src/client.h
                 src/PVRIptvData.h)

addon_version(pvr.iptvsimple IPTV)
add_definitions(-DIPTV_VERSION=${IPTV_VERSION})

build_addon(pvr.iptvsimple IPTV DEPLIBS)

include(CPack)
