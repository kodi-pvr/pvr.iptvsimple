/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "InstanceSettings.h"
#include "data/Channel.h"

#include <memory>
#include <string>
#include <vector>

#include <kodi/addon-instance/pvr/Channels.h>

namespace iptvsimple
{
  class ChannelGroups;

  namespace data
  {
    class ChannelGroup;
  }

  class Channels
  {
  public:
    Channels(std::shared_ptr<iptvsimple::InstanceSettings>& settings);

    bool Init();

    int GetChannelsAmount() const;
    PVR_ERROR GetChannels(kodi::addon::PVRChannelsResultSet& results, bool radio) const;
    bool GetChannel(const kodi::addon::PVRChannel& channel, iptvsimple::data::Channel& myChannel) const;
    bool GetChannel(int uniqueId, iptvsimple::data::Channel& myChannel) const;

    bool AddChannel(iptvsimple::data::Channel& channel, std::vector<int>& groupIdList, iptvsimple::ChannelGroups& channelGroups, bool channelHadGroups);
    iptvsimple::data::Channel* GetChannel(int uniqueId);
    const iptvsimple::data::Channel* FindChannel(const std::string& id, const std::string& displayName) const;
    const std::vector<data::Channel>& GetChannelsList() const { return m_channels; }
    void Clear();

    int GetCurrentChannelNumber() const { return m_currentChannelNumber; }
    void ChannelsLoadFailed() { m_channelsLoadFailed = true; };

  private:
    int GenerateChannelId(const char* channelName, const char* streamUrl);

    int m_currentChannelNumber;
    bool m_channelsLoadFailed = false;

    std::vector<iptvsimple::data::Channel> m_channels;

    std::shared_ptr<iptvsimple::InstanceSettings> m_settings;
  };
} //namespace iptvsimple
