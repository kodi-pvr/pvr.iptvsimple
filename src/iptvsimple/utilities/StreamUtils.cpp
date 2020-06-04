/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "StreamUtils.h"

#include "../Settings.h"
#include "FileUtils.h"
#include "Logger.h"
#include "WebUtils.h"

// TODO: inlcude when C++ API happens
// #include <kodi/General.h>
#include <p8-platform/util/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

void StreamUtils::SetStreamProperty(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const std::string& name, const std::string& value)
{
  if (*propertiesCount < propertiesMax)
  {
    strncpy(properties[*propertiesCount].strName, name.c_str(), sizeof(properties[*propertiesCount].strName) - 1);
    strncpy(properties[*propertiesCount].strValue, value.c_str(), sizeof(properties[*propertiesCount].strValue) - 1);
    (*propertiesCount)++;
  }
  else
  {
    Logger::Log(LogLevel::LEVEL_ERROR, "%s - Could not add property as max number reached: %s=%s - count : %u", __FUNCTION__, name.c_str(), value.c_str(), propertiesMax);
  }
}

void StreamUtils::SetAllStreamProperties(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const iptvsimple::data::Channel& channel, const std::string& streamURL, std::map<std::string, std::string>& catchupProperties)
{
  if (ChannelSpecifiesInputstream(channel))
  {
    // Channel has an inputstream class set so we only set the stream URL
    StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_STREAMURL, streamURL);

    if (channel.GetInputStreamName() == INPUTSTREAM_FFMPEGDIRECT)
      InspectAndSetFFmpegDirectStreamProperties(properties, propertiesCount, propertiesMax, channel, streamURL);
  }
  else
  {
    StreamType streamType = StreamUtils::GetStreamType(streamURL, channel);
    if (streamType == StreamType::OTHER_TYPE)
      streamType = StreamUtils::InspectStreamType(streamURL, channel);

    // Using kodi's built in inputstreams
    if (StreamUtils::UseKodiInputstreams(streamType))
    {
      std::string ffmpegStreamURL = StreamUtils::GetURLWithFFmpegReconnectOptions(streamURL, streamType, channel);

      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_STREAMURL, streamURL);
      if (!channel.HasMimeType() && StreamUtils::HasMimeType(streamType))
        StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_MIMETYPE, StreamUtils::GetMimeType(streamType));

      if (streamType == StreamType::HLS || streamType == StreamType::TS || streamType == StreamType::OTHER_TYPE)
      {
        if (channel.IsCatchupSupported() && CheckInputstreamInstalledAndEnabled(CATCHUP_INPUTSTREAM_NAME))
        {
          StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_INPUTSTREAM, CATCHUP_INPUTSTREAM_NAME);
          SetFFmpegDirectManifestTypeStreamProperty(properties, propertiesCount, propertiesMax, channel, streamURL, streamType);
        }
        else if (channel.SupportsLiveStreamTimeshifting() && CheckInputstreamInstalledAndEnabled(INPUTSTREAM_FFMPEGDIRECT))
        {
          StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_INPUTSTREAM, INPUTSTREAM_FFMPEGDIRECT);
          StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, "inputstream.ffmpegdirect.stream_mode", "timeshift");
          StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, "inputstream.ffmpegdirect.is_realtime_stream", "true");
        }
        else if (streamType == StreamType::HLS || streamType == StreamType::TS)
        {
          StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_INPUTSTREAM, PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG);
        }
      }
    }
    else // inputstream.adaptive
    {
      CheckInputstreamInstalledAndEnabled(INPUTSTREAM_ADAPTIVE);

      bool streamUrlSet = false;

      // If no media headers are explicitly set for inputstream.adaptive,
      // strip the headers from streamURL and put it to media headers property

      if (channel.GetProperty("inputstream.adaptive.stream_headers").empty())
      {
        // No stream headers declared by property, check if stream URL has any
        size_t found = streamURL.find_first_of('|');
        if (found != std::string::npos)
        {
          // Headers found, split and url-encode them
          const std::string& url = streamURL.substr(0, found);
          const std::string& protocolOptions = streamURL.substr(found + 1, streamURL.length());
          const std::string& encodedProtocolOptions = StreamUtils::GetUrlEncodedProtocolOptions(protocolOptions);

          // Set stream URL without headers and encoded headers as property
          StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_STREAMURL, url);
          StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, "inputstream.adaptive.stream_headers", encodedProtocolOptions);
          streamUrlSet = true;
        }
      }

      // Set intact stream URL if not previously set
      if (!streamUrlSet)
        StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_STREAMURL, streamURL);

      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_INPUTSTREAM, INPUTSTREAM_ADAPTIVE);
      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, "inputstream.adaptive.manifest_type", StreamUtils::GetManifestType(streamType));
      if (streamType == StreamType::HLS || streamType == StreamType::DASH)
        StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_MIMETYPE, StreamUtils::GetMimeType(streamType));
      if (streamType == StreamType::DASH)
        StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, "inputstream.adaptive.manifest_update_parameter", "full");
    }
  }

  if (!channel.GetProperties().empty())
  {
    for (auto& prop : channel.GetProperties())
      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, prop.first, prop.second);
  }

  if (!catchupProperties.empty())
  {
    for (auto& prop : catchupProperties)
      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, prop.first, prop.second);
  }
}
bool StreamUtils::CheckInputstreamInstalledAndEnabled(const std::string& inputstreamName)
{
  // TODO: uncomment when C++ API happens

  // std::string version;
  // bool enabled;

  // if (kodi::IsAddonAvailable(inputstreamName, version, enabled))
  // {
  //   if (!enabled)
  //   {
  //     std::string message = StringUtils::Format(kodi::LocalizedString(30502).c_str(), inputstreamName.c_str());
  //     kodi::QueueNotification(QueueMsg::QUEUE_ERROR, kodi::LocalizedString(30500), message);
  //   }
  // }
  // else // Not installed
  // {
  //   std::string message = StringUtils::Format(kodi::LocalizedString(30501).c_str(), inputstreamName.c_str());
  //   kodi::QueueNotification(QueueMsg::QUEUE_ERROR, kodi::LocalizedString(30500), message);
  // }

  return true;
}

