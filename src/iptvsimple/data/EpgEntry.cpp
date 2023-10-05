/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "EpgEntry.h"

#include "../InstanceSettings.h"
#include "../utilities/TimeUtils.h"
#include "../utilities/XMLUtils.h"

#include <cmath>
#include <cstdlib>
#include <regex>

#include <kodi/tools/StringUtils.h>
#include <pugixml.hpp>

using namespace kodi::tools;
using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace pugi;

void EpgEntry::UpdateTo(kodi::addon::PVREPGTag& left, int iChannelUid, int timeShift, const std::vector<EpgGenre>& genreMappings)
{
  left.SetUniqueBroadcastId(m_broadcastId);
  left.SetTitle(m_title);
  left.SetUniqueChannelId(iChannelUid);
  left.SetStartTime(m_startTime + timeShift);
  left.SetEndTime(m_endTime + timeShift);
  left.SetPlotOutline(m_plotOutline);
  left.SetPlot(m_plot);
  left.SetCast(m_cast);
  left.SetDirector(m_director);
  left.SetWriter(m_writer);
  left.SetYear(m_year);
  left.SetIconPath(m_iconPath);
  if (SetEpgGenre(genreMappings))
  {
    left.SetGenreType(m_genreType);
    if (m_settings->UseEpgGenreTextWhenMapping())
    {
      //Setting this value in sub type allows custom text to be displayed
      //while still sending the type used for EPG colour
      left.SetGenreSubType(EPG_GENRE_USE_STRING);
      left.SetGenreDescription(m_genreString);
    }
    else
    {
      left.SetGenreSubType(m_genreSubType);
    }
  }
  else
  {
    left.SetGenreType(EPG_GENRE_USE_STRING);
    left.SetGenreDescription(m_genreString);
  }
  if (m_parentalRatingSystem.empty())
    left.SetParentalRatingCode(m_parentalRating);
  else
    left.SetParentalRatingCode(m_parentalRatingSystem + "-" + m_parentalRating);
  left.SetStarRating(m_starRating);
  left.SetSeriesNumber(m_seasonNumber);
  left.SetEpisodeNumber(m_episodeNumber);
  left.SetEpisodePartNumber(m_episodePartNumber);
  left.SetEpisodeName(m_episodeName);
  left.SetFirstAired(m_firstAired);
  int iFlags = EPG_TAG_FLAG_UNDEFINED;
  if (m_new)
    iFlags |= EPG_TAG_FLAG_IS_NEW;
  if (m_premiere)
    iFlags |= EPG_TAG_FLAG_IS_PREMIERE;
  left.SetFlags(iFlags);
}

bool EpgEntry::SetEpgGenre(const std::vector<EpgGenre> genreMappings)
{
  if (genreMappings.empty())
    return false;

  for (const auto& genre : StringUtils::Split(m_genreString, EPG_STRING_TOKEN_SEPARATOR))
  {
    if (genre.empty())
      continue;

    for (const auto& genreMapping : genreMappings)
    {
      if (StringUtils::EqualsNoCase(genreMapping.GetGenreString(), genre))
      {
        m_genreType = genreMapping.GetGenreType();
        m_genreSubType = genreMapping.GetGenreSubType();
        return true;
      }
    }
  }

  return false;
}

namespace
{

// Adapted from https://stackoverflow.com/a/31533119

// Conversion from UTC date to second, signed 64-bit adjustable epoch version.
// Written by François Grieu, 2015-07-21; public domain.

long long MakeTime(int year, int month, int day)
{
  return static_cast<long long>(year) * 365 + year / 4 - year / 100 * 3 / 4 + (month + 2) * 153 / 5 + day;
}

long long GetUTCTime(int year, int mon, int mday, int hour, int min, int sec)
{
  int m = mon - 1;
  int y = year + 100;

  if (m < 2)
  {
    m += 12;
    --y;
  }

  return (((MakeTime(y, m, mday) - MakeTime(1970 + 99, 12, 1)) * 24 + hour) * 60 + min) * 60 + sec;
}

long long ParseDateTime(const std::string& strDate)
{
  int year = 2000;
  int mon = 1;
  int mday = 1;
  int hour = 0;
  int min = 0;
  int sec = 0;
  char offset_sign = '+';
  int offset_hours = 0;
  int offset_minutes = 0;

  std::sscanf(strDate.c_str(), "%04d%02d%02d%02d%02d%02d %c%02d%02d", &year, &mon, &mday, &hour, &min, &sec, &offset_sign, &offset_hours, &offset_minutes);

  long offset_of_date = (offset_hours * 60 + offset_minutes) * 60;
  if (offset_sign == '-')
    offset_of_date = -offset_of_date;

  return GetUTCTime(year, mon, mday, hour, min, sec) - offset_of_date;
}

std::string ParseAsW3CDateString(const std::string& strDate)
{
  int year = 2000;
  int mon = 1;
  int mday = 1;

  std::sscanf(strDate.c_str(), "%04d%02d%02d", &year, &mon, &mday);

  return StringUtils::Format("%04d-%02d-%02d", year, mon, mday);
}

std::string ParseAsW3CDateString(time_t time)
{
  std::tm tm = SafeLocaltime(time);
  char buffer[16];
  std::strftime(buffer, 16, "%Y-%m-%d", &tm);

  return buffer;
}

int ParseStarRating(const std::string& starRatingString)
{
  float starRating = 0;
  float starRatingScale;

  int ret = std::sscanf(starRatingString.c_str(), "%f/ %f", &starRating, &starRatingScale);

  if (ret == 2 && starRatingScale != STAR_RATING_SCALE && starRatingScale != 0.0f)
  {
    starRating /= starRatingScale;
    starRating *= 10;
  }

  if (ret >= 1 && starRating > STAR_RATING_SCALE)
    starRating = STAR_RATING_SCALE;

  return static_cast<int>(std::round(starRating));
}

bool FirstRun(int start, int end)
{
  return start == 0 && end == 0;
}

} // unnamed namespace

