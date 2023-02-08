/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "../Channels.h"
#include "../Media.h"
#include "EpgEntry.h"

#include <string>
#include <vector>

#include <pugixml.hpp>

namespace iptvsimple
{
  namespace data
  {
    struct DisplayNamePair
    {
      std::string m_displayName;
      std::string m_displayNameWithUnderscores;
    };

    class ChannelEpg
    {
    public:
      const std::string& GetId() const { return m_id; }
      void SetId(const std::string& value) { m_id = value; }

      const std::vector<DisplayNamePair>& GetDisplayNames() const { return m_displayNames; }
      void AddDisplayName(const std::string& value);
      std::string GetJoinedDisplayNames();

      const std::string& GetIconPath() const { return m_iconPath; }
      void SetIconPath(const std::string& value) { m_iconPath = value; }

      std::map<time_t, EpgEntry>& GetEpgEntries() { return m_epgEntries; }
      void AddEpgEntry(const EpgEntry& epgEntry)
      {
        m_epgEntries[epgEntry.GetStartTime()] = epgEntry;
      }

      bool UpdateFrom(const pugi::xml_node& channelNode, iptvsimple::Channels& channels, iptvsimple::Media& media);
      bool CombineNamesAndIconPathFrom(const ChannelEpg& right);

    private:
      std::string m_id;
      std::vector<DisplayNamePair> m_displayNames;
      std::string m_iconPath;
      std::map<time_t, EpgEntry> m_epgEntries;
    };
  } //namespace data
} //namespace iptvsimple
