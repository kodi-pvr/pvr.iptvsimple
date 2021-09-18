/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "StreamManager.h"

#include "utilities/Logger.h"
#include "utilities/StreamUtils.h"

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

StreamManager::StreamManager() {}

void StreamManager::Clear()
{
  m_streamEntryCache.clear();
}

void StreamManager::AddUpdateStreamEntry(const std::string& streamKey, const StreamType& streamType, const std::string& mimeType)
{
  std::shared_ptr<StreamEntry> foundStreamEntry = GetStreamEntry(streamKey);

  if (!foundStreamEntry)
  {
    std::shared_ptr<StreamEntry> newStreamEntry = std::make_shared<StreamEntry>();
    newStreamEntry->SetStreamKey(streamKey);
    newStreamEntry->SetStreamType(streamType);
    newStreamEntry->SetMimeType(mimeType);
    newStreamEntry->SetLastAccessTime(std::time(nullptr));

    std::lock_guard<std::mutex> lock(m_mutex);
    m_streamEntryCache.insert({streamKey, newStreamEntry});
  }
  else
  {
    foundStreamEntry->SetStreamType(streamType);
    foundStreamEntry->SetLastAccessTime(std::time(nullptr));
  }
}

bool StreamManager::HasStreamEntry(const std::string& streamKey) const
{
  return GetStreamEntry(streamKey) != nullptr;
}

std::shared_ptr<StreamEntry> StreamManager::GetStreamEntry(const std::string streamKey) const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto streamEntryPair = m_streamEntryCache.find(streamKey);
  if (streamEntryPair != m_streamEntryCache.end())
    return streamEntryPair->second;

  return {};
}

StreamType StreamManager::StreamTypeLookup(const Channel& channel, const std::string& streamTestUrl, const std::string& streamKey)
{
  return StreamEntryLookup(channel, streamTestUrl, streamKey).GetStreamType();
}

StreamEntry StreamManager::StreamEntryLookup(const Channel& channel, const std::string& streamTestUrl, const std::string& streamKey)
{
  std::shared_ptr<StreamEntry> streamEntry = GetStreamEntry(streamKey);

  if (!streamEntry)
  {
    StreamType streamType = StreamUtils::GetStreamType(streamTestUrl, channel);
    if (streamType == StreamType::OTHER_TYPE)
      streamType = StreamUtils::InspectStreamType(streamTestUrl, channel);

    streamEntry = std::make_shared<StreamEntry>();
    streamEntry->SetStreamKey(streamKey);
    streamEntry->SetStreamType(streamType);
    streamEntry->SetMimeType(StreamUtils::GetMimeType(streamType));
  }

  // If a channel has a MIME Type we always override with that
  if (channel.HasMimeType())
    streamEntry->SetMimeType(channel.GetMimeType());

  AddUpdateStreamEntry(streamEntry->GetStreamKey(), streamEntry->GetStreamType(), streamEntry->GetMimeType());

  return *streamEntry;
}
