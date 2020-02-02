/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Epg.h"

#include "Settings.h"
#include "../client.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"
#include "utilities/XMLUtils.h"

#include <chrono>
#include <regex>
#include <thread>

#include <p8-platform/util/StringUtils.h>
#include <pugixml.hpp>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;
using namespace pugi;

Epg::Epg(Channels& channels)
  : m_channels(channels), m_xmltvLocation(Settings::GetInstance().GetEpgLocation()), m_epgTimeShift(Settings::GetInstance().GetEpgTimeshiftSecs()),
    m_tsOverride(Settings::GetInstance().GetTsOverride()), m_lastStart(0), m_lastEnd(0)
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + GENRE_DIR, GENRE_ADDON_DATA_BASE_DIR, true);

  if (!FileUtils::FileExists(DEFAULT_GENRE_TEXT_MAP_FILE))
  {
    MoveOldGenresXMLFileToNewLocation();
  }
}

void Epg::Clear()
{
  m_channelEpgs.clear();
  m_genreMappings.clear();
}

bool Epg::LoadEPG(time_t start, time_t end)
{
  auto started = std::chrono::high_resolution_clock::now();
  Logger::Log(LEVEL_DEBUG, "%s - EPG Load Start", __FUNCTION__);

  if (m_xmltvLocation.empty())
  {
    Logger::Log(LEVEL_NOTICE, "%s - EPG file path is not configured. EPG not loaded.", __FUNCTION__);
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

    LoadEpgEntries(rootElement, start, end);

    xmlDoc.reset();
  }
  else
  {
    return false;
  }

  LoadGenres();

  if (Settings::GetInstance().GetEpgLogosMode() != EpgLogosMode::IGNORE_XMLTV)
    ApplyChannelsLogosFromEPG();

  int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::high_resolution_clock::now() - started).count();

  Logger::Log(LEVEL_NOTICE, "%s - EPG Loaded - %d (ms)", __FUNCTION__, milliseconds);

  return true;
}

bool Epg::GetXMLTVFileWithRetries(std::string& data)
{
  int bytesRead = 0;
  int count = 0;

  // Cache is only allowed if refresh mode is disabled
  bool useEPGCache = Settings::GetInstance().GetM3URefreshMode() != RefreshMode::DISABLED ? false : Settings::GetInstance().UseEPGCache();

  while (count < 3) // max 3 tries
  {
    if ((bytesRead = FileUtils::GetCachedFileContents(XMLTV_CACHE_FILENAME, m_xmltvLocation, data, useEPGCache)) != 0)
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
      Logger::Log(LEVEL_ERROR, "%s - Invalid EPG file '%s': unable to decompress file.", __FUNCTION__, m_xmltvLocation.c_str());
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

    if (channelEpg.UpdateFrom(channelNode, m_channels))
    {
      ChannelEpg* existingChannelEpg = FindEpgForChannel(channelEpg.GetId());
      if (existingChannelEpg)
      {
        if (existingChannelEpg->CombineNamesAndIconPathFrom(channelEpg))
          Logger::Log(LEVEL_DEBUG, "%s - Combined channel EPG with id '%s' now has display names: '%s'", __FUNCTION__, channelEpg.GetId().c_str(), StringUtils::Join(channelEpg.GetNames(), EPG_STRING_TOKEN_SEPARATOR).c_str());

        continue;
      }

      Logger::Log(LEVEL_DEBUG, "%s - Loaded channel EPG with id '%s' with display names: '%s'", __FUNCTION__, channelEpg.GetId().c_str(), StringUtils::Join(channelEpg.GetNames(), EPG_STRING_TOKEN_SEPARATOR).c_str());

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
    Logger::Log(LEVEL_NOTICE, "%s - Loaded '%d' EPG channels.", __FUNCTION__, m_channelEpgs.size());
  }

  return true;
}

void Epg::LoadEpgEntries(const xml_node& rootElement, int start, int end)
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
  }

  ChannelEpg* channelEpg = nullptr;
  int broadcastId = 0;

  for (const auto& channelNode : rootElement.children("programme"))
  {
    std::string id;
    if (!GetAttributeValue(channelNode, "channel", id))
      continue;

    if (!channelEpg || !StringUtils::EqualsNoCase(channelEpg->GetId(), id))
    {
      if (!(channelEpg = FindEpgForChannel(id)))
        continue;
    }

    EpgEntry entry;
    if (entry.UpdateFrom(channelNode, id, broadcastId + 1, start, end, minShiftTime, maxShiftTime))
    {
      broadcastId++;

      channelEpg->AddEpgEntry(entry);
    }
  }

  Logger::Log(LEVEL_NOTICE, "%s - Loaded '%d' EPG entries.", __FUNCTION__, broadcastId);
}