bool EpgEntry::UpdateFrom(const xml_node& programmeNode, const std::string& id,
                          int epgWindowsStart, int epgWindowsEnd, int minShiftTime, int maxShiftTime)
{
  std::string strStart, strStop;
  if (!GetAttributeValue(programmeNode, "start", strStart) || !GetAttributeValue(programmeNode, "stop", strStop))
    return false;

  long long programmeStart = ParseDateTime(strStart);
  long long programmeEnd = ParseDateTime(strStop);

  GetAttributeValue(programmeNode, "catchup-id", m_catchupId);
  m_catchupId = StringUtils::Trim(m_catchupId);

  // Discard only if this is not a first run AND
  //  - The programme end time + the max timeshift is earlier than the EPG window start OR
  //  - The programme start time + the min timeshift is after than the EPG window end
  // I.e. we discard any programme that does not start of finish during the EPG window
  if (!FirstRun(epgWindowsStart, epgWindowsEnd) && ((programmeEnd + maxShiftTime < epgWindowsStart) || (programmeStart + minShiftTime > epgWindowsEnd)))
    return false;

  m_broadcastId = static_cast<int>(programmeStart);
  m_channelId = std::atoi(id.c_str());
  m_genreType = 0;
  m_genreSubType = 0;
  m_plotOutline.clear();
  m_startTime = static_cast<time_t>(programmeStart);
  m_endTime = static_cast<time_t>(programmeEnd);
  m_year = 0;
  m_starRating = 0;
  m_episodeNumber = EPG_TAG_INVALID_SERIES_EPISODE;
  m_episodePartNumber = EPG_TAG_INVALID_SERIES_EPISODE;
  m_seasonNumber = EPG_TAG_INVALID_SERIES_EPISODE;

  m_title = GetNodeValue(programmeNode, "title");
  m_plot = GetNodeValue(programmeNode, "desc");
  m_episodeName = GetNodeValue(programmeNode, "sub-title");

  m_genreString = GetJoinedNodeValues(programmeNode, "category");

  const std::string dateString = GetNodeValue(programmeNode, "date");
  if (!dateString.empty())
  {
    static const std::regex dateRegex("^[1-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]");
    if (std::regex_match(dateString, dateRegex))
    {
      long long tmpDate = ParseDateTime(dateString.substr(0, DATESTRING_LENGTH) + strStart.substr(DATESTRING_LENGTH));
      // Protect against negative time_t which can crash on some platforms such as localtime_s on Windows
      if (tmpDate < 0)
      {
        m_firstAired = ParseAsW3CDateString(dateString);
        m_new = m_firstAired == ParseAsW3CDateString(strStart);
      }
      else
      {
        m_firstAired = ParseAsW3CDateString(static_cast<time_t>(tmpDate));
        m_new = m_firstAired == ParseAsW3CDateString(m_startTime);
      }
    }

    std::sscanf(dateString.c_str(), "%04d", &m_year);
  }

  const auto& parentalRatingNode = programmeNode.child("rating");
  if (parentalRatingNode)
  {
    m_parentalRating = GetNodeValue(parentalRatingNode, "value");
    GetAttributeValue(parentalRatingNode, "system", m_parentalRatingSystem);

    const auto& ratingIconNode = programmeNode.child("icon");
    std::string ratingIconPath;
    if (!ratingIconNode || !GetAttributeValue(ratingIconNode, "src", ratingIconPath))
      m_parentalRatingIconPath = "";
    else
      m_parentalRatingIconPath = ratingIconPath;
  }

  const auto& starRatingNode = programmeNode.child("star-rating");
  if (starRatingNode)
    m_starRating = ParseStarRating(GetNodeValue(starRatingNode, "value"));

  const auto& newNode = programmeNode.child("new");
  if (newNode)
    m_new = true;

  const auto& premiereNode = programmeNode.child("premiere");
  if (premiereNode)
    m_premiere = true;

  std::vector<std::pair<std::string, std::string>> episodeNumbersList;
  for (const auto& episodeNumNode : programmeNode.children("episode-num"))
  {
    std::string episodeNumberSystem;
    if (GetAttributeValue(episodeNumNode, "system", episodeNumberSystem))
      episodeNumbersList.push_back({episodeNumberSystem, episodeNumNode.child_value()});
  }

  if (!episodeNumbersList.empty())
  {
    ParseEpisodeNumberInfo(episodeNumbersList);

    // If we don't have a season or episode number don't set the year as it's a TV show
    // For TV show year usually represents the year of S01E01 which we don't have
    if (m_episodeNumber != EPG_TAG_INVALID_SERIES_EPISODE || m_seasonNumber != EPG_TAG_INVALID_SERIES_EPISODE)
      m_year = 0;
  }

  const auto& creditsNode = programmeNode.child("credits");
  if (creditsNode)
  {
    m_cast = GetJoinedNodeValues(creditsNode, "actor");
    m_director = GetJoinedNodeValues(creditsNode, "director");
    m_writer = GetJoinedNodeValues(creditsNode, "writer");
  }

  const auto& iconNode = programmeNode.child("icon");
  std::string iconPath;
  if (!iconNode || !GetAttributeValue(iconNode, "src", iconPath))
    m_iconPath = "";
  else
    m_iconPath = iconPath;

  return true;
}

