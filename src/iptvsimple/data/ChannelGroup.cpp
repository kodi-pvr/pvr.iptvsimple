/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ChannelGroup.h"

using namespace iptvsimple;
using namespace iptvsimple::data;

void ChannelGroup::UpdateTo(kodi::addon::PVRChannelGroup& left) const
{
  left.SetIsRadio(m_radio);
  left.SetPosition(0); // groups default order, unused
  left.SetGroupName(m_groupName);
}
