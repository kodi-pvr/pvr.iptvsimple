/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Settings.h"

#include "utilities/FileUtils.h"
#include "utilities/XMLUtils.h"

#include <pugixml.hpp>

using namespace iptvsimple;
using namespace iptvsimple::utilities;
using namespace pugi;

/***************************************************************************
 * PVR settings
 **************************************************************************/
void Settings::ReadFromAddon(const std::string& userPath, const std::string& clientPath)
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + CHANNEL_GROUPS_DIR, CHANNEL_GROUPS_ADDON_DATA_BASE_DIR, true);

  m_userPath = userPath;
  m_clientPath = clientPath;

  // M3U
  m_m3uPathType = kodi::addon::GetSettingEnum<PathType>("m3uPathType", PathType::REMOTE_PATH);
  m_m3uPath = kodi::addon::GetSettingString("m3uPath");
  m_m3uUrl = kodi::addon::GetSettingString("m3uUrl");
  m_cacheM3U = kodi::addon::GetSettingBoolean("m3uCache");
  m_startChannelNumber = kodi::addon::GetSettingInt("startNum", 1);
  m_numberChannelsByM3uOrderOnly = kodi::addon::GetSettingBoolean("numberByOrder", false);
  m_m3uRefreshMode = kodi::addon::GetSettingEnum<RefreshMode>("m3uRefreshMode", RefreshMode::DISABLED);
  m_m3uRefreshIntervalMins = kodi::addon::GetSettingInt("m3uRefreshIntervalMins", 60);
  m_m3uRefreshHour = kodi::addon::GetSettingInt("m3uRefreshHour", 4);
  m_defaultProviderName = kodi::addon::GetSettingString("defaultProviderName");
  m_enableProviderMappings = kodi::addon::GetSettingBoolean("enableProviderMappings", false);
  m_providerMappingFile = kodi::addon::GetSettingString("providerMappingFile", DEFAULT_PROVIDER_NAME_MAP_FILE);

  // Groups
  m_allowTVChannelGroupsOnly = kodi::addon::GetSettingBoolean("tvChannelGroupsOnly", false);
  m_tvChannelGroupMode = kodi::addon::GetSettingEnum<ChannelGroupMode>("tvGroupMode", ChannelGroupMode::ALL_GROUPS);
  m_numTVGroups = kodi::addon::GetSettingInt("numTvGroups", DEFAULT_NUM_GROUPS);
  m_oneTVGroup = kodi::addon::GetSettingString("oneTvGroup");
  m_twoTVGroup = kodi::addon::GetSettingString("twoTvGroup");
  m_threeTVGroup = kodi::addon::GetSettingString("threeTvGroup");
  m_fourTVGroup = kodi::addon::GetSettingString("fourTvGroup");
  m_fiveTVGroup = kodi::addon::GetSettingString("fiveTvGroup");
  if (m_tvChannelGroupMode == ChannelGroupMode::SOME_GROUPS)
  {
    m_customTVChannelGroupNameList.clear();

    if (!m_oneTVGroup.empty() && m_numTVGroups >= 1)
      m_customTVChannelGroupNameList.emplace_back(m_oneTVGroup);
    if (!m_twoTVGroup.empty() && m_numTVGroups >= 2)
      m_customTVChannelGroupNameList.emplace_back(m_twoTVGroup);
    if (!m_threeTVGroup.empty() && m_numTVGroups >= 3)
      m_customTVChannelGroupNameList.emplace_back(m_threeTVGroup);
    if (!m_fourTVGroup.empty() && m_numTVGroups >= 4)
      m_customTVChannelGroupNameList.emplace_back(m_fourTVGroup);
    if (!m_fiveTVGroup.empty() && m_numTVGroups >= 5)
      m_customTVChannelGroupNameList.emplace_back(m_fiveTVGroup);
  }
  m_customTVGroupsFile = kodi::addon::GetSettingString("customTvGroupsFile", DEFAULT_CUSTOM_TV_GROUPS_FILE);
  if (m_tvChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customTVGroupsFile, m_customTVChannelGroupNameList);

  m_allowRadioChannelGroupsOnly = kodi::addon::GetSettingBoolean("radioChannelGroupsOnly", false);
  m_radioChannelGroupMode = kodi::addon::GetSettingEnum<ChannelGroupMode>("radioGroupMode", ChannelGroupMode::ALL_GROUPS);
  m_numRadioGroups = kodi::addon::GetSettingInt("numRadioGroups", DEFAULT_NUM_GROUPS);
  m_oneRadioGroup = kodi::addon::GetSettingString("oneRadioGroup");
  m_twoRadioGroup = kodi::addon::GetSettingString("twoRadioGroup");
  m_threeRadioGroup = kodi::addon::GetSettingString("threeRadioGroup");
  m_fourRadioGroup = kodi::addon::GetSettingString("fourRadioGroup");
  m_fiveRadioGroup = kodi::addon::GetSettingString("fiveRadioGroup");
  if (m_radioChannelGroupMode == ChannelGroupMode::SOME_GROUPS)
  {
    m_customRadioChannelGroupNameList.clear();

    if (!m_oneRadioGroup.empty() && m_numRadioGroups >= 1)
      m_customRadioChannelGroupNameList.emplace_back(m_oneRadioGroup);
    if (!m_twoRadioGroup.empty() && m_numRadioGroups >= 2)
      m_customRadioChannelGroupNameList.emplace_back(m_twoRadioGroup);
    if (!m_threeRadioGroup.empty() && m_numRadioGroups >= 3)
      m_customRadioChannelGroupNameList.emplace_back(m_threeRadioGroup);
    if (!m_fourRadioGroup.empty() && m_numRadioGroups >= 4)
      m_customRadioChannelGroupNameList.emplace_back(m_fourRadioGroup);
    if (!m_fiveRadioGroup.empty() && m_numRadioGroups >= 5)
      m_customRadioChannelGroupNameList.emplace_back(m_fiveRadioGroup);
  }
  m_customRadioGroupsFile = kodi::addon::GetSettingString("customRadioGroupsFile", DEFAULT_CUSTOM_RADIO_GROUPS_FILE);
  if (m_radioChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customRadioGroupsFile, m_customRadioChannelGroupNameList);

  // EPG
  m_epgPathType = kodi::addon::GetSettingEnum<PathType>("epgPathType", PathType::REMOTE_PATH);
  m_epgPath = kodi::addon::GetSettingString("epgPath");
  m_epgUrl = kodi::addon::GetSettingString("epgUrl");
  m_cacheEPG = kodi::addon::GetSettingBoolean("epgCache", true);
  m_epgTimeShiftHours = kodi::addon::GetSettingFloat("epgTimeShift", 0.0f);
  m_tsOverride = kodi::addon::GetSettingBoolean("epgTSOverride", true);
  m_ignoreCaseForEpgChannelIds =  kodi::addon::GetSettingBoolean("epgIgnoreCaseForChannelIds", true);

  //Genres
  m_useEpgGenreTextWhenMapping = kodi::addon::GetSettingBoolean("useEpgGenreText", false);
  m_genresPathType = kodi::addon::GetSettingEnum<PathType>("genresPathType", PathType::LOCAL_PATH);
  m_genresPath = kodi::addon::GetSettingString("genresPath");
  m_genresUrl = kodi::addon::GetSettingString("genresUrl");

  // Channel Logos
  m_logoPathType = kodi::addon::GetSettingEnum<PathType>("logoPathType", PathType::REMOTE_PATH);
  m_logoPath = kodi::addon::GetSettingString("logoPath");
  m_logoBaseUrl = kodi::addon::GetSettingString("logoBaseUrl");
  m_epgLogosMode = kodi::addon::GetSettingEnum<EpgLogosMode>("logoFromEpg", EpgLogosMode::IGNORE_XMLTV);
  m_useLocalLogosOnly = kodi::addon::GetSettingBoolean("useLogosLocalPathOnly", false);

  // Media m_mediaEnabled
  m_mediaEnabled = kodi::addon::GetSettingBoolean("mediaEnabled", true);
  m_showVodAsRecordings = kodi::addon::GetSettingBoolean("mediaVODAsRecordings", true);
  m_groupMediaByTitle = kodi::addon::GetSettingBoolean("mediaGroupByTitle", true);
  m_groupMediaBySeason = kodi::addon::GetSettingBoolean("mediaGroupBySeason", true);
  m_includeShowInfoInMediaTitle = kodi::addon::GetSettingBoolean("mediaTitleSeasonEpisode", false);

  // Timeshift
  m_timeshiftEnabled = kodi::addon::GetSettingBoolean("timeshiftEnabled", false);
  m_timeshiftEnabledAll = kodi::addon::GetSettingBoolean("timeshiftEnabledAll", false);
  m_timeshiftEnabledHttp = kodi::addon::GetSettingBoolean("timeshiftEnabledHttp", false);
  m_timeshiftEnabledUdp = kodi::addon::GetSettingBoolean("timeshiftEnabledUdp", false);
  m_timeshiftEnabledCustom = kodi::addon::GetSettingBoolean("timeshiftEnabledCustom", false);

  // Catchup
  m_catchupEnabled = kodi::addon::GetSettingBoolean("catchupEnabled", false);
  m_catchupQueryFormat = kodi::addon::GetSettingString("catchupQueryFormat");
  m_catchupDays = kodi::addon::GetSettingInt("catchupDays", 5);
  m_allChannelsCatchupMode = kodi::addon::GetSettingEnum<CatchupMode>("allChannelsCatchupMode", CatchupMode::DISABLED);
  m_catchupOverrideMode = kodi::addon::GetSettingEnum<CatchupOverrideMode>("catchupOverrideMode", CatchupOverrideMode::WITHOUT_TAGS);
  m_catchupCorrectionHours = kodi::addon::GetSettingFloat("catchupCorrection", 0.0f);
  m_catchupPlayEpgAsLive = kodi::addon::GetSettingBoolean("catchupPlayEpgAsLive", false);
  m_catchupWatchEpgBeginBufferMins = kodi::addon::GetSettingInt("catchupWatchEpgBeginBufferMins", 5);
  m_catchupWatchEpgEndBufferMins = kodi::addon::GetSettingInt("catchupWatchEpgEndBufferMins", 15);
  m_catchupOnlyOnFinishedProgrammes = kodi::addon::GetSettingBoolean("catchupOnlyOnFinishedProgrammes", false);

  // Advanced
  m_transformMulticastStreamUrls = kodi::addon::GetSettingBoolean("transformMulticastStreamUrls", false);
  m_udpxyHost = kodi::addon::GetSettingString("udpxyHost");
  m_udpxyPort = kodi::addon::GetSettingInt("udpxyPort", DEFAULT_UDPXY_MULTICAST_RELAY_PORT);
  m_useFFmpegReconnect = kodi::addon::GetSettingBoolean("useFFmpegReconnect");
  m_useInputstreamAdaptiveforHls = kodi::addon::GetSettingBoolean("useInputstreamAdaptiveforHls", false);
  m_defaultUserAgent = kodi::addon::GetSettingString("defaultUserAgent");
  m_defaultInputstream = kodi::addon::GetSettingString("defaultInputstream");
  m_defaultMimeType = kodi::addon::GetSettingString("defaultMimeType");
}

