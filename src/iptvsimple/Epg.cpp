/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Epg.h"

#include "utilities/FileUtils.h"
#include "utilities/Logger.h"
#include "utilities/XMLUtils.h"

#include <chrono>
#include <regex>
#include <thread>

#include <kodi/tools/StringUtils.h>
#include <pugixml.hpp>

using namespace kodi::tools;
using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;
using namespace pugi;

Epg::Epg(kodi::addon::CInstancePVRClient* client, Channels& channels, Media& media, std::shared_ptr<InstanceSettings>& settings)
  : m_lastStart(0), m_lastEnd(0), m_channels(channels), m_media(media), m_client(client), m_settings(settings)
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + GENRE_DIR, GENRE_ADDON_DATA_BASE_DIR, true);

  if (!FileUtils::FileExists(DEFAULT_GENRE_TEXT_MAP_FILE))
  {
    MoveOldGenresXMLFileToNewLocation();
  }

  m_media.SetGenreMappings(m_genreMappings);
}

bool Epg::Init(int epgMaxPastDays, int epgMaxFutureDays)
{
  m_xmltvLocation = m_settings->GetEpgLocation();
  m_epgTimeShift = m_settings->GetEpgTimeshiftSecs();
  m_tsOverride = m_settings->GetTsOverride();

  SetEPGMaxPastDays(epgMaxPastDays);
  SetEPGMaxFutureDays(epgMaxFutureDays);

  if (m_settings->IsCatchupEnabled() || m_settings->IsMediaEnabled())
  {
    // Kodi may not load the data on each startup so we need to make sure it's loaded whether
    // or not kodi considers it necessary when either 1) we need the EPG logos or 2) for
    // catchup we need a local store of the EPG data
    time_t now = std::time(nullptr);
    LoadEPG(now - m_epgMaxPastDaysSeconds, now + m_epgMaxFutureDaysSeconds);
    MergeEpgDataIntoMedia();
  }

  return true;
}

void Epg::Clear()
{
  m_channelEpgs.clear();
  m_genreMappings.clear();
}

void Epg::SetEPGMaxPastDays(int epgMaxPastDays)
{
  m_epgMaxPastDays = epgMaxPastDays;

  if (m_epgMaxPastDays > EPG_TIMEFRAME_UNLIMITED)
    m_epgMaxPastDaysSeconds = m_epgMaxPastDays * 24 * 60 * 60;
  else
    m_epgMaxPastDaysSeconds = DEFAULT_EPG_MAX_DAYS * 24 * 60 * 60;
}

void Epg::SetEPGMaxFutureDays(int epgMaxFutureDays)
{
  m_epgMaxFutureDays = epgMaxFutureDays;

  if (m_epgMaxFutureDays > EPG_TIMEFRAME_UNLIMITED)
    m_epgMaxFutureDaysSeconds = m_epgMaxFutureDays * 24 * 60 * 60;
  else
    m_epgMaxFutureDaysSeconds = DEFAULT_EPG_MAX_DAYS * 24 * 60 * 60;
}

