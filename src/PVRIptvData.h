#pragma once
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

#include "iptvsimple/Settings.h"

#include "kodi/libXBMC_pvr.h"
#include "p8-platform/threads/threads.h"

#include "iptvsimple/Channels.h"
#include "iptvsimple/ChannelGroups.h"
#include "iptvsimple/Epg.h"
#include "iptvsimple/PlaylistLoader.h"
#include "iptvsimple/data/Channel.h"

class PVRIptvData : public P8PLATFORM::CThread
{
public:
  PVRIptvData(void);
  ~PVRIptvData(void);

  int GetChannelsAmount();
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  bool GetChannel(const PVR_CHANNEL& channel, iptvsimple::data::Channel& myChannel);
  int GetChannelGroupsAmount();
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd);

protected:
  void* Process() override;

private:
  iptvsimple::Channels m_channels;
  iptvsimple::ChannelGroups m_channelGroups = iptvsimple::ChannelGroups(m_channels);
  iptvsimple::PlaylistLoader m_playlistLoader = iptvsimple::PlaylistLoader(m_channels, m_channelGroups);
  iptvsimple::Epg m_epg = iptvsimple::Epg(m_channels);

  P8PLATFORM::CMutex m_mutex;
};
