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

#include "PlaylistLoader.h"

#include "Settings.h"
#include "../client.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include <chrono>
#include <cstdlib>
#include <map>
#include <regex>
#include <sstream>
#include <vector>

#include <p8-platform/util/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

PlaylistLoader::PlaylistLoader(Channels& channels, ChannelGroups& channelGroups)
  : m_channels(channels), m_channelGroups(channelGroups), m_m3uLocation(Settings::GetInstance().GetM3ULocation()),
    m_logoLocation(Settings::GetInstance().GetLogoLocation()) {}

bool PlaylistLoader::LoadPlayList()
{
  auto started = std::chrono::high_resolution_clock::now();
  Logger::Log(LEVEL_DEBUG, "%s - Playlist Load Start", __FUNCTION__);

  if (m_m3uLocation.empty())
  {
    Logger::Log(LEVEL_ERROR, "%s - Playlist file path is not configured. Channels not loaded.", __FUNCTION__);
    return false;
  }

  // Cache is only allowed if refresh mode is disabled
  bool useM3UCache = Settings::GetInstance().GetM3URefreshMode() != RefreshMode::DISABLED ? false : Settings::GetInstance().UseM3UCache();

  std::string playlistContent;
  if (!FileUtils::GetCachedFileContents(M3U_CACHE_FILENAME, m_m3uLocation, playlistContent, useM3UCache))
  {
    Logger::Log(LEVEL_ERROR, "%s - Unable to load playlist cache file '%s':  file is missing or empty.", __FUNCTION__, m_m3uLocation.c_str());
    return false;
  }

  std::stringstream stream(playlistContent);

  /* load channels */
  bool isFirstLine = true;
  bool isRealTime = true;
  int epgTimeShift = 0;
  std::vector<int> currentChannelGroupIdList;

  Channel tmpChannel;

  std::string line;
  while (std::getline(stream, line))
  {
    line = StringUtils::TrimRight(line, " \t\r\n");
    line = StringUtils::TrimLeft(line, " \t");

    Logger::Log(LEVEL_DEBUG, "%s - M3U line read: '%s'", __FUNCTION__, line.c_str());

    if (line.empty())
      continue;

    if (isFirstLine)
    {
      isFirstLine = false;

      if (StringUtils::Left(line, 3) == "\xEF\xBB\xBF")
        line.erase(0, 3);

      if (StringUtils::StartsWith(line, M3U_START_MARKER)) //#EXTM3U
      {
        double tvgShiftDecimal = std::atof(ReadMarkerValue(line, TVG_INFO_SHIFT_MARKER).c_str());
        epgTimeShift = static_cast<int>(tvgShiftDecimal * 3600.0);
        Settings::GetInstance().SetTvgUrl(ReadMarkerValue(line, TVG_URL_MARKER));
        continue;
      }
      else
      {
        Logger::Log(LEVEL_ERROR, "%s - URL '%s' missing %s descriptor on line 1, attempting to parse it anyway.",
                    __FUNCTION__, m_m3uLocation.c_str(), M3U_START_MARKER.c_str());
      }
    }

    if (StringUtils::StartsWith(line, M3U_INFO_MARKER)) //#EXTINF
    {
      tmpChannel.SetChannelNumber(m_channels.GetCurrentChannelNumber());
      currentChannelGroupIdList.clear();

      const std::string groupNamesListString = ParseIntoChannel(line, tmpChannel, currentChannelGroupIdList, epgTimeShift);

      if (!groupNamesListString.empty())
        ParseAndAddChannelGroups(groupNamesListString, currentChannelGroupIdList, tmpChannel.IsRadio());
    }
    else if (StringUtils::StartsWith(line, KODIPROP_MARKER)) //#KODIPROP:
    {
      ParseSinglePropertyIntoChannel(line, tmpChannel, KODIPROP_MARKER);
    }
    else if (StringUtils::StartsWith(line, EXTVLCOPT_MARKER)) //#EXTVLCOPT:
    {
      ParseSinglePropertyIntoChannel(line, tmpChannel, EXTVLCOPT_MARKER);
    }
    else if (StringUtils::StartsWith(line, EXTVLCOPT_DASH_MARKER)) //#EXTVLCOPT--
    {
      ParseSinglePropertyIntoChannel(line, tmpChannel, EXTVLCOPT_DASH_MARKER);
    }
    else if (StringUtils::StartsWith(line, M3U_GROUP_MARKER)) //#EXTGRP:
    {
      const std::string groupNamesListString = ReadMarkerValue(line, M3U_GROUP_MARKER);
      if (!groupNamesListString.empty())
        ParseAndAddChannelGroups(groupNamesListString, currentChannelGroupIdList, tmpChannel.IsRadio());
    }
    else if (StringUtils::StartsWith(line, PLAYLIST_TYPE_MARKER)) //#EXT-X-PLAYLIST-TYPE:
    {
      if (ReadMarkerValue(line, PLAYLIST_TYPE_MARKER) == "VOD")
        isRealTime = false;
    }
    else if (line[0] != '#')
    {
      Logger::Log(LEVEL_DEBUG, "%s - Adding channel '%s' with URL: '%s'", __FUNCTION__, tmpChannel.GetChannelName().c_str(), line.c_str());

      if (isRealTime)
        tmpChannel.AddProperty(PVR_STREAM_PROPERTY_ISREALTIMESTREAM, "true");

      Channel channel(tmpChannel);
      channel.SetStreamURL(line);

      m_channels.AddChannel(channel, currentChannelGroupIdList, m_channelGroups);

      tmpChannel.Reset();
      isRealTime = true;
    }
  }

  stream.clear();

  int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::high_resolution_clock::now() - started).count();

  Logger::Log(LEVEL_NOTICE, "%s Playlist Loaded - %d (ms)", __FUNCTION__, milliseconds);

  if (m_channels.GetChannelsAmount() == 0)
  {
    Logger::Log(LEVEL_ERROR, "%s - Unable to load channels from file '%s'", __FUNCTION__, m_m3uLocation.c_str());
    return false;
  }

  Logger::Log(LEVEL_NOTICE, "%s - Loaded %d channels.", __FUNCTION__, m_channels.GetChannelsAmount());
  return true;
}

