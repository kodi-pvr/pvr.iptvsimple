/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Channel.h"

#include "../Settings.h"
#include "../utilities/FileUtils.h"
#include "../utilities/Logger.h"
#include "../utilities/StreamUtils.h"
#include "../utilities/WebUtils.h"
#include "../../client.h"

#include <regex>

#include <p8-platform/util/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

const std::string Channel::GetCatchupModeText(const CatchupMode& catchupMode)
{
  switch (catchupMode)
  {
    case CatchupMode::DISABLED:
      return "Disabled";
    case CatchupMode::DEFAULT:
      return "Default";
    case CatchupMode::APPEND:
      return "Append";
    case CatchupMode::TIMESHIFT:
    case CatchupMode::SHIFT:
      return "Shift (SIPTV)";
    case CatchupMode::FLUSSONIC:
      return "Flussonic";
    case CatchupMode::XTREAM_CODES:
      return "Xtream codes";
  }
}

void Channel::UpdateTo(Channel& left) const
{
  left.m_uniqueId         = m_uniqueId;
  left.m_radio            = m_radio;
  left.m_channelNumber    = m_channelNumber;
  left.m_encryptionSystem = m_encryptionSystem;
  left.m_tvgShift         = m_tvgShift;
  left.m_channelName      = m_channelName;
  left.m_iconPath         = m_iconPath;
  left.m_streamURL        = m_streamURL;
  left.m_hasCatchup       = m_hasCatchup;
  left.m_catchupMode      = m_catchupMode;
  left.m_catchupDays      = m_catchupDays;
  left.m_catchupSource    = m_catchupSource;
  left.m_isCatchupTSStream = m_isCatchupTSStream;
  left.m_tvgId            = m_tvgId;
  left.m_tvgName          = m_tvgName;
  left.m_properties       = m_properties;
  left.m_inputStreamClass = m_inputStreamClass;
}

void Channel::UpdateTo(PVR_CHANNEL& left) const
{
  left.iUniqueId = m_uniqueId;
  left.bIsRadio = m_radio;
  left.iChannelNumber = m_channelNumber;
  strncpy(left.strChannelName, m_channelName.c_str(), sizeof(left.strChannelName) - 1);
  left.iEncryptionSystem = m_encryptionSystem;
  strncpy(left.strIconPath, m_iconPath.c_str(), sizeof(left.strIconPath) - 1);
  left.bIsHidden = false;
  left.bHasArchive = IsCatchupSupported();
}

void Channel::Reset()
{
  m_uniqueId = 0;
  m_radio = false;
  m_channelNumber = 0;
  m_encryptionSystem = 0;
  m_tvgShift = 0;
  m_channelName.clear();
  m_iconPath.clear();
  m_streamURL.clear();
  m_hasCatchup = false;
  m_catchupMode = CatchupMode::DISABLED;
  m_catchupDays = 0;
  m_catchupSource.clear();
  m_isCatchupTSStream = false;
  m_tvgId.clear();
  m_tvgName.clear();
  m_properties.clear();
  m_inputStreamClass.clear();
}

void Channel::SetIconPathFromTvgLogo(const std::string& tvgLogo, std::string& channelName)
{
  m_iconPath = tvgLogo;

  bool logoSetFromChannelName = false;
  if (m_iconPath.empty())
  {
    m_iconPath = m_channelName;
    logoSetFromChannelName = true;
  }

  m_iconPath = XBMC->UnknownToUTF8(m_iconPath.c_str());

  // urlencode channel logo when set from channel name and source is Remote Path
  // append extension as channel name wouldn't have it
  if (logoSetFromChannelName && Settings::GetInstance().GetLogoPathType() == PathType::REMOTE_PATH)
    m_iconPath = utilities::WebUtils::UrlEncode(m_iconPath);

  if (m_iconPath.find("://") == std::string::npos)
  {
    const std::string& logoLocation = Settings::GetInstance().GetLogoLocation();
    if (!logoLocation.empty())
    {
      // not absolute path, only append .png in this case.
      m_iconPath = utilities::FileUtils::PathCombine(logoLocation, m_iconPath);

      if (!StringUtils::EndsWithNoCase(m_iconPath, ".png") && !StringUtils::EndsWithNoCase(m_iconPath, ".jpg"))
        m_iconPath += CHANNEL_LOGO_EXTENSION;
    }
  }
}