void StreamUtils::InspectAndSetFFmpegDirectStreamProperties(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const iptvsimple::data::Channel& channel, const std::string& streamURL)
{
  // If there is no MIME type and no manifest type (BOTH!) set then potentially inspect the stream and set them
  if (!channel.HasMimeType() && !channel.GetProperty("inputstream.ffmpegdirect.manifest_type").empty())
  {
    StreamType streamType = StreamUtils::GetStreamType(streamURL, channel);
    if (streamType == StreamType::OTHER_TYPE)
      streamType = StreamUtils::InspectStreamType(streamURL, channel);

    if (!channel.HasMimeType() && StreamUtils::HasMimeType(streamType))
      StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, PVR_STREAM_PROPERTY_MIMETYPE, StreamUtils::GetMimeType(streamType));

    SetFFmpegDirectManifestTypeStreamProperty(properties, propertiesCount, propertiesMax, channel, streamURL, streamType);
  }
}

void StreamUtils::SetFFmpegDirectManifestTypeStreamProperty(PVR_NAMED_VALUE* properties, unsigned int* propertiesCount, unsigned int propertiesMax, const iptvsimple::data::Channel& channel, const std::string& streamURL, const StreamType& streamType)
{
  std::string manifestType = channel.GetProperty("inputstream.ffmpegdirect.manifest_type");
  if (manifestType.empty())
    manifestType = StreamUtils::GetManifestType(streamType);
  if (!manifestType.empty())
    StreamUtils::SetStreamProperty(properties, propertiesCount, propertiesMax, "inputstream.ffmpegdirect.manifest_type", manifestType);
}

std::string StreamUtils::GetEffectiveInputStreamName(const StreamType& streamType, const iptvsimple::data::Channel& channel)
{
  std::string inputStreamName = channel.GetInputStreamName();

  if (inputStreamName.empty())
  {
    if (StreamUtils::UseKodiInputstreams(streamType))
    {
      if (streamType == StreamType::HLS || streamType == StreamType::TS)
      {
        if (channel.IsCatchupSupported() && channel.CatchupSupportsTimeshifting())
          inputStreamName = CATCHUP_INPUTSTREAM_NAME;
        else
          inputStreamName = PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG;
      }
    }
    else // inputstream.adpative
    {
      inputStreamName = "inputstream.adaptive";
    }
  }

  return inputStreamName;
}

const StreamType StreamUtils::GetStreamType(const std::string& url, const Channel& channel)
{
  if (StringUtils::StartsWith(url, "plugin://"))
    return StreamType::PLUGIN;

  std::string mimeType = channel.GetProperty(PVR_STREAM_PROPERTY_MIMETYPE);

  if (url.find(".m3u8") != std::string::npos ||
      mimeType == "application/x-mpegURL" ||
      mimeType == "application/vnd.apple.mpegurl")
    return StreamType::HLS;

  if (url.find(".mpd") != std::string::npos || mimeType == "application/xml+dash")
    return StreamType::DASH;

  if (url.find(".ism") != std::string::npos &&
      !(url.find(".ismv") != std::string::npos || url.find(".isma") != std::string::npos))
    return StreamType::SMOOTH_STREAMING;

  if (mimeType == "video/mp2t" || channel.IsCatchupTSStream())
    return StreamType::TS;

  // it has a MIME type but not one we recognise
  if (!mimeType.empty())
    return StreamType::MIME_TYPE_UNRECOGNISED;

  return StreamType::OTHER_TYPE;
}

