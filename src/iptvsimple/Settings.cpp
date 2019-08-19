#include "Settings.h"

#include "../client.h"
#include "utilities/FileUtils.h"

using namespace ADDON;
using namespace iptvsimple;
using namespace iptvsimple::utilities;

#ifdef TARGET_WINDOWS
#define snprintf _snprintf
#ifdef CreateDirectory
#undef CreateDirectory
#endif
#ifdef DeleteFile
#undef DeleteFile
#endif
#endif

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

  // EPG
  if (!XBMC->GetSetting("epgPathType", &m_epgPathType))
    m_epgPathType = PathType::REMOTE_PATH;
  if (XBMC->GetSetting("epgPath", &buffer))
    m_epgPath = buffer;
  if (XBMC->GetSetting("epgUrl", &buffer))
    m_epgUrl = buffer;
  if (!XBMC->GetSetting("epgCache", &m_cacheEPG))
    m_cacheEPG = true;
  if (!XBMC->GetSetting("epgTimeShift", &m_epgTimeShiftMins))
    m_epgTimeShiftMins = 0;
  if (!XBMC->GetSetting("epgTSOverride", &m_tsOverride))
    m_tsOverride = true;

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

  std::string strFile = FileUtils::GetUserFilePath(M3U_FILE_NAME);
  if (XBMC->FileExists(strFile.c_str(), false))
    XBMC->DeleteFile(strFile.c_str());

  strFile = FileUtils::GetUserFilePath(TVG_FILE_NAME);
  if (XBMC->FileExists(strFile.c_str(), false))
    XBMC->DeleteFile(strFile.c_str());

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
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_epgTimeShiftMins, ADDON_STATUS_OK, ADDON_STATUS_OK);
  if (settingName == "epgTSOverride")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_tsOverride, ADDON_STATUS_OK, ADDON_STATUS_OK);

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