void Channel::SetStreamURL(const std::string& url)
{
  m_streamURL = url;

  if (StringUtils::StartsWith(url, HTTP_PREFIX) || StringUtils::StartsWith(url, HTTPS_PREFIX))
  {
    if (!Settings::GetInstance().GetUserAgent().empty() && GetProperty("http-user-agent").empty())
      AddProperty("http-user-agent", Settings::GetInstance().GetUserAgent());

    TryToAddPropertyAsHeader("http-user-agent", "user-agent");
    TryToAddPropertyAsHeader("http-referrer", "referer"); // spelling differences are correct
  }

  if (Settings::GetInstance().TransformMulticastStreamUrls() &&
      (StringUtils::StartsWith(url, UDP_MULTICAST_PREFIX) || StringUtils::StartsWith(url, RTP_MULTICAST_PREFIX)))
  {
    const std::string typePath = StringUtils::StartsWith(url, "rtp") ? "/rtp/" : "/udp/";

    m_streamURL = "http://" + Settings::GetInstance().GetUdpxyHost() + ":" + std::to_string(Settings::GetInstance().GetUdpxyPort()) + typePath + url.substr(UDP_MULTICAST_PREFIX.length());
    Logger::Log(LEVEL_DEBUG, "%s - Transformed multicast stream URL to local relay url: %s", __FUNCTION__, m_streamURL.c_str());
  }

  m_inputStreamClass = GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAMCLASS);
  if (m_inputStreamClass.empty())
    m_inputStreamClass = GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAMADDON);
}

std::string Channel::GetProperty(const std::string& propName) const
{
  auto propPair = m_properties.find(propName);
  if (propPair != m_properties.end())
    return propPair->second;

  return {};
}

void Channel::RemoveProperty(const std::string& propName)
{
  m_properties.erase(propName);
}

void Channel::TryToAddPropertyAsHeader(const std::string& propertyName, const std::string& headerName)
{
  const std::string value = GetProperty(propertyName);

  if (!value.empty())
  {
    m_streamURL = StreamUtils::AddHeaderToStreamUrl(m_streamURL, headerName, value);

    RemoveProperty(propertyName);
  }
}

void Channel::SetCatchupDays(int catchupDays)
{
  if (catchupDays != 0)
    m_catchupDays = catchupDays;
  else
    m_catchupDays = Settings::GetInstance().GetCatchupDays();
}

bool Channel::IsCatchupSupported() const
{
  return Settings::GetInstance().IsCatchupEnabled() && m_hasCatchup && !m_catchupSource.empty();
}

void Channel::ConfigureCatchupMode()
{
  bool invalidCatchupSource = false;
  bool appendProtocolOptions = true;

  // preserve any kodi protocol options after "|"
  std::string url = m_streamURL;
  std::string protocolOptions;
  size_t found = m_streamURL.find_first_of('|');
  if (found != std::string::npos)
  {
    url = m_streamURL.substr(0, found);
    protocolOptions = m_streamURL.substr(found, m_streamURL.length());
  }

  if (Settings::GetInstance().GetAllChannelsCatchupMode() != CatchupMode::DISABLED &&
      (m_catchupMode == CatchupMode::DISABLED || m_catchupMode == CatchupMode::TIMESHIFT))
  {
    // As CatchupMode::TIMESHIFT is obsolete and some providers use it
    // incorrectly we allow this setting to override it
    m_catchupMode = Settings::GetInstance().GetAllChannelsCatchupMode();
    m_hasCatchup = true;
  }

  switch (m_catchupMode)
  {
    case CatchupMode::DISABLED:
      invalidCatchupSource = true;
      break;
    case CatchupMode::DEFAULT:
      if (!m_catchupSource.empty())
      {
        if (m_catchupSource.find_first_of('|') != std::string::npos)
          appendProtocolOptions = false;
        break;
      }
      if (!GenerateAppendCatchupSource(url))
        invalidCatchupSource = true;
      break;
    case CatchupMode::APPEND:
      if (!GenerateAppendCatchupSource(url))
        invalidCatchupSource = true;
      break;
    case CatchupMode::TIMESHIFT:
    case CatchupMode::SHIFT:
      GenerateShiftCatchupSource(url);
      break;
    case CatchupMode::FLUSSONIC:
      if (!GenerateFlussonicCatchupSource(url))
        invalidCatchupSource = true;
      break;
    case CatchupMode::XTREAM_CODES:
      if (!GenerateXtreamCodesCatchupSource(url))
        invalidCatchupSource = true;
      break;
  }

  if (invalidCatchupSource)
  {
    m_catchupMode = CatchupMode::DISABLED;
    m_hasCatchup = false;
    m_catchupSource.clear();
  }
  else
  {
    if (!protocolOptions.empty() && appendProtocolOptions)
      m_catchupSource += protocolOptions;
  }

  if (m_catchupMode != CatchupMode::DISABLED)
    Logger::Log(LEVEL_DEBUG, "%s - %s - %s: %s", __FUNCTION__, GetCatchupModeText(m_catchupMode).c_str(), m_channelName.c_str(), m_catchupSource.c_str());
}

