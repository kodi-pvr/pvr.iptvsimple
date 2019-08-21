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

using namespace ADDON;
using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

PVRIptvData::PVRIptvData(void)
{
  m_channels.Clear();
  m_channelGroups.Clear();
  m_epg.Clear();

  m_playlistLoader.LoadPlayList();
}

bool PVRIptvData::Start()
{
  P8PLATFORM::CLockObject lock(m_mutex);

  XBMC->Log(LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  CreateThread();

  return IsRunning();
}

void* PVRIptvData::Process()
{
  while(!IsStopped())
  {
    Sleep(PROCESS_LOOP_WAIT_SECS * 1000);

    P8PLATFORM::CLockObject lock(m_mutex);
    if (m_reloadChannelsGroupsAndEPG)
    {
      Sleep(1000);

      m_playlistLoader.ReloadPlayList();
      m_epg.ReloadEPG();

      m_reloadChannelsGroupsAndEPG = false;
    }
  }

  return nullptr;
}

PVRIptvData::~PVRIptvData(void)
{
  P8PLATFORM::CLockObject lock(m_mutex);
  Logger::Log(LEVEL_DEBUG, "%s Stopping update thread...", __FUNCTION__);
  StopThread();

  m_channels.Clear();
  m_channelGroups.Clear();
  m_epg.Clear();
}

int PVRIptvData::GetChannelsAmount()
{
  P8PLATFORM::CLockObject lock(m_mutex);
  return m_channels.GetChannelsAmount();
}

PVR_ERROR PVRIptvData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  std::vector<PVR_CHANNEL> channels;
  {
    P8PLATFORM::CLockObject lock(m_mutex);
    m_channels.GetChannels(channels, bRadio);
  }

  Logger::Log(LEVEL_DEBUG, "%s - channels available '%d', radio = %d", __FUNCTION__, channels.size(), bRadio);

  for (auto& channel : channels)
    PVR->TransferChannelEntry(handle, &channel);

  return PVR_ERROR_NO_ERROR;
}

bool PVRIptvData::GetChannel(const PVR_CHANNEL& channel, Channel& myChannel)
{
  P8PLATFORM::CLockObject lock(m_mutex);

  return m_channels.GetChannel(channel, myChannel);
}

int PVRIptvData::GetChannelGroupsAmount()
{
  P8PLATFORM::CLockObject lock(m_mutex);
  return m_channelGroups.GetChannelGroupsAmount();
}

PVR_ERROR PVRIptvData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  std::vector<PVR_CHANNEL_GROUP> channelGroups;
  {
    P8PLATFORM::CLockObject lock(m_mutex);
    m_channelGroups.GetChannelGroups(channelGroups, bRadio);
  }

  Logger::Log(LEVEL_DEBUG, "%s - channel groups available '%d'", __FUNCTION__, channelGroups.size());

  for (const auto& channelGroup : channelGroups)
    PVR->TransferChannelGroup(handle, &channelGroup);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRIptvData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group)
{
  P8PLATFORM::CLockObject lock(m_mutex);

  return m_channelGroups.GetChannelGroupMembers(handle, group);
}

PVR_ERROR PVRIptvData::GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd)
{
  P8PLATFORM::CLockObject lock(m_mutex);

  return m_epg.GetEPGForChannel(handle, iChannelUid, iStart, iEnd);
}

ADDON_STATUS PVRIptvData::SetSetting(const char* settingName, const void* settingValue)
{
  P8PLATFORM::CLockObject lock(m_mutex);

  if (!m_reloadChannelsGroupsAndEPG)
    m_reloadChannelsGroupsAndEPG = true;

  return Settings::GetInstance().SetValue(settingName, settingValue);
}