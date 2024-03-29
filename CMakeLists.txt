cmake_minimum_required(VERSION 3.5)
project(pvr.iptvsimple)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(pugixml REQUIRED)
find_package(ZLIB REQUIRED)
find_package(lzma REQUIRED)

include_directories(${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways
                    ${PUGIXML_INCLUDE_DIRS}
                    ${ZLIB_INCLUDE_DIRS}
                    ${LZMA_INCLUDE_DIRS})

set(DEPLIBS ${PUGIXML_LIBRARIES}
            ${ZLIB_LIBRARIES}
            ${LZMA_LIBRARIES})

message(STATUS "PUGIXML_LIBRARIES: ${PUGIXML_LIBRARIES}")
message(STATUS "ZLIB_LIBRARIES: ${ZLIB_LIBRARIES}")
message(STATUS "LZMA_LIBRARIES: ${LZMA_LIBRARIES}")

set(IPTV_SOURCES src/addon.cpp
                 src/IptvSimple.cpp
                 src/iptvsimple/AddonSettings.cpp
                 src/iptvsimple/CatchupController.cpp
                 src/iptvsimple/Channels.cpp
                 src/iptvsimple/ChannelGroups.cpp
                 src/iptvsimple/ConnectionManager.cpp
                 src/iptvsimple/Epg.cpp
                 src/iptvsimple/InstanceSettings.cpp
                 src/iptvsimple/Media.cpp
                 src/iptvsimple/PlaylistLoader.cpp
                 src/iptvsimple/Providers.cpp
                 src/iptvsimple/StreamManager.cpp
                 src/iptvsimple/data/Channel.cpp
                 src/iptvsimple/data/ChannelEpg.cpp
                 src/iptvsimple/data/ChannelGroup.cpp
                 src/iptvsimple/data/Provider.cpp
                 src/iptvsimple/data/EpgEntry.cpp
                 src/iptvsimple/data/EpgGenre.cpp
                 src/iptvsimple/data/MediaEntry.cpp
                 src/iptvsimple/utilities/FileUtils.cpp
                 src/iptvsimple/utilities/Logger.cpp
                 src/iptvsimple/utilities/SettingsMigration.cpp
                 src/iptvsimple/utilities/StreamUtils.cpp
                 src/iptvsimple/utilities/WebUtils.cpp)

set(IPTV_HEADERS src/addon.h
                 src/IptvSimple.h
                 src/iptvsimple/AddonSettings.h
                 src/iptvsimple/CatchupController.h
                 src/iptvsimple/Channels.h
                 src/iptvsimple/ChannelGroups.h
                 src/iptvsimple/ConnectionManager.h
                 src/iptvsimple/Epg.h
                 src/iptvsimple/IConnectionListener.h
                 src/iptvsimple/InstanceSettings.h
                 src/iptvsimple/Media.h
                 src/iptvsimple/PlaylistLoader.h
                 src/iptvsimple/Providers.cpp
                 src/iptvsimple/StreamManager.h
                 src/iptvsimple/data/BaseEntry.h
                 src/iptvsimple/data/Channel.h
                 src/iptvsimple/data/ChannelEpg.h
                 src/iptvsimple/data/ChannelGroup.h
                 src/iptvsimple/data/Provider.cpp
                 src/iptvsimple/data/EpgEntry.h
                 src/iptvsimple/data/EpgGenre.h
                 src/iptvsimple/data/MediaEntry.h
                 src/iptvsimple/data/StreamEntry.h
                 src/iptvsimple/utilities/FileUtils.h
                 src/iptvsimple/utilities/Logger.h
                 src/iptvsimple/utilities/SettingsMigration.h
                 src/iptvsimple/utilities/StreamUtils.h
                 src/iptvsimple/utilities/TimeUtils.h
                 src/iptvsimple/utilities/WebUtils.h
                 src/iptvsimple/utilities/XMLUtils.h)

addon_version(pvr.iptvsimple IPTV)
add_definitions(-DIPTV_VERSION=${IPTV_VERSION})

if(WIN32)
  add_definitions(-DLZMA_API_STATIC)
endif()

build_addon(pvr.iptvsimple IPTV DEPLIBS)

include(CPack)
