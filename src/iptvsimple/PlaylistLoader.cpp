/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "PlaylistLoader.h"

#include "InstanceSettings.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include <chrono>
#include <cstdlib>
#include <map>
#include <regex>
#include <sstream>
#include <vector>

#include <kodi/General.h>
#include <kodi/tools/StringUtils.h>

using namespace kodi::tools;
using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

PlaylistLoader::PlaylistLoader(kodi::addon::CInstancePVRClient* client, Channels& channels,
                               ChannelGroups& channelGroups, Providers& providers, Media& media, std::shared_ptr<InstanceSettings>& settings)
  : m_channelGroups(channelGroups), m_channels(channels), m_providers(providers), m_media(media), m_client(client), m_settings(settings) { }

bool PlaylistLoader::Init()
{
  m_m3uLocation = m_settings->GetM3ULocation();
  m_logoLocation = m_settings->GetLogoLocation();
  return true;
}

namespace {

bool GetOverrideRealTime(std::string& line)
{
  size_t realtimeIndex = line.find(REALTIME_OVERRIDE);
  if (realtimeIndex != std::string::npos)
  {
    size_t startValueIndex = realtimeIndex + REALTIME_OVERRIDE.length();
    size_t endQuoteIndex = line.find('"', startValueIndex);
    if (endQuoteIndex != std::string::npos)
    {
      size_t valueLength = endQuoteIndex - startValueIndex;
      std::string value = line.substr(startValueIndex, valueLength);
      StringUtils::ToLower(value);
      // The only value that matters is if the 'realtime' specifier is 'false'
      // that means we want to override the realtime value but not treat the stream
      // like media/VOD in the UI
      // It's a bit confusing, but hey, that's Kodi for you ;)
      return value == "false";
    }
  }

  return false;
}

}

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
  bool useM3UCache = m_settings->GetM3URefreshMode() != RefreshMode::DISABLED ? false : m_settings->UseM3UCache();

  std::string playlistContent;
  if (!FileUtils::GetCachedFileContents(m_settings, m_settings->GetM3UCacheFilename(), m_m3uLocation, playlistContent, useM3UCache))
  {
    Logger::Log(LEVEL_ERROR, "%s - Unable to load playlist cache file '%s':  file is missing or empty.", __FUNCTION__, m_m3uLocation.c_str());
    return false;
  }

  std::stringstream stream(playlistContent);

  /* load channels */
  bool isFirstLine = true;
  bool isRealTime = true;
  bool overrideRealTime = false;
  bool isMediaEntry = false;
  int epgTimeShift = 0;
  int catchupCorrectionSecs = m_settings->GetCatchupCorrectionSecs();
  std::vector<int> currentChannelGroupIdList;
  bool channelHadGroups = false;
  bool xeevCatchup = false;
  bool groupsFromBeginDirective = false; //From EXTGRP begin directive

  Channel tmpChannel{m_settings};
  MediaEntry tmpMediaEntry{m_settings};

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

        std::string strCatchupCorrection = ReadMarkerValue(line, CATCHUP_CORRECTION);
        if (!strCatchupCorrection.empty())
        {
          double catchupCorrectionDecimal = std::atof(strCatchupCorrection.c_str());
          catchupCorrectionSecs = static_cast<int>(catchupCorrectionDecimal * 3600.0);
        }

        //
        // If there are catchup values in the M3U header we read them to be used as defaults later on
        //
        m_m3uHeaderStrings.m_catchup = ReadMarkerValue(line, CATCHUP);
        // There is some xeev specific functionality if specificed in the header
        if (m_m3uHeaderStrings.m_catchup == "xc")
          xeevCatchup = true;
        // Some providers use a 'catchup-type' tag instead of 'catchup'
        if (m_m3uHeaderStrings.m_catchup.empty())
          m_m3uHeaderStrings.m_catchup = ReadMarkerValue(line, CATCHUP_TYPE);
        m_m3uHeaderStrings.m_catchupDays = ReadMarkerValue(line, CATCHUP_DAYS);
        m_m3uHeaderStrings.m_catchupSource = ReadMarkerValue(line, CATCHUP_SOURCE);

        //
        // Read either of the M3U header based EPG xmltv urls
        //
        std::string tvgUrl = ReadMarkerValue(line, TVG_URL_MARKER);
        if (tvgUrl.empty())
          tvgUrl = ReadMarkerValue(line, TVG_URL_OTHER_MARKER);
        // The tvgUrl might be a comma separated list. If it is just take
        // the first one as we don't support multiple list but at least it will work.
        size_t found = tvgUrl.find(',');
        if (found != std::string::npos)
          tvgUrl = tvgUrl.substr(0, found);
        m_settings->SetTvgUrl(tvgUrl);

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

      isMediaEntry = line.find(MEDIA) != std::string::npos ||
                     line.find(MEDIA_DIR) != std::string::npos ||
                     line.find(MEDIA_SIZE) != std::string::npos ||
                     m_settings->MediaForcePlaylist();

      overrideRealTime = GetOverrideRealTime(line);

      const std::string groupNamesListString = ParseIntoChannel(line, tmpChannel, tmpMediaEntry, epgTimeShift, catchupCorrectionSecs, xeevCatchup);

      if (!groupNamesListString.empty())
      {
        ParseAndAddChannelGroups(groupNamesListString, currentChannelGroupIdList, tmpChannel.IsRadio());
        groupsFromBeginDirective = false;
        channelHadGroups = true;
      }
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
      //Clear any previous Group Ids
      currentChannelGroupIdList.clear();
      groupsFromBeginDirective = false;

      const std::string groupNamesListString = ReadMarkerValue(line, M3U_GROUP_MARKER);
      if (!groupNamesListString.empty())
      {
        ParseAndAddChannelGroups(groupNamesListString, currentChannelGroupIdList, tmpChannel.IsRadio());
        groupsFromBeginDirective = true;
        channelHadGroups = true;
      }
    }
    else if (StringUtils::StartsWith(line, PLAYLIST_TYPE_MARKER)) //#EXT-X-PLAYLIST-TYPE:
    {
      if (ReadMarkerValue(line, PLAYLIST_TYPE_MARKER) == "VOD")
        isRealTime = false;
    }
    else if (line[0] != '#')
    {
      Logger::Log(LEVEL_DEBUG, "%s - Adding channel or Media Entry '%s' with URL: '%s'", __FUNCTION__, tmpChannel.GetChannelName().c_str(), line.c_str());

      if (m_settings->IsMediaEnabled() &&
          (isMediaEntry || (m_settings->ShowVodAsRecordings() && !isRealTime)))
      {
        MediaEntry entry = tmpMediaEntry;
        entry.UpdateFrom(tmpChannel);
        entry.SetStreamURL(line);

        if (!m_media.AddMediaEntry(entry, currentChannelGroupIdList, m_channelGroups, channelHadGroups))
          Logger::Log(LEVEL_DEBUG, "%s - Counld not add media entry as an entry with the same gnenerated unique ID already exists", __func__);
      }
      else
      {
        // There are cases where we want the stream to be represetned as a channel with live streaming disabled
        // to allow features such as passthrough to work. We don't want this to be VOD as then it would be treated like media.
        if (!overrideRealTime)
          tmpChannel.AddProperty(PVR_STREAM_PROPERTY_ISREALTIMESTREAM, "true");

        Channel channel = tmpChannel;
        channel.SetStreamURL(line);
        channel.ConfigureCatchupMode();

        if (!m_channels.AddChannel(channel, currentChannelGroupIdList, m_channelGroups, channelHadGroups))
          Logger::Log(LEVEL_DEBUG, "%s - Not adding channel '%s' as only channels with groups are supported for %s channels per add-on settings", __func__, tmpChannel.GetChannelName().c_str(), channel.IsRadio() ? "radio" : "tv");

      }

      tmpChannel.Reset();
      tmpMediaEntry.Reset();
      isRealTime = true;
      overrideRealTime = false;
      isMediaEntry = false;
      channelHadGroups = false;

      // We want to clear the groups if they came from a 'group-title' tag from a channel
      // But if it's from an EXTGRP tag we don't as that's a begin directive.
      if (!groupsFromBeginDirective)
        currentChannelGroupIdList.clear();
    }
  }

  stream.clear();

  //Now we need to remove any emptry channel groups. We do this as we may have added some while loading media entries.
  m_channelGroups.RemoveEmptyGroups();

  int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::high_resolution_clock::now() - started).count();

  Logger::Log(LEVEL_INFO, "%s Playlist Loaded - %d (ms)", __FUNCTION__, milliseconds);

  if (m_channels.GetChannelsAmount() == 0 && m_media.GetNumMedia() == 0)
  {
    Logger::Log(LEVEL_ERROR, "%s - Unable to load channels or media from file '%s'", __FUNCTION__, m_m3uLocation.c_str());
    // We no longer return false as this is just an empty M3U and a missing file error.
    //return false;
  }

  Logger::Log(LEVEL_INFO, "%s - Loaded %d channels.", __FUNCTION__, m_channels.GetChannelsAmount());
  Logger::Log(LEVEL_INFO, "%s - Loaded %d channel groups.", __FUNCTION__, m_channelGroups.GetChannelGroupsAmount());
  Logger::Log(LEVEL_INFO, "%s - Loaded %d providers.", __FUNCTION__, m_providers.GetNumProviders());
  Logger::Log(LEVEL_INFO, "%s - Loaded %d media items.", __FUNCTION__, m_media.GetNumMedia());

  return true;
}

