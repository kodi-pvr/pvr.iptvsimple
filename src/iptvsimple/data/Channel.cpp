/*
 *      Copyright (C) 2005-2019 Team XBMC
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

#include "Channel.h"

#include "../Settings.h"
#include "../utilities/FileUtils.h"
#include "../utilities/Logger.h"
#include "../utilities/StreamUtils.h"
#include "../utilities/WebUtils.h"
#include "../../client.h"

#include <p8-platform/util/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

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
  m_catchupMode = CatchupMode::DEFAULT;
  m_catchupDays = 0;
  m_catchupSource.clear();
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
  return Settings::GetInstance().IsCatchupEnabled() &&
         (m_hasCatchup || Settings::GetInstance().AllChannelsSupportCatchup()) &&
         !(m_catchupSource.empty() && Settings::GetInstance().GetCatchupQueryFormat().empty());
}
