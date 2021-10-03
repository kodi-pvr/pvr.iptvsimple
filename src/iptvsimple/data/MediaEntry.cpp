/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "MediaEntry.h"

#include "../Settings.h"

#include <kodi/General.h>
#include <kodi/tools/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace kodi::tools;

void MediaEntry::Reset()
{
  // From BaseEntry
  m_genreType = 0;
  m_genreSubType = 0;
  m_year = 0;
  m_episodeNumber = EPG_TAG_INVALID_SERIES_EPISODE;
  m_episodePartNumber = EPG_TAG_INVALID_SERIES_EPISODE;
  m_seasonNumber = EPG_TAG_INVALID_SERIES_EPISODE;
  m_firstAired.clear();
  m_title.clear();
  m_episodeName.clear();
  m_plotOutline.clear();
  m_plot.clear();
  m_iconPath.clear();
  m_genreString.clear();
  m_cast.clear();
  m_director.clear();
  m_writer.clear();
  m_parentalRating.clear();
  m_parentalRatingSystem.clear();
  m_parentalRatingIconPath.clear();
  m_starRating = 0;
  m_new = false;
  m_premiere = false;

  // From MediaEntry
  m_mediaEntryId.clear();
  m_radio = false;
  m_duration = 0;
  m_playCount = 0;
  m_lastPlayedPosition = 0;
  m_nextSyncTime = 0;
  m_streamURL.clear();
  m_providerName.clear();
  m_providerUniqueId = PVR_PROVIDER_INVALID_UID;
  m_directory.clear();
  m_sizeInBytes = 0;
}

void MediaEntry::UpdateFrom(iptvsimple::data::Channel channel)
{
  m_radio = channel.IsRadio();
  // we store channel name here in case there is no epg entry
  m_m3uName = channel.GetChannelName();
  m_title = m_m3uName;
  m_iconPath = channel.GetIconPath();
  m_tvgId = channel.GetTvgId();
  m_tvgName = channel.GetTvgId();
  m_startTime = std::time(nullptr);

  m_providerUniqueId = channel.GetProviderUniqueId();
  m_properties = channel.GetProperties();
  m_inputStreamName = channel.GetInputStreamName();
}

void MediaEntry::UpdateFrom(iptvsimple::data::EpgEntry epgEntry)
{
  // All from Base Entry
  m_startTime = epgEntry.GetStartTime();
  m_duration = epgEntry.GetEndTime() - epgEntry.GetStartTime();
  m_genreType = epgEntry.GetGenreType();
  m_genreSubType = epgEntry.GetGenreSubType();
  m_year = epgEntry.GetYear();
  m_episodeNumber = epgEntry.GetEpisodeNumber();
  m_episodePartNumber = epgEntry.GetEpisodePartNumber();
  m_seasonNumber = epgEntry.GetSeasonNumber();
  m_firstAired = epgEntry.GetFirstAired();
  m_title = epgEntry.GetTitle();
  m_episodeName = epgEntry.GetEpisodeName();
  m_plotOutline = epgEntry.GetPlotOutline();
  m_plot = epgEntry.GetPlot();
  if (!epgEntry.GetIconPath().empty())
    m_iconPath = epgEntry.GetIconPath();
  m_genreString = epgEntry.GetGenreString();
  m_cast = epgEntry.GetCast();
  m_director = epgEntry.GetDirector();
  m_writer = epgEntry.GetWriter();

  m_parentalRating = epgEntry.GetParentalRating();
  m_parentalRatingSystem = epgEntry.GetParentalRatingSystem();
  m_parentalRatingIconPath = epgEntry.GetParentalRatingIconPath();
  m_starRating = epgEntry.GetStarRating();

  m_new = epgEntry.IsNew();
  m_premiere = epgEntry.IsPremiere();
}

