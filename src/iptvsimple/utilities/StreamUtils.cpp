/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "StreamUtils.h"

#include "../InstanceSettings.h"
#include "FileUtils.h"
#include "Logger.h"
#include "WebUtils.h"

#include <kodi/General.h>
#include <kodi/tools/StringUtils.h>

using namespace kodi::tools;
using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

namespace
{
bool SplitUrlProtocolOpts(const std::string& streamURL,
                          std::string& url,
                          std::string& encodedProtocolOptions)
{
  size_t found = streamURL.find_first_of('|');
  if (found != std::string::npos)
  {
      // Headers found, split and url-encode them
      url = streamURL.substr(0, found);
      const std::string& protocolOptions = streamURL.substr(found + 1, streamURL.length());
      encodedProtocolOptions = StreamUtils::GetUrlEncodedProtocolOptions(protocolOptions);
      return true;
  }
  return false;
}
} // unnamed namespace

void StreamUtils::SetAllStreamProperties(std::vector<kodi::addon::PVRStreamProperty>& properties, const iptvsimple::data::Channel& channel, const std::string& streamURL, bool isChannelURL, std::map<std::string, std::string>& catchupProperties, std::shared_ptr<InstanceSettings>& settings)
{
  if (ChannelSpecifiesInputstream(channel))
  {
    // Channel has an inputstream class set so we only set the stream URL
    properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, streamURL);

    if (channel.GetInputStreamName() != PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG)
      CheckInputstreamInstalledAndEnabled(channel.GetInputStreamName());

    if (channel.GetInputStreamName() == INPUTSTREAM_FFMPEGDIRECT)
    {
      InspectAndSetFFmpegDirectStreamProperties(properties, channel.GetMimeType(), channel.GetProperty("inputstream.ffmpegdirect.manifest_type"), channel.GetCatchupMode(), channel.IsCatchupTSStream(), streamURL, settings);

      if (channel.SupportsLiveStreamTimeshifting() && isChannelURL &&
          channel.GetProperty("inputstream.ffmpegdirect.stream_mode").empty() &&
          settings->AlwaysEnableTimeshiftModeIfMissing())
      {
        properties.emplace_back("inputstream.ffmpegdirect.stream_mode", "timeshift");
        if (channel.GetProperty("inputstream.ffmpegdirect.is_realtime_stream").empty())
          properties.emplace_back("inputstream.ffmpegdirect.is_realtime_stream", "true");
      }
    }
  }
  else
  {
    StreamType streamType = StreamUtils::GetStreamType(streamURL, channel.GetProperty(PVR_STREAM_PROPERTY_MIMETYPE), channel.IsCatchupTSStream());
    if (streamType == StreamType::OTHER_TYPE)
      streamType = StreamUtils::InspectStreamType(streamURL, channel.GetCatchupMode());

    // Using kodi's built in inputstreams
    if (StreamUtils::UseKodiInputstreams(streamType, settings))
    {
      std::string ffmpegStreamURL = StreamUtils::GetURLWithFFmpegReconnectOptions(streamURL, streamType, channel.GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAM), channel.GetProperty("http-reconnect") == "true" , settings);

      properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, streamURL);
      if (!channel.HasMimeType() && StreamUtils::HasMimeType(streamType))
        properties.emplace_back(PVR_STREAM_PROPERTY_MIMETYPE, StreamUtils::GetMimeType(streamType));

      if (streamType == StreamType::HLS || streamType == StreamType::TS || streamType == StreamType::OTHER_TYPE)
      {
        if (channel.IsCatchupSupported() && channel.CatchupSupportsTimeshifting() &&
            CheckInputstreamInstalledAndEnabled(CATCHUP_INPUTSTREAM_NAME))
        {
          properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, CATCHUP_INPUTSTREAM_NAME);
          // this property is required to force VideoPlayer for Radio channels
          properties.emplace_back("inputstream-player", "videodefaultplayer");
          SetFFmpegDirectManifestTypeStreamProperty(properties, channel.GetProperty("inputstream.ffmpegdirect.manifest_type"), streamURL, streamType);
        }
        else if (channel.SupportsLiveStreamTimeshifting() && isChannelURL &&
                 CheckInputstreamInstalledAndEnabled(INPUTSTREAM_FFMPEGDIRECT))
        {
          properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, INPUTSTREAM_FFMPEGDIRECT);
          // this property is required to force VideoPlayer for Radio channels
          properties.emplace_back("inputstream-player", "videodefaultplayer");
          SetFFmpegDirectManifestTypeStreamProperty(properties, channel.GetProperty("inputstream.ffmpegdirect.manifest_type"), streamURL, streamType);
          properties.emplace_back("inputstream.ffmpegdirect.stream_mode", "timeshift");
          properties.emplace_back("inputstream.ffmpegdirect.is_realtime_stream", "true");
        }
        else if (streamType == StreamType::HLS || streamType == StreamType::TS)
        {
          properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG);
        }
      }
    }
    else // inputstream.adaptive
    {
      CheckInputstreamInstalledAndEnabled(INPUTSTREAM_ADAPTIVE);

      bool streamUrlSet = false;

      // If no media headers are explicitly set for inputstream.adaptive,
      // strip the headers from streamURL and put it to media headers property

      if (channel.GetProperty("inputstream.adaptive.manifest_headers").empty() &&
          channel.GetProperty("inputstream.adaptive.stream_headers").empty())
      {
        // No stream headers declared by property, check if stream URL has any
        std::string url;
        std::string encodedProtocolOptions;
        if (SplitUrlProtocolOpts(streamURL, url, encodedProtocolOptions))
        {
          // Set stream URL without headers and encoded headers as property
          properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, url);
          properties.emplace_back("inputstream.adaptive.manifest_headers", encodedProtocolOptions);
          properties.emplace_back("inputstream.adaptive.stream_headers", encodedProtocolOptions);
          streamUrlSet = true;
        }
      }

      // Set intact stream URL if not previously set
      if (!streamUrlSet)
        properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, streamURL);

      properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, INPUTSTREAM_ADAPTIVE);

      if (streamType == StreamType::HLS || streamType == StreamType::DASH ||
          streamType == StreamType::SMOOTH_STREAMING)
        properties.emplace_back(PVR_STREAM_PROPERTY_MIMETYPE, StreamUtils::GetMimeType(streamType));
    }
  }

  if (!channel.GetProperties().empty())
  {
    for (auto& prop : channel.GetProperties())
      properties.emplace_back(prop.first, prop.second);
  }

  if (!catchupProperties.empty())
  {
    for (auto& prop : catchupProperties)
      properties.emplace_back(prop.first, prop.second);
  }
}