bool Epg::LoadEPG(time_t epgWindowStart, time_t epgWindowEnd)
{
  auto started = std::chrono::high_resolution_clock::now();
  Logger::Log(LEVEL_DEBUG, "%s - EPG Load Start", __FUNCTION__);

  if (m_xmltvLocation.empty())
  {
    Logger::Log(LEVEL_INFO, "%s - EPG file path is not configured. EPG not loaded.", __FUNCTION__);
    return false;
  }

  std::string data;

  if (GetXMLTVFileWithRetries(data))
  {
    std::string decompressedData;
    char* buffer = FillBufferFromXMLTVData(data, decompressedData);

    if (!buffer)
      return false;

    xml_document xmlDoc;
    xml_parse_result result = xmlDoc.load_string(buffer);

    if (!result)
    {
      std::string errorString;
      int offset = GetParseErrorString(buffer, result.offset, errorString);
      Logger::Log(LEVEL_ERROR, "%s - Unable parse EPG XML: %s, offset: %d: \n[ %s \n]", __FUNCTION__, result.description(), offset, errorString.c_str());
      return false;
    }

    const auto& rootElement = xmlDoc.child("tv");
    if (!rootElement)
    {
      Logger::Log(LEVEL_ERROR, "%s - Invalid EPG XML: no <tv> tag found", __FUNCTION__);
      return false;
    }

    if (!LoadChannelEpgs(rootElement))
      return false;

    LoadEpgEntries(rootElement, epgWindowStart, epgWindowEnd);

    xmlDoc.reset();
  }
  else
  {
    return false;
  }

  LoadGenres();

  if (m_settings->GetEpgLogosMode() != EpgLogosMode::IGNORE_XMLTV)
    ApplyChannelsLogosFromEPG();

  int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::high_resolution_clock::now() - started).count();

  Logger::Log(LEVEL_INFO, "%s - EPG Loaded - %d (ms)", __FUNCTION__, milliseconds);

  return true;
}

bool Epg::GetXMLTVFileWithRetries(std::string& data)
{
  int bytesRead = 0;
  int count = 0;

  // Cache is only allowed if refresh mode is disabled
  bool useEPGCache = m_settings->GetM3URefreshMode() != RefreshMode::DISABLED ? false : m_settings->UseEPGCache();

  while (count < 3) // max 3 tries
  {
    if ((bytesRead = FileUtils::GetCachedFileContents(m_settings, m_settings->GetXMLTVCacheFilename(), m_xmltvLocation, data, useEPGCache)) != 0)
      break;

    Logger::Log(LEVEL_ERROR, "%s - Unable to load EPG file '%s':  file is missing or empty. :%dth try.", __FUNCTION__, m_xmltvLocation.c_str(), ++count);

    if (count < 3)
      std::this_thread::sleep_for(std::chrono::microseconds(2 * 1000 * 1000)); // sleep 2 sec before next try.
  }

  if (bytesRead == 0)
  {
    Logger::Log(LEVEL_ERROR, "%s - Unable to load EPG file '%s':  file is missing or empty. After %d tries.", __FUNCTION__, m_xmltvLocation.c_str(), count);
    return false;
  }

  return true;
}

char* Epg::FillBufferFromXMLTVData(std::string& data, std::string& decompressedData)
{
  char* buffer = nullptr;

  // gzip packed
  if (data[0] == '\x1F' && data[1] == '\x8B' && data[2] == '\x08')
  {
    if (!FileUtils::GzipInflate(data, decompressedData))
    {
      Logger::Log(LEVEL_ERROR, "%s - Invalid EPG file '%s': unable to decompress gzip file.", __FUNCTION__, m_xmltvLocation.c_str());
      return nullptr;
    }
    buffer = &(decompressedData[0]);
  }
  // xz packed
  else if (data[0] == '\xFD' && data[1] == '7' && data[2] == 'z' &&
           data[3] == 'X' && data[4] == 'Z' && data[5] == '\x00')
  {
    if (!FileUtils::XzDecompress(data, decompressedData))
    {
      Logger::Log(LEVEL_ERROR, "%s - Invalid EPG file '%s': unable to decompress xz/7z file.", __FUNCTION__, m_xmltvLocation.c_str());
      return nullptr;
    }
    buffer = &(decompressedData[0]);
  }
  else
  {
    buffer = &(data[0]);
  }

  XmltvFileFormat fileFormat = GetXMLTVFileFormat(buffer);

  if (fileFormat == XmltvFileFormat::INVALID)
  {
    Logger::Log(LEVEL_ERROR, "%s - Invalid EPG file '%s': unable to parse file.", __FUNCTION__, m_xmltvLocation.c_str());
    return nullptr;
  }

  if (fileFormat == XmltvFileFormat::TAR_ARCHIVE)
    buffer += 0x200; // RECORDSIZE = 512

  return buffer;
}