void Epg::ReloadEPG()
{
  m_xmltvLocation = Settings::GetInstance().GetEpgLocation();
  m_epgTimeShift = Settings::GetInstance().GetEpgTimeshiftSecs();
  m_tsOverride = Settings::GetInstance().GetTsOverride();
  m_lastStart = 0;
  m_lastEnd = 0;

  Clear();

  if (LoadEPG(m_lastStart, m_lastEnd))
  {
    for (const auto& myChannel : m_channels.GetChannelsList())
      PVR->TriggerEpgUpdate(myChannel.GetUniqueId());
  }
}

PVR_ERROR Epg::GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t start, time_t end)
{
  for (const auto& myChannel : m_channels.GetChannelsList())
  {
    if (myChannel.GetUniqueId() != iChannelUid)
      continue;

    if (start > m_lastStart || end > m_lastEnd)
    {
      // reload EPG for new time interval only
      LoadEPG(start, end);
      {
        // doesn't matter is epg loaded or not we shouldn't try to load it for same interval
        m_lastStart = static_cast<int>(start);
        m_lastEnd = static_cast<int>(end);
      }
    }

    ChannelEpg* channelEpg = FindEpgForChannel(myChannel);
    if (!channelEpg || channelEpg->GetEpgEntries().size() == 0)
      return PVR_ERROR_NO_ERROR;

    int shift = m_tsOverride ? m_epgTimeShift : myChannel.GetTvgShift() + m_epgTimeShift;

    for (auto& epgEntry : channelEpg->GetEpgEntries())
    {
      if ((epgEntry.GetEndTime() + shift) < start)
        continue;

      EPG_TAG tag = {0};

      epgEntry.UpdateTo(tag, iChannelUid, shift, m_genreMappings);

      PVR->TransferEpgEntry(handle, &tag);

      if ((epgEntry.GetStartTime() + shift) > end)
        break;
    }

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

ChannelEpg* Epg::FindEpgForChannel(const std::string& id) const
{
  for (auto& myChannelEpg : m_channelEpgs)
  {
    if (StringUtils::EqualsNoCase(myChannelEpg.GetId(), id))
      return const_cast<ChannelEpg*>(&myChannelEpg);
  }

  return nullptr;
}

ChannelEpg* Epg::FindEpgForChannel(const Channel& channel) const
{
  for (auto& myChannelEpg : m_channelEpgs)
  {
    if (StringUtils::EqualsNoCase(myChannelEpg.GetId(), channel.GetTvgId()))
      return const_cast<ChannelEpg*>(&myChannelEpg);
  }

  for (auto& myChannelEpg : m_channelEpgs)
  {
    for (const std::string& displayName : myChannelEpg.GetNames())
    {
      const std::string convertedDisplayName = std::regex_replace(displayName, std::regex(" "), "_");
      if (StringUtils::EqualsNoCase(convertedDisplayName, channel.GetTvgName()) ||
          StringUtils::EqualsNoCase(displayName, channel.GetTvgName()))
        return const_cast<ChannelEpg*>(&myChannelEpg);
    }
  }

  for (auto& myChannelEpg : m_channelEpgs)
  {
    for (const std::string& displayName : myChannelEpg.GetNames())
    {
      if (StringUtils::EqualsNoCase(displayName, channel.GetChannelName()))
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
    if (!channel.GetIconPath().empty() && Settings::GetInstance().GetEpgLogosMode() == EpgLogosMode::PREFER_M3U)
      continue;

    // 2 - prefer icon from epg
    if (!channelEpg->GetIconPath().empty() && Settings::GetInstance().GetEpgLogosMode() == EpgLogosMode::PREFER_XMLTV)
    {
      m_channels.GetChannel(channel.GetUniqueId())->SetIconPath(channelEpg->GetIconPath());
      updated = true;
    }
  }

  if (updated)
    PVR->TriggerChannelUpdate();
}

bool Epg::LoadGenres()
{
  if (!FileUtils::FileExists(Settings::GetInstance().GetGenresLocation()))
    return false;

  std::string data;
  FileUtils::GetFileContents(Settings::GetInstance().GetGenresLocation(), data);

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
    Logger::Log(LEVEL_NOTICE, "%s - Loaded %d genres", __FUNCTION__, m_genreMappings.size());

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

  int shift = m_tsOverride ? m_epgTimeShift : myChannel.GetTvgShift() + m_epgTimeShift;

  for (auto& epgEntry : channelEpg->GetEpgEntries())
  {
    time_t startTime = epgEntry.GetStartTime() + shift;
    time_t endTime = epgEntry.GetEndTime() + shift;
    if (startTime <= lookupTime && endTime > lookupTime)
      return &epgEntry;
    else if (startTime > lookupTime)
      break;
  }

  return nullptr;
}