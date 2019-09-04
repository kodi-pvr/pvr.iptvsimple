#pragma once
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

#include "kodi/libXBMC_pvr.h"

#include "EpgEntry.h"
#include "../Channels.h"
#include "rapidxml/rapidxml.hpp"

#include <string>
#include <vector>

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

      const std::string& GetIcon() const { return m_icon; }
      void SetIcon(const std::string& value) { m_icon = value; }

      std::vector<EpgEntry>& GetEpgEntries() { return m_epgEntries; }
      void AddEpgEntry(const EpgEntry& epgEntry) { m_epgEntries.emplace_back(epgEntry); }

      bool UpdateFrom(rapidxml::xml_node<>* channelNode, iptvsimple::Channels& channels);

    private:
      std::string m_id;
      std::vector<std::string> m_names;
      std::string m_icon;
      std::vector<EpgEntry> m_epgEntries;
    };
  } //namespace data
} //namespace iptvsimple