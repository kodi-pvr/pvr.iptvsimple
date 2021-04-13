/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "data/MediaEntry.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace iptvsimple
{
  class ATTRIBUTE_HIDDEN Media
  {
  public:
    Media();
    void GetMedia(std::vector<kodi::addon::PVRRecording>& kodiRecordings);
    int GetNumMedia() const;
    void Clear();
    const std::string GetMediaEntryURL(const kodi::addon::PVRRecording& mediaEntry);
    const iptvsimple::data::MediaEntry* FindMediaEntry(const std::string& id, const std::string& displayName) const;

    bool AddMediaEntry(iptvsimple::data::MediaEntry& entry);

    std::vector<iptvsimple::data::MediaEntry>& GetMediaEntryList() { return m_media; }

  private:
    data::MediaEntry GetMediaEntry(const std::string& mediaEntryId) const;
    bool IsInVirtualMediaEntryFolder(const data::MediaEntry& mediaEntry) const;

    std::vector<iptvsimple::data::MediaEntry> m_media;
    std::unordered_map<std::string, iptvsimple::data::MediaEntry> m_mediaIdMap;

    bool m_haveMediaTypes = false;
  };
} //namespace iptvsimple
