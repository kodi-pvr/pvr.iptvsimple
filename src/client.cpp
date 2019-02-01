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
#include "xbmc_pvr_dll.h"
#include "PVRIptvData.h"
#include "p8-platform/util/util.h"

#include "iptvsimple/FileStreamReader.h"
#include "iptvsimple/LocalizedString.h"
#include "iptvsimple/M3U8Streamer.h"
#include "iptvsimple/StreamReader.h"
#include "iptvsimple/TimeshiftBuffer.h"
#include "iptvsimple/hls/HlsParser.h"

#include <stdlib.h>

using namespace ADDON;
using namespace iptvsimple;
using namespace iptvsimple::hls;

#ifdef TARGET_WINDOWS
#define snprintf _snprintf
#ifdef CreateDirectory
#undef CreateDirectory
#endif
#ifdef DeleteFile
#undef DeleteFile
#endif
#endif

bool           m_bCreated       = false;
ADDON_STATUS   m_CurStatus      = ADDON_STATUS_UNKNOWN;
PVRIptvData    *m_data          = NULL;
PVRIptvChannel m_currentChannel;
IStreamReader  *strReader  = nullptr;
M3U8Streamer   *m_m3u8Streamer = nullptr;
int            m_streamReadChunkSize = 0;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string g_strUserPath   = "";
std::string g_strClientPath = "";

CHelper_libXBMC_addon *XBMC = NULL;
CHelper_libXBMC_pvr   *PVR  = NULL;

std::string g_strTvgPath    = "";
std::string g_strM3UPath    = "";
std::string g_strLogoPath   = "";
int         g_iEPGTimeShift = 0;
int         g_iStartNumber  = 1;
bool        g_bTSOverride   = true;
bool        g_bCacheM3U     = false;
bool        g_bCacheEPG     = false;
int         g_iEPGLogos     = 0;
int         g_iEnableTimeshift        = TIMESHIFT_OFF;
std::string g_strTimeshiftBufferPath  = DEFAULT_TSBUFFERPATH;
int         g_iReadTimeout            = 0;

extern std::string PathCombine(const std::string &strPath, const std::string &strFileName)
{
  std::string strResult = strPath;
  if (strResult.at(strResult.size() - 1) == '\\' ||
      strResult.at(strResult.size() - 1) == '/')
  {
    strResult.append(strFileName);
  }
  else
  {
    strResult.append("/");
    strResult.append(strFileName);
  }

  return strResult;
}

extern std::string GetClientFilePath(const std::string &strFileName)
{
  return PathCombine(g_strClientPath, strFileName);
}

extern std::string GetUserFilePath(const std::string &strFileName)
{
  return PathCombine(g_strUserPath, strFileName);
}

