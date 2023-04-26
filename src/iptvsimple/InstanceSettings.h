/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "data/Channel.h"
#include "utilities/Logger.h"

#include <string>
#include <type_traits>

#include <kodi/AddonBase.h>

namespace iptvsimple
{
  static const std::string M3U_CACHE_FILENAME = "iptv.m3u.cache";
  static const std::string XMLTV_CACHE_FILENAME = "xmltv.xml.cache";
  static const std::string ADDON_DATA_BASE_DIR = "special://userdata/addon_data/pvr.iptvsimple";
  static const std::string DEFAULT_PROVIDER_NAME_MAP_FILE = ADDON_DATA_BASE_DIR + "/providers/providerMappings.xml";
  static const std::string DEFAULT_GENRE_TEXT_MAP_FILE = ADDON_DATA_BASE_DIR + "/genres/genreTextMappings/genres.xml";
  static const int DEFAULT_UDPXY_MULTICAST_RELAY_PORT = 4022;

  static const int DEFAULT_NUM_GROUPS = 1;
  static const std::string DEFAULT_CUSTOM_TV_GROUPS_FILE = ADDON_DATA_BASE_DIR + "/channelGroups/customTVGroups-example.xml";
  static const std::string DEFAULT_CUSTOM_RADIO_GROUPS_FILE = ADDON_DATA_BASE_DIR + "/channelGroups/customRadioGroups-example.xml";

  enum class PathType
    : int // same type as addon settings
  {
    LOCAL_PATH = 0,
    REMOTE_PATH
  };

  enum class RefreshMode
    : int // same type as addon settings
  {
    DISABLED = 0,
    REPEATED_REFRESH,
    ONCE_PER_DAY
  };

  enum class ChannelGroupMode
    : int // same type as addon settings
  {
    ALL_GROUPS = 0,
    SOME_GROUPS,
    CUSTOM_GROUPS
  };

  enum class EpgLogosMode
    : int // same type as addon settings
  {
    IGNORE_XMLTV = 0,
    PREFER_M3U,
    PREFER_XMLTV
  };

  enum class MediaUseM3UGroupPathMode
    : int // same type as addon settings
  {
    IGNORE_GROUP_NAME = 0,
    ALWAYS_APPEND,
    ONLY_IF_EMPTY
  };


  enum class CatchupOverrideMode
    : int // same type as addon settings
  {
    WITHOUT_TAGS = 0,
    WITH_TAGS,
    ALL_CHANNELS
  };

  class InstanceSettings
  {
  public:
    explicit InstanceSettings(kodi::addon::IAddonInstance& instance, const kodi::addon::IInstanceInfo& instanceInfo);

    ADDON_STATUS SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue);

    void ReadSettings();
    void ReloadAddonInstanceSettings();

    const std::string GetUserPath() const { return kodi::addon::GetUserPath(); }

    const std::string& GetM3ULocation() const { return m_m3uPathType == PathType::REMOTE_PATH ? m_m3uUrl : m_m3uPath; }
    const PathType& GetM3UPathType() const { return m_m3uPathType; }
    const std::string& GetM3UPath() const { return m_m3uPath; }
    const std::string& GetM3UUrl() const { return m_m3uUrl; }
    bool UseM3UCache() const { return m_m3uPathType == PathType::REMOTE_PATH ? m_cacheM3U : false; }
    int GetStartChannelNumber() const { return m_startChannelNumber; }
    bool NumberChannelsByM3uOrderOnly() const { return m_numberChannelsByM3uOrderOnly; }
    const RefreshMode& GetM3URefreshMode() const { return m_m3uRefreshMode; }
    int GetM3URefreshIntervalMins() const { return m_m3uRefreshIntervalMins; }
    int GetM3URefreshHour() const { return m_m3uRefreshHour; }
    bool HasDefaultProviderName() const { return !m_defaultProviderName.empty(); }
    const std::string& GetDefaultProviderName() const { return m_defaultProviderName; }
    bool ProviderNameMapFileEnabled() const { return m_enableProviderMappings; }
    const std::string& GetProviderNameMapFile() const { return m_providerMappingFile; }

    bool AllowTVChannelGroupsOnly() const { return m_allowTVChannelGroupsOnly; }
    const ChannelGroupMode& GetTVChannelGroupMode() const { return m_tvChannelGroupMode; }
    const std::string& GetCustomTVGroupsFile() const { return m_customTVGroupsFile; }
    bool AllowRadioChannelGroupsOnly() const { return m_allowRadioChannelGroupsOnly; }
    const ChannelGroupMode& GetRadioChannelGroupMode() const { return m_radioChannelGroupMode; }
    const std::string& GetCustomRadioGroupsFile() const { return m_customRadioGroupsFile; }