void StreamUtils::SetAllStreamProperties(std::vector<kodi::addon::PVRStreamProperty>& properties, const iptvsimple::data::MediaEntry& mediaEntry, const std::string& streamURL, std::shared_ptr<InstanceSettings>& settings)
{
  // Check if the media entry has explicitly set up the use of inputstream.adaptive,
  // if so, the best behaviour for media services is:
  // - Always add mimetype to prevent kodi core to make an HTTP HEADER requests
  //   this because in some cases services refuse this request and can also deny downloads
  // - If requested by settings, always add the "user-agent" header to ISA properties
  const bool isISAdaptiveSet =
      mediaEntry.GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAM) == INPUTSTREAM_ADAPTIVE;

  if (!isISAdaptiveSet && !mediaEntry.GetInputStreamName().empty())
  {
    // Media Entry has an inputstream class set so we only set the stream URL
    properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, streamURL);

    if (mediaEntry.GetInputStreamName() != PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG)
      CheckInputstreamInstalledAndEnabled(mediaEntry.GetInputStreamName());

    if (mediaEntry.GetInputStreamName() == INPUTSTREAM_FFMPEGDIRECT)
      InspectAndSetFFmpegDirectStreamProperties(properties, mediaEntry.GetMimeType(), mediaEntry.GetProperty("inputstream.ffmpegdirect.manifest_type"), CatchupMode::DISABLED, false, streamURL, settings);
  }
  else
  {
    StreamType streamType = StreamUtils::GetStreamType(streamURL, mediaEntry.GetProperty(PVR_STREAM_PROPERTY_MIMETYPE), false);
    if (streamType == StreamType::OTHER_TYPE)
      streamType = StreamUtils::InspectStreamType(streamURL, CatchupMode::DISABLED);

    // Using kodi's built in inputstreams
    if (!isISAdaptiveSet && StreamUtils::UseKodiInputstreams(streamType, settings))
    {
      std::string ffmpegStreamURL = StreamUtils::GetURLWithFFmpegReconnectOptions(streamURL, streamType, mediaEntry.GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAM), mediaEntry.GetProperty("http-reconnect") == "true" , settings);

      properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, streamURL);
      if (!mediaEntry.HasMimeType() && StreamUtils::HasMimeType(streamType))
        properties.emplace_back(PVR_STREAM_PROPERTY_MIMETYPE, StreamUtils::GetMimeType(streamType));

      if (streamType == StreamType::HLS || streamType == StreamType::TS)
      {
        properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG);
      }
    }
    else // inputstream.adaptive
    {
      CheckInputstreamInstalledAndEnabled(INPUTSTREAM_ADAPTIVE);

      bool streamUrlSet = false;

      // If no media headers are explicitly set for inputstream.adaptive,
      // strip the headers from streamURL and put it to media headers property

      if (mediaEntry.GetProperty("inputstream.adaptive.manifest_headers").empty() &&
          mediaEntry.GetProperty("inputstream.adaptive.stream_headers").empty())
      {
        // No stream headers declared by property, check if stream URL has any
        std::string url;
        std::string encodedProtocolOptions;
        if (SplitUrlProtocolOpts(streamURL, url, encodedProtocolOptions))
        {
          // Set stream URL without headers and encoded headers as property
          properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, url);
          properties.emplace_back("inputstream.adaptive.manifest_headers", encodedProtocolOptions);
          properties.emplace_back("inputstream.adaptive.stream_headers", encodedProtocolOptions);
          streamUrlSet = true;
        }
      }

      // Set intact stream URL if not previously set
      if (!streamUrlSet)
        properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, streamURL);

      if (!isISAdaptiveSet)
        properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, INPUTSTREAM_ADAPTIVE);

      if (streamType == StreamType::HLS || streamType == StreamType::DASH ||
          streamType == StreamType::SMOOTH_STREAMING)
        properties.emplace_back(PVR_STREAM_PROPERTY_MIMETYPE, StreamUtils::GetMimeType(streamType));
    }
  }

  if (!mediaEntry.GetProperties().empty())
  {
    for (auto& prop : mediaEntry.GetProperties())
      properties.emplace_back(prop.first, prop.second);
  }
}