namespace
{

std::string GetSeasonPrefix(int seasonNumber)
{
  if (seasonNumber != EPG_TAG_INVALID_SERIES_EPISODE)
  {
    if (seasonNumber < 10)
      return "S0" + std::to_string(seasonNumber);
    else
      return "S" + std::to_string(seasonNumber);
  }

  return {};
}

std::string GetEpisodePrefix(int episodeNumber)
{
  if (episodeNumber != EPG_TAG_INVALID_SERIES_EPISODE)
  {
    if (episodeNumber < 10)
      return "E0" + std::to_string(episodeNumber);
    else
      return "E" + std::to_string(episodeNumber);
  }

  return {};
}

std::string CreateTitle(const std::string& title, int seasonNumber, int episodeNumber)
{
  std::string newTitle;

  if (Settings::GetInstance().IncludeShowInfoInMediaTitle() &&
      (seasonNumber != EPG_TAG_INVALID_SERIES_EPISODE ||
       episodeNumber != EPG_TAG_INVALID_SERIES_EPISODE))
  {
    std::string newTitle;
    if (seasonNumber != EPG_TAG_INVALID_SERIES_EPISODE)
      newTitle = GetSeasonPrefix(seasonNumber);
    if (episodeNumber != EPG_TAG_INVALID_SERIES_EPISODE)
      newTitle += GetEpisodePrefix(episodeNumber);

    if (!newTitle.empty())
      return newTitle + " - " + title;
  }

  return title;
}

std::string FixPath(const std::string& path)
{
  std::string newPath = path;

  if (path.empty())
  {
    newPath = "/";
  }
  else
  {
    if (!StringUtils::StartsWith(newPath, "/"))
      newPath = "/" + newPath;

    if (!StringUtils::EndsWith(newPath, "/"))
      newPath = newPath + "/";
  }

  return newPath;
}

} // unamed namespace

void MediaEntry::UpdateTo(kodi::addon::PVRRecording& left, bool isInVirtualMediaEntryFolder, bool haveMediaTypes)
{
  left.SetTitle(CreateTitle(m_title, m_seasonNumber, m_episodeNumber));
  left.SetPlotOutline(m_plotOutline);
  left.SetPlot(m_plot);
  // left.SetCast(m_cast);
  // left.SetDirector(m_director);
  // left.SetWriter(m_writer);
  left.SetYear(m_year);
  left.SetIconPath(m_iconPath);
  left.SetGenreType(m_genreType);
  left.SetGenreSubType(m_genreSubType);
  left.SetGenreDescription(m_genreString);
  // if (m_parentalRatingSystem.empty())
  //   left.SetParentalRatingCode(m_parentalRating);
  // else
  //   left.SetParentalRatingCode(m_parentalRatingSystem + "-" + m_parentalRating);
  // left.SetStarRating(m_starRating);
  left.SetSeriesNumber(m_seasonNumber);
  left.SetEpisodeNumber(m_episodeNumber);
  // left.SetEpisodePartNumber(m_episodePartNumber);
  left.SetEpisodeName(m_episodeName);
  left.SetFirstAired(m_firstAired);
  int iFlags = EPG_TAG_FLAG_UNDEFINED;
  if (m_new)
    iFlags |= EPG_TAG_FLAG_IS_NEW;
  if (m_premiere)
    iFlags |= EPG_TAG_FLAG_IS_PREMIERE;
  left.SetFlags(iFlags);

  // From Media Tag
  left.SetRecordingId(m_mediaEntryId);
  left.SetRecordingTime(m_startTime);
  left.SetDuration(m_duration);
  left.SetPlayCount(m_playCount);
  left.SetLastPlayedPosition(m_lastPlayedPosition);
  left.SetProviderName(m_providerName);
  left.SetClientProviderUid(m_providerUniqueId);
  if (m_radio)
    left.SetChannelType(PVR_RECORDING_CHANNEL_TYPE_RADIO);
  else
    left.SetChannelType(PVR_RECORDING_CHANNEL_TYPE_TV);
  left.SetSizeInBytes(m_sizeInBytes);

  std::string newDirectory = FixPath(m_directory);
  if (Settings::GetInstance().GroupMediaByTitle() && isInVirtualMediaEntryFolder)
  {
    if (Settings::GetInstance().GroupMediaBySeason() && m_seasonNumber != EPG_TAG_INVALID_SERIES_EPISODE)
      newDirectory = StringUtils::Format("%s%s/%s/", newDirectory.c_str(), m_title.c_str(), GetSeasonPrefix(m_seasonNumber).c_str());
    else
      newDirectory = StringUtils::Format("%s%s/", newDirectory.c_str(), m_title.c_str());
  }

  left.SetDirectory(newDirectory);
}