const XmltvFileFormat Epg::GetXMLTVFileFormat(const char* buffer)
{
  if (!buffer)
    return XmltvFileFormat::INVALID;

  // xml should starts with '<?xml'
  if (buffer[0] != '\x3C' || buffer[1] != '\x3F' || buffer[2] != '\x78' ||
      buffer[3] != '\x6D' || buffer[4] != '\x6C')
  {
    // check for BOM
    if (buffer[0] != '\xEF' || buffer[1] != '\xBB' || buffer[2] != '\xBF')
    {
      // check for tar archive
      if (strcmp(buffer + 0x101, "ustar") || strcmp(buffer + 0x101, "GNUtar"))
        return XmltvFileFormat::TAR_ARCHIVE;
      else
        return XmltvFileFormat::INVALID;
    }
  }

  return XmltvFileFormat::NORMAL;
}

bool Epg::LoadChannelEpgs(const xml_node& rootElement)
{
  if (!rootElement)
    return false;

  m_channelEpgs.clear();

  for (const auto& channelNode : rootElement.children("channel"))
  {
    ChannelEpg channelEpg;

    if (channelEpg.UpdateFrom(channelNode, m_channels, m_media))
    {
      ChannelEpg* existingChannelEpg = FindEpgForChannel(channelEpg.GetId());
      if (existingChannelEpg)
      {
        if (existingChannelEpg->CombineNamesAndIconPathFrom(channelEpg))
          Logger::Log(LEVEL_DEBUG, "%s - Combined channel EPG with id '%s' now has display names: '%s'", __FUNCTION__, channelEpg.GetId().c_str(), channelEpg.GetJoinedDisplayNames().c_str());

        continue;
      }

      Logger::Log(LEVEL_DEBUG, "%s - Loaded channel EPG with id '%s' with display names: '%s'", __FUNCTION__, channelEpg.GetId().c_str(), channelEpg.GetJoinedDisplayNames().c_str());

      m_channelEpgs.emplace_back(channelEpg);
    }
  }

  if (m_channelEpgs.size() == 0)
  {
    Logger::Log(LEVEL_ERROR, "%s - EPG channels not found.", __FUNCTION__);
    return false;
  }
  else
  {
    Logger::Log(LEVEL_INFO, "%s - Loaded '%d' EPG channels.", __FUNCTION__, m_channelEpgs.size());
  }

  return true;
}

void Epg::LoadEpgEntries(const xml_node& rootElement, int epgWindowStart, int epgWindowEnd)
{
  int minShiftTime = m_epgTimeShift;
  int maxShiftTime = m_epgTimeShift;
  if (!m_tsOverride)
  {
    minShiftTime = SECONDS_IN_DAY;
    maxShiftTime = -SECONDS_IN_DAY;

    for (const auto& channel : m_channels.GetChannelsList())
    {
      if (channel.GetTvgShift() + m_epgTimeShift < minShiftTime)
        minShiftTime = channel.GetTvgShift() + m_epgTimeShift;
      if (channel.GetTvgShift() + m_epgTimeShift > maxShiftTime)
        maxShiftTime = channel.GetTvgShift() + m_epgTimeShift;
    }

    for (const auto& mediaEntry : m_media.GetMediaEntryList())
    {
      if (mediaEntry.GetTvgShift() + m_epgTimeShift < minShiftTime)
        minShiftTime = mediaEntry.GetTvgShift() + m_epgTimeShift;
      if (mediaEntry.GetTvgShift() + m_epgTimeShift > maxShiftTime)
        maxShiftTime = mediaEntry.GetTvgShift() + m_epgTimeShift;
    }
  }

  ChannelEpg* channelEpg = nullptr;
  int count = 0;

  for (const auto& programmeNode : rootElement.children("programme"))
  {
    std::string id;
    if (!GetAttributeValue(programmeNode, "channel", id))
      continue;

    if (!channelEpg || !StringUtils::EqualsNoCase(channelEpg->GetId(), id))
    {
      if (!(channelEpg = FindEpgForChannel(id)))
        continue;
    }

    EpgEntry entry{m_settings};
    if (entry.UpdateFrom(programmeNode, id, epgWindowStart, epgWindowEnd, minShiftTime, maxShiftTime))
    {
      count++;

      channelEpg->AddEpgEntry(entry);
    }
  }

  Logger::Log(LEVEL_INFO, "%s - Loaded '%d' EPG entries.", __FUNCTION__, count);
}


