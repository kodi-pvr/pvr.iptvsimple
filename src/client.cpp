/*
 *      Copyright (C) 2013-2015 Anton Fedchin
 *      http://github.com/afedchin/xbmc-addon-iptvsimple/
 *
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "client.h"

#include "PVRIptvData.h"
#include "iptvsimple/Settings.h"
#include "iptvsimple/data/Channel.h"
#include "iptvsimple/utilities/Logger.h"
#include "iptvsimple/utilities/StreamUtils.h"
#include "iptvsimple/utilities/WebUtils.h"

#include <algorithm>

#include <kodi/xbmc_pvr_dll.h>
#include <p8-platform/util/StringUtils.h>

using namespace ADDON;
using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

bool m_created = false;
ADDON_STATUS m_currentStatus = ADDON_STATUS_UNKNOWN;
PVRIptvData* m_data = nullptr;
Channel m_currentChannel;
Settings& settings = Settings::GetInstance();
CatchupController* m_catchupController = nullptr;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */

CHelper_libXBMC_addon* XBMC = nullptr;
CHelper_libXBMC_pvr* PVR = nullptr;

template<typename T> void SafeDelete(T*& p)
{
  if (p)
  {
    delete p;
    p = nullptr;
  }
}

extern "C"
{
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
  {
    return ADDON_STATUS_UNKNOWN;
  }

  PVR_PROPERTIES* pvrprops = static_cast<PVR_PROPERTIES*>(props);

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    SafeDelete(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SafeDelete(PVR);
    SafeDelete(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  /* Configure the logger */
  Logger::GetInstance().SetImplementation([](LogLevel level, const char* message)
  {
    /* Convert the log level */
    addon_log_t addonLevel;

    switch (level)
    {
      case LogLevel::LEVEL_ERROR:
        addonLevel = addon_log_t::LOG_ERROR;
        break;
      case LogLevel::LEVEL_INFO:
        addonLevel = addon_log_t::LOG_INFO;
        break;
      case LogLevel::LEVEL_NOTICE:
        addonLevel = addon_log_t::LOG_NOTICE;
        break;
      default:
        addonLevel = addon_log_t::LOG_DEBUG;
    }

    XBMC->Log(addonLevel, "%s", message);
  });

  Logger::GetInstance().SetPrefix("pvr.iptvsimple");

  Logger::Log(LogLevel::LEVEL_INFO, "%s - Creating the PVR IPTV Simple add-on", __FUNCTION__);

  m_currentStatus = ADDON_STATUS_UNKNOWN;
  const std::string userPath = pvrprops->strUserPath;
  const std::string clientPath = pvrprops->strClientPath;

  settings.ReadFromAddon(userPath, clientPath);

  m_data = new PVRIptvData;
  if (!m_data->Start())
  {
    SafeDelete(m_data);
    SafeDelete(PVR);
    SafeDelete(XBMC);
    m_currentStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_currentStatus;
  }
  m_catchupController = &m_data->GetCatchupController();

  m_currentStatus = ADDON_STATUS_OK;
  m_created = true;

  return m_currentStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_currentStatus;
}

void ADDON_Destroy()
{
  delete m_data;
  m_created = false;
  m_currentStatus = ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS ADDON_SetSetting(const char* settingName, const void* settingValue)
{
  if (!XBMC || !m_data)
    return ADDON_STATUS_OK;

  return m_data->SetSetting(settingName, settingValue);
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

void OnSystemSleep() {}

void OnSystemWake() {}

void OnPowerSavingActivated() {}

void OnPowerSavingDeactivated() {}

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsEPG = true;
  pCapabilities->bSupportsTV = true;
  pCapabilities->bSupportsRadio = true;
  pCapabilities->bSupportsChannelGroups = true;
  pCapabilities->bSupportsRecordings = false;
  pCapabilities->bSupportsRecordingsRename = false;
  pCapabilities->bSupportsRecordingsLifetimeChange = false;
  pCapabilities->bSupportsDescrambleInfo = false;

  return PVR_ERROR_NO_ERROR;
}

const char* GetBackendName(void)
{
  static const char* backendName = "IPTV Simple PVR Add-on";
  return backendName;
}

const char* GetBackendVersion(void)
{
  static std::string backendVersion = STR(IPTV_VERSION);
  return backendVersion.c_str();
}

const char* GetConnectionString(void)
{
  static std::string connectionString = "connected";
  return connectionString.c_str();
}

const char* GetBackendHostname(void)
{
  return "";
}

PVR_ERROR GetDriveSpace(long long* iTotal, long long* iUsed)
{
  *iTotal = 0;
  *iUsed = 0;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd)
{
  if (m_data)
    return m_data->GetEPGForChannel(handle, iChannelUid, iStart, iEnd);

  return PVR_ERROR_SERVER_ERROR;
}

int GetChannelsAmount(void)
{
  if (m_data)
    return m_data->GetChannelsAmount();

  return -1;
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (m_data)
    return m_data->GetChannels(handle, bRadio);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannelStreamProperties(const PVR_CHANNEL* channel, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount)
{
  if (!m_data || !channel || !properties || !iPropertiesCount)
    return PVR_ERROR_SERVER_ERROR;

  if (*iPropertiesCount < 1)
    return PVR_ERROR_INVALID_PARAMETERS;

  if (m_data->GetChannel(*channel, m_currentChannel))
  {
    unsigned int propertiesMax = *iPropertiesCount;
    *iPropertiesCount = 0; // Need to initialise here as current value will be size of properties array

    std::string streamURL = m_currentChannel.GetStreamURL();

    m_catchupController->ResetCatchupState(); // TODO: we need this currently until we have a way to know the stream stops.

    // We always call the catchup controller regardless so it can cleanup state
    // whether or not it supports catchup in case there is any houskeeping to do
    std::map<std::string, std::string> catchupProperties;
    m_catchupController->ProcessChannelForPlayback(m_currentChannel, catchupProperties);

    const std::string catchupUrl = m_catchupController->GetCatchupUrl(m_currentChannel);
    if (!catchupUrl.empty())
      streamURL = catchupUrl;

    StreamUtils::SetAllStreamProperties(properties, iPropertiesCount, propertiesMax, m_currentChannel, streamURL, catchupProperties);

    Logger::Log(LogLevel::LEVEL_NOTICE, "%s - Live %s URL: %s", __FUNCTION__, !catchupUrl.empty() ? "Stream" : "Catchup", streamURL.c_str());

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_SERVER_ERROR;
}

int GetChannelGroupsAmount(void)
{
  if (m_data)
    return m_data->GetChannelGroupsAmount();

  return -1;
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (m_data)
    return m_data->GetChannelGroups(handle, bRadio);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group)
{
  if (m_data)
    return m_data->GetChannelGroupMembers(handle, group);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS& signalStatus)
{
  strncpy(signalStatus.strAdapterName, "IPTV Simple Adapter 1", sizeof(signalStatus.strAdapterName) - 1);
  strncpy(signalStatus.strAdapterStatus, "OK", sizeof(signalStatus.strAdapterStatus) - 1);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IsEPGTagPlayable(const EPG_TAG* tag, bool* bIsPlayable)
{
  if (!m_data)
    return PVR_ERROR_SERVER_ERROR;
  if (!settings.IsCatchupEnabled())
    return PVR_ERROR_NOT_IMPLEMENTED;

  const time_t now = std::time(nullptr);
  Channel channel;

  // Get the channel and set the current tag on it if found
  *bIsPlayable = m_data->GetChannel(static_cast<int>(tag->iUniqueChannelId), channel);

  *bIsPlayable = *bIsPlayable &&
                 settings.IsCatchupEnabled() &&
                 tag->startTime < now &&
                 channel.IsCatchupSupported() &&
                 tag->startTime >= (now - static_cast<time_t>(channel.GetCatchupDaysInSeconds())) &&
                 (!settings.CatchupOnlyOnFinishedProgrammes() || tag->endTime < now);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetEPGTagStreamProperties(const EPG_TAG* tag, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount)
{
  Logger::Log(LEVEL_DEBUG, "%s - Tag startTime: %ld \tendTime: %ld", __FUNCTION__, tag->startTime, tag->endTime);

  if (!m_data || !tag || !properties || !iPropertiesCount)
    return PVR_ERROR_SERVER_ERROR;

  if (*iPropertiesCount < 1)
    return PVR_ERROR_INVALID_PARAMETERS;

  if (m_data->GetChannel(static_cast<int>(tag->iUniqueChannelId), m_currentChannel))
  {
    unsigned int propertiesMax = *iPropertiesCount;
    *iPropertiesCount = 0;
    Logger::Log(LEVEL_DEBUG, "%s - GetPlayEpgAsLive is %s", __FUNCTION__, settings.CatchupPlayEpgAsLive() ? "enabled" : "disabled");

    std::map<std::string, std::string> catchupProperties;
    if (settings.CatchupPlayEpgAsLive())
    {
      m_catchupController->ProcessEPGTagForTimeshiftedPlayback(*tag, m_currentChannel, catchupProperties);
    }
    else
    {
      m_catchupController->ResetCatchupState(); // TODO: we need this currently until we have a way to know the stream stops.
      m_catchupController->ProcessEPGTagForVideoPlayback(*tag, m_currentChannel, catchupProperties);
    }

    const std::string catchupUrl = m_catchupController->GetCatchupUrl(m_currentChannel);
    if (!catchupUrl.empty())
    {
      StreamUtils::SetAllStreamProperties(properties, iPropertiesCount, propertiesMax, m_currentChannel, catchupUrl, catchupProperties);

      Logger::Log(LEVEL_NOTICE, "%s - EPG Catchup URL: %s", __FUNCTION__, catchupUrl.c_str());
      return PVR_ERROR_NO_ERROR;
    }
  }

  return PVR_ERROR_FAILED;
}

/** UNUSED API FUNCTIONS */
bool CanPauseStream(void) { return false; }
int GetRecordingsAmount(bool deleted) { return -1; }
PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetRecordingStreamProperties(const PVR_RECORDING*, PVR_NAMED_VALUE*, unsigned int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK& menuhook, const PVR_MENUHOOK_DATA& item) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL& channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL& channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL& channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL& channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
void CloseLiveStream(void) {}
bool OpenRecordedStream(const PVR_RECORDING& recording) { return false; }
bool OpenLiveStream(const PVR_CHANNEL& channel) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char* pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) { return 0; }
long long LengthRecordedStream(void) { return 0; }
void DemuxReset(void) {}
void DemuxFlush(void) {}
void FillBuffer(bool mode) {}
int ReadLiveStream(unsigned char* pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long LengthLiveStream(void) { return -1; }
PVR_ERROR DeleteRecording(const PVR_RECORDING& recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameRecording(const PVR_RECORDING& recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING& recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING& recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING& recording) { return -1; }
PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; };
PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int* size) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetTimersAmount(void) { return -1; }
PVR_ERROR GetTimers(ADDON_HANDLE handle) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR AddTimer(const PVR_TIMER& timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteTimer(const PVR_TIMER& timer, bool bForceDelete) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR UpdateTimer(const PVR_TIMER& timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxAbort(void) {}
DemuxPacket* DemuxRead(void) { return nullptr; }
bool IsRealTimeStream(void) { return true; }
void PauseStream(bool bPaused) {}
bool CanSeekStream(void) { return false; }
bool SeekTime(double, bool, double*) { return false; }
void SetSpeed(int){};
PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteAllRecordingsFromTrash() { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetEPGTimeFrame(int) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetDescrambleInfo(PVR_DESCRAMBLE_INFO*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLifetime(const PVR_RECORDING*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR IsEPGTagRecordable(const EPG_TAG*, bool*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetEPGTagEdl(const EPG_TAG* epgTag, PVR_EDL_ENTRY edl[], int* size) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetStreamReadChunkSize(int* chunksize) { return PVR_ERROR_NOT_IMPLEMENTED; }
} // extern "C"
