/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "IptvSimple.h"

#include "iptvsimple/InstanceSettings.h"
#include "iptvsimple/utilities/Logger.h"
#include "iptvsimple/utilities/TimeUtils.h"
#include "iptvsimple/utilities/WebUtils.h"

#include <ctime>
#include <chrono>

#include <kodi/tools/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;
using namespace kodi::tools;

IptvSimple::IptvSimple(const kodi::addon::IInstanceInfo& instance) : kodi::addon::CInstancePVRClient(instance), m_settings(new InstanceSettings(*this, instance))
{
  m_channels.Clear();
  m_channelGroups.Clear();
  m_providers.Clear();
  m_epg.Clear();
  m_media.Clear();
}

bool IptvSimple::Initialise()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_channels.Init();
  m_channelGroups.Init();
  m_providers.Init();
  m_playlistLoader.Init();
  if (!m_playlistLoader.LoadPlayList())
  {
    m_channels.ChannelsLoadFailed();
    m_channelGroups.ChannelGroupsLoadFailed();
  }
  m_epg.Init(EpgMaxPastDays(), EpgMaxFutureDays());

  kodi::Log(ADDON_LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);

  m_running = true;
  m_thread = std::thread([&] { Process(); });

  return true;
}

PVR_ERROR IptvSimple::GetCapabilities(kodi::addon::PVRCapabilities& capabilities)
{
  capabilities.SetSupportsEPG(true);
  capabilities.SetSupportsTV(true);
  capabilities.SetSupportsRadio(true);
  capabilities.SetSupportsChannelGroups(true);
  capabilities.SetSupportsProviders(true);
  capabilities.SetSupportsRecordingsRename(false);
  capabilities.SetSupportsRecordingsLifetimeChange(false);
  capabilities.SetSupportsDescrambleInfo(false);
  capabilities.SetSupportsRecordings(true);
  capabilities.SetSupportsRecordingsDelete(false);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::GetBackendName(std::string& name)
{
  name = "IPTV Simple";
  return PVR_ERROR_NO_ERROR;
}
PVR_ERROR IptvSimple::GetBackendVersion(std::string& version)
{
  // Some linux platform require the full string initialisation here to compile. No idea why.
  version = std::string(STR(IPTV_VERSION));
  return PVR_ERROR_NO_ERROR;
}
PVR_ERROR IptvSimple::GetConnectionString(std::string& connection)
{
  connection = "connected";
  return PVR_ERROR_NO_ERROR;
}

void IptvSimple::Process()
{
  unsigned int refreshTimer = 0;
  time_t lastRefreshTimeSeconds = std::time(nullptr);
  int lastRefreshHour = m_settings->GetM3URefreshHour(); //ignore if we start during same hour

  while (m_running)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(PROCESS_LOOP_WAIT_SECS * 1000));

    time_t currentRefreshTimeSeconds = std::time(nullptr);
    std::tm timeInfo = SafeLocaltime(currentRefreshTimeSeconds);
    refreshTimer += static_cast<unsigned int>(currentRefreshTimeSeconds - lastRefreshTimeSeconds);
    lastRefreshTimeSeconds = currentRefreshTimeSeconds;

    if (m_settings->GetM3URefreshMode() == RefreshMode::REPEATED_REFRESH &&
        refreshTimer >= (m_settings->GetM3URefreshIntervalMins() * 60))
      m_reloadChannelsGroupsAndEPG = true;

    if (m_settings->GetM3URefreshMode() == RefreshMode::ONCE_PER_DAY &&
        lastRefreshHour != timeInfo.tm_hour && timeInfo.tm_hour == m_settings->GetM3URefreshHour())
      m_reloadChannelsGroupsAndEPG = true;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running && m_reloadChannelsGroupsAndEPG)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      m_settings->ReloadAddonInstanceSettings();
      m_playlistLoader.ReloadPlayList();
      m_epg.ReloadEPG(); // Reloading EPG also updates media

      m_reloadChannelsGroupsAndEPG = false;
      refreshTimer = 0;
    }
    lastRefreshHour = timeInfo.tm_hour;
  }
}