extern "C" {

void ADDON_ReadSettings(void)
{
  char buffer[1024];
  int iPathType = 0;
  if (!XBMC->GetSetting("m3uPathType", &iPathType))
  {
    iPathType = 1;
  }
  if (iPathType)
  {
    if (XBMC->GetSetting("m3uUrl", &buffer))
    {
      g_strM3UPath = buffer;
    }
    if (!XBMC->GetSetting("m3uCache", &g_bCacheM3U))
    {
      g_bCacheM3U = true;
    }
  }
  else
  {
    if (XBMC->GetSetting("m3uPath", &buffer))
    {
      g_strM3UPath = buffer;
    }
    g_bCacheM3U = false;
  }
  if (!XBMC->GetSetting("startNum", &g_iStartNumber))
  {
    g_iStartNumber = 1;
  }
  if (!XBMC->GetSetting("epgPathType", &iPathType))
  {
    iPathType = 1;
  }
  if (iPathType)
  {
    if (XBMC->GetSetting("epgUrl", &buffer))
    {
      g_strTvgPath = buffer;
    }
    if (!XBMC->GetSetting("epgCache", &g_bCacheEPG))
    {
      g_bCacheEPG = true;
    }
  }
  else
  {
    if (XBMC->GetSetting("epgPath", &buffer))
    {
      g_strTvgPath = buffer;
    }
    g_bCacheEPG = false;
  }
  float fShift;
  if (XBMC->GetSetting("epgTimeShift", &fShift))
  {
    g_iEPGTimeShift = (int)(fShift * 3600.0); // hours to seconds
  }
  if (!XBMC->GetSetting("epgTSOverride", &g_bTSOverride))
  {
    g_bTSOverride = true;
  }
  if (!XBMC->GetSetting("logoPathType", &iPathType))
  {
    iPathType = 1;
  }
  if (XBMC->GetSetting(iPathType ? "logoBaseUrl" : "logoPath", &buffer))
  {
    g_strLogoPath = buffer;
  }

  // Logos from EPG
  if (!XBMC->GetSetting("logoFromEpg", &g_iEPGLogos))
    g_iEPGLogos = 0;

  // read setting "enabletimeshift" from settings.xml
  if (!XBMC->GetSetting("enabletimeshift", &g_iEnableTimeshift))
    g_iEnableTimeshift = TIMESHIFT_OFF;

    // read setting "timeshiftbufferpath" from settings.xml
  if (XBMC->GetSetting("timeshiftbufferpath", buffer) && !std::string(buffer).empty())
    g_strTimeshiftBufferPath = buffer;
  else
    g_strTimeshiftBufferPath = DEFAULT_TSBUFFERPATH;

  // read setting "enabletimeshift" from settings.xml
  if (!XBMC->GetSetting("readtimeout", &g_iReadTimeout))
    g_iReadTimeout = 0;
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
  {
    return ADDON_STATUS_UNKNOWN;
  }

  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  XBMC->Log(LOG_DEBUG, "%s - Creating the PVR IPTV Simple add-on", __FUNCTION__);

  m_CurStatus     = ADDON_STATUS_UNKNOWN;
  g_strUserPath   = pvrprops->strUserPath;
  g_strClientPath = pvrprops->strClientPath;

  if (!XBMC->DirectoryExists(g_strUserPath.c_str()))
  {
    XBMC->CreateDirectory(g_strUserPath.c_str());
  }

  ADDON_ReadSettings();

  m_data = new PVRIptvData;
  m_CurStatus = ADDON_STATUS_OK;
  m_bCreated = true;

  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  delete m_data;
  m_bCreated = false;
  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  std::string str = settingName;
  // reset cache and restart addon

  std::string strFile = GetUserFilePath(M3U_FILE_NAME);
  if (XBMC->FileExists(strFile.c_str(), false))
  {
    XBMC->DeleteFile(strFile.c_str());
  }

  strFile = GetUserFilePath(TVG_FILE_NAME);
  if (XBMC->FileExists(strFile.c_str(), false))
  {
    XBMC->DeleteFile(strFile.c_str());
  }

  if (str == "enabletimeshift")
  {
    int iNewValue = *(int*) settingValue;

    if (g_iEnableTimeshift != iNewValue)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'enabletimeshift' from %u to %u", __FUNCTION__, g_iEnableTimeshift, iNewValue);
      g_iEnableTimeshift = iNewValue;
      return ADDON_STATUS_OK;
    }
  }

  return ADDON_STATUS_NEED_RESTART;
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

void OnSystemSleep()
{
}

void OnSystemWake()
{
}

void OnPowerSavingActivated()
{
}

void OnPowerSavingDeactivated()
{
}

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsEPG             = true;
  pCapabilities->bSupportsTV              = true;
  pCapabilities->bSupportsRadio           = true;
  pCapabilities->bSupportsChannelGroups   = true;
  pCapabilities->bSupportsRecordings      = false;
  pCapabilities->bSupportsRecordingsRename = false;
  pCapabilities->bSupportsRecordingsLifetimeChange = false;
  pCapabilities->bSupportsDescrambleInfo = false;
  pCapabilities->bHandlesInputStream     = true;
  pCapabilities->bHandlesDemuxing = false;

  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const char *strBackendName = "IPTV Simple PVR Add-on";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static std::string strBackendVersion = STR(IPTV_VERSION);
  return strBackendVersion.c_str();
}

