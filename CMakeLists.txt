project(pvr.iptvsimple)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(Rapidxml REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    ${RAPIDXML_INCLUDE_DIRS}
                    ${ZLIB_INCLUDE_DIRS}
                    ${OPENSSL_INCLUDE_DIR})

set(DEPLIBS ${p8-platform_LIBRARIES}
            ${ZLIB_LIBRARIES} ${OPENSSL_LIBRARIES})

message(STATUS "ZLIB_LIBRARIES: ${ZLIB_LIBRARIES}")
message(STATUS "OpenSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")
message(STATUS "OpenSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")

set(IPTV_SOURCES  src/client.cpp
                  src/PVRIptvData.cpp
                  src/iptvsimple/FileStreamReader.cpp
                  src/iptvsimple/M3U8Streamer.cpp
                  src/iptvsimple/StreamReader.cpp
                  src/iptvsimple/TimeshiftBuffer.cpp
                  src/iptvsimple/hls/HlsDownloader.cpp
                  src/iptvsimple/hls/HlsParser.cpp
                  src/iptvsimple/hls/HlsUpdater.cpp
                  src/iptvsimple/hls/utilities/AesUtils.cpp
                  src/iptvsimple/hls/utilities/WebUtils.cpp
                  src/iptvsimple/hls/mpegts/aes.c
                  src/iptvsimple/hls/mpegts/aes_openssl.c
                  src/iptvsimple/hls/mpegts/mpegts.c
                  src/iptvsimple/hls/mpegts/msg.c)



set(IPTV_HEADERS  src/client.h
                  src/PVRIptvData.h
                  src/iptvsimple/FileStreamReader.h
                  src/iptvsimple/IStreamReader.h
                  src/iptvsimple/M3U8Streamer.h
                  src/iptvsimple/StreamReader.h
                  src/iptvsimple/TimeshiftBuffer.h
                  src/iptvsimple/LocalizedString.h
                  src/iptvsimple/hls/HlsDownloader.h
                  src/iptvsimple/hls/HlsParser.h
                  src/iptvsimple/hls/HlsUpdater.h
                  src/iptvsimple/hls/utilities/AesUtils.h
                  src/iptvsimple/hls/utilities/WebUtils.h
                  src/iptvsimple/hls/data/ByteBuffer.h
                  src/iptvsimple/hls/data/EncAes128.h
                  src/iptvsimple/hls/data/HlsAudio.h
                  src/iptvsimple/hls/data/HlsMasterPlaylist.h
                  src/iptvsimple/hls/data/HlsMediaPlaylist.h
                  src/iptvsimple/hls/data/HlsMediaSegment.h
                  src/iptvsimple/hls/mpegts/aes.h
                  src/iptvsimple/hls/mpegts/misc.h
                  src/iptvsimple/hls/mpegts/mpegts.h
                  src/iptvsimple/hls/mpegts/msg.h
                  src/iptvsimple/hls/queue/atomicops.h
                  src/iptvsimple/hls/queue/readerwriterqueue.h)

addon_version(pvr.iptvsimple IPTV)
add_definitions(-DIPTV_VERSION=${IPTV_VERSION})

build_addon(pvr.iptvsimple IPTV DEPLIBS)

include(CPack)
