/*
 *      Copyright (C) 2013-2015 Anton Fedchin
 *      http://github.com/afedchin/xbmc-addon-iptvsimple/
 *
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <cctype>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

#include "zlib.h"
#include "rapidxml/rapidxml.hpp"
#include "PVRIptvData.h"
#include "p8-platform/util/StringUtils.h"
#include "client.h"

#define M3U_START_MARKER        "#EXTM3U"
#define M3U_INFO_MARKER         "#EXTINF"
#define TVG_INFO_ID_MARKER      "tvg-id="
#define TVG_INFO_ID_MARKER_UC   "tvg-ID=" //some providers incorrecty use an uppercase ID.
#define TVG_INFO_NAME_MARKER    "tvg-name="
#define TVG_INFO_LOGO_MARKER    "tvg-logo="
#define TVG_INFO_SHIFT_MARKER   "tvg-shift="
#define TVG_INFO_CHNO_MARKER    "tvg-chno="
#define GROUP_NAME_MARKER       "group-title="
#define KODIPROP_MARKER         "#KODIPROP:"
#define EXTVLCOPT_MARKER        "#EXTVLCOPT:"
#define RADIO_MARKER            "radio="
#define PLAYLIST_TYPE_MARKER    "#EXT-X-PLAYLIST-TYPE:"
#define CHANNEL_LOGO_EXTENSION  ".png"
#define SECONDS_IN_DAY          86400
#define GENRES_MAP_FILENAME     "genres.xml"
#define STAR_RATING_SCALE       10.0f
#define REMOTE_PATH_TYPE        1

using namespace ADDON;
using namespace rapidxml;

template<class Ch>
inline bool GetNodeValue(const xml_node<Ch> * pRootNode, const char* strTag, std::string& strStringValue)
{
  xml_node<Ch> *pChildNode = pRootNode->first_node(strTag);
  if (pChildNode == NULL)
  {
    return false;
  }
  strStringValue = pChildNode->value();
  return true;
}

template<class Ch>
inline bool GetJoinedNodeValues(const xml_node<Ch>* pRootNode, const char* strTag, std::string& strStringValue)
{
  for (xml_node<Ch>* pChildNode = pRootNode->first_node(strTag); pChildNode; pChildNode = pChildNode->next_sibling(strTag))
  {
    if (pChildNode)
    {
      if (!strStringValue.empty())
        strStringValue += ",";
      strStringValue += pChildNode->value();
    }
  }

  return !strStringValue.empty();
}

template<class Ch>
inline bool GetAttributeValue(const xml_node<Ch> * pNode, const char* strAttributeName, std::string& strStringValue)
{
  xml_attribute<Ch> *pAttribute = pNode->first_attribute(strAttributeName);
  if (pAttribute == NULL)
  {
    return false;
  }
  strStringValue = pAttribute->value();
  return true;
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

int ParseStarRating(const std::string& starRatingString)
{
  float starRating = 0;
  float starRatingScale;

  int ret = std::sscanf(starRatingString.c_str(), "%f/%f", &starRating, &starRatingScale);

  if (ret == 2 && starRatingScale != STAR_RATING_SCALE && starRatingScale != 0.0f)
  {
    starRating /= starRatingScale;
    starRating *= 10;
  }

  if (ret >= 1 && starRating > STAR_RATING_SCALE)
    starRating = STAR_RATING_SCALE;

  return static_cast<int>(std::round(starRating));
}

// http://stackoverflow.com/a/17708801
const std::string UrlEncode(const std::string& value)
{
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (auto c : value)
  {
    // Keep alphanumeric and other accepted characters intact
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
    {
      escaped << c;
      continue;
    }
    // Any other characters are percent-encoded
    escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
  }

  return escaped.str();
}

} // unnamed namespace


PVRIptvData::PVRIptvData(void)
{
  m_strXMLTVUrl   = g_strTvgPath;
  m_strM3uUrl     = g_strM3UPath;
  m_strLogoPath   = g_strLogoPath;
  m_logoPathType  = g_logoPathType;
  m_iEPGTimeShift = g_iEPGTimeShift;
  m_bTSOverride   = g_bTSOverride;
  m_iLastStart    = 0;
  m_iLastEnd      = 0;

  m_channels.clear();
  m_groups.clear();
  m_epg.clear();
  m_genres.clear();

  LoadPlayList();
}

void *PVRIptvData::Process(void)
{
  return NULL;
}

PVRIptvData::~PVRIptvData(void)
{
  m_channels.clear();
  m_groups.clear();
  m_epg.clear();
  m_genres.clear();
}

bool PVRIptvData::LoadEPG(time_t iStart, time_t iEnd)
{
  auto started = std::chrono::high_resolution_clock::now();
  XBMC->Log(LOG_DEBUG, "%s EPG Load Start", __FUNCTION__);

  if (m_strXMLTVUrl.empty())
  {
    XBMC->Log(LOG_NOTICE, "EPG file path is not configured. EPG not loaded.");
    return false;
  }

  std::string data;
  std::string decompressed;
  int iReaded = 0;

  int iCount = 0;
  while(iCount < 3) // max 3 tries
  {
    if ((iReaded = GetCachedFileContents(TVG_FILE_NAME, m_strXMLTVUrl, data, g_bCacheEPG)) != 0)
    {
      break;
    }
    XBMC->Log(LOG_ERROR, "Unable to load EPG file '%s':  file is missing or empty. :%dth try.", m_strXMLTVUrl.c_str(), ++iCount);
    if (iCount < 3)
    {
      usleep(2 * 1000 * 1000); // sleep 2 sec before next try.
    }
  }

  if (iReaded == 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to load EPG file '%s':  file is missing or empty. After %d tries.", m_strXMLTVUrl.c_str(), iCount);
    return false;
  }

  char * buffer;

  // gzip packed
  if (data[0] == '\x1F' && data[1] == '\x8B' && data[2] == '\x08')
  {
    if (!GzipInflate(data, decompressed))
    {
      XBMC->Log(LOG_ERROR, "Invalid EPG file '%s': unable to decompress file.", m_strXMLTVUrl.c_str());
      return false;
    }
    buffer = &(decompressed[0]);
  }
  else
    buffer = &(data[0]);

  // xml should starts with '<?xml'
  if (buffer[0] != '\x3C' || buffer[1] != '\x3F' || buffer[2] != '\x78' ||
      buffer[3] != '\x6D' || buffer[4] != '\x6C')
  {
    // check for BOM
    if (buffer[0] != '\xEF' || buffer[1] != '\xBB' || buffer[2] != '\xBF')
    {
      // check for tar archive
      if (strcmp(buffer + 0x101, "ustar") || strcmp(buffer + 0x101, "GNUtar"))
        buffer += 0x200; // RECORDSIZE = 512
      else
      {
        XBMC->Log(LOG_ERROR, "Invalid EPG file '%s': unable to parse file.", m_strXMLTVUrl.c_str());
        return false;
      }
    }
  }

  xml_document<> xmlDoc;
  try
  {
    xmlDoc.parse<0>(buffer);
  }
  catch(parse_error& p)
  {
    XBMC->Log(LOG_ERROR, "Unable parse EPG XML: %s", p.what());
    return false;
  }

  xml_node<> *pRootElement = xmlDoc.first_node("tv");
  if (!pRootElement)
  {
    XBMC->Log(LOG_ERROR, "Invalid EPG XML: no <tv> tag found");
    return false;
  }

  // clear previously loaded epg
  if (m_epg.size() > 0)
    m_epg.clear();

  int iBroadCastId = 0;
  xml_node<> *pChannelNode = NULL;
  for(pChannelNode = pRootElement->first_node("channel"); pChannelNode; pChannelNode = pChannelNode->next_sibling("channel"))
  {
    PVRIptvEpgChannel epgChannel;

    if (!GetAttributeValue(pChannelNode, "id", epgChannel.strId))
      continue;

    bool foundChannel = false;
    bool haveDisplayNames = false;
    for (xml_node<>* pDisplayNameNode = pChannelNode->first_node("display-name"); pDisplayNameNode; pDisplayNameNode = pDisplayNameNode->next_sibling("display-name"))
    {
      haveDisplayNames = true;

      const std::string strName = pDisplayNameNode->value();
      if (FindChannel(epgChannel.strId, strName))
      {
        foundChannel = true;
        epgChannel.strNames.emplace_back(strName);
      }
    }

    // If there are no display names just check if the id matches a channel
    if (!haveDisplayNames && FindChannel(epgChannel.strId, ""))
      foundChannel = true;

    if (!foundChannel)
      continue;

    // get icon if available
    xml_node<> *pIconNode = pChannelNode->first_node("icon");
    if (pIconNode == NULL || !GetAttributeValue(pIconNode, "src", epgChannel.strIcon))
      epgChannel.strIcon = "";

    PVRIptvEpgChannel* existingEpgChannel = FindEpgForChannel(epgChannel.strId);
    if (existingEpgChannel)
    {
      for (const std::string& displayName : epgChannel.strNames)
        existingEpgChannel->strNames.emplace_back(displayName);

      if (existingEpgChannel->strIcon.empty() && !epgChannel.strIcon.empty())
        existingEpgChannel->strIcon = epgChannel.strIcon;

      continue;
    }

    m_epg.push_back(epgChannel);
  }

  if (m_epg.size() == 0)
  {
    XBMC->Log(LOG_ERROR, "EPG channels not found.");
    return false;
  }

  int iMinShiftTime = m_iEPGTimeShift;
  int iMaxShiftTime = m_iEPGTimeShift;
  if (!m_bTSOverride)
  {
    iMinShiftTime = SECONDS_IN_DAY;
    iMaxShiftTime = -SECONDS_IN_DAY;

    std::vector<PVRIptvChannel>::iterator it;
    for (it = m_channels.begin(); it < m_channels.end(); ++it)
    {
      if (it->iTvgShift + m_iEPGTimeShift < iMinShiftTime)
        iMinShiftTime = it->iTvgShift + m_iEPGTimeShift;
      if (it->iTvgShift + m_iEPGTimeShift > iMaxShiftTime)
        iMaxShiftTime = it->iTvgShift + m_iEPGTimeShift;
    }
  }

  PVRIptvEpgChannel *epg = NULL;
  for(pChannelNode = pRootElement->first_node("programme"); pChannelNode; pChannelNode = pChannelNode->next_sibling("programme"))
  {
    std::string strId;
    if (!GetAttributeValue(pChannelNode, "channel", strId))
      continue;

    if (NULL == epg || StringUtils::CompareNoCase(epg->strId, strId) != 0)
    {
      if ((epg = FindEpg(strId)) == NULL)
        continue;
    }

    std::string strStart, strStop;
    if ( !GetAttributeValue(pChannelNode, "start", strStart)
      || !GetAttributeValue(pChannelNode, "stop", strStop))
      continue;

    long long iTmpStart = ParseDateTime(strStart);
    long long iTmpEnd = ParseDateTime(strStop);

    if ( (iTmpEnd   + iMaxShiftTime < iStart)
      || (iTmpStart + iMinShiftTime > iEnd))
      continue;

    PVRIptvEpgEntry entry;
    entry.iBroadcastId = ++iBroadCastId;
    entry.iChannelId = atoi(strId.c_str());
    entry.iGenreType = 0;
    entry.iGenreSubType = 0;
    entry.strPlotOutline = "";
    entry.startTime = static_cast<time_t>(iTmpStart);
    entry.endTime = static_cast<time_t>(iTmpEnd);
    entry.iYear = 0;
    entry.firstAired = 0;
    entry.iStarRating = 0;
    entry.iSeasonNumber = 0;
    entry.iEpisodeNumber = 0;
    entry.iEpisodePartNumber = 0;

    GetNodeValue(pChannelNode, "title", entry.strTitle);
    GetNodeValue(pChannelNode, "desc", entry.strPlot);
    GetNodeValue(pChannelNode, "category", entry.strGenreString);
    GetNodeValue(pChannelNode, "sub-title", entry.strEpisodeName);

    std::string dateString;
    GetNodeValue(pChannelNode, "date", dateString);
    if (!dateString.empty())
    {
      static const std::regex dateRegex("^[1-9][0-9][0-9][0-9][0-9][1-9][0-9][1-9]");
      if (std::regex_match(dateString, dateRegex))
        entry.firstAired = static_cast<time_t>(ParseDateTime(dateString));

      std::sscanf(dateString.c_str(), "%04d", &entry.iYear);
    }

    xml_node<>* pStarRatingNode = pChannelNode->first_node("star-rating");
    if (pStarRatingNode)
    {
      std::string starRatingString;
      GetNodeValue(pStarRatingNode, "value", starRatingString);

      if (!starRatingString.empty())
        entry.iStarRating = ParseStarRating(starRatingString);
    }

    std::vector<std::pair<std::string, std::string>> episodeNumbersList;
    for (xml_node<>* pEpisodeNumNode = pChannelNode->first_node("episode-num"); pEpisodeNumNode; pEpisodeNumNode = pEpisodeNumNode->next_sibling("episode-num"))
    {
      std::string strEpisodeNumberSystem;
      if (GetAttributeValue(pEpisodeNumNode, "system", strEpisodeNumberSystem))
        episodeNumbersList.push_back({strEpisodeNumberSystem, pEpisodeNumNode->value()});
    }

    if (!episodeNumbersList.empty())
      ParseEpisodeNumberInfo(episodeNumbersList, entry);

    xml_node<>* pCreditsNode = pChannelNode->first_node("credits");
    if (pCreditsNode)
    {
      GetJoinedNodeValues(pCreditsNode, "actor", entry.strCast);
      GetJoinedNodeValues(pCreditsNode, "director", entry.strDirector);
      GetJoinedNodeValues(pCreditsNode, "writer", entry.strWriter);
    }

    xml_node<> *pIconNode = pChannelNode->first_node("icon");
    if (pIconNode == NULL || !GetAttributeValue(pIconNode, "src", entry.strIconPath))
      entry.strIconPath = "";

    epg->epg.push_back(entry);
  }

  xmlDoc.clear();

  LoadGenres();

  if (g_iEPGLogos > 0)
    ApplyChannelsLogosFromEPG();

  int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::high_resolution_clock::now() - started).count();

  XBMC->Log(LOG_NOTICE, "%s EPG Loaded - %d (ms)", __FUNCTION__, milliseconds);

  return true;
}

bool PVRIptvData::ParseEpisodeNumberInfo(const std::vector<std::pair<std::string, std::string>>& episodeNumbersList, PVRIptvEpgEntry& entry)
{
  //First check xmltv_ns
  for (const auto& pair : episodeNumbersList)
  {
    if (pair.first == "xmltv_ns" && ParseXmltvNsEpisodeNumberInfo(pair.second, entry))
      return true;
  }

  //If not found try onscreen
  for (const auto& pair : episodeNumbersList)
  {
    if (pair.first == "onscreen" && ParseOnScreenEpisodeNumberInfo(pair.second, entry))
      return true;
  }

  return false;
}

bool PVRIptvData::ParseXmltvNsEpisodeNumberInfo(const std::string& episodeNumberString, PVRIptvEpgEntry& entry)
{
  size_t found = episodeNumberString.find(".");
  if (found != std::string::npos)
  {
    std::string seasonString = episodeNumberString.substr(0, found);
    std::string episodeString = episodeNumberString.substr(found + 1);
    std::string episodePartString;

    found = episodeString.find(".");
    if (found != std::string::npos)
    {
      episodePartString = episodeString.substr(found + 1);
      episodeString = episodeString.substr(0, found);
    }

    if (std::sscanf(seasonString.c_str(), "%d", &entry.iSeasonNumber) == 1)
      entry.iSeasonNumber++;

    if (std::sscanf(episodeString.c_str(), "%d", &entry.iEpisodeNumber) == 1)
      entry.iEpisodeNumber++;

    if (!episodePartString.empty())
    {
      int totalNumberOfParts;
      int numElementsParsed = std::sscanf(episodePartString.c_str(), "%d/%d", &entry.iEpisodePartNumber, &totalNumberOfParts);

      if (numElementsParsed == 2)
        entry.iEpisodePartNumber++;
      else if (numElementsParsed == 1)
        entry.iEpisodePartNumber = 0;
    }
  }

  return entry.iEpisodeNumber != 0;
}

bool PVRIptvData::ParseOnScreenEpisodeNumberInfo(const std::string& episodeNumberString, PVRIptvEpgEntry& entry)
{
  static const std::regex numRegex("[ \\txX_\\.]");
  const std::string text = std::regex_replace(episodeNumberString, numRegex, "");

  std::smatch match;
  static const std::regex epRegex("^[sS]([0-9][0-9]*)[eE][pP]?([0-9][0-9]*)$");
  if (std::regex_match(text, match, epRegex))
  {
    if (match.size() == 3)
    {
      entry.iSeasonNumber = std::atoi(match[1].str().c_str());
      entry.iEpisodeNumber = std::atoi(match[2].str().c_str());

      return true;
    }
  }

  return false;
}

bool PVRIptvData::LoadPlayList(void)
{
  auto started = std::chrono::high_resolution_clock::now();
  XBMC->Log(LOG_DEBUG, "%s PlayList Load Start", __FUNCTION__);

  if (m_strM3uUrl.empty())
  {
    XBMC->Log(LOG_NOTICE, "Playlist file path is not configured. Channels not loaded.");
    return false;
  }

  std::string strPlaylistContent;
  if (!GetCachedFileContents(M3U_FILE_NAME, m_strM3uUrl, strPlaylistContent, g_bCacheM3U))
  {
    XBMC->Log(LOG_ERROR, "Unable to load playlist file '%s':  file is missing or empty.", m_strM3uUrl.c_str());
    return false;
  }

  std::stringstream stream(strPlaylistContent);

  /* load channels */
  bool bFirst = true;
  bool bIsRealTime  = true;
  int iChannelIndex     = 0;
  int iUniqueGroupId    = 0;
  int iChannelNum       = g_iStartNumber;
  int iEPGTimeShift     = 0;
  std::vector<int> iCurrentGroupId;

  PVRIptvChannel tmpChannel;
  tmpChannel.strTvgId       = "";
  tmpChannel.strChannelName = "";
  tmpChannel.strTvgName     = "";
  tmpChannel.strTvgLogo     = "";
  tmpChannel.iTvgShift      = 0;

  std::string strLine;
  while(std::getline(stream, strLine))
  {
    strLine = StringUtils::TrimRight(strLine, " \t\r\n");
    strLine = StringUtils::TrimLeft(strLine, " \t");

    XBMC->Log(LOG_DEBUG, "Read line: '%s'", strLine.c_str());

    if (strLine.empty())
    {
      continue;
    }

    if (bFirst)
    {
      bFirst = false;
      if (StringUtils::Left(strLine, 3) == "\xEF\xBB\xBF")
      {
        strLine.erase(0, 3);
      }
      if (StringUtils::Left(strLine, strlen(M3U_START_MARKER)) == M3U_START_MARKER)
      {
        double fTvgShift = atof(ReadMarkerValue(strLine, TVG_INFO_SHIFT_MARKER).c_str());
        iEPGTimeShift = (int) (fTvgShift * 3600.0);
        continue;
      }
      else
      {
        XBMC->Log(LOG_ERROR,
                  "URL '%s' missing %s descriptor on line 1, attempting to "
                  "parse it anyway.",
                  m_strM3uUrl.c_str(), M3U_START_MARKER);
      }
    }

    if (StringUtils::Left(strLine, strlen(M3U_INFO_MARKER)) == M3U_INFO_MARKER)
    {
      bool        bRadio       = false;
      double      fTvgShift    = 0;
      std::string strChnlNo    = "";
      std::string strChnlName  = "";
      std::string strTvgId     = "";
      std::string strTvgName   = "";
      std::string strTvgLogo   = "";
      std::string strTvgShift  = "";
      std::string strGroupName = "";
      std::string strRadio     = "";

      iCurrentGroupId.clear();

      // parse line
      int iColon = (int)strLine.find(':');
      int iComma = (int)strLine.rfind(',');
      if (iColon >= 0 && iComma >= 0 && iComma > iColon)
      {
        // parse name
        iComma++;
        strChnlName = StringUtils::Right(strLine, (int)strLine.size() - iComma);
        strChnlName = StringUtils::Trim(strChnlName);
        tmpChannel.strChannelName = XBMC->UnknownToUTF8(strChnlName.c_str());

        // parse info
        iColon++;
        iComma--;
        std::string strInfoLine = StringUtils::Mid(strLine, iColon, iComma - iColon);

        strTvgId      = ReadMarkerValue(strInfoLine, TVG_INFO_ID_MARKER);
        strTvgName    = ReadMarkerValue(strInfoLine, TVG_INFO_NAME_MARKER);
        strTvgLogo    = ReadMarkerValue(strInfoLine, TVG_INFO_LOGO_MARKER);
        strChnlNo     = ReadMarkerValue(strInfoLine, TVG_INFO_CHNO_MARKER);
        strGroupName  = ReadMarkerValue(strInfoLine, GROUP_NAME_MARKER);
        strRadio      = ReadMarkerValue(strInfoLine, RADIO_MARKER);
        strTvgShift   = ReadMarkerValue(strInfoLine, TVG_INFO_SHIFT_MARKER);

        if (strTvgId.empty())
          strTvgId = ReadMarkerValue(strInfoLine, TVG_INFO_ID_MARKER_UC);

        if (strTvgId.empty())
        {
          char buff[255];
          sprintf(buff, "%d", atoi(strInfoLine.c_str()));
          strTvgId.append(buff);
        }
        bool logoSetFromChannelName = false;
        if (strTvgLogo.empty())
        {
          strTvgLogo = strChnlName;
          logoSetFromChannelName = true;
        }
        if (!strChnlNo.empty())
        {
          iChannelNum = atoi(strChnlNo.c_str());
        }
        fTvgShift = atof(strTvgShift.c_str());

        bRadio                = !StringUtils::CompareNoCase(strRadio, "true");
        tmpChannel.strTvgId   = strTvgId;
        tmpChannel.strTvgName = XBMC->UnknownToUTF8(strTvgName.c_str());
        tmpChannel.strTvgLogo = XBMC->UnknownToUTF8(strTvgLogo.c_str());
        tmpChannel.iTvgShift  = (int)(fTvgShift * 3600.0);
        tmpChannel.bRadio     = bRadio;

        // If remote type URL Encode and append as when built from channel name it would be missing
        if (m_logoPathType == REMOTE_PATH_TYPE && logoSetFromChannelName)
          tmpChannel.strTvgLogo = UrlEncode(tmpChannel.strTvgLogo);

        if (tmpChannel.strTvgLogo.find("://") == std::string::npos &&
            !StringUtils::EndsWithNoCase(tmpChannel.strTvgLogo, ".png") &&
            !StringUtils::EndsWithNoCase(tmpChannel.strTvgLogo, ".jpg"))
          tmpChannel.strTvgLogo += CHANNEL_LOGO_EXTENSION;

        if (strTvgShift.empty())
        {
          tmpChannel.iTvgShift = iEPGTimeShift;
        }

        if (!strGroupName.empty())
        {
          std::stringstream streamGroups(strGroupName);
          PVRIptvChannelGroup * pGroup;

          while(std::getline(streamGroups, strGroupName, ';'))
          {
            strGroupName = XBMC->UnknownToUTF8(strGroupName.c_str());

            if ((pGroup = FindGroup(strGroupName)) == NULL)
            {
              PVRIptvChannelGroup group;
              group.strGroupName = strGroupName;
              group.iGroupId = ++iUniqueGroupId;
              group.bRadio = bRadio;

              m_groups.push_back(group);
              iCurrentGroupId.push_back(iUniqueGroupId);
            }
            else
            {
              iCurrentGroupId.push_back(pGroup->iGroupId);
            }
          }
        }
      }
    }
    else if (StringUtils::Left(strLine, strlen(KODIPROP_MARKER)) == KODIPROP_MARKER)
    {
      std::string value = ReadMarkerValue(strLine, KODIPROP_MARKER);
      auto pos = value.find('=');
      if (pos != std::string::npos)
      {
        std::string prop = value.substr(0,pos);
        std::string propValue = value.substr(pos+1);
        tmpChannel.properties.insert({prop, propValue});
      }
    }
    else if (StringUtils::Left(strLine, strlen(EXTVLCOPT_MARKER)) == EXTVLCOPT_MARKER)
    {
      const std::string value = ReadMarkerValue(strLine, EXTVLCOPT_MARKER);
      auto pos = value.find('=');
      if (pos != std::string::npos)
      {
        const std::string prop = value.substr(0, pos);
        const std::string propValue = value.substr(pos + 1);
        tmpChannel.properties.insert({prop, propValue});

        XBMC->Log(LOG_DEBUG,
                  "Found #EXTVLCOPT property: '%s' value: '%s'",
                  prop.c_str(), propValue.c_str());
      }
    }
    else if (StringUtils::Left(strLine, strlen(PLAYLIST_TYPE_MARKER)) == PLAYLIST_TYPE_MARKER)
    {
      if (ReadMarkerValue(strLine, PLAYLIST_TYPE_MARKER) == "VOD")
        bIsRealTime = false;
    }
    else if (strLine[0] != '#')
    {
      XBMC->Log(LOG_DEBUG,
                "Found URL: '%s' (current channel name: '%s')",
                strLine.c_str(), tmpChannel.strChannelName.c_str());

      if (bIsRealTime)
        tmpChannel.properties.insert({PVR_STREAM_PROPERTY_ISREALTIMESTREAM, "true"});

      PVRIptvChannel channel;
      channel.iUniqueId         = GetChannelId(tmpChannel.strChannelName.c_str(), strLine.c_str());
      channel.iChannelNumber    = iChannelNum;
      channel.strTvgId          = tmpChannel.strTvgId;
      channel.strChannelName    = tmpChannel.strChannelName;
      channel.strTvgName        = tmpChannel.strTvgName;
      channel.strTvgLogo        = tmpChannel.strTvgLogo;
      channel.iTvgShift         = tmpChannel.iTvgShift;
      channel.bRadio            = tmpChannel.bRadio;
      channel.properties        = tmpChannel.properties;
      
      auto propPair = channel.properties.find("http-user-agent");
      if (propPair != channel.properties.end())
      {
        channel.strStreamURL = AddHeaderToStreamUrl(strLine, "user-agent", propPair->second);
      }
      else if (!g_userAgent.empty())
      {
        channel.strStreamURL = AddHeaderToStreamUrl(strLine, "user-agent", g_userAgent);
      }
      else
      {
        channel.strStreamURL = strLine;
      }
      
      channel.iEncryptionSystem = 0;

      iChannelNum++;

      std::vector<int>::iterator it;
      for (auto it = iCurrentGroupId.begin(); it != iCurrentGroupId.end(); ++it)
      {
        channel.bRadio = m_groups.at(*it - 1).bRadio;
        m_groups.at(*it - 1).members.push_back(iChannelIndex);
      }

      m_channels.push_back(channel);
      iChannelIndex++;

      tmpChannel.strTvgId       = "";
      tmpChannel.strChannelName = "";
      tmpChannel.strTvgName     = "";
      tmpChannel.strTvgLogo     = "";
      tmpChannel.iTvgShift      = 0;
      tmpChannel.bRadio         = false;
      tmpChannel.properties.clear();
      bIsRealTime = true;
    }
  }

  stream.clear();

  int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::high_resolution_clock::now() - started).count();

  XBMC->Log(LOG_NOTICE, "%s PlayList Loaded - %d (ms)", __FUNCTION__, milliseconds);

  if (m_channels.size() == 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to load channels from file '%s':  file is corrupted.", m_strM3uUrl.c_str());
    return false;
  }

  ApplyChannelsLogos();

  XBMC->Log(LOG_NOTICE, "Loaded %d channels.", m_channels.size());
  return true;
}

