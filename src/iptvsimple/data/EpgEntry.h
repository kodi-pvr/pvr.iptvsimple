/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "BaseEntry.h"
#include "EpgGenre.h"

#include <string>
#include <vector>

#include <kodi/addon-instance/pvr/EPG.h>
#include <pugixml.hpp>

namespace iptvsimple
{
  namespace data
  {
    static const float STAR_RATING_SCALE = 10.0f;
    constexpr int DATESTRING_LENGTH = 8;

    class EpgEntry : public BaseEntry
    {
    public:
      EpgEntry() {};
      EpgEntry(std::shared_ptr<iptvsimple::InstanceSettings> settings)
      {
        m_settings = settings;
      };

      int GetBroadcastId() const { return m_broadcastId; }
      void SetBroadcastId(int value) { m_broadcastId = value; }

      int GetChannelId() const { return m_channelId; }
      void SetChannelId(int value) { m_channelId = value; }

      time_t GetStartTime() const { return m_startTime; }
      void SetStartTime(time_t value) { m_startTime = value; }

      time_t GetEndTime() const { return m_endTime; }
      void SetEndTime(time_t value) { m_endTime = value; }

      const std::string& GetCatchupId() const { return m_catchupId; }
      void SetCatchupId(const std::string& value) { m_catchupId = value; }

      void UpdateTo(kodi::addon::PVREPGTag& left, int iChannelUid, int timeShift, const std::vector<EpgGenre>& genres);
      bool UpdateFrom(const pugi::xml_node& programmeNode, const std::string& id,
                      int epgWindowsStart, int epgWindowsEnd, int minShiftTime, int maxShiftTime);

    private:
      bool SetEpgGenre(std::vector<EpgGenre> genreMappings);
      bool ParseEpisodeNumberInfo(std::vector<std::pair<std::string, std::string>>& episodeNumbersList);
      bool ParseXmltvNsEpisodeNumberInfo(const std::string& episodeNumberString);
      bool ParseOnScreenEpisodeNumberInfo(const std::string& episodeNumberString);

      int m_broadcastId;
      int m_channelId;
      time_t m_startTime;
      time_t m_endTime;
      std::string m_catchupId;
    };
  } //namespace data
} //namespace iptvsimple
