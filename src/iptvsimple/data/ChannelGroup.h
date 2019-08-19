#pragma once
/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "libXBMC_pvr.h"

#include <string>
#include <vector>

namespace iptvsimple
{
  namespace data
  {
    class ChannelGroup
    {
    public:
      bool IsRadio() const { return m_radio; }
      void SetRadio(bool value) { m_radio = value; }

      int GetUniqueId() const { return m_uniqueId; }
      void SetUniqueId(int value) { m_uniqueId = value; }

      const std::string& GetGroupName() const { return m_groupName; }
      void SetGroupName(const std::string& value) { m_groupName = value; }

      std::vector<int>& GetMemberChannels() { return m_memberChannels; }

      void UpdateTo(PVR_CHANNEL_GROUP& left) const;

    private:
      bool m_radio;
      int m_uniqueId;
      std::string m_groupName;
      std::vector<int> m_memberChannels;
    };
  } //namespace data
} //namespace iptvsimple