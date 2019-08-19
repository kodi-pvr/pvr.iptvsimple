#include "ChannelGroup.h"

using namespace iptvsimple;
using namespace iptvsimple::data;

void ChannelGroup::UpdateTo(PVR_CHANNEL_GROUP& left) const
{
  left.bIsRadio = m_radio;
  left.iPosition = 0; // groups default order, unused
  strncpy(left.strGroupName, m_groupName.c_str(), sizeof(left.strGroupName));
}