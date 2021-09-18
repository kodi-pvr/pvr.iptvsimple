/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "data/Channel.h"
#include "data/StreamEntry.h"

#include <map>
#include <mutex>
#include <string>

namespace iptvsimple
{
  class StreamManager
  {
  public:
    StreamManager();

    StreamType StreamTypeLookup(const data::Channel& channel, const std::string& streamTestUrl, const std::string& streamKey);
    void Clear();

  private:
    void AddUpdateStreamEntry(const std::string& streamKey, const StreamType& streamType, const std::string& mimeType);
    bool HasStreamEntry(const std::string& streamKey) const;
    std::shared_ptr<data::StreamEntry> GetStreamEntry(const std::string streamKey) const;
    data::StreamEntry StreamEntryLookup(const data::Channel& channel, const std::string& streamTestUrl, const std::string& streamKey);

    mutable std::mutex m_mutex;

    std::map<std::string, std::shared_ptr<data::StreamEntry>> m_streamEntryCache;
  };
} //namespace iptvsimple