bool PVRIptvData::LoadGenres(void)
{
  std::string data;

  // try to load genres from userdata folder
  std::string strFilePath = GetUserFilePath(GENRES_MAP_FILENAME);
  if (!XBMC->FileExists(strFilePath.c_str(), false))
  {
    // try to load file from addom folder
    strFilePath = GetClientFilePath(GENRES_MAP_FILENAME);
    if (!XBMC->FileExists(strFilePath.c_str(), false))
      return false;
  }

  GetFileContents(strFilePath, data);

  if (data.empty())
    return false;

  m_genres.clear();

  char* buffer = &(data[0]);
  xml_document<> xmlDoc;
  try
  {
    xmlDoc.parse<0>(buffer);
  }
  catch (parse_error& p)
  {
    return false;
  }

  xml_node<> *pRootElement = xmlDoc.first_node("genres");
  if (!pRootElement)
    return false;

  for (xml_node<> *pGenreNode = pRootElement->first_node("genre"); pGenreNode; pGenreNode = pGenreNode->next_sibling("genre"))
  {
    std::string buff;
    if (!GetAttributeValue(pGenreNode, "type", buff))
      continue;

    if (!StringUtils::IsNaturalNumber(buff))
      continue;

    PVRIptvEpgGenre genre;
    genre.strGenre = pGenreNode->value();
    genre.iGenreType = atoi(buff.c_str());
    genre.iGenreSubType = 0;

    if ( GetAttributeValue(pGenreNode, "subtype", buff)
      && StringUtils::IsNaturalNumber(buff))
      genre.iGenreSubType = atoi(buff.c_str());

    m_genres.push_back(genre);
  }

  xmlDoc.clear();
  return true;
}

