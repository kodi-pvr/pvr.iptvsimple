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

#include "Channels.h"
#include "data/ChannelGroup.h"

#include <string>
#include <vector>

namespace iptvsimple
{
  class ChannelGroups
  {
  public:
    ChannelGroups(iptvsimple::Channels& channels);

    int GetChannelGroupsAmount();
    void GetChannelGroups(std::vector<PVR_CHANNEL_GROUP>& kodiChannelGroups, bool radio) const;
    PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group);

    int AddChannelGroup(iptvsimple::data::ChannelGroup& channelGroup);
    iptvsimple::data::ChannelGroup* GetChannelGroup(int uniqueId);
    iptvsimple::data::ChannelGroup* FindChannelGroup(const std::string& name);
    const std::vector<data::ChannelGroup>& GetChannelGroupsList() const { return m_channelGroups; }
    void Clear();

  private:
    iptvsimple::Channels& m_channels;
    std::vector<iptvsimple::data::ChannelGroup> m_channelGroups;
  };
} //namespace iptvsimple