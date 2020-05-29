/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "../data/Channel.h"
#include "../data/StreamEntry.h"

#include <map>
#include <string>

#include <kodi/xbmc_pvr_types.h>

namespace iptvsimple
{
  namespace utilities
  {
    static const std::string INPUTSTREAM_FFMPEGDIRECT = "inputstream.ffmpegdirect";
    static const std::string CATCHUP_INPUTSTREAM_NAME = INPUTSTREAM_FFMPEGDIRECT;

    class StreamUtils
    {
    public:
      static void SetAllStreamProperties(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const iptvsimple::data::Channel& channel, const std::string& streamUrl, std::map<std::string, std::string>& catchupProperties);
      static void SetStreamProperty(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const std::string& name, const std::string& value);
      static const StreamType GetStreamType(const std::string& url, const iptvsimple::data::Channel& channel);
      static const StreamType InspectStreamType(const std::string& url, const iptvsimple::data::Channel& channel);
      static const std::string GetManifestType(const StreamType& streamType);
      static const std::string GetMimeType(const StreamType& streamType);
      static bool HasMimeType(const StreamType& streamType);
      static std::string GetURLWithFFmpegReconnectOptions(const std::string& streamUrl, const StreamType& streamType, const iptvsimple::data::Channel& channel);
      static std::string AddHeader(const std::string& headerTarget, const std::string& headerName, const std::string& headerValue, bool encodeHeaderValue);
      static std::string AddHeaderToStreamUrl(const std::string& streamUrl, const std::string& headerName, const std::string& headerValue);
      static bool UseKodiInputstreams(const StreamType& streamType);
      static bool ChannelSpecifiesInputstream(const iptvsimple::data::Channel& channe);
      static std::string GetUrlEncodedProtocolOptions(const std::string& protocolOptions);
      static std::string GetEffectiveInputStreamName(const StreamType& streamType, const iptvsimple::data::Channel& channel);

    private:
      static bool SupportsFFmpegReconnect(const StreamType& streamType, const iptvsimple::data::Channel& channel);
      static void InspectAndSetFFmpegDirectStreamProperties(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const iptvsimple::data::Channel& channel, const std::string& streamUrl);
      static void SetFFmpegDirectManifestTypeStreamProperty(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const iptvsimple::data::Channel& channel, const std::string& streamURL, const StreamType& streamType);

    };
  } // namespace utilities
} // namespace iptvsimple