bool Channel::GenerateAppendCatchupSource(const std::string& url)
{
  if (!m_catchupSource.empty())
  {
    m_catchupSource = url + m_catchupSource;
    return true;
  }
  else
  {
    if (!Settings::GetInstance().GetCatchupQueryFormat().empty())
    {
      m_catchupSource = url + Settings::GetInstance().GetCatchupQueryFormat();
      return true;
    }
  }

  return false;
}

void Channel::GenerateShiftCatchupSource(const std::string& url)
{
  if (m_streamURL.find('?') != std::string::npos)
    m_catchupSource = url + "&utc={utc}&lutc={lutc}";
  else
    m_catchupSource = url + "?utc={utc}&lutc={lutc}";
}

bool Channel::GenerateFlussonicCatchupSource(const std::string& url)
{
  // Example stream and catchup URLs
  // stream:  http://ch01.spr24.net/151/mpegts?token=my_token
  // catchup: http://ch01.spr24.net/151/timeshift_abs-{utc}.ts?token=my_token
  // stream:  http://list.tv:8888/325/index.m3u8?token=secret
  // catchup: http://list.tv:8888/325/timeshift_rel-{offset:1}.m3u8?token=secret
  // stream:  http://list.tv:8888/325/mono.m3u8?token=secret
  // catchup: http://list.tv:8888/325/mono-timeshift_rel-{offset:1}.m3u8?token=secret

  static std::regex fsRegex("^(http[s]?://[^/]+)/([^/]+)/([^/]*)(mpegts|\\.m3u8)(\\?.+=.+)?$");
  std::smatch matches;

  if (std::regex_match(m_streamURL, matches, fsRegex))
  {
    if (matches.size() == 6)
    {
      const std::string fsHost = matches[1].str();
      const std::string fsChannelId = matches[2].str();
      const std::string fsListType = matches[3].str();
      const std::string fsStreamType = matches[4].str();
      const std::string fsUrlAppend = matches[5].str();

      m_isCatchupTSStream = fsStreamType == "mpegts";
      if (m_isCatchupTSStream)
      {
        m_catchupSource = fsHost + "/" + fsChannelId + "/timeshift_abs-${start}.ts" + fsUrlAppend;
      }
      else
      {
        if (fsListType == "index")
          m_catchupSource = fsHost + "/" + fsChannelId + "/timeshift_rel-{offset:1}.m3u8" + fsUrlAppend;
        else
          m_catchupSource = fsHost + "/" + fsChannelId + "/" + fsListType + "-timeshift_rel-{offset:1}.m3u8" + fsUrlAppend;
      }

      return true;
    }
  }

  return false;
}

bool Channel::GenerateXtreamCodesCatchupSource(const std::string& url)
{
  // Example stream and catchup URLs
  // stream:  http://list.tv:8080/my@account.xc/my_password/1477
  // catchup: http://list.tv:8080/timeshift/my@account.xc/my_password/{duration}/{Y}-{m}-{d}:{H}-{M}/1477.ts
  // stream:  http://list.tv:8080/live/my@account.xc/my_password/1477.m3u8
  // catchup: http://list.tv:8080/timeshift/my@account.xc/my_password/{duration}/{Y}-{m}-{d}:{H}-{M}/1477.m3u8

  static std::regex xcRegex("^(http[s]?://[^/]+)/(?:live/)?([^/]+)/([^/]+)/([^/\\.]+)(\\.m3u[8]?)?$");
  std::smatch matches;

  if (std::regex_match(m_streamURL, matches, xcRegex))
  {
    if (matches.size() == 6)
    {
      const std::string xcHost = matches[1].str();
      const std::string xcUsername = matches[2].str();
      const std::string xcPasssword = matches[3].str();
      const std::string xcChannelId = matches[4].str();
      std::string xcExtension;
      if (matches[5].matched)
        xcExtension = matches[5].str();

      if (xcExtension.empty())
      {
        m_isCatchupTSStream = true;
        xcExtension = ".ts";
      }

      m_catchupSource = xcHost + "/timeshift/" + xcUsername + "/" + xcPasssword +
                        "/{duration:60}/{Y}-{m}-{d}:{H}-{M}/" + xcChannelId + xcExtension;

      return true;
    }
  }

  return false;
}