#include "ChannelEpg.h"

#include "../utilities/XMLUtils.h"

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace rapidxml;

bool ChannelEpg::UpdateFrom(xml_node<>* channelNode, Channels& channels)
{
  std::string id;
  if (!GetAttributeValue(channelNode, "id", id))
    return false;

  const std::string name = GetNodeValue(channelNode, "display-name");
  if (!channels.FindChannel(id, name))
    return false;

  m_id = id;
  m_name = name;

  // get icon if available
  xml_node<>* iconNode = channelNode->first_node("icon");
  std::string icon = m_icon;
  if (!iconNode || !GetAttributeValue(iconNode, "src", icon))
    m_icon.clear();
  else
    m_icon = icon;

  return true;
}