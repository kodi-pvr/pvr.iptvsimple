/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
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
  m_m3uPathType = kodi::GetSettingEnum<PathType>("m3uPathType", PathType::REMOTE_PATH);
  m_m3uPath = kodi::GetSettingString("m3uPath");
  m_m3uUrl = kodi::GetSettingString("m3uUrl");
  m_cacheM3U = kodi::GetSettingBoolean("m3uCache");
  m_startChannelNumber = kodi::GetSettingInt("startNum", 1);
  m_numberChannelsByM3uOrderOnly = kodi::GetSettingBoolean("numberByOrder", false);
  m_m3uRefreshMode = kodi::GetSettingEnum<RefreshMode>("m3uRefreshMode", RefreshMode::DISABLED);
  m_m3uRefreshIntervalMins = kodi::GetSettingInt("m3uRefreshIntervalMins", 60);
  m_m3uRefreshHour = kodi::GetSettingInt("m3uRefreshHour", 4);
  m_defaultProviderName = kodi::GetSettingString("defaultProviderName");
  m_enableProviderMappings = kodi::GetSettingBoolean("enableProviderMappings", false);
  m_providerMappingFile = kodi::GetSettingString("providerMappingFile", DEFAULT_PROVIDER_NAME_MAP_FILE);

  // Groups
  m_allowTVChannelGroupsOnly = kodi::GetSettingBoolean("tvChannelGroupsOnly", false);
  m_tvChannelGroupMode = kodi::GetSettingEnum<ChannelGroupMode>("tvGroupMode", ChannelGroupMode::ALL_GROUPS);
  m_numTVGroups = kodi::GetSettingInt("numTvGroups", DEFAULT_NUM_GROUPS);
  m_oneTVGroup = kodi::GetSettingString("oneTvGroup");
  m_twoTVGroup = kodi::GetSettingString("twoTvGroup");
  m_threeTVGroup = kodi::GetSettingString("threeTvGroup");
  m_fourTVGroup = kodi::GetSettingString("fourTvGroup");
  m_fiveTVGroup = kodi::GetSettingString("fiveTvGroup");
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
  m_customTVGroupsFile = kodi::GetSettingString("customTvGroupsFile", DEFAULT_CUSTOM_TV_GROUPS_FILE);
  if (m_tvChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customTVGroupsFile, m_customTVChannelGroupNameList);

  m_allowRadioChannelGroupsOnly = kodi::GetSettingBoolean("radioChannelGroupsOnly", false);
  m_radioChannelGroupMode = kodi::GetSettingEnum<ChannelGroupMode>("radioGroupMode", ChannelGroupMode::ALL_GROUPS);
  m_numRadioGroups = kodi::GetSettingInt("numRadioGroups", DEFAULT_NUM_GROUPS);
  m_oneRadioGroup = kodi::GetSettingString("oneRadioGroup");
  m_twoRadioGroup = kodi::GetSettingString("twoRadioGroup");
  m_threeRadioGroup = kodi::GetSettingString("threeRadioGroup");
  m_fourRadioGroup = kodi::GetSettingString("fourRadioGroup");
  m_fiveRadioGroup = kodi::GetSettingString("fiveRadioGroup");
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
  m_customRadioGroupsFile = kodi::GetSettingString("customRadioGroupsFile", DEFAULT_CUSTOM_RADIO_GROUPS_FILE);
  if (m_radioChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customRadioGroupsFile, m_customRadioChannelGroupNameList);

  // EPG
  m_epgPathType = kodi::GetSettingEnum<PathType>("epgPathType", PathType::REMOTE_PATH);
  m_epgPath = kodi::GetSettingString("epgPath");
  m_epgUrl = kodi::GetSettingString("epgUrl");
  m_cacheEPG = kodi::GetSettingBoolean("epgCache", true);
  m_epgTimeShiftHours = kodi::GetSettingFloat("epgTimeShift", 0.0f);
  m_tsOverride = kodi::GetSettingBoolean("epgTSOverride", true);

  //Genres
  m_useEpgGenreTextWhenMapping = kodi::GetSettingBoolean("useEpgGenreText", false);
  m_genresPathType = kodi::GetSettingEnum<PathType>("genresPathType", PathType::LOCAL_PATH);
  m_genresPath = kodi::GetSettingString("genresPath");
  m_genresUrl = kodi::GetSettingString("genresUrl");

  // Channel Logos
  m_logoPathType = kodi::GetSettingEnum<PathType>("logoPathType", PathType::REMOTE_PATH);
  m_logoPath = kodi::GetSettingString("logoPath");
  m_logoBaseUrl = kodi::GetSettingString("logoBaseUrl");
  m_epgLogosMode = kodi::GetSettingEnum<EpgLogosMode>("logoFromEpg", EpgLogosMode::IGNORE_XMLTV);
  m_useLocalLogosOnly = kodi::GetSettingBoolean("useLogosLocalPathOnly", false);

  // Timeshift
  m_timeshiftEnabled = kodi::GetSettingBoolean("timeshiftEnabled", false);
  m_timeshiftEnabledAll = kodi::GetSettingBoolean("timeshiftEnabledAll", false);
  m_timeshiftEnabledHttp = kodi::GetSettingBoolean("timeshiftEnabledHttp", false);
  m_timeshiftEnabledUdp = kodi::GetSettingBoolean("timeshiftEnabledUdp", false);
  m_timeshiftEnabledCustom = kodi::GetSettingBoolean("timeshiftEnabledCustom", false);

  // Catchup
  m_catchupEnabled = kodi::GetSettingBoolean("catchupEnabled", false);
  m_catchupQueryFormat = kodi::GetSettingString("catchupQueryFormat");
  m_catchupDays = kodi::GetSettingInt("catchupDays", 5);
  m_allChannelsCatchupMode = kodi::GetSettingEnum<CatchupMode>("allChannelsCatchupMode", CatchupMode::DISABLED);
  m_catchupOverrideMode = kodi::GetSettingEnum<CatchupOverrideMode>("catchupOverrideMode", CatchupOverrideMode::WITHOUT_TAGS);
  m_catchupCorrectionHours = kodi::GetSettingFloat("catchupCorrection", 0.0f);
  m_catchupPlayEpgAsLive = kodi::GetSettingBoolean("catchupPlayEpgAsLive", false);
  m_catchupWatchEpgBeginBufferMins = kodi::GetSettingInt("catchupWatchEpgBeginBufferMins", 5);
  m_catchupWatchEpgEndBufferMins = kodi::GetSettingInt("catchupWatchEpgEndBufferMins", 15);
  m_catchupOnlyOnFinishedProgrammes = kodi::GetSettingBoolean("catchupOnlyOnFinishedProgrammes", false);

  // Advanced
  m_transformMulticastStreamUrls = kodi::GetSettingBoolean("transformMulticastStreamUrls", false);
  m_udpxyHost = kodi::GetSettingString("udpxyHost");
  m_udpxyPort = kodi::GetSettingInt("udpxyPort", DEFAULT_UDPXY_MULTICAST_RELAY_PORT);
  m_useFFmpegReconnect = kodi::GetSettingBoolean("useFFmpegReconnect");
  m_useInputstreamAdaptiveforHls = kodi::GetSettingBoolean("useInputstreamAdaptiveforHls", false);
  m_defaultUserAgent = kodi::GetSettingString("defaultUserAgent");
  m_defaultInputstream = kodi::GetSettingString("defaultInputstream");
  m_defaultMimeType = kodi::GetSettingString("defaultMimeType");
}

void Settings::ReloadAddonSettings()
{
  ReadFromAddon(m_userPath, m_clientPath);
}

ADDON_STATUS Settings::SetValue(const std::string& settingName, const kodi::CSettingValue& settingValue)
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