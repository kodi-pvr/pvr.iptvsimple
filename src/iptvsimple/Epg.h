/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "Channels.h"
#include "Media.h"
#include "InstanceSettings.h"
#include "data/ChannelEpg.h"
#include "data/EpgEntry.h"
#include "data/EpgGenre.h"

#include <memory>
#include <string>
#include <vector>

#include <kodi/addon-instance/PVR.h>

namespace iptvsimple
{
  static const int SECONDS_IN_DAY = 86400;
  static const std::string GENRES_MAP_FILENAME = "genres.xml";
  static const std::string GENRE_DIR = "/genres";
  static const std::string GENRE_ADDON_DATA_BASE_DIR = ADDON_DATA_BASE_DIR + GENRE_DIR;
  static const int DEFAULT_EPG_MAX_DAYS = 3;

  enum class XmltvFileFormat
  {
    NORMAL,
    TAR_ARCHIVE,
    INVALID
  };

  class InstanceSettings;

  class Epg
  {
  public:
    Epg(kodi::addon::CInstancePVRClient* client, iptvsimple::Channels& channels, iptvsimple::Media& media, std::shared_ptr<iptvsimple::InstanceSettings>& settings);

    bool Init(int epgMaxPastDays, int epgMaxFutureDays);

    PVR_ERROR GetEPGForChannel(int channelUid, time_t epgWindowStart, time_t epgWindowEnd, kodi::addon::PVREPGTagsResultSet& results);
    void SetEPGMaxPastDays(int epgMaxPastDays);
    void SetEPGMaxFutureDays(int epgMaxFutureDays);
    void Clear();
    void ReloadEPG();

    data::EpgEntry* GetLiveEPGEntry(const data::Channel& myChannel) const;
    data::EpgEntry* GetEPGEntry(const data::Channel& myChannel, time_t lookupTime) const;
    int GetEPGTimezoneShiftSecs(const data::Channel& myChannel) const;

  private:
    static const XmltvFileFormat GetXMLTVFileFormat(const char* buffer);
    static void MoveOldGenresXMLFileToNewLocation();

    bool LoadEPG(time_t iStart, time_t iEnd);
    bool GetXMLTVFileWithRetries(std::string& data);
    char* FillBufferFromXMLTVData(std::string& data, std::string& decompressedData);
    bool LoadChannelEpgs(const pugi::xml_node& rootElement);
    void LoadEpgEntries(const pugi::xml_node& rootElement, int epgWindowStart, int epgWindowEnd);
    bool LoadGenres();

    void MergeEpgDataIntoMedia();

    data::ChannelEpg* FindEpgForChannel(const std::string& id) const;
    data::ChannelEpg* FindEpgForChannel(const data::Channel& channel) const;
    data::ChannelEpg* FindEpgForMediaEntry(const data::MediaEntry& mediaEntry) const;
    void ApplyChannelsLogosFromEPG();

    std::string m_xmltvLocation;
    int m_epgTimeShift;
    bool m_tsOverride;
    int m_lastStart;
    int m_lastEnd;
    int m_epgMaxPastDays;
    int m_epgMaxFutureDays;
    long m_epgMaxPastDaysSeconds;
    long m_epgMaxFutureDaysSeconds;

    iptvsimple::Channels& m_channels;
    iptvsimple::Media& m_media;
    std::vector<data::ChannelEpg> m_channelEpgs;
    std::vector<iptvsimple::data::EpgGenre> m_genreMappings;

    kodi::addon::CInstancePVRClient* m_client;

    std::shared_ptr<iptvsimple::InstanceSettings> m_settings;
  };
} //namespace iptvsimple
