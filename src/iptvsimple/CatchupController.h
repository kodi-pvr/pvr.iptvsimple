/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

#include "InstanceSettings.h"
#include "data/Channel.h"
#include "data/EpgEntry.h"
#include "utilities/StreamUtils.h"
#include "StreamManager.h"

#include <memory>
#include <mutex>

#include <kodi/addon-instance/pvr/EPG.h>

namespace iptvsimple
{
  class Epg;

  class CatchupController
  {
  public:
    CatchupController(iptvsimple::Epg& epg, std::mutex* mutex, std::shared_ptr<iptvsimple::InstanceSettings>& settings);

    void ProcessChannelForPlayback(const data::Channel& channel, std::map<std::string, std::string>& catchupProperties);
    void ProcessEPGTagForTimeshiftedPlayback(const kodi::addon::PVREPGTag& epgTag, const data::Channel& channel, std::map<std::string, std::string>& catchupProperties);
    void ProcessEPGTagForVideoPlayback(const kodi::addon::PVREPGTag& epgTag, const data::Channel& channel, std::map<std::string, std::string>& catchupProperties);

    std::string GetCatchupUrlFormatString(const data::Channel& channel) const;
    std::string GetCatchupUrl(const data::Channel& channel) const;
    std::string ProcessStreamUrl(const data::Channel& channel) const;

    bool ControlsLiveStream() const { return m_controlsLiveStream; }
    void ResetCatchupState() { m_resetCatchupState = true; }
    data::EpgEntry* GetEPGEntry(const iptvsimple::data::Channel& myChannel, time_t lookupTime);

  private:
    data::EpgEntry* GetLiveEPGEntry(const iptvsimple::data::Channel& myChannel);
    void SetCatchupInputStreamProperties(bool playbackAsLive, const iptvsimple::data::Channel& channel, std::map<std::string, std::string>& catchupProperties, const StreamType& streamType);
    StreamType StreamTypeLookup(const data::Channel& channel, bool fromEpg = false);
    std::string GetStreamTestUrl(const data::Channel& channel, bool fromEpg) const;
    std::string GetStreamKey(const data::Channel& channel, bool fromEpg) const;

    // Programme helpers
    void UpdateProgrammeFrom(const kodi::addon::PVREPGTag& epgTag, int tvgShift);
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

    StreamManager m_streamManager;

    std::shared_ptr<iptvsimple::InstanceSettings> m_settings;
  };
} //namespace iptvsimple