std::string PlaylistLoader::ParseIntoChannel(const std::string& line, Channel& channel, MediaEntry& mediaEntry, int epgTimeShift, int catchupCorrectionSecs, bool xeevCatchup)
{
  size_t colonIndex = line.find(':');
  size_t commaIndex = line.rfind(','); //default to last comma on line in case we don't find a better match

  size_t lastQuoteIndex = line.rfind('"');
  if (lastQuoteIndex != std::string::npos)
  {
    // This is a better way to find the correct comma in
    // case there is a comma embedded in the channel name
    std::string possibleName = line.substr(lastQuoteIndex + 1);
    std::string commaName = possibleName;
    StringUtils::Trim(commaName);

    if (StringUtils::StartsWith(commaName, ","))
    {
      commaIndex = lastQuoteIndex + possibleName.find(',') + 1;
      std::string temp = line.substr(commaIndex + 1);
    }
  }

  if (colonIndex != std::string::npos && commaIndex != std::string::npos && commaIndex > colonIndex)
  {
    // parse name
    std::string channelName = line.substr(commaIndex + 1);
    channelName = StringUtils::Trim(channelName);
    kodi::UnknownToUTF8(channelName, channelName);
    channel.SetChannelName(channelName);

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
    std::string strTvgRec        = ReadMarkerValue(infoLine, TVG_INFO_REC);
    std::string strCatchupSource = ReadMarkerValue(infoLine, CATCHUP_SOURCE);
    std::string strCatchupSiptv = ReadMarkerValue(infoLine, CATCHUP_SIPTV);
    std::string strCatchupCorrection = ReadMarkerValue(infoLine, CATCHUP_CORRECTION);
    std::string strProviderName = ReadMarkerValue(infoLine, PROVIDER);
    std::string strProviderType = ReadMarkerValue(infoLine, PROVIDER_TYPE);
    std::string strProviderIconPath = ReadMarkerValue(infoLine, PROVIDER_LOGO);
    std::string strProviderCountries = ReadMarkerValue(infoLine, PROVIDER_COUNTRIES);
    std::string strProviderLanguages = ReadMarkerValue(infoLine, PROVIDER_LANGUAGES);
    std::string strMedia = ReadMarkerValue(infoLine, MEDIA);
    std::string strMediaDir = ReadMarkerValue(infoLine, MEDIA_DIR);
    std::string strMediaSize = ReadMarkerValue(infoLine, MEDIA_SIZE);

    kodi::UnknownToUTF8(strTvgName, strTvgName);
    kodi::UnknownToUTF8(strCatchupSource, strCatchupSource);

    // Some providers use a 'catchup-type' tag instead of 'catchup'
    if (strCatchup.empty())
      strCatchup = ReadMarkerValue(infoLine, CATCHUP_TYPE);
    // If we still don't have a value use the header supplied value if there is one
    if (strCatchup.empty() && !m_m3uHeaderStrings.m_catchup.empty())
      strCatchup = m_m3uHeaderStrings.m_catchup;

    if (strTvgId.empty())
      strTvgId = ReadMarkerValue(infoLine, TVG_INFO_ID_MARKER_UC);

    if (strTvgId.empty())
    {
      char buff[255];
      snprintf(buff, 255, "%d", std::atoi(infoLine.c_str()));
      strTvgId.append(buff);
    }

    // If don't have a channel number try another format
    if (strChnlNo.empty())
      strChnlNo = ReadMarkerValue(infoLine, CHANNEL_NUMBER_MARKER);

    if (!strChnlNo.empty() && !m_settings->NumberChannelsByM3uOrderOnly())
    {
      size_t found = strChnlNo.find('.');
      if (found != std::string::npos)
      {
        channel.SetChannelNumber(std::atoi(strChnlNo.substr(0, found).c_str()));
        channel.SetSubChannelNumber(std::atoi(strChnlNo.substr(found + 1).c_str()));
      }
      else
      {
        channel.SetChannelNumber(std::atoi(strChnlNo.c_str()));
      }
    }

    double tvgShiftDecimal = std::atof(strTvgShift.c_str());

    bool isRadio = StringUtils::EqualsNoCase(strRadio, "true");
    channel.SetTvgId(strTvgId);
    channel.SetTvgName(strTvgName);
    channel.SetCatchupSource(strCatchupSource);
    // If we still don't have a value use the header supplied value if there is one
    if (strCatchupSource.empty() && !m_m3uHeaderStrings.m_catchupSource.empty())
      strCatchupSource = m_m3uHeaderStrings.m_catchupSource;
    channel.SetTvgShift(static_cast<int>(tvgShiftDecimal * 3600.0));
    channel.SetRadio(isRadio);
    if (m_settings->GetLogoPathType() == PathType::LOCAL_PATH && m_settings->UseLocalLogosOnlyIgnoreM3U())
      channel.SetIconPathFromTvgLogo("", channelName);
    else
      channel.SetIconPathFromTvgLogo(strTvgLogo, channelName);
    if (strTvgShift.empty())
      channel.SetTvgShift(epgTimeShift);

    double catchupCorrectionDecimal = std::atof(strCatchupCorrection.c_str());
    channel.SetCatchupCorrectionSecs(static_cast<int>(catchupCorrectionDecimal * 3600.0));
    if (strCatchupCorrection.empty())
      channel.SetCatchupCorrectionSecs(catchupCorrectionSecs);

    if (StringUtils::EqualsNoCase(strCatchup, "default") || StringUtils::EqualsNoCase(strCatchup, "append") ||
        StringUtils::EqualsNoCase(strCatchup, "shift") || StringUtils::EqualsNoCase(strCatchup, "flussonic") ||
        StringUtils::EqualsNoCase(strCatchup, "flussonic-hls") || StringUtils::EqualsNoCase(strCatchup, "flussonic-ts") ||
        StringUtils::EqualsNoCase(strCatchup, "fs") || StringUtils::EqualsNoCase(strCatchup, "xc") ||
        StringUtils::EqualsNoCase(strCatchup, "vod"))
      channel.SetHasCatchup(true);

    if (StringUtils::EqualsNoCase(strCatchup, "default"))
      channel.SetCatchupMode(CatchupMode::DEFAULT);
    else if (StringUtils::EqualsNoCase(strCatchup, "append"))
      channel.SetCatchupMode(CatchupMode::APPEND);
    else if (StringUtils::EqualsNoCase(strCatchup, "shift"))
      channel.SetCatchupMode(CatchupMode::SHIFT);
    else if (StringUtils::EqualsNoCase(strCatchup, "flussonic") || StringUtils::EqualsNoCase(strCatchup, "flussonic-hls") ||
             StringUtils::EqualsNoCase(strCatchup, "flussonic-ts") || StringUtils::EqualsNoCase(strCatchup, "fs"))
      channel.SetCatchupMode(CatchupMode::FLUSSONIC);
    else if (StringUtils::EqualsNoCase(strCatchup, "xc"))
      channel.SetCatchupMode(CatchupMode::XTREAM_CODES);
    else if (StringUtils::EqualsNoCase(strCatchup, "vod"))
      channel.SetCatchupMode(CatchupMode::VOD);

    if (StringUtils::EqualsNoCase(strCatchup, "flussonic-ts") || StringUtils::EqualsNoCase(strCatchup, "fs"))
      channel.SetCatchupTSStream(true);

    if (!channel.HasCatchup() && xeevCatchup && (StringUtils::StartsWith(channelName, "* ") || StringUtils::StartsWith(channelName, "[+] ")))
    {
      channel.SetHasCatchup(true);
      channel.SetCatchupMode(CatchupMode::XTREAM_CODES);
    }

    int siptvTimeshiftDays = 0;
    if (!strCatchupSiptv.empty())
      siptvTimeshiftDays = atoi(strCatchupSiptv.c_str());
    // treat tvg-rec tag like siptv if siptv has not been used
    if (!strTvgRec.empty() && siptvTimeshiftDays == 0)
      siptvTimeshiftDays = atoi(strTvgRec.c_str());

    if (!strCatchupDays.empty())
      channel.SetCatchupDays(atoi(strCatchupDays.c_str()));
    // If we still don't have a value use the header supplied value if there is one
    else if (!m_m3uHeaderStrings.m_catchupSource.empty())
      channel.SetCatchupDays(atoi(m_m3uHeaderStrings.m_catchupDays.c_str()));
    else if (channel.GetCatchupMode() == CatchupMode::VOD)
      channel.SetCatchupDays(IGNORE_CATCHUP_DAYS);
    else if (siptvTimeshiftDays > 0)
      channel.SetCatchupDays(siptvTimeshiftDays);
    else
      channel.SetCatchupDays(m_settings->GetCatchupDays());

    // We also need to support the timeshift="days" tag from siptv
    // this was used before the catchup tags were introduced
    // it is the same as catchup="shift" except it also includes days
    if (!channel.HasCatchup() && siptvTimeshiftDays > 0)
    {
      channel.SetCatchupMode(CatchupMode::TIMESHIFT);
      channel.SetHasCatchup(true);
    }

    if (strProviderName.empty() && m_settings->HasDefaultProviderName())
      strProviderName = m_settings->GetDefaultProviderName();

    auto provider = m_providers.AddProvider(strProviderName);
    if (provider)
    {
      StringUtils::ToLower(strProviderType);
      if (!strProviderType.empty())
      {
        if (strProviderType == "addon")
          provider->SetProviderType(PVR_PROVIDER_TYPE_ADDON);
        else if (strProviderType == "satellite")
          provider->SetProviderType(PVR_PROVIDER_TYPE_SATELLITE);
        else if (strProviderType == "cable")
          provider->SetProviderType(PVR_PROVIDER_TYPE_CABLE);
        else if (strProviderType == "aerial")
          provider->SetProviderType(PVR_PROVIDER_TYPE_AERIAL);
        else if (strProviderType == "iptv")
          provider->SetProviderType(PVR_PROVIDER_TYPE_IPTV);
        else
          provider->SetProviderType(PVR_PROVIDER_TYPE_UNKNOWN);
      }

      if (!strProviderIconPath.empty())
        provider->SetIconPath(strProviderIconPath);

      if (!strProviderCountries.empty())
      {
        std::vector<std::string> countries = StringUtils::Split(strProviderCountries, PROVIDER_STRING_TOKEN_SEPARATOR);
        provider->SetCountries(countries);
      }

      if (!strProviderLanguages.empty())
      {
        std::vector<std::string> languages = StringUtils::Split(strProviderLanguages, PROVIDER_STRING_TOKEN_SEPARATOR);
        provider->SetLanguages(languages);
      }

      channel.SetProviderUniqueId(provider->GetUniqueId());
    }

    if (!strMediaDir.empty())
      mediaEntry.SetDirectory(strMediaDir);

    std::string groupNames = ReadMarkerValue(infoLine, GROUP_NAME_MARKER);
    auto& m3uGroupPathMode = m_settings->GetMediaUseM3UGroupPathMode();
    if (m3uGroupPathMode != MediaUseM3UGroupPathMode::IGNORE_GROUP_NAME)
    {
      if (m3uGroupPathMode == MediaUseM3UGroupPathMode::ALWAYS_APPEND || strMediaDir.empty())
      {
        if (!groupNames.empty() && groupNames.find(';') == std::string::npos)
        {
          //A media entry directory will always end with a "/"
          mediaEntry.SetDirectory(mediaEntry.GetDirectory() + groupNames);
        }
      }
    }

    if (!strMediaSize.empty())
      mediaEntry.SetSizeInBytes(std::strtoll(strMediaSize.c_str(), nullptr, 10));

    return groupNames;
  }

  return "";
}