IptvSimple::~IptvSimple()
{
  Logger::Log(LEVEL_DEBUG, "%s Stopping update thread...", __FUNCTION__);
  m_running = false;
  if (m_thread.joinable())
    m_thread.join();

  std::lock_guard<std::mutex> lock(m_mutex);
  m_channels.Clear();
  m_channelGroups.Clear();
  m_providers.Clear();
  m_epg.Clear();
}

/***************************************************************************
 * Providers
 **************************************************************************/

PVR_ERROR IptvSimple::GetProvidersAmount(int& amount)
{
  amount = m_providers.GetNumProviders();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::GetProviders(kodi::addon::PVRProvidersResultSet& results)
{
  std::vector<kodi::addon::PVRProvider> providers;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_providers.GetProviders(providers);
  }

  Logger::Log(LEVEL_DEBUG, "%s - providers available '%d'", __func__, providers.size());

  for (const auto& provider : providers)
    results.Add(provider);

  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * Channels
 **************************************************************************/

PVR_ERROR IptvSimple::GetChannelsAmount(int& amount)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  amount = m_channels.GetChannelsAmount();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channels.GetChannels(results, radio);
}

PVR_ERROR IptvSimple::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel, std::vector<kodi::addon::PVRStreamProperty>& properties)
{
  if (GetChannel(channel, m_currentChannel))
  {
    std::string streamURL = m_currentChannel.GetStreamURL();

    m_catchupController.ResetCatchupState(); // TODO: we need this currently until we have a way to know the stream stops.

    // We always call the catchup controller regardless so it can cleanup state
    // whether or not it supports catchup in case there is any houskeeping to do
    // This also allows us to check if this is a catchup stream or not when we try to get the URL.
    std::map<std::string, std::string> catchupProperties;
    m_catchupController.ProcessChannelForPlayback(m_currentChannel, catchupProperties);

    const std::string catchupUrl = m_catchupController.GetCatchupUrl(m_currentChannel);
    if (!catchupUrl.empty())
      streamURL = catchupUrl;
    else
      streamURL = m_catchupController.ProcessStreamUrl(m_currentChannel);

    StreamUtils::SetAllStreamProperties(properties, m_currentChannel, streamURL, catchupUrl.empty(), catchupProperties, m_settings);

    Logger::Log(LogLevel::LEVEL_INFO, "%s - Live %s URL: %s", __FUNCTION__, catchupUrl.empty() ? "Stream" : "Catchup", WebUtils::RedactUrl(streamURL).c_str());

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_SERVER_ERROR;
}

bool IptvSimple::GetChannel(const kodi::addon::PVRChannel& channel, Channel& myChannel)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channels.GetChannel(channel, myChannel);
}

bool IptvSimple::GetChannel(unsigned int uniqueChannelId, iptvsimple::data::Channel& myChannel)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channels.GetChannel(uniqueChannelId, myChannel);
}

/***************************************************************************
 * Channel Groups
 **************************************************************************/

PVR_ERROR IptvSimple::GetChannelGroupsAmount(int& amount)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  amount = m_channelGroups.GetChannelGroupsAmount();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channelGroups.GetChannelGroups(results, radio);
}

PVR_ERROR IptvSimple::GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group, kodi::addon::PVRChannelGroupMembersResultSet& results)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channelGroups.GetChannelGroupMembers(group, results);
}

/***************************************************************************
 * EPG
 **************************************************************************/

PVR_ERROR IptvSimple::GetEPGForChannel(int channelUid, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_epg.GetEPGForChannel(channelUid, start, end, results);
}

