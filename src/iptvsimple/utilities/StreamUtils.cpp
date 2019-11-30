/*
 *      Copyright (C) 2005-2019 Team Kodi
 *      http://kodi.tv
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

#include "StreamUtils.h"

#include "../Settings.h"
#include "FileUtils.h"
#include "Logger.h"
#include "WebUtils.h"

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

const StreamType StreamUtils::GetStreamType(const std::string& url, const Channel& channel)
{
  if (url.find(".m3u8") != std::string::npos ||
      channel.GetProperty(PVR_STREAM_PROPERTY_MIMETYPE) == "application/x-mpegURL" ||
      channel.GetProperty(PVR_STREAM_PROPERTY_MIMETYPE) == "application/vnd.apple.mpegurl")
    return StreamType::HLS;

  if (url.find(".mpd") != std::string::npos ||
      channel.GetProperty(PVR_STREAM_PROPERTY_MIMETYPE) == "application/xml+dash")
    return StreamType::DASH;

  if (url.find(".ism") != std::string::npos &&
      !(url.find(".ismv") != std::string::npos || url.find(".isma") != std::string::npos))
    return StreamType::SMOOTH_STREAMING;

  return StreamType::OTHER_TYPE;
}

const StreamType StreamUtils::InspectStreamType(const std::string& url)
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
    default:
      return "";
  }
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
  std::string newStreamUrl = streamUrl;

  bool hasProtocolOptions = false;
  bool addHeader = true;
  size_t found = newStreamUrl.find("|");

  if (found != std::string::npos)
  {
    hasProtocolOptions = true;
    addHeader = newStreamUrl.find(headerName + "=", found + 1) == std::string::npos;
  }

  if (addHeader)
  {
    if (!hasProtocolOptions)
      newStreamUrl += "|";
    else
      newStreamUrl += "&";

    newStreamUrl += headerName + "=" + headerValue;
  }

  return newStreamUrl;
}

bool StreamUtils::UseKodiInputstreams(const StreamType& streamType)
{
  return streamType == StreamType::OTHER_TYPE || (streamType == StreamType::HLS && !Settings::GetInstance().UseInputstreamAdaptiveforHls());
}

bool StreamUtils::ChannelSpecifiesInputstream(const iptvsimple::data::Channel& channel)
{
  return !(channel.GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAMCLASS).empty() && channel.GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAMADDON).empty());
}

bool StreamUtils::SupportsFFmpegReconnect(const StreamType& streamType, const iptvsimple::data::Channel& channel)
{
  return streamType == StreamType::HLS ||
         channel.GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAMCLASS) == PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG ||
         channel.GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAMADDON) == PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG;
}