void Settings::ReloadAddonSettings()
{
  ReadFromAddon(m_userPath, m_clientPath);
}

ADDON_STATUS Settings::SetValue(const std::string& settingName, const kodi::addon::CSettingValue& settingValue)
{
  // reset cache and restart addon

  std::string strFile = FileUtils::GetUserDataAddonFilePath(M3U_CACHE_FILENAME);
  if (FileUtils::FileExists(strFile))
    FileUtils::DeleteFile(strFile);

  strFile = FileUtils::GetUserDataAddonFilePath(XMLTV_CACHE_FILENAME);
  if (FileUtils::FileExists(strFile))
    FileUtils::DeleteFile(strFile);

  // M3U
  if (settingName == "m3uPathType")
    return SetEnumSetting<PathType, ADDON_STATUS>(settingName, settingValue, m_m3uPathType, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "m3uPath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_m3uPath, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "m3uUrl")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_m3uUrl, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "m3uCache")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_cacheM3U, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "startNum")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_startChannelNumber, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "numberByOrder")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_numberChannelsByM3uOrderOnly, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "m3uRefreshMode")
    return SetEnumSetting<RefreshMode, ADDON_STATUS>(settingName, settingValue, m_m3uRefreshMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "m3uRefreshIntervalMins")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_m3uRefreshIntervalMins, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "m3uRefreshHour")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_m3uRefreshHour, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "defaultProviderName")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_defaultProviderName, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "enableProviderMappings")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_enableProviderMappings, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "providerMappingFile")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_providerMappingFile, ADDON_STATUS_OK, ADDON_STATUS_OK);
  // Groups
  else if (settingName == "tvChannelGroupsOnly")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_allowTVChannelGroupsOnly, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "tvGroupMode")
    return SetEnumSetting<ChannelGroupMode, ADDON_STATUS>(settingName, settingValue, m_tvChannelGroupMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "numTvGroups")
    return SetSetting<unsigned int, ADDON_STATUS>(settingName, settingValue, m_numTVGroups, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "oneTvGroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_oneTVGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "twoTvGroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_twoTVGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "threeTvGroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_threeTVGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "twoTvGroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_fourTVGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "fiveTvGroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_fiveTVGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "customTvGroupsFile")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_customTVGroupsFile, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "radioChannelGroupsOnly")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_allowRadioChannelGroupsOnly, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "radioGroupMode")
    return SetEnumSetting<ChannelGroupMode, ADDON_STATUS>(settingName, settingValue, m_radioChannelGroupMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "numRadioGroups")
    return SetSetting<unsigned int, ADDON_STATUS>(settingName, settingValue, m_numRadioGroups, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "oneRadioGroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_oneRadioGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "twoRadioGroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_twoRadioGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "threeRadioGroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_threeRadioGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "fourRadioGroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_fourRadioGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "fiveRadioGroup")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_fiveRadioGroup, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "customRadioGroupsFile")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_customRadioGroupsFile, ADDON_STATUS_OK, ADDON_STATUS_OK);
  // EPG
  else if (settingName == "epgPathType")
    return SetEnumSetting<PathType, ADDON_STATUS>(settingName, settingValue, m_epgPathType, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "epgPath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_epgPath, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "epgUrl")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_epgUrl, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "epgCache")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_cacheEPG, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "epgTimeShift")
    return SetSetting<float, ADDON_STATUS>(settingName, settingValue, m_epgTimeShiftHours, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "epgTSOverride")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_tsOverride, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "epgIgnoreCaseForChannelIds")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_ignoreCaseForEpgChannelIds, ADDON_STATUS_OK, ADDON_STATUS_OK);
  // Genres
  else if (settingName == "useEpgGenreText")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useEpgGenreTextWhenMapping, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "genresPathType")
    return SetEnumSetting<PathType, ADDON_STATUS>(settingName, settingValue, m_genresPathType, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "genresPath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_genresPath, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "genresUrl")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_genresUrl, ADDON_STATUS_OK, ADDON_STATUS_OK);
  // Channel Logos
  else if (settingName == "logoPathType")
    return SetEnumSetting<PathType, ADDON_STATUS>(settingName, settingValue, m_logoPathType, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "logoPath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_logoPath, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "logoBaseUrl")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_logoBaseUrl, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "logoFromEpg")
    return SetEnumSetting<EpgLogosMode, ADDON_STATUS>(settingName, settingValue, m_epgLogosMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "useLogosLocalPathOnly")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useLocalLogosOnly, ADDON_STATUS_OK, ADDON_STATUS_OK);
  // Media
  else if (settingName == "mediaEnabled")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_mediaEnabled, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "mediaGroupByTitle")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_groupMediaByTitle, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "mediaGroupBySeason")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_groupMediaBySeason, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "mediaTitleSeasonEpisode")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_includeShowInfoInMediaTitle, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "mediaVODAsRecordings")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_showVodAsRecordings, ADDON_STATUS_OK, ADDON_STATUS_OK);
  // Timeshift
  else if (settingName == "timeshiftEnabled")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_timeshiftEnabled, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "timeshiftEnabledAll")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_timeshiftEnabledAll, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "timeshiftEnabledHttp")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_timeshiftEnabledHttp, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "timeshiftEnabledUdp")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_timeshiftEnabledUdp, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "timeshiftEnabledCustom")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_timeshiftEnabledCustom, ADDON_STATUS_OK, ADDON_STATUS_OK);
  // Catchup
  else if (settingName == "catchupEnabled")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_catchupEnabled, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "catchupQueryFormat")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_catchupQueryFormat, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "catchupDays")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_catchupDays, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "allChannelsCatchupMode")
    return SetEnumSetting<CatchupMode, ADDON_STATUS>(settingName, settingValue, m_allChannelsCatchupMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "catchupOverrideMode")
    return SetEnumSetting<CatchupOverrideMode, ADDON_STATUS>(settingName, settingValue, m_catchupOverrideMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "catchupCorrection")
    return SetSetting<float, ADDON_STATUS>(settingName, settingValue, m_catchupCorrectionHours, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "catchupPlayEpgAsLive")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_catchupPlayEpgAsLive, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "catchupWatchEpgBeginBufferMins")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_catchupWatchEpgBeginBufferMins, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "catchupWatchEpgEndBufferMins")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_catchupWatchEpgEndBufferMins, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "catchupOnlyOnFinishedProgrammes")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_catchupOnlyOnFinishedProgrammes, ADDON_STATUS_OK, ADDON_STATUS_OK);
  // Advanced
  else if (settingName == "transformMulticastStreamUrls")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_transformMulticastStreamUrls, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "udpxyHost")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_udpxyHost, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "udpxyPort")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_udpxyPort, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "useFFmpegReconnect")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useFFmpegReconnect, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "useInputstreamAdaptiveforHls")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useInputstreamAdaptiveforHls, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "defaultUserAgent")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_defaultUserAgent, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "defaultInputstream")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_defaultInputstream, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "defaultMimeType")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_defaultMimeType, ADDON_STATUS_OK, ADDON_STATUS_OK);

  return ADDON_STATUS_OK;
}

