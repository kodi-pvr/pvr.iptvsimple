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

#include "PVRIptvData.h"

#include "client.h"
#include "iptvsimple/Settings.h"
#include "iptvsimple/utilities/Logger.h"

#include <ctime>
#include <chrono>

using namespace ADDON;
using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

PVRIptvData::PVRIptvData()
{
  m_channels.Clear();
  m_channelGroups.Clear();
  m_epg.Clear();

  m_playlistLoader.LoadPlayList();
}

bool PVRIptvData::Start()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  XBMC->Log(LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  m_running = true;
  m_thread = std::thread([&] { Process(); });

  return m_running;
}

void PVRIptvData::Process()
{
  unsigned int refreshTimer = 0;
  time_t lastRefreshTimeSeconds = std::time(nullptr);
  int lastRefreshHour = Settings::GetInstance().GetM3URefreshHour(); //ignore if we start during same hour

  while (m_running)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(PROCESS_LOOP_WAIT_SECS * 1000));

    time_t currentRefreshTimeSeconds = std::time(nullptr);
    std::tm timeInfo = *std::localtime(&currentRefreshTimeSeconds);
    refreshTimer += static_cast<unsigned int>(currentRefreshTimeSeconds - lastRefreshTimeSeconds);
    lastRefreshTimeSeconds = currentRefreshTimeSeconds;

    if (Settings::GetInstance().GetM3URefreshMode() == RefreshMode::REPEATED_REFRESH &&
        refreshTimer >= (Settings::GetInstance().GetM3URefreshIntervalMins() * 60))
      m_reloadChannelsGroupsAndEPG = true;

    if (Settings::GetInstance().GetM3URefreshMode() == RefreshMode::ONCE_PER_DAY &&
        lastRefreshHour != timeInfo.tm_hour && timeInfo.tm_hour == Settings::GetInstance().GetM3URefreshHour())
      m_reloadChannelsGroupsAndEPG = true;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running && m_reloadChannelsGroupsAndEPG)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      m_playlistLoader.ReloadPlayList();
      m_epg.ReloadEPG();

      m_reloadChannelsGroupsAndEPG = false;
      refreshTimer = 0;
    }
    lastRefreshHour = timeInfo.tm_hour;
  }
}

PVRIptvData::~PVRIptvData()
{
  Logger::Log(LEVEL_DEBUG, "%s Stopping update thread...", __FUNCTION__);
  m_running = false;
  if (m_thread.joinable())
    m_thread.join();

  std::lock_guard<std::mutex> lock(m_mutex);
  m_channels.Clear();
  m_channelGroups.Clear();
  m_epg.Clear();
}

int PVRIptvData::GetChannelsAmount()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_channels.GetChannelsAmount();
}

PVR_ERROR PVRIptvData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  std::vector<PVR_CHANNEL> channels;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_channels.GetChannels(channels, bRadio);
  }

  Logger::Log(LEVEL_DEBUG, "%s - channels available '%d', radio = %d", __FUNCTION__, channels.size(), bRadio);

  for (auto& channel : channels)
    PVR->TransferChannelEntry(handle, &channel);

  return PVR_ERROR_NO_ERROR;
}

bool PVRIptvData::GetChannel(const PVR_CHANNEL& channel, Channel& myChannel)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channels.GetChannel(channel, myChannel);
}

bool PVRIptvData::GetChannel(unsigned int uniqueChannelId, iptvsimple::data::Channel& myChannel)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channels.GetChannel(uniqueChannelId, myChannel);
}

int PVRIptvData::GetChannelGroupsAmount()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_channelGroups.GetChannelGroupsAmount();
}

PVR_ERROR PVRIptvData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  std::vector<PVR_CHANNEL_GROUP> channelGroups;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_channelGroups.GetChannelGroups(channelGroups, bRadio);
  }

  Logger::Log(LEVEL_DEBUG, "%s - channel groups available '%d'", __FUNCTION__, channelGroups.size());

  for (const auto& channelGroup : channelGroups)
    PVR->TransferChannelGroup(handle, &channelGroup);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRIptvData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channelGroups.GetChannelGroupMembers(handle, group);
}

PVR_ERROR PVRIptvData::GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_epg.GetEPGForChannel(handle, iChannelUid, iStart, iEnd);
}

ADDON_STATUS PVRIptvData::SetSetting(const char* settingName, const void* settingValue)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // When a number of settings change set this on the first one so it can be picked up
  // in the process call for a reload of channels, groups and EPG.
  if (!m_reloadChannelsGroupsAndEPG)
    m_reloadChannelsGroupsAndEPG = true;

  return Settings::GetInstance().SetValue(settingName, settingValue);
}
