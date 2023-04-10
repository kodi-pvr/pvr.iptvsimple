/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "InstanceSettings.h"

#include "utilities/FileUtils.h"
#include "utilities/XMLUtils.h"

#include <pugixml.hpp>

using namespace iptvsimple;
using namespace iptvsimple::utilities;
using namespace pugi;

/***************************************************************************
 * PVR settings
 **************************************************************************/

InstanceSettings::InstanceSettings(kodi::addon::IAddonInstance& instance, const kodi::addon::IInstanceInfo& instanceInfo)
  : m_instance(instance), m_instanceNumber(instanceInfo.GetNumber())
{
  ReadSettings();
}

void InstanceSettings::ReadSettings()
{
  // M3U
  m_instance.CheckInstanceSettingEnum<PathType>("m3uPathType", m_m3uPathType);
  m_instance.CheckInstanceSettingString("m3uPath", m_m3uPath);
  m_instance.CheckInstanceSettingString("m3uUrl", m_m3uUrl);
  m_instance.CheckInstanceSettingBoolean("m3uCache", m_cacheM3U);
  m_instance.CheckInstanceSettingInt("startNum", m_startChannelNumber);
  m_instance.CheckInstanceSettingBoolean("numberByOrder", m_numberChannelsByM3uOrderOnly);
  m_instance.CheckInstanceSettingEnum<RefreshMode>("m3uRefreshMode", m_m3uRefreshMode);
  m_instance.CheckInstanceSettingInt("m3uRefreshIntervalMins", m_m3uRefreshIntervalMins);
  m_instance.CheckInstanceSettingInt("m3uRefreshHour", m_m3uRefreshHour);
  m_instance.CheckInstanceSettingString("defaultProviderName", m_defaultProviderName);
  m_instance.CheckInstanceSettingBoolean("enableProviderMappings", m_enableProviderMappings);
  m_instance.CheckInstanceSettingString("providerMappingFile", m_providerMappingFile);

  // Groups
  m_instance.CheckInstanceSettingBoolean("tvChannelGroupsOnly", m_allowTVChannelGroupsOnly);
  m_instance.CheckInstanceSettingEnum<ChannelGroupMode>("tvGroupMode", m_tvChannelGroupMode);
  m_instance.CheckInstanceSettingInt("numTvGroups", m_numTVGroups);
  m_instance.CheckInstanceSettingString("oneTvGroup", m_oneTVGroup);
  m_instance.CheckInstanceSettingString("twoTvGroup", m_twoTVGroup);
  m_instance.CheckInstanceSettingString("threeTvGroup", m_threeTVGroup);
  m_instance.CheckInstanceSettingString("fourTvGroup", m_fourTVGroup);
  m_instance.CheckInstanceSettingString("fiveTvGroup", m_fiveTVGroup);
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
  m_instance.CheckInstanceSettingString("customTvGroupsFile", m_customTVGroupsFile);
  if (m_tvChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customTVGroupsFile, m_customTVChannelGroupNameList);

  m_instance.CheckInstanceSettingBoolean("radioChannelGroupsOnly", m_allowRadioChannelGroupsOnly);
  m_instance.CheckInstanceSettingEnum<ChannelGroupMode>("radioGroupMode", m_radioChannelGroupMode);
  m_instance.CheckInstanceSettingInt("numRadioGroups", m_numRadioGroups);
  m_instance.CheckInstanceSettingString("oneRadioGroup", m_oneRadioGroup);
  m_instance.CheckInstanceSettingString("twoRadioGroup", m_twoRadioGroup);
  m_instance.CheckInstanceSettingString("threeRadioGroup", m_threeRadioGroup);
  m_instance.CheckInstanceSettingString("fourRadioGroup", m_fourRadioGroup);
  m_instance.CheckInstanceSettingString("fiveRadioGroup", m_fiveRadioGroup);
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
  m_instance.CheckInstanceSettingString("customRadioGroupsFile", m_customRadioGroupsFile);
  if (m_radioChannelGroupMode == ChannelGroupMode::CUSTOM_GROUPS)
    LoadCustomChannelGroupFile(m_customRadioGroupsFile, m_customRadioChannelGroupNameList);

  // EPG
  m_instance.CheckInstanceSettingEnum<PathType>("epgPathType", m_epgPathType);
  m_instance.CheckInstanceSettingString("epgPath", m_epgPath);
  m_instance.CheckInstanceSettingString("epgUrl", m_epgUrl);
  m_instance.CheckInstanceSettingBoolean("epgCache", m_cacheEPG);
  m_instance.CheckInstanceSettingFloat("epgTimeShift", m_epgTimeShiftHours);
  m_instance.CheckInstanceSettingBoolean("epgTSOverride", m_tsOverride);
  m_instance.CheckInstanceSettingBoolean("epgIgnoreCaseForChannelIds", m_ignoreCaseForEpgChannelIds);

  //Genres
  m_instance.CheckInstanceSettingBoolean("useEpgGenreText", m_useEpgGenreTextWhenMapping);
  m_instance.CheckInstanceSettingEnum<PathType>("genresPathType", m_genresPathType);
  m_instance.CheckInstanceSettingString("genresPath", m_genresPath);
  m_instance.CheckInstanceSettingString("genresUrl", m_genresUrl);

  // Channel Logos
  m_instance.CheckInstanceSettingEnum<PathType>("logoPathType", m_logoPathType);
  m_instance.CheckInstanceSettingString("logoPath", m_logoPath);
  m_instance.CheckInstanceSettingString("logoBaseUrl", m_logoBaseUrl);
  m_instance.CheckInstanceSettingEnum<EpgLogosMode>("logoFromEpg", m_epgLogosMode);
  m_instance.CheckInstanceSettingBoolean("useLogosLocalPathOnly", m_useLocalLogosOnly);

  // Media
  m_instance.CheckInstanceSettingBoolean("mediaEnabled", m_mediaEnabled);
  m_instance.CheckInstanceSettingBoolean("mediaVODAsRecordings", m_showVodAsRecordings);
  m_instance.CheckInstanceSettingBoolean("mediaGroupByTitle", m_groupMediaByTitle);
  m_instance.CheckInstanceSettingBoolean("mediaGroupBySeason", m_groupMediaBySeason);
  m_instance.CheckInstanceSettingEnum<MediaUseM3UGroupPathMode>("mediaM3UGroupPath", m_mediaUseM3UGroupPathMode);
  m_instance.CheckInstanceSettingBoolean("mediaForcePlaylist", m_mediaForcePlaylist);
  m_instance.CheckInstanceSettingBoolean("mediaTitleSeasonEpisode", m_includeShowInfoInMediaTitle);

  // Timeshift
  m_instance.CheckInstanceSettingBoolean("timeshiftEnabled", m_timeshiftEnabled);
  m_instance.CheckInstanceSettingBoolean("timeshiftEnabledAll", m_timeshiftEnabledAll);
  m_instance.CheckInstanceSettingBoolean("timeshiftEnabledHttp", m_timeshiftEnabledHttp);
  m_instance.CheckInstanceSettingBoolean("timeshiftEnabledUdp", m_timeshiftEnabledUdp);
  m_instance.CheckInstanceSettingBoolean("timeshiftEnabledCustom", m_timeshiftEnabledCustom);

  // Catchup
  m_instance.CheckInstanceSettingBoolean("catchupEnabled", m_catchupEnabled);
  m_instance.CheckInstanceSettingString("catchupQueryFormat", m_catchupQueryFormat);
  m_instance.CheckInstanceSettingInt("catchupDays", m_catchupDays);
  m_instance.CheckInstanceSettingEnum<CatchupMode>("allChannelsCatchupMode", m_allChannelsCatchupMode);
  m_instance.CheckInstanceSettingEnum<CatchupOverrideMode>("catchupOverrideMode", m_catchupOverrideMode);
  m_instance.CheckInstanceSettingFloat("catchupCorrection", m_catchupCorrectionHours);
  m_instance.CheckInstanceSettingBoolean("catchupPlayEpgAsLive", m_catchupPlayEpgAsLive);
  m_instance.CheckInstanceSettingInt("catchupWatchEpgBeginBufferMins", m_catchupWatchEpgBeginBufferMins);
  m_instance.CheckInstanceSettingInt("catchupWatchEpgEndBufferMins", m_catchupWatchEpgEndBufferMins);
  m_instance.CheckInstanceSettingBoolean("catchupOnlyOnFinishedProgrammes", m_catchupOnlyOnFinishedProgrammes);

  // Advanced
  m_instance.CheckInstanceSettingBoolean("transformMulticastStreamUrls", m_transformMulticastStreamUrls);
  m_instance.CheckInstanceSettingString("udpxyHost", m_udpxyHost);
  m_instance.CheckInstanceSettingInt("udpxyPort", m_udpxyPort);
  m_instance.CheckInstanceSettingBoolean("useFFmpegReconnect", m_useFFmpegReconnect);
  m_instance.CheckInstanceSettingBoolean("useInputstreamAdaptiveforHls", m_useInputstreamAdaptiveforHls);
  m_instance.CheckInstanceSettingString("defaultUserAgent", m_defaultUserAgent);
  m_instance.CheckInstanceSettingString("defaultInputstream", m_defaultInputstream);
  m_instance.CheckInstanceSettingString("defaultMimeType", m_defaultMimeType);
}

void InstanceSettings::ReloadAddonInstanceSettings()
{
  ReadSettings();
}

ADDON_STATUS InstanceSettings::SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue)
{
  // reset cache and restart addon

  std::string strFile = FileUtils::GetUserDataAddonFilePath(GetUserPath(), GetM3UCacheFilename());
  if (FileUtils::FileExists(strFile))
    FileUtils::DeleteFile(strFile);

  strFile = FileUtils::GetUserDataAddonFilePath(GetUserPath(), GetXMLTVCacheFilename());
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
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_numTVGroups, ADDON_STATUS_OK, ADDON_STATUS_OK);
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
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_numRadioGroups, ADDON_STATUS_OK, ADDON_STATUS_OK);
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
  else if (settingName == "mediaM3UGroupPath")
    return SetEnumSetting<MediaUseM3UGroupPathMode, ADDON_STATUS>(settingName, settingValue, m_mediaUseM3UGroupPathMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "mediaForcePlaylist")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_mediaForcePlaylist, ADDON_STATUS_OK, ADDON_STATUS_OK);
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

bool InstanceSettings::LoadCustomChannelGroupFile(std::string& xmlFile, std::vector<std::string>& channelGroupNameList)
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