    const std::string& GetEpgLocation() const
    {
      const std::string& epgLocation = m_epgPathType == PathType::REMOTE_PATH ? m_epgUrl : m_epgPath;
      return epgLocation.empty() ? m_tvgUrl : epgLocation;
    }
    const PathType& GetEpgPathType() const { return m_epgPathType; }
    const std::string& GetEpgPath() const { return m_epgPath; }
    const std::string& GetEpgUrl() const { return m_epgUrl; }
    bool UseEPGCache() const { return m_epgPathType == PathType::REMOTE_PATH ? m_cacheEPG : false; }
    float GetEpgTimeshiftHours() const { return m_epgTimeShiftHours; }
    int GetEpgTimeshiftSecs() const { return static_cast<int>(m_epgTimeShiftHours * 60 * 60); }
    bool GetTsOverride() const { return m_tsOverride; }
    bool AlwaysLoadEPGData() const { return m_epgLogosMode == EpgLogosMode::PREFER_XMLTV || IsCatchupEnabled(); }
    bool IgnoreCaseForEpgChannelIds() const { return m_ignoreCaseForEpgChannelIds; }

    const std::string& GetGenresLocation() const { return m_genresPathType == PathType::REMOTE_PATH ? m_genresUrl : m_genresPath; }
    bool UseEpgGenreTextWhenMapping() const { return m_useEpgGenreTextWhenMapping; }
    const PathType& GetGenresPathType() const { return m_genresPathType; }
    const std::string& GetGenresPath() const { return m_genresPath; }
    const std::string& GetGenresUrl() const { return m_genresUrl; }

    const std::string& GetLogoLocation() const { return m_logoPathType == PathType::REMOTE_PATH ? m_logoBaseUrl : m_logoPath; }
    const PathType& GetLogoPathType() const { return m_logoPathType; }
    const std::string& GetLogoPath() const { return m_logoPath; }
    const std::string& GetLogoBaseUrl() const { return m_logoBaseUrl; }
    const EpgLogosMode& GetEpgLogosMode() const { return m_epgLogosMode; }
    bool UseLocalLogosOnlyIgnoreM3U() const { return m_useLocalLogosOnly; }

    bool IsMediaEnabled() const { return m_mediaEnabled; }
    bool ShowVodAsRecordings() const { return m_showVodAsRecordings; }
    bool GroupMediaByTitle() const { return m_groupMediaByTitle; }
    bool GroupMediaBySeason() const { return m_groupMediaBySeason; }
    const MediaUseM3UGroupPathMode& GetMediaUseM3UGroupPathMode() { return m_mediaUseM3UGroupPathMode; }
    bool MediaForcePlaylist() const { return m_mediaForcePlaylist; }
    bool IncludeShowInfoInMediaTitle() const { return m_includeShowInfoInMediaTitle; }

    bool IsTimeshiftEnabled() const { return m_timeshiftEnabled; }
    bool IsTimeshiftEnabledAll() const { return m_timeshiftEnabledAll; }
    bool IsTimeshiftEnabledHttp() const { return m_timeshiftEnabledHttp; }
    bool IsTimeshiftEnabledUdp() const { return m_timeshiftEnabledUdp; }
    bool AlwaysEnableTimeshiftModeIfMissing() const { return m_timeshiftEnabledCustom; }

    bool IsCatchupEnabled() const { return m_catchupEnabled; }
    const std::string& GetCatchupQueryFormat() const { return m_catchupQueryFormat; }
    int GetCatchupDays() const { return m_catchupDays; }
    time_t GetCatchupDaysInSeconds() const { return static_cast<time_t>(m_catchupDays) * 24 * 60 * 60; }
    const CatchupMode& GetAllChannelsCatchupMode() const { return m_allChannelsCatchupMode; }
    const CatchupOverrideMode& GetCatchupOverrideMode() const { return m_catchupOverrideMode; }
    float GetCatchupCorrectionHours() const { return m_catchupCorrectionHours; }
    int GetCatchupCorrectionSecs() const { return static_cast<int>(m_catchupCorrectionHours * 60 * 60); }
    bool CatchupPlayEpgAsLive() const { return m_catchupPlayEpgAsLive; }
    int GetCatchupWatchEpgBeginBufferMins() const { return m_catchupWatchEpgBeginBufferMins; }
    time_t GetCatchupWatchEpgBeginBufferSecs() const { return static_cast<time_t>(m_catchupWatchEpgBeginBufferMins) * 60; }
    int GetCatchupWatchEpgEndBufferMins() const { return m_catchupWatchEpgEndBufferMins; }
    time_t GetCatchupWatchEpgEndBufferSecs() const { return static_cast<time_t>(m_catchupWatchEpgEndBufferMins) * 60; }
    bool CatchupOnlyOnFinishedProgrammes() const { return m_catchupOnlyOnFinishedProgrammes; }

