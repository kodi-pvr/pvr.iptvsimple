#include "Channel.h"

using namespace iptvsimple;
using namespace iptvsimple::data;

void Channel::UpdateTo(Channel& left) const
{
  left.m_uniqueId         = m_uniqueId;
  left.m_radio            = m_radio;
  left.m_channelNumber    = m_channelNumber;
  left.m_encryptionSystem = m_encryptionSystem;
  left.m_tvgShift         = m_tvgShift;
  left.m_channelName      = m_channelName;
  left.m_logoPath         = m_logoPath;
  left.m_streamURL        = m_streamURL;
  left.m_tvgId            = m_tvgId;
  left.m_tvgName          = m_tvgName;
  left.m_tvgLogo          = m_tvgLogo;
  left.m_properties       = m_properties;
}

void Channel::UpdateTo(PVR_CHANNEL& left) const
{
  left.iUniqueId = m_uniqueId;
  left.bIsRadio = m_radio;
  left.iChannelNumber = m_channelNumber;
  strncpy(left.strChannelName, m_channelName.c_str(), sizeof(left.strChannelName) - 1);
  left.iEncryptionSystem = m_encryptionSystem;
  strncpy(left.strIconPath, m_logoPath.c_str(), sizeof(left.strIconPath) - 1);
  left.bIsHidden = false;
}

void Channel::Reset()
{
  m_uniqueId = 0;
  m_radio = false;
  m_channelNumber = 0;
  m_encryptionSystem = 0;
  m_tvgShift = 0;
  m_channelName.clear();
  m_logoPath.clear();
  m_streamURL.clear();
  m_tvgId.clear();
  m_tvgName.clear();
  m_tvgLogo.clear();
  m_properties.clear();
}