bool EpgEntry::ParseEpisodeNumberInfo(std::vector<std::pair<std::string, std::string>>& episodeNumbersList)
{
  //First check xmltv_ns
  for (const auto& pair : episodeNumbersList)
  {
    if (pair.first == "xmltv_ns" && ParseXmltvNsEpisodeNumberInfo(pair.second))
      return true;
  }

  //If not found try onscreen
  for (const auto& pair : episodeNumbersList)
  {
    if (pair.first == "onscreen" && ParseOnScreenEpisodeNumberInfo(pair.second))
      return true;
  }

  return false;
}

bool EpgEntry::ParseXmltvNsEpisodeNumberInfo(const std::string& episodeNumberString)
{
  size_t found = episodeNumberString.find(".");
  if (found != std::string::npos)
  {
    const std::string seasonString = episodeNumberString.substr(0, found);
    std::string episodeString = episodeNumberString.substr(found + 1);
    std::string episodePartString;

    found = episodeString.find(".");
    if (found != std::string::npos)
    {
      episodePartString = episodeString.substr(found + 1);
      episodeString = episodeString.substr(0, found);
    }

    if (std::sscanf(seasonString.c_str(), "%d", &m_seasonNumber) == 1)
      m_seasonNumber++;

    if (std::sscanf(episodeString.c_str(), "%d", &m_episodeNumber) == 1)
      m_episodeNumber++;

    if (!episodePartString.empty())
    {
      int totalNumberOfParts;
      int numElementsParsed = std::sscanf(episodePartString.c_str(), "%d/%d", &m_episodePartNumber, &totalNumberOfParts);

      if (numElementsParsed == 2)
        m_episodePartNumber++;
      else if (numElementsParsed == 1)
        m_episodePartNumber = EPG_TAG_INVALID_SERIES_EPISODE;
    }
  }

  return m_episodeNumber;
}

bool EpgEntry::ParseOnScreenEpisodeNumberInfo(const std::string& episodeNumberString)
{
  static const std::regex unwantedCharsRegex("[ \\txX_\\.]");
  const std::string text = std::regex_replace(episodeNumberString, unwantedCharsRegex, "");

  if (StringUtils::StartsWithNoCase(text, "S"))
  {
    std::smatch match;
    static const std::regex seasonEpisodeRegex("^[sS]([0-9][0-9]*)[eE][pP]?([0-9][0-9]*)$");
    if (std::regex_match(text, match, seasonEpisodeRegex))
    {
      if (match.size() == 3)
      {
        m_seasonNumber = std::atoi(match[1].str().c_str());
        m_episodeNumber = std::atoi(match[2].str().c_str());

        return true;
      }
    }
  }
  else if (StringUtils::StartsWithNoCase(text, "E"))
  {
    static const std::regex episodeOnlyRegex("^[eE][pP]?([0-9][0-9]*)$");
    std::smatch matchEpOnly;
    if (std::regex_match(text, matchEpOnly, episodeOnlyRegex))
    {
      if (matchEpOnly.size() == 2)
      {
        m_episodeNumber = std::atoi(matchEpOnly[1].str().c_str());

        return true;
      }
    }
  }

  return false;
}
