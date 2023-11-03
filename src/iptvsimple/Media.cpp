/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Media.h"

#include "../IptvSimple.h"
#include "utilities/Logger.h"

#include <kodi/tools/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;
using namespace kodi::tools;

Media::Media(std::shared_ptr<iptvsimple::InstanceSettings>& settings) : m_settings(settings)
{
}

void Media::GetMedia(std::vector<kodi::addon::PVRRecording>& kodiRecordings)
{
  for (auto& mediaEntry : m_media)
  {
    Logger::Log(LEVEL_DEBUG, "%s - Transfer mediaEntry '%s', MediaEntry Id '%s'", __func__, mediaEntry.GetTitle().c_str(), mediaEntry.GetMediaEntryId().c_str());
    kodi::addon::PVRRecording kodiRecording;

    mediaEntry.UpdateTo(kodiRecording, IsInVirtualMediaEntryFolder(mediaEntry), m_haveMediaTypes);

    kodiRecordings.emplace_back(kodiRecording);
  }
}

int Media::GetNumMedia() const
{
  return m_media.size();
}

void Media::Clear()
{
  m_media.clear();
  m_mediaIdMap.clear();
  m_haveMediaTypes = false;
}

namespace
{

int GenerateMediaEntryId(const char* providerName, const char* streamUrl)
{
  std::string concat(providerName);
  concat.append(streamUrl);

  const char* calcString = concat.c_str();
  int iId = 0;
  int c;
  while ((c = *calcString++))
    iId = ((iId << 5) + iId) + c; /* iId * 33 + c */

  return abs(iId);
}

} // unamed namespace

bool Media::AddMediaEntry(MediaEntry& mediaEntry, std::vector<int>& groupIdList, ChannelGroups& channelGroups, bool channelHadGroups)
{
  // If we have no groups set for this media check it that's ok before adding it.
  // Note that TV is a proxy for Media. There is no concept of radio for media
  if (m_settings->AllowTVChannelGroupsOnly() && groupIdList.empty())
    return false;

  std::string mediaEntryId = std::to_string(GenerateMediaEntryId(mediaEntry.GetProviderName().c_str(),
                                                                 mediaEntry.GetStreamURL().c_str()));
  mediaEntryId.append("-" + mediaEntry.GetDirectory() + mediaEntry.GetTitle());
  mediaEntry.SetMediaEntryId(mediaEntryId);

  // Media Ids must be unique
  if (m_mediaIdMap.find(mediaEntryId) != m_mediaIdMap.end())
    return false;

  bool belongsToGroup = false;
  for (int myGroupId : groupIdList)
  {
    if (channelGroups.GetChannelGroup(myGroupId) != nullptr)
      belongsToGroup = true;
  }


  // We only care if a media entry belongs to a group if it had groups to begin with
  // Note that a channel can have had groups but no have no groups valid currently.
  if (!belongsToGroup && channelHadGroups)
    return false;

  m_media.emplace_back(mediaEntry);
  m_mediaIdMap.insert({mediaEntryId, mediaEntry});

  return true;
}

MediaEntry Media::GetMediaEntry(const std::string& mediaEntryId) const
{
  MediaEntry entry{m_settings};

  auto mediaEntryPair = m_mediaIdMap.find(mediaEntryId);
  if (mediaEntryPair != m_mediaIdMap.end())
  {
    entry = mediaEntryPair->second;
  }

  return entry;
}

bool Media::IsInVirtualMediaEntryFolder(const MediaEntry& mediaEntryToCheck) const
{
  const std::string& mediaEntryFolderToCheck = mediaEntryToCheck.GetFolderTitle();

  int iMatches = 0;
  for (const auto& mediaEntry : m_media)
  {
    if (mediaEntryFolderToCheck == mediaEntry.GetFolderTitle())
    {
      iMatches++;
      Logger::Log(LEVEL_DEBUG, "%s Found MediaEntry title '%s' in media vector!", __func__, mediaEntryFolderToCheck.c_str());
      if (iMatches > 1)
      {
        Logger::Log(LEVEL_DEBUG, "%s Found MediaEntry title twice '%s' in media vector!", __func__, mediaEntryFolderToCheck.c_str());
        return true;
      }
    }
  }

  return false;
}

const std::string Media::GetMediaEntryURL(const kodi::addon::PVRRecording& recording)
{
  Logger::Log(LEVEL_INFO, "%s", __func__);

  auto mediaEntry = GetMediaEntry(recording.GetRecordingId());

  if (!mediaEntry.GetMediaEntryId().empty())
    return mediaEntry.GetStreamURL();

  return "";
}

const MediaEntry* Media::FindMediaEntry(const std::string& id, const std::string& displayName) const
{
  for (const auto& myMediaEntry : m_media)
  {
    if (StringUtils::EqualsNoCase(myMediaEntry.GetTvgId(), id))
      return &myMediaEntry;
  }

  if (displayName.empty())
    return nullptr;

  const std::string convertedDisplayName = std::regex_replace(displayName, std::regex(" "), "_");
  for (const auto& myMediaEntry : m_media)
  {
    if (StringUtils::EqualsNoCase(myMediaEntry.GetTvgName(), convertedDisplayName) ||
        StringUtils::EqualsNoCase(myMediaEntry.GetTvgName(), displayName))
      return &myMediaEntry;
  }

  for (const auto& myMediaEntry : m_media)
  {
    if (StringUtils::EqualsNoCase(myMediaEntry.GetM3UName(), displayName))
      return &myMediaEntry;
  }

  return nullptr;
}