    bool TransformMulticastStreamUrls() const { return m_transformMulticastStreamUrls; }
    const std::string& GetUdpxyHost() const { return m_udpxyHost; }
    int GetUdpxyPort() const { return m_udpxyPort; }
    bool UseFFmpegReconnect() const { return m_useFFmpegReconnect; }
    bool UseInputstreamAdaptiveforHls() const { return m_useInputstreamAdaptiveforHls; }
    const std::string& GetDefaultUserAgent() const { return m_defaultUserAgent; }
    const std::string& GetDefaultInputstream() const { return m_defaultInputstream; }
    const std::string& GetDefaultMimeType() const { return m_defaultMimeType; }

    const std::string& GetTvgUrl() const { return m_tvgUrl; }
    void SetTvgUrl(const std::string& tvgUrl) { m_tvgUrl = tvgUrl; }

    std::vector<std::string>& GetCustomTVChannelGroupNameList() { return m_customTVChannelGroupNameList; }
    std::vector<std::string>& GetCustomRadioChannelGroupNameList() { return m_customRadioChannelGroupNameList; }

    const std::string GetM3UCacheFilename() { return M3U_CACHE_FILENAME + "-" + std::to_string(m_instanceNumber); }
    const std::string GetXMLTVCacheFilename() { return XMLTV_CACHE_FILENAME + "-" + std::to_string(m_instanceNumber); }

  private:

    template<typename T, typename V>
    V SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue, T& currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      T newValue;
      if (std::is_same<T, float>::value)
        newValue = static_cast<T>(settingValue.GetFloat());
      else if (std::is_same<T, bool>::value)
        newValue = static_cast<T>(settingValue.GetBoolean());
      else if (std::is_same<T, unsigned int>::value)
        newValue = static_cast<T>(settingValue.GetUInt());
      else
        newValue = static_cast<T>(settingValue.GetInt());

      if (newValue != currentValue)
      {
        std::string formatString = "%s - Changed Setting '%s' from %d to %d";
        if (std::is_same<T, float>::value)
          formatString = "%s - Changed Setting '%s' from %f to %f";
        utilities::Logger::Log(utilities::LogLevel::LEVEL_INFO, formatString.c_str(), __FUNCTION__, settingName.c_str(), currentValue, newValue);
        currentValue = newValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    }

    template<typename T, typename V>
    V SetEnumSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue, T& currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      T newValue = settingValue.GetEnum<T>();
      if (newValue != currentValue)
      {
        utilities::Logger::Log(utilities::LogLevel::LEVEL_INFO, "%s - Changed Setting '%s' from %d to %d", __FUNCTION__, settingName.c_str(), currentValue, newValue);
        currentValue = newValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    }

    template<typename V>
    V SetStringSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue, std::string& currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      const std::string strSettingValue = settingValue.GetString();

      if (strSettingValue != currentValue)
      {
        utilities::Logger::Log(utilities::LogLevel::LEVEL_INFO, "%s - Changed Setting '%s' from '%s' to '%s'", __FUNCTION__, settingName.c_str(), currentValue.c_str(), strSettingValue.c_str());
        currentValue = strSettingValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    }

    static bool LoadCustomChannelGroupFile(std::string& file, std::vector<std::string>& channelGroupNameList);

    // M3U
    PathType m_m3uPathType = PathType::REMOTE_PATH;
    std::string m_m3uPath;
    std::string m_m3uUrl;
    bool m_cacheM3U = true;
    int m_startChannelNumber = 1;
    bool m_numberChannelsByM3uOrderOnly = false;
    RefreshMode m_m3uRefreshMode = RefreshMode::DISABLED;
    int m_m3uRefreshIntervalMins = 60;
    int m_m3uRefreshHour = 4;
    std::string m_defaultProviderName;
    bool m_enableProviderMappings = false;
    std::string m_providerMappingFile = DEFAULT_PROVIDER_NAME_MAP_FILE;