bool StreamUtils::CheckInputstreamInstalledAndEnabled(const std::string& inputstreamName)
{
  std::string version;
  bool enabled;

  if (kodi::IsAddonAvailable(inputstreamName, version, enabled))
  {
    if (!enabled)
    {
      std::string message = StringUtils::Format(kodi::addon::GetLocalizedString(30502).c_str(), inputstreamName.c_str());
      kodi::QueueNotification(QueueMsg::QUEUE_ERROR, kodi::addon::GetLocalizedString(30500), message);
    }
  }
  else // Not installed
  {
    std::string message = StringUtils::Format(kodi::addon::GetLocalizedString(30501).c_str(), inputstreamName.c_str());
    kodi::QueueNotification(QueueMsg::QUEUE_ERROR, kodi::addon::GetLocalizedString(30500), message);
  }

  return true;
}

void StreamUtils::InspectAndSetFFmpegDirectStreamProperties(std::vector<kodi::addon::PVRStreamProperty>& properties, const std::string& mimeType, const std::string& manifestType, CatchupMode catchupMode, bool isCatchupTSStream, const std::string& streamURL, std::shared_ptr<InstanceSettings>& settings)
{
  // If there is no MIME type and no manifest type (BOTH!) set then potentially inspect the stream and set them
  if (!mimeType.empty() && !manifestType.empty())
  {
    StreamType streamType = StreamUtils::GetStreamType(streamURL, mimeType, isCatchupTSStream);
    if (streamType == StreamType::OTHER_TYPE)
      streamType = StreamUtils::InspectStreamType(streamURL, catchupMode);

    if (mimeType.empty() && StreamUtils::HasMimeType(streamType))
      properties.emplace_back(PVR_STREAM_PROPERTY_MIMETYPE, StreamUtils::GetMimeType(streamType));

    SetFFmpegDirectManifestTypeStreamProperty(properties, manifestType, streamURL, streamType);
  }
}

