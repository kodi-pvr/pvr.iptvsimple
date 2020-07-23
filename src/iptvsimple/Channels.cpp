/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Channels.h"

#include "ChannelGroups.h"
#include "Settings.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"

#include <regex>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

bool Channels::Init()
{
  Clear();
  return true;
}

void Channels::Clear()
{
  m_channels.clear();
  m_currentChannelNumber = Settings::GetInstance().GetStartChannelNumber();
}

int Channels::GetChannelsAmount() const
{
  return m_channels.size();
}

PVR_ERROR Channels::GetChannels(kodi::addon::PVRChannelsResultSet& results, bool radio) const
{
  // We set a channel order here that applies to the 'Any channels' group in kodi-pvr
  // This allows the users to use the 'Backend Order' sort option in the left to
  // have the same order as the backend (regardles of the channel numbering used)
  int channelOrder = 1;
  for (const auto& channel : m_channels)
  {
    if (channel.IsRadio() == radio)
    {
      Logger::Log(LEVEL_DEBUG, "%s - Transfer channel '%s', ChannelId '%d', ChannelNumber: '%d'", __FUNCTION__, channel.GetChannelName().c_str(),
                  channel.GetUniqueId(), channel.GetChannelNumber());
      kodi::addon::PVRChannel kodiChannel;

      channel.UpdateTo(kodiChannel);
      kodiChannel.SetOrder(channelOrder++); // Keep the channels in list order as per the M3U

      results.Add(kodiChannel);
    }
  }

  Logger::Log(LEVEL_DEBUG, "%s - channels available '%d', radio = %d", __FUNCTION__, m_channels.size(), radio);

  return PVR_ERROR_NO_ERROR;
}

bool Channels::GetChannel(const kodi::addon::PVRChannel& channel, Channel& myChannel) const
{
  return GetChannel(static_cast<int>(channel.GetUniqueId()), myChannel);
}

bool Channels::GetChannel(int uniqueId, Channel& myChannel) const
{
  for (const auto& thisChannel : m_channels)
  {
    if (thisChannel.GetUniqueId() == uniqueId)
    {
      thisChannel.UpdateTo(myChannel);

      return true;
    }
  }

  return false;
}

void Channels::AddChannel(Channel& channel, std::vector<int>& groupIdList, ChannelGroups& channelGroups)
{
  m_currentChannelNumber = channel.GetChannelNumber();
  channel.SetUniqueId(GenerateChannelId(channel.GetChannelName().c_str(), channel.GetStreamURL().c_str()));

  for (int myGroupId : groupIdList)
  {
    channel.SetRadio(channelGroups.GetChannelGroup(myGroupId)->IsRadio());
    channelGroups.GetChannelGroup(myGroupId)->AddMemberChannelIndex(m_channels.size());
  }

  m_channels.emplace_back(channel);

  m_currentChannelNumber++;
}

Channel* Channels::GetChannel(int uniqueId)
{
  for (auto& myChannel : m_channels)
  {
    if (myChannel.GetUniqueId() == uniqueId)
      return &myChannel;
  }

  return nullptr;
}

const Channel* Channels::FindChannel(const std::string& id, const std::string& displayName) const
{
  for (const auto& myChannel : m_channels)
  {
    if (StringUtils::EqualsNoCase(myChannel.GetTvgId(), id))
      return &myChannel;
  }

  if (displayName.empty())
    return nullptr;

  const std::string convertedDisplayName = std::regex_replace(displayName, std::regex(" "), "_");
  for (const auto& myChannel : m_channels)
  {
    if (StringUtils::EqualsNoCase(myChannel.GetTvgName(), convertedDisplayName) ||
        StringUtils::EqualsNoCase(myChannel.GetTvgName(), displayName))
      return &myChannel;
  }

  for (const auto& myChannel : m_channels)
  {
    if (StringUtils::EqualsNoCase(myChannel.GetChannelName(), displayName))
      return &myChannel;
  }

  return nullptr;
}

int Channels::GenerateChannelId(const char* channelName, const char* streamUrl)
{
  std::string concat(channelName);
  concat.append(streamUrl);

  const char* calcString = concat.c_str();
  int iId = 0;
  int c;
  while ((c = *calcString++))
    iId = ((iId << 5) + iId) + c; /* iId * 33 + c */

  return abs(iId);
}