    // Groups
    bool m_allowTVChannelGroupsOnly = false;
    ChannelGroupMode m_tvChannelGroupMode = ChannelGroupMode::ALL_GROUPS;
    int m_numTVGroups = DEFAULT_NUM_GROUPS;
    std::string m_oneTVGroup = "";
    std::string m_twoTVGroup = "";
    std::string m_threeTVGroup = "";
    std::string m_fourTVGroup = "";
    std::string m_fiveTVGroup = "";
    std::string m_customTVGroupsFile = DEFAULT_CUSTOM_TV_GROUPS_FILE;
    bool m_allowRadioChannelGroupsOnly = false;
    ChannelGroupMode m_radioChannelGroupMode = ChannelGroupMode::ALL_GROUPS;
    int m_numRadioGroups = DEFAULT_NUM_GROUPS;
    std::string m_oneRadioGroup = "";
    std::string m_twoRadioGroup = "";
    std::string m_threeRadioGroup = "";
    std::string m_fourRadioGroup = "";
    std::string m_fiveRadioGroup = "";
    std::string m_customRadioGroupsFile = DEFAULT_CUSTOM_RADIO_GROUPS_FILE;

    // EPG
    PathType m_epgPathType = PathType::REMOTE_PATH;
    std::string m_epgPath;
    std::string m_epgUrl;
    bool m_cacheEPG = true;
    float m_epgTimeShiftHours = 0.0f;
    bool m_tsOverride = false;
    bool m_ignoreCaseForEpgChannelIds = true;

    // Genres
    bool m_useEpgGenreTextWhenMapping = false;
    PathType m_genresPathType = PathType::LOCAL_PATH;
    std::string m_genresPath = DEFAULT_GENRE_TEXT_MAP_FILE;
    std::string m_genresUrl;

    // Channel Logos
    PathType m_logoPathType = PathType::REMOTE_PATH;
    std::string m_logoPath;
    std::string m_logoBaseUrl;
    EpgLogosMode m_epgLogosMode = EpgLogosMode::PREFER_M3U;
    bool m_useLocalLogosOnly = false;

    // Media
    bool m_mediaEnabled = true;
    bool m_groupMediaByTitle = true;
    bool m_groupMediaBySeason = true;
    bool m_includeShowInfoInMediaTitle = false;
    MediaUseM3UGroupPathMode m_mediaUseM3UGroupPathMode = MediaUseM3UGroupPathMode::IGNORE_GROUP_NAME;
    bool m_mediaForcePlaylist = false;
    bool m_showVodAsRecordings = true;

    // Timeshift
    bool m_timeshiftEnabled = false;
    bool m_timeshiftEnabledAll = true;
    bool m_timeshiftEnabledHttp = true;
    bool m_timeshiftEnabledUdp = true;
    bool m_timeshiftEnabledCustom = false;

    // Catchup
    bool m_catchupEnabled = false;
    std::string m_catchupQueryFormat;
    int m_catchupDays = 3;
    CatchupMode m_allChannelsCatchupMode = CatchupMode::DISABLED;
    CatchupOverrideMode m_catchupOverrideMode = CatchupOverrideMode::WITHOUT_TAGS;
    float m_catchupCorrectionHours = 0;
    bool m_catchupPlayEpgAsLive = false;
    int m_catchupWatchEpgBeginBufferMins = 5;
    int m_catchupWatchEpgEndBufferMins = 15;
    bool m_catchupOnlyOnFinishedProgrammes = false;

    // Advanced
    bool m_transformMulticastStreamUrls = false;
    std::string m_udpxyHost;
    int m_udpxyPort = DEFAULT_UDPXY_MULTICAST_RELAY_PORT;
    bool m_useFFmpegReconnect = true;
    bool m_useInputstreamAdaptiveforHls = false;
    std::string m_defaultUserAgent;
    std::string m_defaultInputstream;
    std::string m_defaultMimeType;

    std::vector<std::string> m_customTVChannelGroupNameList;
    std::vector<std::string> m_customRadioChannelGroupNameList;

    std::string m_tvgUrl;

    kodi::addon::IAddonInstance& m_instance;
    int m_instanceNumber = 0;
  };
} //namespace iptvsimple
