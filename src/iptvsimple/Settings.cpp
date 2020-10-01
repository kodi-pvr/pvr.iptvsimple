/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Settings.h"

#include "utilities/FileUtils.h"

using namespace iptvsimple;
using namespace iptvsimple::utilities;

/***************************************************************************
 * PVR settings
 **************************************************************************/
void Settings::ReadFromAddon(const std::string& userPath, const std::string& clientPath)
{
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
