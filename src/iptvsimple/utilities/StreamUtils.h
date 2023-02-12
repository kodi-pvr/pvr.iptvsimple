/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "../data/Channel.h"
#include "../data/StreamEntry.h"

#include <map>
#include <string>

#include <kodi/addon-instance/pvr/General.h>

namespace iptvsimple
{
  class InstanceSettings;

  namespace utilities
  {
    static const std::string INPUTSTREAM_ADAPTIVE = "inputstream.adaptive";
    static const std::string INPUTSTREAM_FFMPEGDIRECT = "inputstream.ffmpegdirect";
    static const std::string CATCHUP_INPUTSTREAM_NAME = INPUTSTREAM_FFMPEGDIRECT;

    class StreamUtils
    {
    public:
      static void SetAllStreamProperties(std::vector<kodi::addon::PVRStreamProperty>& properties, const iptvsimple::data::Channel& channel, const std::string& streamUrl, bool isChannelURL, std::map<std::string, std::string>& catchupProperties, std::shared_ptr<iptvsimple::InstanceSettings>& settings);
      static const StreamType GetStreamType(const std::string& url, const iptvsimple::data::Channel& channel);
      static const StreamType InspectStreamType(const std::string& url, const iptvsimple::data::Channel& channel);
      static const std::string GetManifestType(const StreamType& streamType);
      static const std::string GetMimeType(const StreamType& streamType);
      static bool HasMimeType(const StreamType& streamType);
      static std::string GetURLWithFFmpegReconnectOptions(const std::string& streamUrl, const StreamType& streamType, const iptvsimple::data::Channel& channel, std::shared_ptr<iptvsimple::InstanceSettings>& settings);
      static std::string AddHeader(const std::string& headerTarget, const std::string& headerName, const std::string& headerValue, bool encodeHeaderValue);
      static std::string AddHeaderToStreamUrl(const std::string& streamUrl, const std::string& headerName, const std::string& headerValue);
      static bool UseKodiInputstreams(const StreamType& streamType, std::shared_ptr<iptvsimple::InstanceSettings>& settings);
      static bool ChannelSpecifiesInputstream(const iptvsimple::data::Channel& channe);
      static std::string GetUrlEncodedProtocolOptions(const std::string& protocolOptions);
      static std::string GetEffectiveInputStreamName(const StreamType& streamType, const iptvsimple::data::Channel& channel, std::shared_ptr<iptvsimple::InstanceSettings>& settings);

    private:
      static bool SupportsFFmpegReconnect(const StreamType& streamType, const iptvsimple::data::Channel& channel);
      static void InspectAndSetFFmpegDirectStreamProperties(std::vector<kodi::addon::PVRStreamProperty>& properties, const iptvsimple::data::Channel& channel, const std::string& streamUrl, bool isChannelURL, std::shared_ptr<iptvsimple::InstanceSettings>& settings);
      static void SetFFmpegDirectManifestTypeStreamProperty(std::vector<kodi::addon::PVRStreamProperty>& properties, const iptvsimple::data::Channel& channel, const std::string& streamURL, const StreamType& streamType);
      static bool CheckInputstreamInstalledAndEnabled(const std::string& inputstreamName);

    };
  } // namespace utilities
} // namespace iptvsimple
