/*
 *      Copyright (C) 2005-2019 Team XBMC
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

#include "EpgEntry.h"

#include "../Settings.h"
#include "../utilities/XMLUtils.h"

#include <cmath>
#include <cstdlib>
#include <regex>

#include <p8-platform/util/StringUtils.h>
#include <pugixml.hpp>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace pugi;

void EpgEntry::UpdateTo(EPG_TAG& left, int iChannelUid, int timeShift, const std::vector<EpgGenre>& genreMappings)
{
  left.iUniqueBroadcastId  = m_broadcastId;
  left.strTitle            = m_title.c_str();
  left.iUniqueChannelId    = iChannelUid;
  left.startTime           = m_startTime + timeShift;
  left.endTime             = m_endTime + timeShift;
  left.strPlotOutline      = m_plotOutline.c_str();
  left.strPlot             = m_plot.c_str();
  left.strOriginalTitle    = nullptr;  /* not supported */
  left.strCast             = m_cast.c_str();
  left.strDirector         = m_director.c_str();
  left.strWriter           = m_writer.c_str();
  left.iYear               = m_year;
  left.strIMDBNumber       = nullptr;  /* not supported */
  left.strIconPath         = m_iconPath.c_str();
  if (SetEpgGenre(genreMappings))
  {
    left.iGenreType           = m_genreType;
    if (Settings::GetInstance().UseEpgGenreTextWhenMapping())
    {
      //Setting this value in sub type allows custom text to be displayed
      //while still sending the type used for EPG colour
      left.iGenreSubType       = EPG_GENRE_USE_STRING;
      left.strGenreDescription = m_genreString.c_str();
    }
    else
    {
      left.iGenreSubType       = m_genreSubType;
      left.strGenreDescription = nullptr;
    }
  }
  else
  {
    left.iGenreType          = EPG_GENRE_USE_STRING;
    left.iGenreSubType       = 0;     /* not supported */
    left.strGenreDescription = m_genreString.c_str();
  }
  left.iParentalRating     = 0;     /* not supported */
  left.iStarRating         = m_starRating;
  left.iSeriesNumber       = m_seasonNumber;
  left.iEpisodeNumber      = m_episodeNumber;
  left.iEpisodePartNumber  = m_episodePartNumber;
  left.strEpisodeName      = m_episodeName.c_str();
  left.iFlags              = EPG_TAG_FLAG_UNDEFINED;
  left.firstAired          = m_firstAired;
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

} // unnamed namespace

bool EpgEntry::UpdateFrom(const xml_node& channelNode, const std::string& id, int broadcastId,
                          int start, int end, int minShiftTime, int maxShiftTime)
{
  std::string strStart, strStop;
  if (!GetAttributeValue(channelNode, "start", strStart) || !GetAttributeValue(channelNode, "stop", strStop))
    return false;

  long long tmpStart = ParseDateTime(strStart);
  long long tmpEnd = ParseDateTime(strStop);

  if ((tmpEnd + maxShiftTime < start) || (tmpStart + minShiftTime > end))
    return false;

  m_broadcastId = broadcastId;
  m_channelId = std::atoi(id.c_str());
  m_genreType = 0;
  m_genreSubType = 0;
  m_plotOutline.clear();
  m_startTime = static_cast<time_t>(tmpStart);
  m_endTime = static_cast<time_t>(tmpEnd);
  m_year = 0;
  m_firstAired = 0;
  m_starRating = 0;
  m_episodeNumber = 0;
  m_episodePartNumber = 0;
  m_seasonNumber = 0;

  m_title = GetNodeValue(channelNode, "title");
  m_plot = GetNodeValue(channelNode, "desc");
  m_episodeName = GetNodeValue(channelNode, "sub-title");

  m_genreString = GetJoinedNodeValues(channelNode, "category");

  const std::string dateString = GetNodeValue(channelNode, "date");
  if (!dateString.empty())
  {
    static const std::regex dateRegex("^[1-9][0-9][0-9][0-9][0-9][1-9][0-9][1-9]");
    if (std::regex_match(dateString, dateRegex))
      m_firstAired = static_cast<time_t>(ParseDateTime(dateString));

    std::sscanf(dateString.c_str(), "%04d", &m_year);
  }

  const auto& starRatingNode = channelNode.child("star-rating");
  if (starRatingNode)
    m_starRating = ParseStarRating(GetNodeValue(starRatingNode, "value"));

  std::vector<std::pair<std::string, std::string>> episodeNumbersList;
  for (const auto& episodeNumNode : channelNode.children("episode-num"))
  {
    std::string episodeNumberSystem;
    if (GetAttributeValue(episodeNumNode, "system", episodeNumberSystem))
      episodeNumbersList.push_back({episodeNumberSystem, episodeNumNode.child_value()});
  }

  if (!episodeNumbersList.empty())
    ParseEpisodeNumberInfo(episodeNumbersList);

  const auto& creditsNode = channelNode.child("credits");
  if (creditsNode)
  {
    m_cast = GetJoinedNodeValues(creditsNode, "actor");
    m_director = GetJoinedNodeValues(creditsNode, "director");
    m_writer = GetJoinedNodeValues(creditsNode, "writer");
  }

  const auto& iconNode = channelNode.child("icon");
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
        m_episodePartNumber = 0;
    }
  }

  return m_episodeNumber;
}

bool EpgEntry::ParseOnScreenEpisodeNumberInfo(const std::string& episodeNumberString)
{
  static const std::regex unwantedCharsRegex("[ \\txX_\\.]");
  const std::string text = std::regex_replace(episodeNumberString, unwantedCharsRegex, "");

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

  return false;
}