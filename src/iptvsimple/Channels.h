/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "data/Channel.h"

#include <string>
#include <vector>

#include <kodi/libXBMC_pvr.h>

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
    Channels();

    int GetChannelsAmount() const;
    void GetChannels(std::vector<PVR_CHANNEL>& kodiChannels, bool radio) const;
    bool GetChannel(const PVR_CHANNEL& channel, iptvsimple::data::Channel& myChannel) const;
    bool GetChannel(int uniqueId, iptvsimple::data::Channel& myChannel) const;

    void AddChannel(iptvsimple::data::Channel& channel, std::vector<int>& groupIdList, iptvsimple::ChannelGroups& channelGroups);
    iptvsimple::data::Channel* GetChannel(int uniqueId);
    const iptvsimple::data::Channel* FindChannel(const std::string& id, const std::string& displayName) const;
    const std::vector<data::Channel>& GetChannelsList() const { return m_channels; }
    void Clear();

    int GetCurrentChannelNumber() const { return m_currentChannelNumber; }

  private:
    int GenerateChannelId(const char* channelName, const char* streamUrl);

    int m_currentChannelNumber;

    std::vector<iptvsimple::data::Channel> m_channels;
  };
} //namespace iptvsimple