void StreamUtils::SetFFmpegDirectManifestTypeStreamProperty(std::vector<kodi::addon::PVRStreamProperty>& properties, const std::string& manifestType, const std::string& streamURL, const StreamType& streamType)
{
  std::string newManifestType;
  if (manifestType.empty())
    newManifestType = StreamUtils::GetManifestType(streamType);
  if (!newManifestType.empty())
    properties.emplace_back("inputstream.ffmpegdirect.manifest_type", newManifestType);
}

std::string StreamUtils::GetEffectiveInputStreamName(const StreamType& streamType, const iptvsimple::data::Channel& channel, std::shared_ptr<InstanceSettings>& settings)
{
  std::string inputStreamName = channel.GetInputStreamName();

  if (inputStreamName.empty())
  {
    if (StreamUtils::UseKodiInputstreams(streamType, settings))
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

const StreamType StreamUtils::GetStreamType(const std::string& url, const std::string& mimeType, bool isCatchupTSStream)
{
  if (StringUtils::StartsWith(url, "plugin://"))
    return StreamType::PLUGIN;

  if (url.find(".m3u8") != std::string::npos ||
      mimeType == "application/x-mpegURL" ||
      mimeType == "application/vnd.apple.mpegurl")
    return StreamType::HLS;

  if (url.find(".mpd") != std::string::npos || mimeType == "application/xml+dash")
    return StreamType::DASH;

  if (url.find(".ism") != std::string::npos &&
      !(url.find(".ismv") != std::string::npos || url.find(".isma") != std::string::npos))
    return StreamType::SMOOTH_STREAMING;

  if (mimeType == "video/mp2t" || isCatchupTSStream)
    return StreamType::TS;

  // it has a MIME type but not one we recognise
  if (!mimeType.empty())
    return StreamType::MIME_TYPE_UNRECOGNISED;

  return StreamType::OTHER_TYPE;
}

const StreamType StreamUtils::InspectStreamType(const std::string& url, CatchupMode catchupMode)
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

  // If we can't inspect the stream type the only option left for default, append or shift mode is TS
  if (catchupMode == CatchupMode::DEFAULT ||
      catchupMode == CatchupMode::APPEND ||
      catchupMode == CatchupMode::SHIFT ||
      catchupMode == CatchupMode::TIMESHIFT)
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
    case StreamType::SMOOTH_STREAMING:
      return "application/vnd.ms-sstr+xml";
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

std::string StreamUtils::GetURLWithFFmpegReconnectOptions(const std::string& streamUrl, const StreamType& streamType, const std::string& inputstreamName, bool hasHTTPReconnect, std::shared_ptr<InstanceSettings>& settings)
{
  std::string newStreamUrl = streamUrl;

  if (WebUtils::IsHttpUrl(streamUrl) && SupportsFFmpegReconnect(streamType, inputstreamName) &&
      (hasHTTPReconnect|| settings->UseFFmpegReconnect()))
  {
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect", "1");
    if (streamType != StreamType::HLS)
      newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_at_eof", "1");
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_streamed", "1");
    newStreamUrl = AddHeaderToStreamUrl(newStreamUrl, "reconnect_delay_max", "4294");

    Logger::Log(LogLevel::LEVEL_DEBUG, "%s - FFmpeg Reconnect Stream URL: %s", __FUNCTION__, WebUtils::RedactUrl(newStreamUrl).c_str());
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

bool StreamUtils::UseKodiInputstreams(const StreamType& streamType, std::shared_ptr<iptvsimple::InstanceSettings>& settings)
{
  return streamType == StreamType::OTHER_TYPE || streamType == StreamType::TS || streamType == StreamType::PLUGIN ||
        (streamType == StreamType::HLS && !settings->UseInputstreamAdaptiveforHls());
}

bool StreamUtils::ChannelSpecifiesInputstream(const iptvsimple::data::Channel& channel)
{
  return !channel.GetInputStreamName().empty();
}

bool StreamUtils::SupportsFFmpegReconnect(const StreamType& streamType, const std::string& inputstreamName)
{
  return streamType == StreamType::HLS ||
         inputstreamName == PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG;
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