void PlaylistLoader::ParseAndAddChannelGroups(const std::string& groupNamesListString, std::vector<int>& groupIdList, bool isRadio)
{
  // groupNamesListString may have a single or multiple group names seapareted by ';'

  std::stringstream streamGroups(groupNamesListString);
  std::string groupName;

  while (std::getline(streamGroups, groupName, ';'))
  {
    kodi::UnknownToUTF8(groupName, groupName);

    ChannelGroup group;
    group.SetGroupName(groupName);
    group.SetRadio(isRadio);

    if (m_channelGroups.CheckChannelGroupAllowed(group))
    {
      int uniqueGroupId = m_channelGroups.AddChannelGroup(group);
      groupIdList.emplace_back(uniqueGroupId);
    }
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
    else if (markerName == KODIPROP_MARKER && (prop == "inputstreamaddon" || prop == "inputstreamclass"))
    {
      prop = PVR_STREAM_PROPERTY_INPUTSTREAM;
    }

    if (addProperty)
      channel.AddProperty(prop, propValue);

    Logger::Log(LEVEL_DEBUG, "%s - Found %s property: '%s' value: '%s' added: %s", __FUNCTION__, markerName.c_str(), prop.c_str(), propValue.c_str(), addProperty ? "true" : "false");
  }
}

void PlaylistLoader::ReloadPlayList()
{
  m_m3uLocation = m_settings->GetM3ULocation();

  m_channels.Clear();
  m_channelGroups.Clear();
  m_providers.Clear();
  m_media.Clear();

  if (LoadPlayList())
  {
    m_client->TriggerChannelUpdate();
    m_client->TriggerChannelGroupsUpdate();
    m_client->TriggerProvidersUpdate();
    m_client->TriggerRecordingUpdate();
  }
  else
  {
    m_channels.ChannelsLoadFailed();
    m_channelGroups.ChannelGroupsLoadFailed();
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
      if (marker == M3U_GROUP_MARKER && line[markerStart] != '"')
      {
        //For this case we just want to return the full string without splitting it
        //This is because groups use semi-colons and not spaces as a delimiter
        return line.substr(markerStart, line.length());
      }

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