bool Settings::LoadCustomChannelGroupFile(std::string& xmlFile, std::vector<std::string>& channelGroupNameList)
{
  channelGroupNameList.clear();

  if (!FileUtils::FileExists(xmlFile.c_str()))
  {
    Logger::Log(LEVEL_ERROR, "%s No XML file found: %s", __func__, xmlFile.c_str());
    return false;
  }

  Logger::Log(LEVEL_DEBUG, "%s Loading XML File: %s", __func__, xmlFile.c_str());

  std::string data;
  FileUtils::GetFileContents(xmlFile, data);

  if (data.empty())
    return false;

  char* buffer = &(data[0]);
  xml_document xmlDoc;
  xml_parse_result result = xmlDoc.load_string(buffer);

  if (!result)
  {
    std::string errorString;
    int offset = GetParseErrorString(buffer, result.offset, errorString);
    Logger::Log(LEVEL_ERROR, "%s - Unable parse CustomChannelGroup XML: %s, offset: %d: \n[ %s \n]", __FUNCTION__, result.description(), offset, errorString.c_str());
    return false;
  }

  const auto& rootElement = xmlDoc.child("customChannelGroups");
  if (!rootElement)
    return false;

  for (const auto& groupNameNode : rootElement.children("channelGroupName"))
  {
    std::string channelGroupName = groupNameNode.child_value();
    channelGroupNameList.emplace_back(channelGroupName);
    Logger::Log(LEVEL_DEBUG, "%s Read CustomChannelGroup Name: %s, from file: %s", __func__, channelGroupName.c_str(), xmlFile.c_str());
  }

  xmlDoc.reset();

  return true;
}