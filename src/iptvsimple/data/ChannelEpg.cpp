/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ChannelEpg.h"

#include "../utilities/XMLUtils.h"

#include <kodi/tools/StringUtils.h>

using namespace kodi::tools;
using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace pugi;

bool ChannelEpg::UpdateFrom(const xml_node& channelNode, Channels& channels, Media& media)
{
  if (!GetAttributeValue(channelNode, "id", m_id) || m_id.empty())
    return false;

  bool foundChannel = false;
  bool haveDisplayNames = false;
  for (const auto& displayNameNode : channelNode.children("display-name"))
  {
    haveDisplayNames = true;

    const std::string name = displayNameNode.child_value();
    if (channels.FindChannel(m_id, name) || media.FindMediaEntry(m_id, name))
    {
      foundChannel = true;
      AddDisplayName(name);
    }
  }

  // If there are no display names just check if the id matches a channel
  if (!haveDisplayNames && (channels.FindChannel(m_id, "") || media.FindMediaEntry(m_id, "")))
    foundChannel = true;

  if (!foundChannel)
    return false;

  // get icon if available
  const auto& iconNode = channelNode.child("icon");
  std::string iconPath = m_iconPath;
  if (!iconNode || !GetAttributeValue(iconNode, "src", iconPath))
    m_iconPath.clear();
  else
    m_iconPath = iconPath;

  return true;
}

bool ChannelEpg::CombineNamesAndIconPathFrom(const ChannelEpg& right)
{
  bool combined = false;

  for (const DisplayNamePair& namePair : right.m_displayNames)
  {
    AddDisplayName(namePair.m_displayName);
    combined = true;
  }

  if (m_iconPath.empty() && !right.m_iconPath.empty())
  {
    m_iconPath = right.m_iconPath;
    combined = true;
  }

  return combined;
}

void ChannelEpg::AddDisplayName(const std::string& value)
{
  DisplayNamePair pair;
  pair.m_displayName = value;
  pair.m_displayNameWithUnderscores = value;
  StringUtils::Replace(pair.m_displayNameWithUnderscores, ' ', '_');
  m_displayNames.emplace_back(pair);
}

std::string ChannelEpg::GetJoinedDisplayNames()
{
  std::vector<std::string> names;
  for (auto& displayNamePair : m_displayNames)
    names.emplace_back(displayNamePair.m_displayName);

  return StringUtils::Join(names, EPG_STRING_TOKEN_SEPARATOR);
}