const char *GetConnectionString(void)
{
  static std::string strConnectionString = "connected";
  return strConnectionString.c_str();
}

const char *GetBackendHostname(void)
{
  return "";
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  *iTotal = 0;
  *iUsed  = 0;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (m_data)
    return m_data->GetEPGForChannel(handle, channel, iStart, iEnd);

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
  if (g_iEnableTimeshift != TIMESHIFT_OFF) // Only use an input stream if timeshift is enabled
    return PVR_ERROR_NOT_IMPLEMENTED;

  if (!channel || !properties || !iPropertiesCount)
    return PVR_ERROR_SERVER_ERROR;

  if (*iPropertiesCount < 1)
    return PVR_ERROR_INVALID_PARAMETERS;

  if (m_data && m_data->GetChannel(*channel, m_currentChannel))
  {
    strncpy(properties[0].strName, PVR_STREAM_PROPERTY_STREAMURL, sizeof(properties[0].strName) - 1);
    strncpy(properties[0].strValue, m_currentChannel.strStreamURL.c_str(), sizeof(properties[0].strValue) - 1);
    *iPropertiesCount = 1;
    if (!m_currentChannel.properties.empty())
    {
      for (auto& prop : m_currentChannel.properties)
      {
        strncpy(properties[*iPropertiesCount].strName, prop.first.c_str(),
                sizeof(properties[*iPropertiesCount].strName) - 1);
        strncpy(properties[*iPropertiesCount].strValue, prop.second.c_str(),
                sizeof(properties[*iPropertiesCount].strName) - 1);
        (*iPropertiesCount)++;
      }
    }
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

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (m_data)
    return m_data->GetChannelGroupMembers(handle, group);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  snprintf(signalStatus.strAdapterName, sizeof(signalStatus.strAdapterName), "IPTV Simple Adapter 1");
  snprintf(signalStatus.strAdapterStatus, sizeof(signalStatus.strAdapterStatus), "OK");

  return PVR_ERROR_NO_ERROR;
}

//Timeshifting
PVR_ERROR GetStreamReadChunkSize(int* chunksize)
{
  if (!chunksize)
    return PVR_ERROR_INVALID_PARAMETERS;
  *chunksize = m_streamReadChunkSize;
  return PVR_ERROR_NO_ERROR;
}

/* live stream functions */
bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (!m_data)
    return false;

  /* queue a warning if the timeshift buffer path does not exist */
  if (g_iEnableTimeshift != TIMESHIFT_OFF
      && !XBMC->DirectoryExists(g_strTimeshiftBufferPath.c_str()))
    XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30500).c_str());

  m_data->GetChannel(channel, m_currentChannel); //Copy channel details

  XBMC->Log(LOG_NOTICE, "%s Stream URL: %s", __FUNCTION__, m_currentChannel.strStreamURL.c_str());

  if (HlsParser::IsValidM3U8Url(m_currentChannel.strStreamURL))
  {
    m_m3u8Streamer = new M3U8Streamer(m_currentChannel.strStreamURL, g_iReadTimeout);
    bool streamReady = m_m3u8Streamer->Start();

    if (!streamReady)
    {
      XBMC->QueueNotification(QUEUE_ERROR, LocalizedString(30501).c_str(), m_currentChannel.strChannelName.c_str());

      m_m3u8Streamer->Stop();
      SAFE_DELETE(m_m3u8Streamer);

      return false;
    }
    else
    {
      strReader = new StreamReader(m_m3u8Streamer);
      if (g_iEnableTimeshift == TIMESHIFT_ON_PLAYBACK)
        strReader = new TimeshiftBuffer(strReader, g_strTimeshiftBufferPath, g_iReadTimeout);

      return strReader->Start();
    }
  }
  else
  {
    strReader = new FileStreamReader(m_currentChannel.strStreamURL, g_iReadTimeout);
    if (g_iEnableTimeshift == TIMESHIFT_ON_PLAYBACK)
      strReader = new TimeshiftBuffer(strReader, g_strTimeshiftBufferPath, g_iReadTimeout);

    return strReader->Start();
  }
}

