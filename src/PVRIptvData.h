/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "iptvsimple/CatchupController.h"
#include "iptvsimple/Channels.h"
#include "iptvsimple/ChannelGroups.h"
#include "iptvsimple/Epg.h"
#include "iptvsimple/PlaylistLoader.h"
#include "iptvsimple/data/Channel.h"

#include <atomic>
#include <mutex>
#include <thread>

#include <kodi/libXBMC_pvr.h>

class PVRIptvData
{
public:
  PVRIptvData();
  ~PVRIptvData();

  bool Start();
  int GetChannelsAmount();
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  bool GetChannel(const PVR_CHANNEL& channel, iptvsimple::data::Channel& myChannel);
  int GetChannelGroupsAmount();
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd);
  ADDON_STATUS SetSetting(const char* settingName, const void* settingValue);

  // For catchup
  bool GetChannel(unsigned int uniqueChannelId, iptvsimple::data::Channel& myChannel);
  iptvsimple::CatchupController& GetCatchupController() { return m_catchupController; }

protected:
  void Process();

private:
  static const int PROCESS_LOOP_WAIT_SECS = 2;

  iptvsimple::Channels m_channels;
  iptvsimple::ChannelGroups m_channelGroups{m_channels};
  iptvsimple::PlaylistLoader m_playlistLoader{m_channels, m_channelGroups};
  iptvsimple::Epg m_epg{m_channels};
  iptvsimple::CatchupController m_catchupController{m_epg, &m_mutex};

  std::atomic<bool> m_running{false};
  std::thread m_thread;
  std::mutex m_mutex;
  std::atomic_bool m_reloadChannelsGroupsAndEPG{false};
};