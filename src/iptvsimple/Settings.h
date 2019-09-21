#pragma once
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

#include "utilities/Logger.h"

#include <string>

#include <kodi/xbmc_addon_types.h>

namespace iptvsimple
{
  static const std::string M3U_CACHE_FILENAME = "iptv.m3u.cache";
  static const std::string XMLTV_CACHE_FILENAME = "xmltv.xml.cache";
  static const std::string ADDON_DATA_BASE_DIR = "special://userdata/addon_data/pvr.iptvsimple";
  static const std::string DEFAULT_GENRE_TEXT_MAP_FILE = ADDON_DATA_BASE_DIR + "/genres/genreTextMappings/genres.xml";

  enum class PathType
    : int // same type as addon settings
  {
    LOCAL_PATH = 0,
    REMOTE_PATH
  };

  enum class EpgLogosMode
    : int // same type as addon settings
  {
    IGNORE_XMLTV = 0,
    PREFER_M3U,
    PREFER_XMLTV
  };

  class Settings
  {
  public:
    /**
     * Singleton getter for the instance
     */
    static Settings& GetInstance()
    {
      static Settings settings;
      return settings;
    }

    void ReadFromAddon(const std::string& userPath, const std::string clientPath);
    ADDON_STATUS SetValue(const std::string& settingName, const void* settingValue);

    const std::string& GetUserPath() const { return m_userPath; }
    const std::string& GetClientPath() const { return m_clientPath; }

    const std::string& GetM3ULocation() const { return m_m3uPathType == PathType::REMOTE_PATH ? m_m3uUrl : m_m3uPath; }
    const PathType& GetM3UPathType() const { return m_m3uPathType; }
    const std::string& GetM3UPath() const { return m_m3uPath; }
    const std::string& GetM3UUrl() const { return m_m3uUrl; }
    bool UseM3UCache() const { return m_m3uPathType == PathType::REMOTE_PATH ? m_cacheM3U : false; }
    int GetStartChannelNumber() const { return m_startChannelNumber; }
    bool NumberChannelsByM3uOrderOnly() const { return m_numberChannelsByM3uOrderOnly; }

    const std::string& GetEpgLocation() const { return m_epgPathType == PathType::REMOTE_PATH ? m_epgUrl : m_epgPath; }
    const PathType& GetEpgPathType() const { return m_epgPathType; }
    const std::string& GetEpgPath() const { return m_epgPath; }
    const std::string& GetEpgUrl() const { return m_epgUrl; }
    bool UseEPGCache() const { return m_epgPathType == PathType::REMOTE_PATH ? m_cacheEPG : false; }
    int GetEpgTimeshiftMins() const { return m_epgTimeShiftMins; }
    int GetEpgTimeshiftSecs() const { return m_epgTimeShiftMins * 60; }
    bool GetTsOverride() const { return m_tsOverride; }

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

  private:
    Settings() = default;

    Settings(Settings const&) = delete;
    void operator=(Settings const&) = delete;

    template <typename T, typename V>
    V SetSetting(const std::string& settingName, const void* settingValue, T& currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      T newValue =  *static_cast<const T*>(settingValue);
      if (newValue != currentValue)
      {
        utilities::Logger::Log(utilities::LogLevel::LEVEL_NOTICE, "%s - Changed Setting '%s' from %d to %d", __FUNCTION__, settingName.c_str(), currentValue, newValue);
        currentValue = newValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    };

    template <typename V>
    V SetStringSetting(const std::string &settingName, const void* settingValue, std::string &currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      const std::string strSettingValue = static_cast<const char*>(settingValue);

      if (strSettingValue != currentValue)
      {
        utilities::Logger::Log(utilities::LogLevel::LEVEL_NOTICE, "%s - Changed Setting '%s' from '%s' to '%s'", __FUNCTION__, settingName.c_str(), currentValue.c_str(), strSettingValue.c_str());
        currentValue = strSettingValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    }

    std::string m_userPath;
    std::string m_clientPath;

    PathType m_m3uPathType = PathType::REMOTE_PATH;
    std::string m_m3uPath;
    std::string m_m3uUrl;
    bool m_cacheM3U = false;
    int m_startChannelNumber = 1;
    bool m_numberChannelsByM3uOrderOnly = false;

    PathType m_epgPathType = PathType::REMOTE_PATH;
    std::string m_epgPath;
    std::string m_epgUrl;
    bool m_cacheEPG = false;
    int m_epgTimeShiftMins = 0;
    bool m_tsOverride = true;

    bool m_useEpgGenreTextWhenMapping = false;
    PathType m_genresPathType = PathType::LOCAL_PATH;
    std::string m_genresPath;
    std::string m_genresUrl;

    PathType m_logoPathType = PathType::REMOTE_PATH;
    std::string m_logoPath;
    std::string m_logoBaseUrl;
    EpgLogosMode m_epgLogosMode = EpgLogosMode::IGNORE_XMLTV;
  };
} //namespace iptvsimple
