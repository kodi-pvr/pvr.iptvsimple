/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

#include "data/Channel.h"
#include "data/EpgEntry.h"
#include "utilities/StreamUtils.h"

#include <mutex>

#include <kodi/libXBMC_pvr.h>

namespace iptvsimple
{
  class Epg;

  class CatchupController
  {
  public:
    CatchupController(iptvsimple::Epg& epg, std::mutex* mutex);

    void ProcessChannelForPlayback(data::Channel& channel, std::map<std::string, std::string>& catchupProperties); //should be const channel
    void ProcessEPGTagForTimeshiftedPlayback(const EPG_TAG& epgTag, data::Channel& channel, std::map<std::string, std::string>& catchupProperties); //should be const channel
    void ProcessEPGTagForVideoPlayback(const EPG_TAG& epgTag, data::Channel& channel, std::map<std::string, std::string>& catchupProperties); //should be const channel

    std::string GetTimeshiftInputStreamClass(const utilities::StreamType& streamType);

    std::string GetCatchupUrlFormatString(const data::Channel& channel) const;
    std::string GetCatchupUrl(const data::Channel& channel) const;
    std::string GetStreamTestUrl(const data::Channel& channel) const;

    bool ControlsLiveStream() const { return m_controlsLiveStream; }
    void ResetCatchupState() { m_resetCatchupState = true; }

  private:
    data::EpgEntry* GetLiveEPGEntry(const iptvsimple::data::Channel& myChannel);
    data::EpgEntry* GetEPGEntry(const iptvsimple::data::Channel& myChannel, time_t lookupTime);
    void SetCatchupInputStreamProperties(bool playbackAsLive, const iptvsimple::data::Channel& channel, std::map<std::string, std::string>& catchupProperties);
    void TestAndStoreStreamType(data::Channel& channel);

    // Programme helpers
    void UpdateProgrammeFrom(const EPG_TAG& epgTag, int tvgShift);
    void UpdateProgrammeFrom(const data::EpgEntry& epgEntry, int tvgShift);
    void ClearProgramme();

    // State of current stream
    time_t m_catchupStartTime = 0;
    time_t m_catchupEndTime = 0;
    time_t m_timeshiftBufferStartTime = 0;
    long long m_timeshiftBufferOffset = 0;
    bool m_resetCatchupState = false;
    bool m_playbackIsVideo = false;
    bool m_fromEpgTag = false;

    // Current programme details
    time_t m_programmeStartTime = 0;
    time_t m_programmeEndTime = 0;
    std::string m_programmeTitle;
    unsigned int m_programmeUniqueChannelId = 0;
    int m_programmeChannelTvgShift = 0;
    std::string m_programmeCatchupId;

    bool m_controlsLiveStream = false;
    iptvsimple::Epg& m_epg;
    std::mutex* m_mutex = nullptr;
  };
} //namespace iptvsimple