/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>
#include <vector>

#include <kodi/addon-instance/pvr/ChannelGroups.h>

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

      const std::vector<int>& GetMemberChannelIndexes() const { return m_memberChannelIndexes; }
      void AddMemberChannelIndex(int channelIndex) { m_memberChannelIndexes.emplace_back(channelIndex); }

      bool IsEmpty() const { return m_memberChannelIndexes.empty(); }

      void UpdateTo(kodi::addon::PVRChannelGroup& left) const;

    private:
      bool m_radio;
      int m_uniqueId;
      std::string m_groupName;
      std::vector<int> m_memberChannelIndexes;
    };
  } //namespace data
} //namespace iptvsimple
