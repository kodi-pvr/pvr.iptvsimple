#include "ChannelGroups.h"

#include "../client.h"
#include "utilities/Logger.h"

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

ChannelGroups::ChannelGroups(Channels& channels)
      : m_channels(channels)
{
}

void ChannelGroups::Clear()
{
  m_channelGroups.clear();
}

int ChannelGroups::GetChannelGroupsAmount()
{
  return m_channelGroups.size();
}

void ChannelGroups::GetChannelGroups(std::vector<PVR_CHANNEL_GROUP>& kodiChannelGroups, bool radio) const
{
  Logger::Log(LEVEL_DEBUG, "%s - Starting to get ChannelGroups for PVR", __FUNCTION__);

  for (const auto& channelGroup : m_channelGroups)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer channelGroup '%s', ChannelGroupIndex '%d'", __FUNCTION__, channelGroup.GetGroupName().c_str(), channelGroup.GetUniqueId());

    if (channelGroup.IsRadio() == radio)
    {
      PVR_CHANNEL_GROUP kodiChannelGroup = {0};

      channelGroup.UpdateTo(kodiChannelGroup);

      kodiChannelGroups.emplace_back(kodiChannelGroup);
    }
  }

  Logger::Log(LEVEL_DEBUG, "%s - Finished getting ChannelGroups for PVR", __FUNCTION__);
}

PVR_ERROR ChannelGroups::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group)
{
  ChannelGroup* myGroup = FindChannelGroup(group.strGroupName);
  if (myGroup)
  {
    for (int memberId : myGroup->GetMemberChannels())
    {
      if ((memberId) < 0 || (memberId) >= static_cast<int>(m_channels.GetChannelsAmount()))
        continue;

      const Channel& channel = m_channels.GetChannelsList().at(memberId);
      PVR_CHANNEL_GROUP_MEMBER xbmcGroupMember;
      memset(&xbmcGroupMember, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

      strncpy(xbmcGroupMember.strGroupName, group.strGroupName, sizeof(xbmcGroupMember.strGroupName) - 1);
      xbmcGroupMember.iChannelUniqueId = channel.GetUniqueId();
      xbmcGroupMember.iChannelNumber = channel.GetChannelNumber();

      PVR->TransferChannelGroupMember(handle, &xbmcGroupMember);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

int ChannelGroups::AddChannelGroup(iptvsimple::data::ChannelGroup& channelGroup)
{
  const ChannelGroup* existingChannelGroup = FindChannelGroup(channelGroup.GetGroupName());

  if (!existingChannelGroup)
  {
    channelGroup.SetUniqueId(m_channelGroups.size() + 1);

    m_channelGroups.emplace_back(channelGroup);

    Logger::Log(LEVEL_DEBUG, "%s - Added group: %s, with uniqueId: %d", __FUNCTION__, channelGroup.GetGroupName().c_str(), channelGroup.GetUniqueId());

    return channelGroup.GetUniqueId();
  }

  Logger::Log(LEVEL_DEBUG, "%s - Did not add group: %s, as it already exists with uniqueId: %d", __FUNCTION__, existingChannelGroup->GetGroupName().c_str(), existingChannelGroup->GetUniqueId());

  return existingChannelGroup->GetUniqueId();
}

ChannelGroup* ChannelGroups::GetChannelGroup(int uniqueId)
{
  for (auto& myChannelGroup : m_channelGroups)
  {
    if (myChannelGroup.GetUniqueId() == uniqueId)
      return &myChannelGroup;
  }

  return nullptr;
}

ChannelGroup* ChannelGroups::FindChannelGroup(const std::string& name)
{
  for (auto& myGroup : m_channelGroups)
  {
    if (myGroup.GetGroupName() == name)
      return &myGroup;
  }

  return nullptr;
}