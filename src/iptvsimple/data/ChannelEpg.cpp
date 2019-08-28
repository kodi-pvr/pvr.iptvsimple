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
using namespace rapidxml;

bool ChannelEpg::UpdateFrom(xml_node<>* channelNode, Channels& channels)
{
  if (!GetAttributeValue(channelNode, "id", m_id))
    return false;

  bool foundChannel = false;
  for (xml_node<>* displayNameNode = channelNode->first_node("display-name"); displayNameNode; displayNameNode = displayNameNode->next_sibling("display-name"))
  {
    const std::string name = displayNameNode->value();
    if (channels.FindChannel(m_id, name))
    {
      foundChannel = true;
      m_names.emplace_back(name);
    }
  }

  if (!foundChannel)
    return false;    

  // get icon if available
  xml_node<>* iconNode = channelNode->first_node("icon");
  std::string icon = m_icon;
  if (!iconNode || !GetAttributeValue(iconNode, "src", icon))
    m_icon.clear();
  else
    m_icon = icon;

  return true;
}