/*
 *      Copyright (C) 2005-2020 Team Kodi
 *      https://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "../Channels.h"
#include "EpgEntry.h"

#include <string>
#include <vector>

#include <kodi/libXBMC_pvr.h>
#include <pugixml.hpp>

namespace iptvsimple
{
  namespace data
  {
    class ChannelEpg
    {
    public:
      const std::string& GetId() const { return m_id; }
      void SetId(const std::string& value) { m_id = value; }

      const std::vector<std::string>& GetNames() const { return m_names; }
      void AddName(const std::string& value) { m_names.emplace_back(value); }

      const std::string& GetIconPath() const { return m_iconPath; }
      void SetIconPath(const std::string& value) { m_iconPath = value; }

      std::map<time_t, EpgEntry>& GetEpgEntries() { return m_epgEntries; }
      void AddEpgEntry(const EpgEntry& epgEntry) { m_epgEntries[epgEntry.GetStartTime()] = epgEntry; }

      bool UpdateFrom(const pugi::xml_node& channelNode, iptvsimple::Channels& channels);
      bool CombineNamesAndIconPathFrom(const ChannelEpg& right);

    private:
      std::string m_id;
      std::vector<std::string> m_names;
      std::string m_iconPath;
      std::map<time_t, EpgEntry> m_epgEntries;
    };
  } //namespace data
} //namespace iptvsimple