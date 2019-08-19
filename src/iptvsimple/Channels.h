#pragma once
/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://xbmc.org
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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "libXBMC_pvr.h"

#include "data/Channel.h"

#include <string>
#include <vector>

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

    int GetChannelsAmount();
    void GetChannels(std::vector<PVR_CHANNEL>& kodiChannels, bool radio) const;
    bool GetChannel(const PVR_CHANNEL& channel, iptvsimple::data::Channel& myChannel);

    void AddChannel(iptvsimple::data::Channel& channel, std::vector<int>& groupIdList, iptvsimple::ChannelGroups& channelGroups);
    iptvsimple::data::Channel* GetChannel(int uniqueId);
    const iptvsimple::data::Channel* FindChannel(const std::string& id, const std::string& name) const;
    const std::vector<data::Channel>& GetChannelsList() const { return m_channels; }
    void ReapplyChannelLogos(const char* strNewPath);
    void Clear();
    void ApplyChannelLogos();

    int GetCurrentChannelNumber() const { return m_currentChannelNumber; }

  private:
    int GenerateChannelId(const char* channelName, const char* streamUrl);

    std::string m_logoLocation;
    int m_currentChannelNumber;

    std::vector<iptvsimple::data::Channel> m_channels;
  };
} //namespace iptvsimple