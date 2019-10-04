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

#include "Settings.h"

#include "../client.h"
#include "utilities/FileUtils.h"

using namespace ADDON;
using namespace iptvsimple;
using namespace iptvsimple::utilities;

/***************************************************************************
 * PVR settings
 **************************************************************************/
void Settings::ReadFromAddon(const std::string& userPath, const std::string clientPath)
{
  m_userPath = userPath;
  m_clientPath = clientPath;

  char buffer[1024];

  // M3U
  if (!XBMC->GetSetting("m3uPathType", &m_m3uPathType))
    m_m3uPathType = PathType::REMOTE_PATH;
  if (XBMC->GetSetting("m3uPath", &buffer))
    m_m3uPath = buffer;
  if (XBMC->GetSetting("m3uUrl", &buffer))
    m_m3uUrl = buffer;
  if (!XBMC->GetSetting("m3uCache", &m_cacheM3U))
      m_cacheM3U = true;
  if (!XBMC->GetSetting("startNum", &m_startChannelNumber))
    m_startChannelNumber = 1;
  if (!XBMC->GetSetting("numberByOrder", &m_numberChannelsByM3uOrderOnly))
    m_numberChannelsByM3uOrderOnly = false;
  if (!XBMC->GetSetting("m3uRefreshMode", &m_m3uRefreshMode))
    m_m3uRefreshMode = RefreshMode::DISABLED;
  if (!XBMC->GetSetting("m3uRefreshIntervalMins", &m_m3uRefreshIntervalMins))
    m_m3uRefreshIntervalMins = 60;
  if (!XBMC->GetSetting("m3uRefreshHour", &m_m3uRefreshHour))
    m_m3uRefreshHour = 4;

  // EPG
  if (!XBMC->GetSetting("epgPathType", &m_epgPathType))
    m_epgPathType = PathType::REMOTE_PATH;
  if (XBMC->GetSetting("epgPath", &buffer))
    m_epgPath = buffer;
  if (XBMC->GetSetting("epgUrl", &buffer))
    m_epgUrl = buffer;
  if (!XBMC->GetSetting("epgCache", &m_cacheEPG))
    m_cacheEPG = true;
  if (!XBMC->GetSetting("epgTimeShift", &m_epgTimeShiftHours))
    m_epgTimeShiftHours = 0.0f;
  if (!XBMC->GetSetting("epgTSOverride", &m_tsOverride))
    m_tsOverride = true;

  //Genres
  if (!XBMC->GetSetting("useEpgGenreText", &m_useEpgGenreTextWhenMapping))
    m_useEpgGenreTextWhenMapping = false;
  if (!XBMC->GetSetting("genresPathType", &m_genresPathType))
    m_genresPathType = PathType::LOCAL_PATH;
  if (XBMC->GetSetting("genresPath", &buffer))
    m_genresPath = buffer;
  if (XBMC->GetSetting("genresUrl", &buffer))
    m_genresUrl = buffer;

  // Channel Logos
  if (!XBMC->GetSetting("logoPathType", &m_logoPathType))
    m_logoPathType = PathType::REMOTE_PATH;
  if (XBMC->GetSetting("logoPath", &buffer))
    m_logoPath = buffer;
  if (XBMC->GetSetting("logoBaseUrl", &buffer))
    m_logoBaseUrl = buffer;
  if (!XBMC->GetSetting("logoFromEpg", &m_epgLogosMode))
    m_epgLogosMode = EpgLogosMode::IGNORE_XMLTV;
}

ADDON_STATUS Settings::SetValue(const std::string& settingName, const void* settingValue)
{
  // reset cache and restart addon

  std::string strFile = FileUtils::GetUserDataAddonFilePath(M3U_CACHE_FILENAME);
  if (FileUtils::FileExists(strFile.c_str()))
    FileUtils::DeleteFile(strFile);

  strFile = FileUtils::GetUserDataAddonFilePath(XMLTV_CACHE_FILENAME);
  if (FileUtils::FileExists(strFile.c_str()))
    FileUtils::DeleteFile(strFile);

  // M3U
  if (settingName == "m3uPathType")
    return SetSetting<PathType, ADDON_STATUS>(settingName, settingValue, m_m3uPathType, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "m3uPath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_m3uPath, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "m3uUrl")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_m3uUrl, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "m3uCache")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_cacheM3U, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "startNum")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_startChannelNumber, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "numberByOrder")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_numberChannelsByM3uOrderOnly, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "m3uRefreshMode")
    return SetSetting<RefreshMode, ADDON_STATUS>(settingName, settingValue, m_m3uRefreshMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "m3uRefreshIntervalMins")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_m3uRefreshIntervalMins, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "m3uRefreshHour")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_m3uRefreshHour, ADDON_STATUS_OK, ADDON_STATUS_OK);

  // EPG
  if (settingName == "epgPathType")
    return SetSetting<PathType, ADDON_STATUS>(settingName, settingValue, m_epgPathType, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "epgPath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_epgPath, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "epgUrl")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_epgUrl, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "epgCache")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_cacheEPG, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "epgTimeShift")
    return SetSetting<float, ADDON_STATUS>(settingName, settingValue, m_epgTimeShiftHours, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "epgTSOverride")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_tsOverride, ADDON_STATUS_OK, ADDON_STATUS_OK);

  // Genres
  if (settingName == "useEpgGenreText")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_useEpgGenreTextWhenMapping, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "genresPathType")
    return SetSetting<PathType, ADDON_STATUS>(settingName, settingValue, m_genresPathType, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "genresPath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_genresPath, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "genresUrl")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_genresUrl, ADDON_STATUS_OK, ADDON_STATUS_OK);

  // Channel Logos
  if (settingName == "logoPathType")
    return SetSetting<PathType, ADDON_STATUS>(settingName, settingValue, m_logoPathType, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "logoPath")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_logoPath, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "logoBaseUrl")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_logoBaseUrl, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "logoFromEpg")
    return SetSetting<EpgLogosMode, ADDON_STATUS>(settingName, settingValue, m_epgLogosMode, ADDON_STATUS_OK, ADDON_STATUS_OK);

  return ADDON_STATUS_OK;
}
