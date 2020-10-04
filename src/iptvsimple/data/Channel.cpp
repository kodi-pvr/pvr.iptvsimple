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

#include <regex>

#include <kodi/General.h>
#include <kodi/tools/StringUtils.h>

using namespace kodi::tools;
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
    case CatchupMode::VOD:
      return "VOD";
    default:
      return "";
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
  left.m_catchupSupportsTimeshifting = m_catchupSupportsTimeshifting;
  left.m_catchupSourceTerminates = m_catchupSourceTerminates;
  left.m_catchupGranularitySeconds = m_catchupGranularitySeconds;
  left.m_catchupCorrectionSecs = m_catchupCorrectionSecs;
  left.m_tvgId            = m_tvgId;
  left.m_tvgName          = m_tvgName;
  left.m_properties       = m_properties;
  left.m_inputStreamName = m_inputStreamName;
}

void Channel::UpdateTo(kodi::addon::PVRChannel& left) const
{
  left.SetUniqueId(m_uniqueId);
  left.SetIsRadio(m_radio);
  left.SetChannelNumber(m_channelNumber);
  left.SetChannelName(m_channelName);
  left.SetEncryptionSystem(m_encryptionSystem);
  left.SetIconPath(m_iconPath);
  left.SetIsHidden(false);
  left.SetHasArchive(IsCatchupSupported());
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
  m_catchupSupportsTimeshifting = false;
  m_catchupSourceTerminates = false;
  m_catchupGranularitySeconds = 1;
  m_isCatchupTSStream = false;
  m_catchupCorrectionSecs = 0;
  m_tvgId.clear();
  m_tvgName.clear();
  m_properties.clear();
  m_inputStreamName.clear();
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

  kodi::UnknownToUTF8(m_iconPath, m_iconPath);

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
    if (!Settings::GetInstance().GetDefaultUserAgent().empty() && GetProperty("http-user-agent").empty())
      AddProperty("http-user-agent", Settings::GetInstance().GetDefaultUserAgent());

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

  if (!Settings::GetInstance().GetDefaultInputstream().empty() && GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAM).empty())
    AddProperty(PVR_STREAM_PROPERTY_INPUTSTREAM, Settings::GetInstance().GetDefaultInputstream());

  if (!Settings::GetInstance().GetDefaultMimeType().empty() && GetProperty(PVR_STREAM_PROPERTY_MIMETYPE).empty())
    AddProperty(PVR_STREAM_PROPERTY_MIMETYPE, Settings::GetInstance().GetDefaultMimeType());

  m_inputStreamName = GetProperty(PVR_STREAM_PROPERTY_INPUTSTREAM);
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
  if (catchupDays > 0 || catchupDays == IGNORE_CATCHUP_DAYS)
    m_catchupDays = catchupDays;
  else
    m_catchupDays = Settings::GetInstance().GetCatchupDays();
}

bool Channel::IsCatchupSupported() const
{
  return Settings::GetInstance().IsCatchupEnabled() && m_hasCatchup && !m_catchupSource.empty();
}

bool Channel::SupportsLiveStreamTimeshifting() const
{
  return Settings::GetInstance().IsTimeshiftEnabled() && GetProperty(PVR_STREAM_PROPERTY_ISREALTIMESTREAM) == "true" &&
         (Settings::GetInstance().IsTimeshiftEnabledAll() ||
          (Settings::GetInstance().IsTimeshiftEnabledHttp() && StringUtils::StartsWith(m_streamURL, "http")) ||
          (Settings::GetInstance().IsTimeshiftEnabledUdp() && StringUtils::StartsWith(m_streamURL, "udp"))
         );
}

namespace
{
bool IsValidTimeshiftingCatchupSource(const std::string& formatString, const CatchupMode& catchupMode)
{
  // match any specifier, i.e. anything inside curly braces
  std::regex const specifierRegex("\\{[^{]+\\}");

  std::ptrdiff_t const numSpecifiers(std::distance(
    std::sregex_iterator(formatString.begin(), formatString.end(), specifierRegex),
    std::sregex_iterator()));

  if (numSpecifiers > 0)
  {
    // If we only have a catchup-id specifier and nothing else then we can't timeshift
    if ((formatString.find("{catchup-id}") != std::string::npos && numSpecifiers == 1) ||
        catchupMode == CatchupMode::VOD)
      return false;

    return true;
  }

  return false;
}

bool IsTerminatingCatchupSource(const std::string& formatString)
{
  // A catchup stream terminates if it has an end time specifier
  if (formatString.find("{duration}") != std::string::npos ||
      formatString.find("{duration:") != std::string::npos ||
      formatString.find("{lutc}") != std::string::npos ||
      formatString.find("${timestamp}") != std::string::npos ||
      formatString.find("{utcend}") != std::string::npos ||
      formatString.find("${end}") != std::string::npos)
    return true;

  return false;
}

int FindCatchupSourceGranularitySeconds(const std::string& formatString)
{
  // A catchup stream has one second granularity if it supports these specifiers
  if (formatString.find("{utc}") != std::string::npos ||
      formatString.find("${start}") != std::string::npos ||
      formatString.find("{S}") != std::string::npos ||
      formatString.find("{offset:1}") != std::string::npos)
    return 1;

  return 60;
}

} // unnamed namespace

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
    case CatchupMode::VOD:
      if (!m_catchupSource.empty())
      {
        if (m_catchupSource.find_first_of('|') != std::string::npos)
          appendProtocolOptions = false;
        break;
      }
      m_catchupSource = "{catchup-id}";
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

    m_catchupSupportsTimeshifting = IsValidTimeshiftingCatchupSource(m_catchupSource, m_catchupMode);
    m_catchupSourceTerminates = IsTerminatingCatchupSource(m_catchupSource);
    m_catchupGranularitySeconds = FindCatchupSourceGranularitySeconds(m_catchupSource);
    Logger::Log(LEVEL_DEBUG, "Channel Catchup Format string properties: %s, valid timeshifting source: %s, terminating source: %s, granularity secs: %d",
                m_channelName.c_str(), m_catchupSupportsTimeshifting ? "true" : "false", m_catchupSourceTerminates ? "true" : "false", m_catchupGranularitySeconds);
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
  if (url.find('?') != std::string::npos)
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

  if (std::regex_match(url, matches, fsRegex))
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

  if (std::regex_match(url, matches, xcRegex))
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
