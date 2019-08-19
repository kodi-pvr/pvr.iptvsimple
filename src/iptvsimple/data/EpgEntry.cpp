#include "EpgEntry.h"

#include "ChannelEpg.h"
#include "../utilities/XMLUtils.h"

#include "p8-platform/util/StringUtils.h"
#include "rapidxml/rapidxml.hpp"

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace rapidxml;

void EpgEntry::UpdateTo(EPG_TAG& left, int iChannelUid, int timeShift, std::vector<EpgGenre>& genres)
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
  left.iYear               = 0;     /* not supported */
  left.strIMDBNumber       = nullptr;  /* not supported */
  left.strIconPath         = m_iconPath.c_str();
  if (SetEpgGenre(genres, m_genreString))
  {
    left.iGenreType          = m_genreType;
    left.iGenreSubType       = m_genreSubType;
    left.strGenreDescription = nullptr;
  }
  else
  {
    left.iGenreType          = EPG_GENRE_USE_STRING;
    left.iGenreSubType       = 0;     /* not supported */
    left.strGenreDescription = m_genreString.c_str();
  }
  left.iParentalRating     = 0;     /* not supported */
  left.iStarRating         = 0;     /* not supported */
  left.iSeriesNumber       = 0;     /* not supported */
  left.iEpisodeNumber      = 0;     /* not supported */
  left.iEpisodePartNumber  = 0;     /* not supported */
  left.strEpisodeName      = m_episodeName.c_str();
  left.iFlags              = EPG_TAG_FLAG_UNDEFINED;
}

bool EpgEntry::SetEpgGenre(std::vector<EpgGenre> genres, const std::string& genreToFind)
{
  if (genres.empty())
    return false;

  for (const auto& myGenre : genres)
  {
    if (StringUtils::CompareNoCase(myGenre.GetGenreString(), genreToFind) == 0)
    {
      m_genreType = myGenre.GetGenreType();
      m_genreSubType = myGenre.GetGenreSubType();
      return true;
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

  sscanf(strDate.c_str(), "%04d%02d%02d%02d%02d%02d %c%02d%02d", &year, &mon, &mday, &hour, &min, &sec, &offset_sign, &offset_hours, &offset_minutes);

  long offset_of_date = (offset_hours * 60 + offset_minutes) * 60;
  if (offset_sign == '-')
    offset_of_date = -offset_of_date;

  return GetUTCTime(year, mon, mday, hour, min, sec) - offset_of_date;
}

} // unnamed namespace

bool EpgEntry::UpdateFrom(rapidxml::xml_node<>* channelNode, ChannelEpg* channelEpg, const std::string& id, int broadcastId,
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
  m_channelId = atoi(id.c_str());
  m_genreType = 0;
  m_genreSubType = 0;
  m_plotOutline= "";
  m_startTime = static_cast<time_t>(tmpStart);
  m_endTime = static_cast<time_t>(tmpEnd);

  m_title = GetNodeValue(channelNode, "title");
  m_plot = GetNodeValue(channelNode, "desc");
  m_genreString = GetNodeValue(channelNode, "category");
  m_episodeName = GetNodeValue(channelNode, "sub-title");

  xml_node<> *creditsNode = channelNode->first_node("credits");
  if (creditsNode != NULL)
  {
    m_cast = GetNodeValue(creditsNode, "actor");
    m_director = GetNodeValue(creditsNode, "director");
    m_writer = GetNodeValue(creditsNode, "writer");
  }

  xml_node<>* iconNode = channelNode->first_node("icon");
  std::string iconPath;
  if (!iconNode || !GetAttributeValue(iconNode, "src", iconPath))
    m_iconPath = "";
  else
    m_iconPath = iconPath;

  return true;
}