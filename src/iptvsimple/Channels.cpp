#include "Channels.h"

#include "../client.h"
#include "ChannelGroups.h"
#include "Settings.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"

#include <regex>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

Channels::Channels()
{
  m_logoLocation = Settings::GetInstance().GetLogoLocation();
  m_currentChannelNumber = Settings::GetInstance().GetStartChannelNumber();
}

void Channels::Clear()
{
  m_channels.clear();
  m_logoLocation = Settings::GetInstance().GetLogoLocation();
  m_currentChannelNumber = Settings::GetInstance().GetStartChannelNumber();
}

int Channels::GetChannelsAmount()
{
  return m_channels.size();
}

void Channels::GetChannels(std::vector<PVR_CHANNEL>& kodiChannels, bool radio) const
{
  for (const auto& channel : m_channels)
  {
    if (channel.IsRadio() == radio)
    {
      Logger::Log(LEVEL_DEBUG, "%s - Transfer channel '%s', ChannelIndex '%d'", __FUNCTION__, channel.GetChannelName().c_str(),
                  channel.GetUniqueId());
      PVR_CHANNEL kodiChannel = {0};

      channel.UpdateTo(kodiChannel);

      kodiChannels.emplace_back(kodiChannel);
    }
  }
}

bool Channels::GetChannel(const PVR_CHANNEL& channel, Channel& myChannel)
{
  for (const auto& thisChannel : m_channels)
  {
    if (thisChannel.GetUniqueId() == static_cast<int>(channel.iUniqueId))
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
    channelGroups.GetChannelGroup(myGroupId)->GetMemberChannels().push_back(m_channels.size());
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

const Channel* Channels::FindChannel(const std::string& id, const std::string& name) const
{
  const std::string tvgName = std::regex_replace(name, std::regex(" "), "_");

  for (const auto& myChannel : m_channels)
  {
    if (myChannel.GetTvgId() == id)
      return &myChannel;

    if (tvgName == "")
      continue;

    if (myChannel.GetTvgName() == tvgName)
      return &myChannel;

    if (myChannel.GetChannelName() == name)
      return &myChannel;
  }

  return nullptr;
}

void Channels::ApplyChannelLogos()
{
  for (auto& channel : m_channels)
  {
    if (!channel.GetTvgLogo().empty())
    {
      if (!m_logoLocation.empty() && channel.GetTvgLogo().find("://") == std::string::npos) // special proto
        channel.SetLogoPath(FileUtils::PathCombine(m_logoLocation, channel.GetTvgLogo()));
      else
        channel.SetLogoPath(channel.GetTvgLogo());
    }
  }
}

void Channels::ReapplyChannelLogos(const char* newLogoPath)
{
  //P8PLATFORM::CLockObject lock(m_mutex);
  //TODO Lock should happen in calling class
  if (strlen(newLogoPath) > 0)
  {
    m_logoLocation = newLogoPath;
    ApplyChannelLogos();

    PVR->TriggerChannelUpdate();
    PVR->TriggerChannelGroupsUpdate();
  }
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