int PVRIptvData::GetChannelsAmount(void)
{
  P8PLATFORM::CLockObject lock(m_mutex);
  return m_channels.size();
}

PVR_ERROR PVRIptvData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  P8PLATFORM::CLockObject lock(m_mutex);
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    PVRIptvChannel &channel = m_channels.at(iChannelPtr);
    if (channel.bRadio == bRadio)
    {
      PVR_CHANNEL xbmcChannel;
      memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

      xbmcChannel.iUniqueId         = channel.iUniqueId;
      xbmcChannel.bIsRadio          = channel.bRadio;
      xbmcChannel.iChannelNumber    = channel.iChannelNumber;
      strncpy(xbmcChannel.strChannelName, channel.strChannelName.c_str(), sizeof(xbmcChannel.strChannelName) - 1);
      xbmcChannel.iEncryptionSystem = channel.iEncryptionSystem;
      strncpy(xbmcChannel.strIconPath, channel.strLogoPath.c_str(), sizeof(xbmcChannel.strIconPath) - 1);
      xbmcChannel.bIsHidden         = false;

      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

bool PVRIptvData::GetChannel(const PVR_CHANNEL &channel, PVRIptvChannel &myChannel)
{
  P8PLATFORM::CLockObject lock(m_mutex);
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    PVRIptvChannel &thisChannel = m_channels.at(iChannelPtr);
    if (thisChannel.iUniqueId == (int) channel.iUniqueId)
    {
      myChannel.iUniqueId         = thisChannel.iUniqueId;
      myChannel.bRadio            = thisChannel.bRadio;
      myChannel.iChannelNumber    = thisChannel.iChannelNumber;
      myChannel.iEncryptionSystem = thisChannel.iEncryptionSystem;
      myChannel.strChannelName    = thisChannel.strChannelName;
      myChannel.strLogoPath       = thisChannel.strLogoPath;
      myChannel.strStreamURL      = thisChannel.strStreamURL;
      myChannel.properties        = thisChannel.properties;

      return true;
    }
  }

  return false;
}