const StreamType StreamUtils::InspectStreamType(const std::string& url, const Channel& channel)
{
  if (!FileUtils::FileExists(url))
    return StreamType::OTHER_TYPE;

  int httpCode = 0;
  const std::string source = WebUtils::ReadFileContentsStartOnly(url, &httpCode);

  if (httpCode == 200)
  {
    if (StringUtils::StartsWith(source, "#EXTM3U") && (source.find("#EXT-X-STREAM-INF") != std::string::npos || source.find("#EXT-X-VERSION") != std::string::npos))
      return StreamType::HLS;

    if (source.find("<MPD") != std::string::npos)
      return StreamType::DASH;

    if (source.find("<SmoothStreamingMedia") != std::string::npos)
      return StreamType::SMOOTH_STREAMING;
  }

  // If we can't inspect the stream type the only option left for shift mode is TS
  if (channel.GetCatchupMode() == CatchupMode::SHIFT ||
      channel.GetCatchupMode() == CatchupMode::TIMESHIFT)
    return StreamType::TS;

  return StreamType::OTHER_TYPE;
}

const std::string StreamUtils::GetManifestType(const StreamType& streamType)
{
  switch (streamType)
  {
    case StreamType::HLS:
      return "hls";
    case StreamType::DASH:
      return "mpd";
    case StreamType::SMOOTH_STREAMING:
      return "ism";
    default:
      return "";
  }
}

const std::string StreamUtils::GetMimeType(const StreamType& streamType)
{
  switch (streamType)
  {
    case StreamType::HLS:
      return "application/x-mpegURL";
    case StreamType::DASH:
      return "application/xml+dash";
    case StreamType::TS:
      return "video/mp2t";
    default:
      return "";
  }
}

bool StreamUtils::HasMimeType(const StreamType& streamType)
{
  return streamType != StreamType::OTHER_TYPE && streamType != StreamType::SMOOTH_STREAMING;
}

std::string StreamUtils::GetURLWithFFmpegReconnectOptions(const std::string& streamUrl, const StreamType& streamType, const iptvsimple::data::Channel& channel)
{
  std::string newStreamUrl = streamUrl;

  if (WebUtils::IsHttpUrl(streamUrl) && SupportsFFmpegReconnect(streamType, channel) &&
      (channel.GetProperty("http-reconnect") == "true" || Settings::GetInstance().UseFFmpegReconnect()))
  {
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect", "1");
    if (streamType != StreamType::HLS)
      newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_at_eof", "1");
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_streamed", "1");
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_delay_max", "4294");

    Logger::Log(LogLevel::LEVEL_DEBUG, "%s - FFmpeg Reconnect Stream URL: %s", __FUNCTION__, newStreamUrl.c_str());
  }

  return newStreamUrl;
}

std::string StreamUtils::AddHeaderToStreamUrl(const std::string& streamUrl, const std::string& headerName, const std::string& headerValue)
{
  return StreamUtils::AddHeader(streamUrl, headerName, headerValue, false);
}

std::string StreamUtils::AddHeader(const std::string& headerTarget, const std::string& headerName, const std::string& headerValue, bool encodeHeaderValue)
{
  std::string newHeaderTarget = headerTarget;

  bool hasProtocolOptions = false;
  bool addHeader = true;
  size_t found = newHeaderTarget.find("|");

  if (found != std::string::npos)
  {
    hasProtocolOptions = true;
    addHeader = newHeaderTarget.find(headerName + "=", found + 1) == std::string::npos;
  }

  if (addHeader)
  {
    if (!hasProtocolOptions)
      newHeaderTarget += "|";
    else
      newHeaderTarget += "&";

    newHeaderTarget += headerName + "=" + (encodeHeaderValue ? WebUtils::UrlEncode(headerValue) : headerValue);
  }

  return newHeaderTarget;
}

bool StreamUtils::UseKodiInputstreams(const StreamType& streamType)
{
  return streamType == StreamType::OTHER_TYPE || streamType == StreamType::TS || streamType == StreamType::PLUGIN ||
        (streamType == StreamType::HLS && !Settings::GetInstance().UseInputstreamAdaptiveforHls());
}

bool StreamUtils::ChannelSpecifiesInputstream(const iptvsimple::data::Channel& channel)
{
  return !channel.GetInputStreamName().empty();
}

bool StreamUtils::SupportsFFmpegReconnect(const StreamType& streamType, const iptvsimple::data::Channel& channel)
{
  return streamType == StreamType::HLS ||
         channel.GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAM) == PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG;
}

std::string StreamUtils::GetUrlEncodedProtocolOptions(const std::string& protocolOptions)
{
  std::string encodedProtocolOptions = "";

  std::vector<std::string> headers = StringUtils::Split(protocolOptions, "&");
  for (std::string header : headers)
  {
    std::string::size_type pos(header.find('='));
    if(pos == std::string::npos)
      continue;

    encodedProtocolOptions = StreamUtils::AddHeader(encodedProtocolOptions, header.substr(0, pos), header.substr(pos + 1), true);
  }

  // We'll return the protocol options without the leading '|'
  if (!encodedProtocolOptions.empty() && encodedProtocolOptions[0] == '|')
    encodedProtocolOptions.erase(0, 1);

  return encodedProtocolOptions;
}
