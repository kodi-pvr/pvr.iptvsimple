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
#include "iptvsimple/Providers.h"
#include "iptvsimple/Epg.h"
#include "iptvsimple/PlaylistLoader.h"
#include "iptvsimple/data/Channel.h"

#include <atomic>
#include <mutex>
#include <thread>

#include <kodi/addon-instance/PVR.h>

class ATTRIBUTE_HIDDEN PVRIptvData
  : public kodi::addon::CAddonBase,
    public kodi::addon::CInstancePVRClient
{
public:
  PVRIptvData();
  ~PVRIptvData() override;

  // kodi::addon::CAddonBase functions
  //@{
  ADDON_STATUS Create() override;
  ADDON_STATUS SetSetting(const std::string& settingName, const kodi::CSettingValue& settingValue) override;
  //@}

  // kodi::addon::CInstancePVRClient functions
  //@{
  PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override;
  PVR_ERROR GetBackendName(std::string& name) override;
  PVR_ERROR GetBackendVersion(std::string& version) override;
  PVR_ERROR GetConnectionString(std::string& connection) override;

  PVR_ERROR OnSystemSleep() override { return PVR_ERROR_NO_ERROR; }
  PVR_ERROR OnSystemWake() override { return PVR_ERROR_NO_ERROR; }
  PVR_ERROR OnPowerSavingActivated() override { return PVR_ERROR_NO_ERROR; }
  PVR_ERROR OnPowerSavingDeactivated() override { return PVR_ERROR_NO_ERROR; }

  PVR_ERROR GetProvidersAmount(int& amount) override;
  PVR_ERROR GetProviders(kodi::addon::PVRProvidersResultSet& results) override;

  PVR_ERROR GetChannelsAmount(int& amount) override;
  PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results) override;
  PVR_ERROR GetChannelStreamProperties(const kodi::addon::PVRChannel& channel, std::vector<kodi::addon::PVRStreamProperty>& properties) override;

  PVR_ERROR GetChannelGroupsAmount(int& amount) override;
  PVR_ERROR GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results) override;
  PVR_ERROR GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group, kodi::addon::PVRChannelGroupMembersResultSet& results) override;

  PVR_ERROR GetEPGForChannel(int channelUid, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results) override;
  PVR_ERROR GetEPGTagStreamProperties(const kodi::addon::PVREPGTag& tag, std::vector<kodi::addon::PVRStreamProperty>& properties) override;
  PVR_ERROR IsEPGTagPlayable(const kodi::addon::PVREPGTag& tag, bool& bIsPlayable) override;
  PVR_ERROR SetEPGMaxPastDays(int epgMaxPastDays) override;
  PVR_ERROR SetEPGMaxFutureDays(int epgMaxFutureDays) override;

  PVR_ERROR GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus) override;
  //@}

  // Internal functions
  //@{
  bool GetChannel(const kodi::addon::PVRChannel& channel, iptvsimple::data::Channel& myChannel);

  // For catchup
  bool GetChannel(unsigned int uniqueChannelId, iptvsimple::data::Channel& myChannel);
  iptvsimple::CatchupController& GetCatchupController() { return m_catchupController; }
  //@}

protected:
  void Process();

private:
  static const int PROCESS_LOOP_WAIT_SECS = 2;

  iptvsimple::data::Channel m_currentChannel;
  iptvsimple::Providers m_providers;
  iptvsimple::Channels m_channels;
  iptvsimple::ChannelGroups m_channelGroups{m_channels};
  iptvsimple::PlaylistLoader m_playlistLoader{this, m_channels, m_channelGroups, m_providers};
  iptvsimple::Epg m_epg{this, m_channels};
  iptvsimple::CatchupController m_catchupController{m_epg, &m_mutex};

  std::atomic<bool> m_running{false};
  std::thread m_thread;
  std::mutex m_mutex;
  std::atomic_bool m_reloadChannelsGroupsAndEPG{false};
};
