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
#include "ChannelGroups.h"

#include <string>

namespace iptvsimple
{
  static const std::string M3U_START_MARKER        = "#EXTM3U";
  static const std::string M3U_INFO_MARKER         = "#EXTINF";
  static const std::string M3U_GROUP_MARKER        = "#EXTGRP:";
  static const std::string TVG_INFO_ID_MARKER      = "tvg-id=";
  static const std::string TVG_INFO_NAME_MARKER    = "tvg-name=";
  static const std::string TVG_INFO_LOGO_MARKER    = "tvg-logo=";
  static const std::string TVG_INFO_SHIFT_MARKER   = "tvg-shift=";
  static const std::string TVG_INFO_CHNO_MARKER    = "tvg-chno=";
  static const std::string GROUP_NAME_MARKER       = "group-title=";
  static const std::string KODIPROP_MARKER         = "#KODIPROP:";
  static const std::string EXTVLCOPT_MARKER        = "#EXTVLCOPT:";
  static const std::string RADIO_MARKER            = "radio=";
  static const std::string PLAYLIST_TYPE_MARKER    = "#EXT-X-PLAYLIST-TYPE:";
  static const std::string CHANNEL_LOGO_EXTENSION  = ".png";

  class PlaylistLoader
  {
  public:
    PlaylistLoader(iptvsimple::Channels& channels, iptvsimple::ChannelGroups& channelGroups);

    bool LoadPlayList(void);
    void ReloadPlayList(const char* strNewPath);

  private:
    static std::string ReadMarkerValue(const std::string& line, const std::string& markerName);
    static void ParseSinglePropertyIntoChannel(const std::string& line, iptvsimple::data::Channel& channel, const std::string& markerName);

    std::string ParseIntoChannel(const std::string& line, iptvsimple::data::Channel& channel, std::vector<int>& groupIdList, int epgTimeShift);
    void ParseAndAddChannelGroups(const std::string& groupNamesListString, std::vector<int>& groupIdList, bool isRadio);

    std::string m_m3uLocation;

    iptvsimple::ChannelGroups& m_channelGroups;
    iptvsimple::Channels& m_channels;
  };
} //namespace iptvsimple