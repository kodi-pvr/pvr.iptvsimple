/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ChannelGroups.h"

#include "utilities/Logger.h"

#include <kodi/General.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

ChannelGroups::ChannelGroups(const Channels& channels) : m_channels(channels) {}

void ChannelGroups::Clear()
{
  m_channelGroups.clear();
}

int ChannelGroups::GetChannelGroupsAmount() const
{
  return m_channelGroups.size();
}

PVR_ERROR ChannelGroups::GetChannelGroups(kodi::addon::PVRChannelGroupsResultSet& results, bool radio) const
{
  Logger::Log(LEVEL_DEBUG, "%s - Starting to get ChannelGroups for PVR", __FUNCTION__);

  for (const auto& channelGroup : m_channelGroups)
  {
    if (channelGroup.IsRadio() == radio)
    {
      Logger::Log(LEVEL_DEBUG, "%s - Transfer channelGroup '%s', ChannelGroupId '%d'", __FUNCTION__, channelGroup.GetGroupName().c_str(), channelGroup.GetUniqueId());

      kodi::addon::PVRChannelGroup kodiChannelGroup;

      channelGroup.UpdateTo(kodiChannelGroup);

      results.Add(kodiChannelGroup);
    }
  }

  Logger::Log(LEVEL_DEBUG, "%s - channel groups available '%d'", __FUNCTION__, m_channelGroups.size());

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR ChannelGroups::GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group, kodi::addon::PVRChannelGroupMembersResultSet& results)
{
  const ChannelGroup* myGroup = FindChannelGroup(group.GetGroupName());
  if (myGroup)
  {
    // We set a channel order here that applies to this group in kodi-pvr
    // This allows the users to use the 'Backend Order' sort option in the left to
    // have the same order as the backend (regardles of the channel numbering used)
    //
    // We don't set a channel number within this group as different channel numbers
    // per group are not supported in M3U files
    int channelOrder = 1;

    for (int memberId : myGroup->GetMemberChannelIndexes())
    {
      if (memberId < 0 || memberId >= static_cast<int>(m_channels.GetChannelsAmount()))
        continue;

      const Channel& channel = m_channels.GetChannelsList().at(memberId);
      kodi::addon::PVRChannelGroupMember kodiGroupMember;

      kodiGroupMember.SetGroupName(group.GetGroupName());
      kodiGroupMember.SetChannelUniqueId(channel.GetUniqueId());
      kodiGroupMember.SetOrder(channelOrder++); // Keep the channels in list order as per the M3U

      Logger::Log(LEVEL_DEBUG, "%s - Transfer channel group '%s' member '%s', ChannelId '%d', ChannelOrder: '%d'", __FUNCTION__,
                  myGroup->GetGroupName().c_str(), channel.GetChannelName().c_str(), channel.GetUniqueId(), channelOrder);

      results.Add(kodiGroupMember);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

int ChannelGroups::AddChannelGroup(iptvsimple::data::ChannelGroup& channelGroup)
{
  const ChannelGroup* existingChannelGroup = FindChannelGroup(channelGroup.GetGroupName());

  if (existingChannelGroup && channelGroup.IsRadio() != existingChannelGroup->IsRadio())
  {
    // Ok, we have the same channel group name for both TV and Radio which is not allowed
    // so let's add ' (Radio)' or ' (TV)' depending on which group was added first.

    if (existingChannelGroup->IsRadio())
      channelGroup.SetGroupName(channelGroup.GetGroupName() + " (" + kodi::GetLocalizedString(30450) + ")"); // ' (TV)';
    else
      channelGroup.SetGroupName(channelGroup.GetGroupName() + " (" + kodi::GetLocalizedString(30451) + ")"); // ' (Radio)';

    existingChannelGroup = FindChannelGroup(channelGroup.GetGroupName());
  }

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
