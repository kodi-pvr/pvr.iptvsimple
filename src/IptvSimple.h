/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
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
#include "iptvsimple/Media.h"
#include "iptvsimple/PlaylistLoader.h"
#include "iptvsimple/data/Channel.h"

#include <atomic>
#include <mutex>
#include <thread>

#include <kodi/addon-instance/PVR.h>

class ATTR_DLL_LOCAL IptvSimple : public kodi::addon::CInstancePVRClient
{
public:
  IptvSimple(const kodi::addon::IInstanceInfo& instance);
  ~IptvSimple() override;

  bool Initialise();

  // kodi::addon::CInstancePVRClient -> kodi::addon::IAddonInstance overrides
  ADDON_STATUS SetInstanceSetting(const std::string& settingName,
                                  const kodi::addon::CSettingValue& settingValue) override;

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

  PVR_ERROR GetRecordingsAmount(bool deleted, int& amount) override;
  PVR_ERROR GetRecordings(bool deleted, kodi::addon::PVRRecordingsResultSet& results) override;
  PVR_ERROR GetRecordingStreamProperties(const kodi::addon::PVRRecording& recording, std::vector<kodi::addon::PVRStreamProperty>& properties) override;

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

  std::shared_ptr<iptvsimple::InstanceSettings> m_settings;

  iptvsimple::data::Channel m_currentChannel{m_settings};
  iptvsimple::Providers m_providers{m_settings};
  iptvsimple::Channels m_channels{m_settings};
  iptvsimple::ChannelGroups m_channelGroups{m_channels, m_settings};
  iptvsimple::Media m_media{m_settings};
  iptvsimple::PlaylistLoader m_playlistLoader{this, m_channels, m_channelGroups, m_providers, m_media, m_settings};
  iptvsimple::Epg m_epg{this, m_channels, m_media, m_settings};
  iptvsimple::CatchupController m_catchupController{m_epg, &m_mutex, m_settings};

  std::atomic<bool> m_running{false};
  std::thread m_thread;
  std::mutex m_mutex;
  std::atomic_bool m_reloadChannelsGroupsAndEPG{false};
};