std::string PlaylistLoader::ParseIntoChannel(const std::string& line, Channel& channel, std::vector<int>& groupIdList, int epgTimeShift)
{
  // parse line
  size_t colonIndex = line.find(':');
  size_t commaIndex = line.rfind(',');
  if (colonIndex != std::string::npos && commaIndex != std::string::npos && commaIndex > colonIndex)
  {
    // parse name
    std::string channelName = line.substr(commaIndex + 1);
    channelName = StringUtils::Trim(channelName);
    channel.SetChannelName(XBMC->UnknownToUTF8(channelName.c_str()));

    // parse info line containng the attributes for a channel
    const std::string infoLine = line.substr(colonIndex + 1, commaIndex - colonIndex - 1);

    std::string strTvgId      = ReadMarkerValue(infoLine, TVG_INFO_ID_MARKER);
    std::string strTvgName    = ReadMarkerValue(infoLine, TVG_INFO_NAME_MARKER);
    std::string strTvgLogo    = ReadMarkerValue(infoLine, TVG_INFO_LOGO_MARKER);
    std::string strChnlNo     = ReadMarkerValue(infoLine, TVG_INFO_CHNO_MARKER);
    std::string strRadio      = ReadMarkerValue(infoLine, RADIO_MARKER);
    std::string strTvgShift   = ReadMarkerValue(infoLine, TVG_INFO_SHIFT_MARKER);
    std::string strCatchup       = ReadMarkerValue(infoLine, CATCHUP);
    std::string strCatchupDays   = ReadMarkerValue(infoLine, CATCHUP_DAYS);
    std::string strCatchupSource = ReadMarkerValue(infoLine, CATCHUP_SOURCE);

    if (strTvgId.empty())
      strTvgId = ReadMarkerValue(infoLine, TVG_INFO_ID_MARKER_UC);

    if (strTvgId.empty())
    {
      char buff[255];
      sprintf(buff, "%d", std::atoi(infoLine.c_str()));
      strTvgId.append(buff);
    }

    if (!strChnlNo.empty() && !Settings::GetInstance().NumberChannelsByM3uOrderOnly())
      channel.SetChannelNumber(std::atoi(strChnlNo.c_str()));

    double tvgShiftDecimal = std::atof(strTvgShift.c_str());

    bool isRadio = StringUtils::EqualsNoCase(strRadio, "true");
    channel.SetTvgId(strTvgId);
    channel.SetTvgName(XBMC->UnknownToUTF8(strTvgName.c_str()));
    channel.SetCatchupSource(XBMC->UnknownToUTF8(strCatchupSource.c_str()));
    channel.SetTvgShift(static_cast<int>(tvgShiftDecimal * 3600.0));
    channel.SetRadio(isRadio);
    channel.SetIconPathFromTvgLogo(strTvgLogo, channelName);
    if (strTvgShift.empty())
      channel.SetTvgShift(epgTimeShift);
    if (!strCatchupDays.empty())
      channel.SetCatchupDays(atoi(strCatchupDays.c_str()));
    else
      channel.SetCatchupDays(Settings::GetInstance().GetCatchupDays());

    if (StringUtils::EqualsNoCase(strCatchup, "default") || StringUtils::EqualsNoCase(strCatchup, "append") ||
        !strCatchupDays.empty() || !strCatchupSource.empty())
      channel.SetHasCatchup(true);

    if (StringUtils::EqualsNoCase(strCatchup, "default"))
      channel.SetCatchupMode(CatchupMode::DEFAULT);
    else if (StringUtils::EqualsNoCase(strCatchup, "append"))
      channel.SetCatchupMode(CatchupMode::APPEND);

    return ReadMarkerValue(infoLine, GROUP_NAME_MARKER);
  }

  return "";
}