void Epg::ReloadEPG()
{
  m_xmltvLocation = m_settings->GetEpgLocation();
  m_epgTimeShift = m_settings->GetEpgTimeshiftSecs();
  m_tsOverride = m_settings->GetTsOverride();
  m_lastStart = 0;
  m_lastEnd = 0;

  Clear();

  if (LoadEPG(m_lastStart, m_lastEnd))
  {
    MergeEpgDataIntoMedia();

    for (const auto& myChannel : m_channels.GetChannelsList())
      m_client->TriggerEpgUpdate(myChannel.GetUniqueId());

    m_client->TriggerRecordingUpdate();
  }
}

PVR_ERROR Epg::GetEPGForChannel(int channelUid, time_t epgWindowStart, time_t epgWindowEnd, kodi::addon::PVREPGTagsResultSet& results)
{
  for (const auto& myChannel : m_channels.GetChannelsList())
  {
    if (myChannel.GetUniqueId() != channelUid)
      continue;

    if (epgWindowStart > m_lastStart || epgWindowEnd > m_lastEnd)
    {
      // reload EPG for new time interval only
      LoadEPG(epgWindowStart, epgWindowEnd);
      {
        MergeEpgDataIntoMedia();

        // doesn't matter is epg loaded or not we shouldn't try to load it for same interval
        m_lastStart = static_cast<int>(epgWindowStart);
        m_lastEnd = static_cast<int>(epgWindowEnd);
      }
    }

    ChannelEpg* channelEpg = FindEpgForChannel(myChannel);
    if (!channelEpg || channelEpg->GetEpgEntries().size() == 0)
      return PVR_ERROR_NO_ERROR;

    int shift = GetEPGTimezoneShiftSecs(myChannel);

    for (auto& epgEntryPair : channelEpg->GetEpgEntries())
    {
      auto& epgEntry = epgEntryPair.second;
      if ((epgEntry.GetEndTime() + shift) < epgWindowStart)
        continue;

      kodi::addon::PVREPGTag tag;

      epgEntry.UpdateTo(tag, channelUid, shift, m_genreMappings);

      results.Add(tag);

      if ((epgEntry.GetStartTime() + shift) > epgWindowEnd)
        break;
    }

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

namespace
{
  bool TvgIdMatchesCaseOrNoCase(const std::string& idOne, const std::string& idTwo, bool ignoreCaseForEpgChannelIds)
  {
    if (ignoreCaseForEpgChannelIds)
      return StringUtils::EqualsNoCase(idOne, idTwo);
    else
      return idOne == idTwo;
  }
}

ChannelEpg* Epg::FindEpgForChannel(const std::string& id) const
{
  for (auto& myChannelEpg : m_channelEpgs)
  {
    if (TvgIdMatchesCaseOrNoCase(myChannelEpg.GetId(), id, m_settings->IgnoreCaseForEpgChannelIds()))
      return const_cast<ChannelEpg*>(&myChannelEpg);
  }

  return nullptr;
}

ChannelEpg* Epg::FindEpgForChannel(const Channel& channel) const
{
  for (auto& myChannelEpg : m_channelEpgs)
  {
    if (TvgIdMatchesCaseOrNoCase(myChannelEpg.GetId(), channel.GetTvgId(), m_settings->IgnoreCaseForEpgChannelIds()))
      return const_cast<ChannelEpg*>(&myChannelEpg);
  }

  for (auto& myChannelEpg : m_channelEpgs)
  {
    for (const DisplayNamePair& displayNamePair : myChannelEpg.GetDisplayNames())
    {
      if (StringUtils::EqualsNoCase(displayNamePair.m_displayNameWithUnderscores, channel.GetTvgName()) ||
          StringUtils::EqualsNoCase(displayNamePair.m_displayName, channel.GetTvgName()))
        return const_cast<ChannelEpg*>(&myChannelEpg);
    }
  }

  for (auto& myChannelEpg : m_channelEpgs)
  {
    for (const DisplayNamePair& displayNamePair : myChannelEpg.GetDisplayNames())
    {
      if (StringUtils::EqualsNoCase(displayNamePair.m_displayName, channel.GetChannelName()))
        return const_cast<ChannelEpg*>(&myChannelEpg);
    }
  }

  return nullptr;
}

ChannelEpg* Epg::FindEpgForMediaEntry(const MediaEntry& mediaEntry) const
{
  for (auto& myChannelEpg : m_channelEpgs)
  {
    if (TvgIdMatchesCaseOrNoCase(myChannelEpg.GetId(), mediaEntry.GetTvgId(), m_settings->IgnoreCaseForEpgChannelIds()))
      return const_cast<ChannelEpg*>(&myChannelEpg);
  }

  for (auto& myChannelEpg : m_channelEpgs)
  {
    for (const DisplayNamePair& displayNamePair : myChannelEpg.GetDisplayNames())
    {
      if (StringUtils::EqualsNoCase(displayNamePair.m_displayNameWithUnderscores, mediaEntry.GetTvgName()) ||
          StringUtils::EqualsNoCase(displayNamePair.m_displayName, mediaEntry.GetTvgName()))
        return const_cast<ChannelEpg*>(&myChannelEpg);
    }
  }

  // Note that prior to merging EPG data a media entries title will be the same a a channels name.
  for (auto& myChannelEpg : m_channelEpgs)
  {
    for (const DisplayNamePair& displayNamePair : myChannelEpg.GetDisplayNames())
    {
      if (StringUtils::EqualsNoCase(displayNamePair.m_displayName, mediaEntry.GetM3UName()))
        return const_cast<ChannelEpg*>(&myChannelEpg);
    }
  }

  return nullptr;
}

void Epg::ApplyChannelsLogosFromEPG()
{
  bool updated = false;

  for (const auto& channel : m_channels.GetChannelsList())
  {
    const ChannelEpg* channelEpg = FindEpgForChannel(channel);
    if (!channelEpg || channelEpg->GetIconPath().empty())
      continue;

    // 1 - prefer icon from playlist
    if (!channel.GetIconPath().empty() && m_settings->GetEpgLogosMode() == EpgLogosMode::PREFER_M3U)
      continue;

    // 2 - prefer icon from epg
    if (!channelEpg->GetIconPath().empty() && m_settings->GetEpgLogosMode() == EpgLogosMode::PREFER_XMLTV)
    {
      m_channels.GetChannel(channel.GetUniqueId())->SetIconPath(channelEpg->GetIconPath());
      updated = true;
    }
  }

  if (updated)
    m_client->TriggerChannelUpdate();
}

bool Epg::LoadGenres()
{
  if (!FileUtils::FileExists(m_settings->GetGenresLocation()))
    return false;

  std::string data;
  FileUtils::GetFileContents(m_settings->GetGenresLocation(), data);

  if (data.empty())
    return false;

  m_genreMappings.clear();

  char* buffer = &(data[0]);
  xml_document xmlDoc;
  xml_parse_result result = xmlDoc.load_string(buffer);

  if (!result)
  {
    std::string errorString;
    int offset = GetParseErrorString(buffer, result.offset, errorString);
    Logger::Log(LEVEL_ERROR, "%s - Unable parse EPG XML: %s, offset: %d: \n[ %s \n]", __FUNCTION__, result.description(), offset, errorString.c_str());
    return false;
  }

  const auto& rootElement = xmlDoc.child("genres");
  if (!rootElement)
    return false;

  for (const auto& genreNode : rootElement.children("genre"))
  {
    EpgGenre genreMapping;

    if (genreMapping.UpdateFrom(genreNode))
      m_genreMappings.emplace_back(genreMapping);
  }

  xmlDoc.reset();

  if (!m_genreMappings.empty())
    Logger::Log(LEVEL_INFO, "%s - Loaded %d genres", __FUNCTION__, m_genreMappings.size());

  return true;
}

void Epg::MoveOldGenresXMLFileToNewLocation()
{
  //If we don't have a genres.xml file yet copy it if it exists in any of the other old locations.
  //If not copy a placeholder file that allows the settings dialog to function.
  if (FileUtils::FileExists(ADDON_DATA_BASE_DIR + "/" + GENRES_MAP_FILENAME))
    FileUtils::CopyFile(ADDON_DATA_BASE_DIR + "/" + GENRES_MAP_FILENAME, DEFAULT_GENRE_TEXT_MAP_FILE);
  else if (FileUtils::FileExists(FileUtils::GetSystemAddonPath() + "/" + GENRES_MAP_FILENAME))
    FileUtils::CopyFile(FileUtils::GetSystemAddonPath() + "/" + GENRES_MAP_FILENAME, DEFAULT_GENRE_TEXT_MAP_FILE);
  else
    FileUtils::CopyFile(FileUtils::GetResourceDataPath() + "/" + GENRES_MAP_FILENAME, DEFAULT_GENRE_TEXT_MAP_FILE);

  FileUtils::DeleteFile(ADDON_DATA_BASE_DIR + "/" + GENRES_MAP_FILENAME.c_str());
  FileUtils::DeleteFile(FileUtils::GetSystemAddonPath() + "/" + GENRES_MAP_FILENAME.c_str());
}

EpgEntry* Epg::GetLiveEPGEntry(const Channel& myChannel) const
{
  return GetEPGEntry(myChannel, time(nullptr));
}

EpgEntry* Epg::GetEPGEntry(const Channel& myChannel, time_t lookupTime) const
{
  ChannelEpg* channelEpg = FindEpgForChannel(myChannel);
  if (!channelEpg || channelEpg->GetEpgEntries().size() == 0)
    return nullptr;

  int shift = GetEPGTimezoneShiftSecs(myChannel);

  for (auto& epgEntryPair : channelEpg->GetEpgEntries())
  {
    auto& epgEntry = epgEntryPair.second;
    time_t startTime = epgEntry.GetStartTime() + shift;
    time_t endTime = epgEntry.GetEndTime() + shift;
    if (startTime <= lookupTime && endTime > lookupTime)
      return &epgEntry;
    else if (startTime > lookupTime)
      break;
  }

  return nullptr;
}

int Epg::GetEPGTimezoneShiftSecs(const Channel& myChannel) const
{
  return m_tsOverride ? m_epgTimeShift : myChannel.GetTvgShift() + m_epgTimeShift;
}

void Epg::MergeEpgDataIntoMedia()
{
  for (auto& mediaEntry : m_media.GetMediaEntryList())
  {
    ChannelEpg* channelEpg = FindEpgForMediaEntry(mediaEntry);

    // If we have a channel EPG with entries for this media entry
    // then return the first entry as matching. This is a common pattern
    // for channel that only contain a single media item.
    if (channelEpg && !channelEpg->GetEpgEntries().empty())
      mediaEntry.UpdateFrom(channelEpg->GetEpgEntries().begin()->second, m_genreMappings);
  }
}