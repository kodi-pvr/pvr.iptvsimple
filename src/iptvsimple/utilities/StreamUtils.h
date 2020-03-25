/*
 *      Copyright (C) 2005-2020 Team Kodi
 *      https://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "../data/Channel.h"

#include <map>
#include <string>

#include <kodi/xbmc_pvr_types.h>

namespace iptvsimple
{
  namespace utilities
  {
    enum class StreamType
      : int // same type as addon settings
    {
      HLS = 0,
      DASH,
      SMOOTH_STREAMING,
      TS,
      OTHER_TYPE
    };

    static const std::string CATCHUP_INPUTSTREAMCLASS = "inputstream.ffmpegdirect";

    class StreamUtils
    {
    public:
      static void SetAllStreamProperties(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const iptvsimple::data::Channel& channel, const std::string& streamUrl, std::map<std::string, std::string>& catchupProperties);
      static void SetStreamProperty(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const std::string& name, const std::string& value);
      static const StreamType GetStreamType(const std::string& url, const iptvsimple::data::Channel& channel);
      static const StreamType InspectStreamType(const std::string& url, const iptvsimple::data::Channel& channel);
      static const std::string GetManifestType(const StreamType& streamType);
      static const std::string GetMimeType(const StreamType& streamType);
      static std::string GetURLWithFFmpegReconnectOptions(const std::string& streamUrl, const StreamType& streamType, const iptvsimple::data::Channel& channel);
      static std::string AddHeaderToStreamUrl(const std::string& streamUrl, const std::string& headerName, const std::string& headerValue);
      static bool UseKodiInputstreams(const StreamType& streamType);
      static bool ChannelSpecifiesInputstream(const iptvsimple::data::Channel& channe);

      static std::string GetEffectiveInputStreamClass(const StreamType& streamType, const iptvsimple::data::Channel& channel);

    private:
      static bool SupportsFFmpegReconnect(const StreamType& streamType, const iptvsimple::data::Channel& channel);
    };
  } // namespace utilities
} // namespace iptvsimple