void CloseLiveStream(void)
{
  SAFE_DELETE(strReader);
  SAFE_DELETE(m_m3u8Streamer);
}

bool IsRealTimeStream()
{
  return (strReader) ? strReader->IsRealTime() : false;
}

bool CanPauseStream(void)
{
  if (!m_data)
    return false;

  if (g_iEnableTimeshift != TIMESHIFT_OFF && strReader)
    return (strReader->IsTimeshifting() || XBMC->DirectoryExists(g_strTimeshiftBufferPath.c_str()));

  return false;
}

bool CanSeekStream(void)
{
  if (!m_data)
    return false;

  return (g_iEnableTimeshift != TIMESHIFT_OFF);
}

int ReadLiveStream(unsigned char *buffer, unsigned int size)
{
  return (strReader) ? strReader->ReadData(buffer, size) : 0;
}

long long SeekLiveStream(long long position, int whence)
{
  return (strReader) ? strReader->Seek(position, whence) : -1;
}

long long LengthLiveStream(void)
{
  return (strReader) ? strReader->Length() : -1;
}

bool IsTimeshifting(void)
{
  return (strReader && strReader->IsTimeshifting());
}

PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES *times)
{
  if (!times)
    return PVR_ERROR_INVALID_PARAMETERS;
  if (strReader)
  {
    times->startTime = strReader->TimeStart();
    times->ptsStart  = 0;
    times->ptsBegin  = 0;
    times->ptsEnd    = (!strReader->IsTimeshifting()) ? 0
      : (strReader->TimeEnd() - strReader->TimeStart()) * DVD_TIME_BASE;

    return PVR_ERROR_NO_ERROR;
  }
  return PVR_ERROR_NOT_IMPLEMENTED;
}

void PauseStream(bool paused)
{
  if (!m_data)
    return;

  /* start timeshift on pause */
  if (paused && g_iEnableTimeshift == TIMESHIFT_ON_PAUSE
      && strReader && !strReader->IsTimeshifting()
      && XBMC->DirectoryExists(g_strTimeshiftBufferPath.c_str()))
  {
    strReader = new TimeshiftBuffer(strReader, g_strTimeshiftBufferPath ,g_iReadTimeout);
    (void)strReader->Start();
  }
}

/** UNUSED API FUNCTIONS */
int GetRecordingsAmount(bool deleted) { return -1; }
PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetRecordingStreamProperties(const PVR_RECORDING*, PVR_NAMED_VALUE*, unsigned int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool OpenRecordedStream(const PVR_RECORDING &recording) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) { return 0; }
long long LengthRecordedStream(void) { return 0; }
void DemuxReset(void) {}
void DemuxFlush(void) {}
PVR_ERROR DeleteRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return -1; }
PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; };
PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int *size) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetTimersAmount(void) { return -1; }
PVR_ERROR GetTimers(ADDON_HANDLE handle) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR AddTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR UpdateTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxAbort(void) {}
DemuxPacket* DemuxRead(void) { return NULL; }
bool SeekTime(double,bool,double*) { return false; }
void SetSpeed(int) {};
PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteAllRecordingsFromTrash() { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetEPGTimeFrame(int) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetDescrambleInfo(PVR_DESCRAMBLE_INFO*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLifetime(const PVR_RECORDING*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR IsEPGTagRecordable(const EPG_TAG*, bool*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR IsEPGTagPlayable(const EPG_TAG*, bool*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetEPGTagStreamProperties(const EPG_TAG*, PVR_NAMED_VALUE*, unsigned int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetEPGTagEdl(const EPG_TAG* epgTag, PVR_EDL_ENTRY edl[], int *size) { return PVR_ERROR_NOT_IMPLEMENTED; }

} // extern "C"
