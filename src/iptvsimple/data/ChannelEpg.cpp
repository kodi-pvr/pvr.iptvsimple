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

#include "ChannelEpg.h"

#include "../utilities/XMLUtils.h"

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace pugi;

bool ChannelEpg::UpdateFrom(const xml_node& channelNode, Channels& channels)
{
  if (!GetAttributeValue(channelNode, "id", m_id))
    return false;

  bool foundChannel = false;
  bool haveDisplayNames = false;
  for (const auto& displayNameNode : channelNode.children("display-name"))
  {
    haveDisplayNames = true;

    const std::string name = displayNameNode.child_value();
    if (channels.FindChannel(m_id, name))
    {
      foundChannel = true;
      m_names.emplace_back(name);
    }
  }

  // If there are no display names just check if the id matches a channel
  if (!haveDisplayNames && channels.FindChannel(m_id, ""))
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

  for (const std::string& name : right.m_names)
  {
    AddName(name);
    combined = true;
  }

  if (m_iconPath.empty() && !right.m_iconPath.empty())
  {
    m_iconPath = right.m_iconPath;
    combined = true;
  }

  return combined;
}