int PVRIptvData::GetChannelGroupsAmount(void)
{
  P8PLATFORM::CLockObject lock(m_mutex);
  return m_groups.size();
}

PVR_ERROR PVRIptvData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  P8PLATFORM::CLockObject lock(m_mutex);
  std::vector<PVRIptvChannelGroup>::iterator it;
  for (it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    if (it->bRadio == bRadio)
    {
      PVR_CHANNEL_GROUP xbmcGroup;
      memset(&xbmcGroup, 0, sizeof(PVR_CHANNEL_GROUP));

      xbmcGroup.iPosition = 0;      /* not supported  */
      xbmcGroup.bIsRadio  = bRadio; /* is radio group */
      strncpy(xbmcGroup.strGroupName, it->strGroupName.c_str(), sizeof(xbmcGroup.strGroupName) - 1);

      PVR->TransferChannelGroup(handle, &xbmcGroup);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRIptvData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  P8PLATFORM::CLockObject lock(m_mutex);
  PVRIptvChannelGroup *myGroup;
  if ((myGroup = FindGroup(group.strGroupName)) != NULL)
  {
    std::vector<int>::iterator it;
    for (it = myGroup->members.begin(); it != myGroup->members.end(); ++it)
    {
      if ((*it) < 0 || (*it) >= (int)m_channels.size())
        continue;

      PVRIptvChannel &channel = m_channels.at(*it);
      PVR_CHANNEL_GROUP_MEMBER xbmcGroupMember;
      memset(&xbmcGroupMember, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

      strncpy(xbmcGroupMember.strGroupName, group.strGroupName, sizeof(xbmcGroupMember.strGroupName) - 1);
      xbmcGroupMember.iChannelUniqueId = channel.iUniqueId;
      xbmcGroupMember.iChannelNumber   = channel.iChannelNumber;

      PVR->TransferChannelGroupMember(handle, &xbmcGroupMember);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRIptvData::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  P8PLATFORM::CLockObject lock(m_mutex);
  std::vector<PVRIptvChannel>::iterator myChannel;
  for (myChannel = m_channels.begin(); myChannel < m_channels.end(); ++myChannel)
  {
    if (myChannel->iUniqueId != (int) channel.iUniqueId)
      continue;

    if (iStart > m_iLastStart || iEnd > m_iLastEnd)
    {
      // reload EPG for new time interval only
      LoadEPG(iStart, iEnd);
      {
        // doesn't matter is epg loaded or not we shouldn't try to load it for same interval
        m_iLastStart = iStart;
        m_iLastEnd = iEnd;
      }
    }

    PVRIptvEpgChannel *epg;
    if ((epg = FindEpgForChannel(*myChannel)) == NULL || epg->epg.size() == 0)
      return PVR_ERROR_NO_ERROR;

    int iShift = m_bTSOverride ? m_iEPGTimeShift : myChannel->iTvgShift + m_iEPGTimeShift;

    std::vector<PVRIptvEpgEntry>::iterator myTag;
    for (myTag = epg->epg.begin(); myTag < epg->epg.end(); ++myTag)
    {
      if ((myTag->endTime + iShift) < iStart)
        continue;

      int iGenreType, iGenreSubType;

      EPG_TAG tag;
      memset(&tag, 0, sizeof(EPG_TAG));

      tag.iUniqueBroadcastId  = myTag->iBroadcastId;
      tag.strTitle            = myTag->strTitle.c_str();
      tag.iUniqueChannelId    = channel.iUniqueId;
      tag.startTime           = myTag->startTime + iShift;
      tag.endTime             = myTag->endTime + iShift;
      tag.strPlotOutline      = myTag->strPlotOutline.c_str();
      tag.strPlot             = myTag->strPlot.c_str();
      tag.strOriginalTitle    = NULL;  /* not supported */
      tag.strCast             = myTag->strCast.c_str();
      tag.strDirector         = myTag->strDirector.c_str();
      tag.strWriter           = myTag->strWriter.c_str();
      tag.iYear               = myTag->iYear;
      tag.strIMDBNumber       = NULL;  /* not supported */
      tag.strIconPath         = myTag->strIconPath.c_str();
      if (FindEpgGenre(myTag->strGenreString, iGenreType, iGenreSubType))
      {
        tag.iGenreType          = iGenreType;
        tag.iGenreSubType       = iGenreSubType;
        tag.strGenreDescription = NULL;
      }
      else
      {
        tag.iGenreType          = EPG_GENRE_USE_STRING;
        tag.iGenreSubType       = 0;     /* not supported */
        tag.strGenreDescription = myTag->strGenreString.c_str();
      }
      tag.iParentalRating     = 0;     /* not supported */
      tag.iStarRating         = myTag->iStarRating;
      tag.bNotify             = false; /* not supported */
      tag.iSeriesNumber       = myTag->iSeasonNumber;
      tag.iEpisodeNumber      = myTag->iEpisodeNumber;
      tag.iEpisodePartNumber  = myTag->iEpisodePartNumber;
      tag.strEpisodeName      = myTag->strEpisodeName.c_str();
      tag.iFlags              = EPG_TAG_FLAG_UNDEFINED;
      tag.firstAired          = myTag->firstAired;

      PVR->TransferEpgEntry(handle, &tag);

      if ((myTag->startTime + iShift) > iEnd)
        break;
    }

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

int PVRIptvData::GetFileContents(std::string& url, std::string &strContent)
{
  strContent.clear();
  void* fileHandle = XBMC->OpenFile(url.c_str(), 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
      strContent.append(buffer, bytesRead);
    XBMC->CloseFile(fileHandle);
  }

  return strContent.length();
}

PVRIptvChannel * PVRIptvData::FindChannel(const std::string &strId, const std::string &strName)
{
  for (auto& channel : m_channels)
  {
    if (StringUtils::EqualsNoCase(channel.strTvgId, strId))
      return &channel;
  }

  if (strName.empty())
    return nullptr;

  const std::string strTvgName = std::regex_replace(strName, std::regex(" "), "_");
  for (auto& channel : m_channels)
  {
    if (StringUtils::EqualsNoCase(channel.strTvgName, strTvgName) ||
        StringUtils::EqualsNoCase(channel.strTvgName, strName))
      return &channel;
  }

  for (auto& channel : m_channels)
  {
    if (StringUtils::EqualsNoCase(channel.strChannelName, strName))
      return &channel;
  }

  return nullptr;
}

PVRIptvChannelGroup * PVRIptvData::FindGroup(const std::string &strName)
{
  std::vector<PVRIptvChannelGroup>::iterator it;
  for(it = m_groups.begin(); it < m_groups.end(); ++it)
  {
    if (it->strGroupName == strName)
      return &*it;
  }

  return NULL;
}

PVRIptvEpgChannel * PVRIptvData::FindEpg(const std::string &strId)
{
  std::vector<PVRIptvEpgChannel>::iterator it;
  for(it = m_epg.begin(); it < m_epg.end(); ++it)
  {
    if (StringUtils::CompareNoCase(it->strId, strId) == 0)
      return &*it;
  }

  return NULL;
}

PVRIptvEpgChannel* PVRIptvData::FindEpgForChannel(const std::string& id)
{
  for (auto& epgChannel : m_epg)
  {
    if (StringUtils::EqualsNoCase(epgChannel.strId, id))
      return &epgChannel;
  }

  return nullptr;
}

PVRIptvEpgChannel * PVRIptvData::FindEpgForChannel(PVRIptvChannel &channel)
{
  for (auto& epgChannel : m_epg)
  {
    if (StringUtils::EqualsNoCase(epgChannel.strId, channel.strTvgId))
      return &epgChannel;
  }

  std::string strName;
  for (auto& epgChannel : m_epg)
  {
    for (const std::string& strOrigName : epgChannel.strNames)
    {
      strName = std::regex_replace(strOrigName, std::regex(" "), "_");

      if (StringUtils::EqualsNoCase(strName, channel.strTvgName) ||
          StringUtils::EqualsNoCase(strOrigName, channel.strTvgName))
        return &epgChannel;
    }
  }

  for (auto& epgChannel : m_epg)
  {
    for (const std::string& strOrigName : epgChannel.strNames)
    {
      if (StringUtils::EqualsNoCase(strOrigName, channel.strChannelName))
        return &epgChannel;
    }
  }

  return NULL;
}

bool PVRIptvData::FindEpgGenre(const std::string& strGenre, int& iType, int& iSubType)
{
  if (m_genres.empty())
    return false;

  std::vector<PVRIptvEpgGenre>::iterator it;
  for (it = m_genres.begin(); it != m_genres.end(); ++it)
  {
    if (StringUtils::CompareNoCase(it->strGenre, strGenre) == 0)
    {
      iType = it->iGenreType;
      iSubType = it->iGenreSubType;
      return true;
    }
  }

  return false;
}

/*
 * This method uses zlib to decompress a gzipped file in memory.
 * Author: Andrew Lim Chong Liang
 * http://windrealm.org
 */
bool PVRIptvData::GzipInflate( const std::string& compressedBytes, std::string& uncompressedBytes ) {

#define HANDLE_CALL_ZLIB(status) {   \
  if(status != Z_OK) {        \
    free(uncomp);             \
    return false;             \
  }                           \
}

  if ( compressedBytes.size() == 0 )
  {
    uncompressedBytes = compressedBytes ;
    return true ;
  }

  uncompressedBytes.clear() ;

  unsigned full_length = compressedBytes.size() ;
  unsigned half_length = compressedBytes.size() / 2;

  unsigned uncompLength = full_length ;
  char* uncomp = (char*) calloc( sizeof(char), uncompLength );

  z_stream strm;
  strm.next_in = (Bytef *) compressedBytes.c_str();
  strm.avail_in = compressedBytes.size() ;
  strm.total_out = 0;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;

  bool done = false ;

  HANDLE_CALL_ZLIB(inflateInit2(&strm, (16+MAX_WBITS)));

  while (!done)
  {
    // If our output buffer is too small
    if (strm.total_out >= uncompLength )
    {
      // Increase size of output buffer
      uncomp = (char *) realloc(uncomp, uncompLength + half_length);
      if (uncomp == NULL)
        return false;
      uncompLength += half_length ;
    }

    strm.next_out = (Bytef *) (uncomp + strm.total_out);
    strm.avail_out = uncompLength - strm.total_out;

    // Inflate another chunk.
    int err = inflate (&strm, Z_SYNC_FLUSH);
    if (err == Z_STREAM_END)
      done = true;
    else if (err != Z_OK)
    {
      break;
    }
  }

  HANDLE_CALL_ZLIB(inflateEnd (&strm));

  for ( size_t i=0; i<strm.total_out; ++i )
  {
    uncompressedBytes += uncomp[ i ];
  }

  free( uncomp );
  return true ;
}

int PVRIptvData::GetCachedFileContents(const std::string &strCachedName, const std::string &filePath,
                                       std::string &strContents, const bool bUseCache /* false */)
{
  bool bNeedReload = false;
  std::string strCachedPath = GetUserFilePath(strCachedName);
  std::string strFilePath = filePath;

  // check cached file is exists
  if (bUseCache && XBMC->FileExists(strCachedPath.c_str(), false))
  {
    struct __stat64 statCached;
    struct __stat64 statOrig;

    XBMC->StatFile(strCachedPath.c_str(), &statCached);
    XBMC->StatFile(strFilePath.c_str(), &statOrig);

    bNeedReload = statCached.st_mtime < statOrig.st_mtime || statOrig.st_mtime == 0;
  }
  else
    bNeedReload = true;

  if (bNeedReload)
  {
    GetFileContents(strFilePath, strContents);

    // write to cache
    if (bUseCache && strContents.length() > 0)
    {
      void* fileHandle = XBMC->OpenFileForWrite(strCachedPath.c_str(), true);
      if (fileHandle)
      {
        XBMC->WriteFile(fileHandle, strContents.c_str(), strContents.length());
        XBMC->CloseFile(fileHandle);
      }
    }
    return strContents.length();
  }

  return GetFileContents(strCachedPath, strContents);
}

void PVRIptvData::ApplyChannelsLogos()
{
  std::vector<PVRIptvChannel>::iterator channel;
  for(channel = m_channels.begin(); channel < m_channels.end(); ++channel)
  {
    if (!channel->strTvgLogo.empty())
    {
      if (!m_strLogoPath.empty()
        // special proto
        && channel->strTvgLogo.find("://") == std::string::npos)
        channel->strLogoPath = PathCombine(m_strLogoPath, channel->strTvgLogo);
      else
        channel->strLogoPath = channel->strTvgLogo;
    }
  }
}

void PVRIptvData::ApplyChannelsLogosFromEPG()
{
  bool bUpdated = false;

  std::vector<PVRIptvChannel>::iterator channel;
  for (channel = m_channels.begin(); channel < m_channels.end(); ++channel)
  {
    PVRIptvEpgChannel *epg;
    if ((epg = FindEpgForChannel(*channel)) == NULL || epg->strIcon.empty())
      continue;

    // 1 - prefer logo from playlist
    if (!channel->strLogoPath.empty() && g_iEPGLogos == 1)
      continue;

    // 2 - prefer logo from epg
    if (!epg->strIcon.empty() && g_iEPGLogos == 2)
    {
      channel->strLogoPath = epg->strIcon;
      bUpdated = true;
    }
  }

  if (bUpdated)
    PVR->TriggerChannelUpdate();
}

void PVRIptvData::ReaplyChannelsLogos(const char * strNewPath)
{
  P8PLATFORM::CLockObject lock(m_mutex);
  if (strlen(strNewPath) > 0)
  {
    m_strLogoPath = strNewPath;
    ApplyChannelsLogos();

    PVR->TriggerChannelUpdate();
    PVR->TriggerChannelGroupsUpdate();
  }
}

void PVRIptvData::ReloadEPG(const char * strNewPath)
{
  P8PLATFORM::CLockObject lock(m_mutex);
  if (strNewPath != m_strXMLTVUrl)
  {
    m_strXMLTVUrl = strNewPath;
    // TODO clear epg for all channels

    if (LoadEPG(m_iLastStart, m_iLastEnd))
    {
      for(unsigned int iChannelPtr = 0, max = m_channels.size(); iChannelPtr < max; iChannelPtr++)
      {
        PVRIptvChannel &myChannel = m_channels.at(iChannelPtr);
        PVR->TriggerEpgUpdate(myChannel.iUniqueId);
      }
    }
  }
}

void PVRIptvData::ReloadPlayList(const char * strNewPath)
{
  P8PLATFORM::CLockObject lock(m_mutex);
  if (strNewPath != m_strM3uUrl)
  {
    m_strM3uUrl = strNewPath;
    m_channels.clear();

    if (LoadPlayList())
    {
      PVR->TriggerChannelUpdate();
      PVR->TriggerChannelGroupsUpdate();
    }
  }
}

std::string PVRIptvData::ReadMarkerValue(std::string &strLine, const char* strMarkerName)
{
  int iMarkerStart = (int) strLine.find(strMarkerName);
  if (iMarkerStart >= 0)
  {
    std::string strMarker = strMarkerName;
    iMarkerStart += strMarker.length();
    if (iMarkerStart < (int)strLine.length())
    {
      char cFind = ' ';
      if (strLine[iMarkerStart] == '"')
      {
        cFind = '"';
        iMarkerStart++;
      }
      int iMarkerEnd = (int)strLine.find(cFind, iMarkerStart);
      if (iMarkerEnd < 0)
      {
        iMarkerEnd = strLine.length();
      }
      return strLine.substr(iMarkerStart, iMarkerEnd - iMarkerStart);
    }
  }

  return std::string("");
}

int PVRIptvData::GetChannelId(const char * strChannelName, const char * strStreamUrl)
{
  std::string concat(strChannelName);
  concat.append(strStreamUrl);

  const char* strString = concat.c_str();
  int iId = 0;
  int c;
  while ((c = *strString++))
    iId = ((iId << 5) + iId) + c; /* iId * 33 + c */

  return abs(iId);
}

std::string PVRIptvData::AddHeaderToStreamUrl (const std::string& url, const std::string& header, const std::string& value) const
{
  char separator = '|';
  size_t found = url.find('|');

  if (found != std::string::npos)
  {
    if (url.find(header + '=', found + 1) != std::string::npos)
    {
      return url;
    }
    else
    {
      separator = '&';
    }
  }

  return url + separator + header + '=' + value;
}