void PlaylistLoader::ParseAndAddChannelGroups(const std::string& groupNamesListString, std::vector<int>& groupIdList, bool isRadio)
{
  //groupNamesListString may have a single or multiple group names seapareted by ';'

  std::stringstream streamGroups(groupNamesListString);
  std::string groupName;

  while (std::getline(streamGroups, groupName, ';'))
  {
    groupName = XBMC->UnknownToUTF8(groupName.c_str());

    ChannelGroup group;
    group.SetGroupName(groupName);
    group.SetRadio(isRadio);

    int uniqueGroupId = m_channelGroups.AddChannelGroup(group);
    groupIdList.emplace_back(uniqueGroupId);
  }
}

void PlaylistLoader::ParseSinglePropertyIntoChannel(const std::string& line, Channel& channel, const std::string& markerName)
{
  const std::string value = ReadMarkerValue(line, markerName);
  auto pos = value.find('=');
  if (pos != std::string::npos)
  {
    std::string prop = value.substr(0, pos);
    StringUtils::ToLower(prop);
    const std::string propValue = value.substr(pos + 1);

    bool addProperty = true;
    if (markerName == EXTVLCOPT_DASH_MARKER)
    {
      addProperty &= prop == "http-reconnect";
    }
    else if (markerName == EXTVLCOPT_MARKER)
    {
      addProperty &= prop == "http-user-agent" || prop == "http-referrer" || prop == "program";
    }

    if (addProperty)
      channel.AddProperty(prop, propValue);

    Logger::Log(LEVEL_DEBUG, "%s - Found %s property: '%s' value: '%s' added: %s", __FUNCTION__, markerName.c_str(), prop.c_str(), propValue.c_str(), addProperty ? "true" : "false");
  }
}

void PlaylistLoader::ReloadPlayList()
{
  m_m3uLocation = Settings::GetInstance().GetM3ULocation();

  m_channels.Clear();
  m_channelGroups.Clear();

  if (LoadPlayList())
  {
    PVR->TriggerChannelUpdate();
    PVR->TriggerChannelGroupsUpdate();
  }
}

std::string PlaylistLoader::ReadMarkerValue(const std::string& line, const std::string& markerName)
{
  size_t markerStart = line.find(markerName);
  if (markerStart != std::string::npos)
  {
    const std::string marker = markerName;
    markerStart += marker.length();
    if (markerStart < line.length())
    {
      char find = ' ';
      if (line[markerStart] == '"')
      {
        find = '"';
        markerStart++;
      }
      size_t markerEnd = line.find(find, markerStart);
      if (markerEnd == std::string::npos)
      {
        markerEnd = line.length();
      }
      return line.substr(markerStart, markerEnd - markerStart);
    }
  }

  return "";
}