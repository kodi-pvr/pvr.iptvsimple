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

#include "EpgGenre.h"

#include "rapidxml/rapidxml.hpp"

#include <string>
#include <vector>

namespace iptvsimple
{
  namespace data
  {
    class ChannelEpg;

    class EpgEntry
    {
    public:
      int GetBroadcastId() const { return m_broadcastId; }
      void SetBroadcastId(int value) { m_broadcastId = value; }

      int GetChannelId() const { return m_channelId; }
      void SetChannelId(int value) { m_channelId = value; }

      int GetGenreType() const { return m_genreType; }
      void SetGenreType(int value) { m_genreType = value; }

      int GetGenreSubType() const { return m_genreSubType; }
      void SetGenreSubType(int value) { m_genreSubType = value; }

      time_t GetStartTime() const { return m_startTime; }
      void SetStartTime(time_t value) { m_startTime = value; }

      time_t GetEndTime() const { return m_endTime; }
      void SetEndTime(time_t value) { m_endTime = value; }

      const std::string& GetTitle() const { return m_title; }
      void SetTitle(const std::string& value) { m_title = value; }

      const std::string& GetEpisodeName() const { return m_episodeName; }
      void SetEpisodeName(const std::string& value) { m_episodeName = value; }

      const std::string& GetPlotOutline() const { return m_plotOutline; }
      void SetPlotOutline(const std::string& value) { m_plotOutline = value; }

      const std::string& GetPlot() const { return m_plot; }
      void SetPlot(const std::string& value) { m_plot = value; }

      const std::string& GetIconPath() const { return m_iconPath; }
      void SetIconPath(const std::string& value) { m_iconPath = value; }

      const std::string& GetGenreString() const { return m_genreString; }
      void SetGenreString(const std::string& value) { m_genreString = value; }

      const std::string& GetCast() const { return m_cast; }
      void SetCast(const std::string& value) { m_cast = value; }

      const std::string& GetDirector() const { return m_director; }
      void SetDirector(const std::string& value) { m_director = value; }

      const std::string& GetWriter() const { return m_writer; }
      void SetWriter(const std::string& value) { m_writer = value; }

      void UpdateTo(EPG_TAG& left, int iChannelUid, int timeShift, std::vector<EpgGenre>& genres);
      bool UpdateFrom(rapidxml::xml_node<>* channelNode, iptvsimple::data::ChannelEpg* channelEpg, const std::string& id, int broadcastId,
                      int start, int end, int minShiftTime, int maxShiftTime);

    private:
      bool SetEpgGenre(std::vector<EpgGenre> genres, const std::string& genreToFind);

      int m_broadcastId;
      int m_channelId;
      int m_genreType;
      int m_genreSubType;
      time_t m_startTime;
      time_t m_endTime;
      std::string m_title;
      std::string m_episodeName;
      std::string m_plotOutline;
      std::string m_plot;
      std::string m_iconPath;
      std::string m_genreString;
      std::string m_cast;
      std::string m_director;
      std::string m_writer;
    };
  } //namespace data
} //namespace iptvsimple