PVR_ERROR IptvSimple::GetEPGTagStreamProperties(const kodi::addon::PVREPGTag& tag, std::vector<kodi::addon::PVRStreamProperty>& properties)
{
  Logger::Log(LEVEL_DEBUG, "%s - Tag startTime: %ld \tendTime: %ld", __FUNCTION__, tag.GetStartTime(), tag.GetEndTime());

  if (GetChannel(static_cast<int>(tag.GetUniqueChannelId()), m_currentChannel))
  {
    Logger::Log(LEVEL_DEBUG, "%s - GetPlayEpgAsLive is %s", __FUNCTION__, m_settings->CatchupPlayEpgAsLive() ? "enabled" : "disabled");

    std::map<std::string, std::string> catchupProperties;
    if (m_settings->CatchupPlayEpgAsLive() && m_currentChannel.CatchupSupportsTimeshifting())
    {
      m_catchupController.ProcessEPGTagForTimeshiftedPlayback(tag, m_currentChannel, catchupProperties);
    }
    else
    {
      m_catchupController.ResetCatchupState(); // TODO: we need this currently until we have a way to know the stream stops.
      m_catchupController.ProcessEPGTagForVideoPlayback(tag, m_currentChannel, catchupProperties);
    }

    const std::string catchupUrl = m_catchupController.GetCatchupUrl(m_currentChannel);
    if (!catchupUrl.empty())
    {
      StreamUtils::SetAllStreamProperties(properties, m_currentChannel, catchupUrl, false, catchupProperties, m_settings);

      Logger::Log(LEVEL_INFO, "%s - EPG Catchup URL: %s", __FUNCTION__, WebUtils::RedactUrl(catchupUrl).c_str());
      return PVR_ERROR_NO_ERROR;
    }
  }

  return PVR_ERROR_FAILED;
}

PVR_ERROR IptvSimple::IsEPGTagPlayable(const kodi::addon::PVREPGTag& tag, bool& bIsPlayable)
{
  if (!m_settings->IsCatchupEnabled())
    return PVR_ERROR_NOT_IMPLEMENTED;

  const time_t now = std::time(nullptr);
  Channel channel{m_settings};

  // Get the channel and set the current tag on it if found
  bIsPlayable = GetChannel(static_cast<int>(tag.GetUniqueChannelId()), channel) &&
                m_settings->IsCatchupEnabled() && channel.IsCatchupSupported();

  if (channel.IgnoreCatchupDays())
  {
    // If we ignore catchup days then any tag can be played but only if it has a catchup ID
    bool hasCatchupId = false;
    EpgEntry* epgEntry = m_catchupController.GetEPGEntry(channel, tag.GetStartTime());
    if (epgEntry)
      hasCatchupId = !epgEntry->GetCatchupId().empty();

    bIsPlayable = bIsPlayable && hasCatchupId;
  }
  else
  {
    bIsPlayable = bIsPlayable &&
                  tag.GetStartTime() < now &&
                  tag.GetStartTime() >= (now - static_cast<time_t>(channel.GetCatchupDaysInSeconds())) &&
                  (!m_settings->CatchupOnlyOnFinishedProgrammes() || tag.GetEndTime() < now);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::SetEPGMaxPastDays(int epgMaxPastDays)
{
  m_epg.SetEPGMaxPastDays(epgMaxPastDays);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::SetEPGMaxFutureDays(int epgMaxFutureDays)
{
  m_epg.SetEPGMaxFutureDays(epgMaxFutureDays);
  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * Media
 **************************************************************************/

PVR_ERROR IptvSimple::GetRecordingsAmount(bool deleted, int& amount)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (deleted)
    amount = 0;
  else
    amount = m_media.GetNumMedia();

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::GetRecordings(bool deleted, kodi::addon::PVRRecordingsResultSet& results)
{
  if (!deleted)
  {
    std::vector<kodi::addon::PVRRecording> media;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_media.GetMedia(media);
    }

    for (const auto& mediaTag : media)
      results.Add(mediaTag);

    Logger::Log(LEVEL_DEBUG, "%s - media available '%d'", __func__, media.size());
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::GetRecordingStreamProperties(const kodi::addon::PVRRecording& recording, std::vector<kodi::addon::PVRStreamProperty>& properties)
{
  std::string url = m_media.GetMediaEntryURL(recording);

  if (!url.empty())
  {
    properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, url);

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_SERVER_ERROR;
}

/***************************************************************************
 * Signal Status
 **************************************************************************/

PVR_ERROR IptvSimple::GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus)
{
  signalStatus.SetAdapterName("IPTV Simple Adapter 1");
  signalStatus.SetAdapterStatus("OK");

  return PVR_ERROR_NO_ERROR;
}

/***************************************************************************
 * InstanceSettings
 **************************************************************************/

ADDON_STATUS IptvSimple::SetInstanceSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // When a number of settings change set this on the first one so it can be picked up
  // in the process call for a reload of channels, groups and EPG.
  if (!m_reloadChannelsGroupsAndEPG)
    m_reloadChannelsGroupsAndEPG = true;

  return m_settings->SetSetting(settingName, settingValue);
}
