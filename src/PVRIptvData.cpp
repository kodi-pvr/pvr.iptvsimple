/*
 *      Copyright (C) 2013 Anton Fedchin
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

#include <sstream>
#include <string>
#include <fstream>
#include <map>
#include "zlib.h"
#include "rapidxml/rapidxml.hpp"
#include "PVRIptvData.h"

#define M3U_START_MARKER        "#EXTM3U"
#define M3U_INFO_MARKER         "#EXTINF"
#define TVG_INFO_ID_MARKER      "tvg-id="
#define TVG_INFO_NAME_MARKER    "tvg-name="
#define TVG_INFO_LOGO_MARKER    "tvg-logo="
#define TVG_INFO_SHIFT_MARKER   "tvg-shift="
#define GROUP_NAME_MARKER       "group-title="
#define RADIO_MARKER            "radio="
#define CHANNEL_LOGO_EXTENSION  ".png"
#define SECONDS_IN_DAY          86400

using namespace std;
using namespace ADDON;
using namespace rapidxml;

template<class Ch>
inline bool GetNodeValue(const xml_node<Ch> * pRootNode, const char* strTag, CStdString& strStringValue)
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
inline bool GetAttributeValue(const xml_node<Ch> * pNode, const char* strAttributeName, CStdString& strStringValue)
{
  xml_attribute<Ch> *pAttribute = pNode->first_attribute(strAttributeName);
  if (pAttribute == NULL)
  {
    return false;
  }
  strStringValue = pAttribute->value();
  return true;
}

PVRIptvData::PVRIptvData(void)
{
  m_strXMLTVUrl   = g_strTvgPath;
  m_strM3uUrl     = g_strM3UPath;
  m_strLogoPath   = g_strLogoPath;
  m_iEPGTimeShift = g_iEPGTimeShift;
  m_bTSOverride   = g_bTSOverride;
  m_iLastStart    = 0;
  m_iLastEnd      = 0;

  m_bEGPLoaded = false;

  if (LoadPlayList())
  {
    XBMC->QueueNotification(QUEUE_INFO, "%d channels loaded.", m_channels.size());
  }
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
}

bool PVRIptvData::LoadEPG(time_t iStart, time_t iEnd) 
{
  if (m_strXMLTVUrl.IsEmpty())
  {
    XBMC->Log(LOG_NOTICE, "EPG file path is not configured. EPG not loaded.");
    m_bEGPLoaded = true;
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
    m_bEGPLoaded = true;
    m_iLastStart = iStart;
    m_iLastEnd = iEnd;
    return false;
  }

  char * buffer;

  // gzip packed
  if (data[0] == '\x1F' && data[1] == '\x8B' && data[2] == '\x08') 
  {
    if (!GzipInflate(data, decompressed))
    {
      XBMC->Log(LOG_ERROR, "Invalid EPG file '%s': unable to decompress file.", m_strXMLTVUrl.c_str());
      m_bEGPLoaded = true;
      return false;
    }
    buffer = &(decompressed[0]);
  }
  else
  {
    buffer = &(data[0]);
  }

  // xml should starts with '<?xml'
  if (buffer[0] != '\x3C' || buffer[1] != '\x3F' || buffer[2] != '\x78' ||
      buffer[3] != '\x6D' || buffer[4] != '\x6C')
  {
    // check for BOM
    if (buffer[0] != '\xEF' || buffer[1] != '\xBB' || buffer[2] != '\xBF')
    {
      // check for tar archive
      if (strcmp(buffer + 0x101, "ustar") || strcmp(buffer + 0x101, "GNUtar"))
      {
        buffer += 0x200; // RECORDSIZE = 512
      }
      else
      {
        XBMC->Log(LOG_ERROR, "Invalid EPG file '%s': unable to parse file.", m_strXMLTVUrl.c_str());
        m_bEGPLoaded = true;
        return false;
      }
    }
  }

  xml_document<> xmlDoc;
  try 
  {
    xmlDoc.parse<0>(buffer);
  } 
  catch(parse_error p) 
  {
    XBMC->Log(LOG_ERROR, "Unable parse EPG XML: %s", p.what());
    m_bEGPLoaded = true;
    return false;
  }

  xml_node<> *pRootElement = xmlDoc.first_node("tv");
  if (!pRootElement)
  {
    XBMC->Log(LOG_ERROR, "Invalid EPG XML: no <tv> tag found");
    m_bEGPLoaded = true;
    return false;
  }

  // clear previously loaded epg
  if (m_epg.size() > 0) 
  {
    m_epg.clear();
  }

  int iBroadCastId = 0;
  xml_node<> *pChannelNode = NULL;
  for(pChannelNode = pRootElement->first_node("channel"); pChannelNode; pChannelNode = pChannelNode->next_sibling("channel"))
  {
    CStdString strName;
    CStdString strId;
    if(!GetAttributeValue(pChannelNode, "id", strId))
    {
      continue;
    }
    GetNodeValue(pChannelNode, "display-name", strName);

    if (FindChannel(strId, strName) == NULL)
    {
      continue;
    }

    PVRIptvEpgChannel epgChannel;
    epgChannel.strId = strId;
    epgChannel.strName = strName;

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

    vector<PVRIptvChannel>::iterator it;
    for (it = m_channels.begin(); it < m_channels.end(); it++)
    {
      if (it->iTvgShift + m_iEPGTimeShift < iMinShiftTime)
        iMinShiftTime = it->iTvgShift + m_iEPGTimeShift;
      if (it->iTvgShift + m_iEPGTimeShift > iMaxShiftTime)
        iMaxShiftTime = it->iTvgShift + m_iEPGTimeShift;
    }
  }

  CStdString strEmpty = "";
  PVRIptvEpgChannel *epg = NULL;
  for(pChannelNode = pRootElement->first_node("programme"); pChannelNode; pChannelNode = pChannelNode->next_sibling("programme"))
  {
    CStdString strId;
    if (!GetAttributeValue(pChannelNode, "channel", strId))
      continue;

    if (epg == NULL || epg->strId != strId) 
    {
      if ((epg = FindEpg(strId)) == NULL) 
        continue;
    }

    CStdString strStart;
    CStdString strStop;

    if (!GetAttributeValue(pChannelNode, "start", strStart) || !GetAttributeValue(pChannelNode, "stop", strStop)) 
    {
      continue;
    }

    int iTmpStart = ParseDateTime(strStart);
    int iTmpEnd = ParseDateTime(strStop);

    if ((iTmpEnd + iMaxShiftTime < iStart) || (iTmpStart + iMinShiftTime > iEnd))
    {
      continue;
    }

    CStdString strTitle;
    CStdString strCategory;
    CStdString strDesc;

    GetNodeValue(pChannelNode, "title", strTitle);
    GetNodeValue(pChannelNode, "category", strCategory);
    GetNodeValue(pChannelNode, "desc", strDesc);

    CStdString strIconPath;
    xml_node<> *pIconNode = pChannelNode->first_node("icon");
    if (pIconNode != NULL)
    {
      if (!GetAttributeValue(pIconNode, "src", strIconPath)) 
      {
        strIconPath = "";
      }
    }

    PVRIptvEpgEntry entry;
    entry.iBroadcastId    = ++iBroadCastId;
    entry.iGenreType      = 0;
    entry.iGenreSubType   = 0;
    entry.strTitle        = strTitle;
    entry.strPlot         = strDesc;
    entry.strPlotOutline  = "";
    entry.strIconPath     = strIconPath;
    entry.startTime       = iTmpStart;
    entry.endTime         = iTmpEnd;
    entry.strGenreString  = strCategory;

    epg->epg.push_back(entry);
  }

  xmlDoc.clear();
  m_bEGPLoaded = true;

  XBMC->Log(LOG_NOTICE, "EPG Loaded.");

  return true;
}

bool PVRIptvData::LoadPlayList(void) 
{
  if (m_strM3uUrl.IsEmpty())
  {
    XBMC->Log(LOG_NOTICE, "Playlist file path is not configured. Channels not loaded.");
    return false;
  }

  CStdString strPlaylistContent;
  if (!GetCachedFileContents(M3U_FILE_NAME, m_strM3uUrl, strPlaylistContent, g_bCacheM3U))
  {
    XBMC->Log(LOG_ERROR, "Unable to load playlist file '%s':  file is missing or empty.", m_strM3uUrl.c_str());
    return false;
  }

  std::stringstream stream(strPlaylistContent);

  /* load channels */
  bool bFirst = true;

  int iChannelIndex     = 0;
  int iUniqueGroupId    = 0;
  int iCurrentGroupId   = 0;
  int iChannelNum       = g_iStartNumber;
  int iEPGTimeShift     = 0;

  PVRIptvChannel tmpChannel;
  tmpChannel.strTvgId       = "";
  tmpChannel.strChannelName = "";
  tmpChannel.strTvgName     = "";
  tmpChannel.strTvgLogo     = "";
  tmpChannel.iTvgShift      = 0;

  char szLine[1024];
  while(stream.getline(szLine, 1024)) 
  {
  
    CStdString strLine = "";
    strLine.append(szLine);
    strLine.TrimRight(" \t\r\n");
    strLine.TrimLeft(" \t");

    if (strLine.IsEmpty())
    {
      continue;
    }

    if (bFirst) 
    {
      bFirst = false;
      if (strLine.Left(3) == "\xEF\xBB\xBF")
      {
        strLine.Delete(0, 3);
      }
      if (strLine.Left((int)strlen(M3U_START_MARKER)) == M3U_START_MARKER) 
      {
        double fTvgShift = atof(ReadMarkerValue(strLine, TVG_INFO_SHIFT_MARKER));
        iEPGTimeShift = (int) (fTvgShift * 3600.0);
        continue;
      }
      else
      {
        break;
      }
    }

    if (strLine.Left((int)strlen(M3U_INFO_MARKER)) == M3U_INFO_MARKER) 
    {
      bool        bRadio       = false;
      double      fTvgShift    = 0;
      CStdString  strChnlName  = "";
      CStdString  strTvgId     = "";
      CStdString  strTvgName   = "";
      CStdString  strTvgLogo   = "";
      CStdString  strGroupName = "";
      CStdString  strRadio     = "";

      // parse line
      int iColon = (int)strLine.Find(':');
      int iComma = (int)strLine.ReverseFind(',');
      if (iColon >= 0 && iComma >= 0 && iComma > iColon) 
      {
        // parse name
        iComma++;
        strChnlName = strLine.Right((int)strLine.size() - iComma).Trim();
        tmpChannel.strChannelName = XBMC->UnknownToUTF8(strChnlName);

        // parse info
        CStdString strInfoLine = strLine.Mid(++iColon, --iComma - iColon);

        strTvgId      = ReadMarkerValue(strInfoLine, TVG_INFO_ID_MARKER);
        strTvgName    = ReadMarkerValue(strInfoLine, TVG_INFO_NAME_MARKER);
        strTvgLogo    = ReadMarkerValue(strInfoLine, TVG_INFO_LOGO_MARKER);
        strGroupName  = ReadMarkerValue(strInfoLine, GROUP_NAME_MARKER);
        strRadio      = ReadMarkerValue(strInfoLine, RADIO_MARKER);
        fTvgShift     = atof(ReadMarkerValue(strInfoLine, TVG_INFO_SHIFT_MARKER));

        if (strTvgId.IsEmpty())
        {
          char buff[255];
          sprintf(buff, "%d", atoi(strInfoLine));
          strTvgId.append(buff);
        }
        if (strTvgLogo.IsEmpty())
        {
          strTvgLogo = strChnlName;
        }

        bRadio                = !strRadio.CompareNoCase("true");
        tmpChannel.strTvgId   = strTvgId;
        tmpChannel.strTvgName = XBMC->UnknownToUTF8(strTvgName);
        tmpChannel.strTvgLogo = XBMC->UnknownToUTF8(strTvgLogo);
        tmpChannel.iTvgShift  = (int)(fTvgShift * 3600.0);
        tmpChannel.bRadio     = bRadio;

        if (tmpChannel.iTvgShift == 0 && iEPGTimeShift != 0)
        {
          tmpChannel.iTvgShift = iEPGTimeShift;
        }

        if (!strGroupName.IsEmpty())
        {
          strGroupName = XBMC->UnknownToUTF8(strGroupName);

          PVRIptvChannelGroup * pGroup;
          if ((pGroup = FindGroup(strGroupName)) == NULL)
          {
            PVRIptvChannelGroup group;
            group.strGroupName = strGroupName;
            group.iGroupId = ++iUniqueGroupId;
            group.bRadio = bRadio;

            m_groups.push_back(group);
            iCurrentGroupId = iUniqueGroupId;
          }
          else
          {
            iCurrentGroupId = pGroup->iGroupId;
          }
        }
      }
    } 
    else if (strLine[0] != '#')
    {
      PVRIptvChannel channel;
      channel.iUniqueId         = GetChannelId(tmpChannel.strChannelName.c_str(), strLine);
      channel.iChannelNumber    = iChannelNum++;
      channel.strTvgId          = tmpChannel.strTvgId;
      channel.strChannelName    = tmpChannel.strChannelName;
      channel.strTvgName        = tmpChannel.strTvgName;
      channel.strTvgLogo        = tmpChannel.strTvgLogo;
      channel.iTvgShift         = tmpChannel.iTvgShift;
      channel.bRadio            = tmpChannel.bRadio;
      channel.strStreamURL      = strLine;
      channel.iEncryptionSystem = 0;

      if (iCurrentGroupId > 0) 
      {
        channel.bRadio = m_groups.at(iCurrentGroupId - 1).bRadio;
        m_groups.at(iCurrentGroupId - 1).members.push_back(iChannelIndex);
      }

      m_channels.push_back(channel);
      iChannelIndex++;

      tmpChannel.strTvgId       = "";
      tmpChannel.strChannelName = "";
      tmpChannel.strTvgName     = "";
      tmpChannel.strTvgLogo     = "";
      tmpChannel.iTvgShift      = 0;
      tmpChannel.bRadio         = false;
    }
  }
  
  stream.clear();

  if (m_channels.size() == 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to load channels from file '%s':  file is corrupted.", m_strM3uUrl.c_str());
    return false;
  }

  ApplyChannelsLogos();

  XBMC->Log(LOG_NOTICE, "Loaded %d channels.", m_channels.size());
  return true;
}

int PVRIptvData::GetChannelsAmount(void)
{
  return m_channels.size();
}

PVR_ERROR PVRIptvData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
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
      strncpy(xbmcChannel.strStreamURL, channel.strStreamURL.c_str(), sizeof(xbmcChannel.strStreamURL) - 1);
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

      return true;
    }
  }

  return false;
}

int PVRIptvData::GetChannelGroupsAmount(void)
{
  return m_groups.size();
}

PVR_ERROR PVRIptvData::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  for (unsigned int iGroupPtr = 0; iGroupPtr < m_groups.size(); iGroupPtr++)
  {
    PVRIptvChannelGroup &group = m_groups.at(iGroupPtr);
    if (group.bRadio == bRadio)
    {
      PVR_CHANNEL_GROUP xbmcGroup;
      memset(&xbmcGroup, 0, sizeof(PVR_CHANNEL_GROUP));

      xbmcGroup.bIsRadio = bRadio;
      strncpy(xbmcGroup.strGroupName, group.strGroupName.c_str(), sizeof(xbmcGroup.strGroupName) - 1);

      PVR->TransferChannelGroup(handle, &xbmcGroup);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRIptvData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  PVRIptvChannelGroup *myGroup;
  if ((myGroup = FindGroup(group.strGroupName)) != NULL)
  {
    for (unsigned int iPtr = 0; iPtr < myGroup->members.size(); iPtr++)
    {
      int iIndex = myGroup->members.at(iPtr);
      if (iIndex < 0 || iIndex >= (int) m_channels.size())
        continue;

      PVRIptvChannel &channel = m_channels.at(iIndex);
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
  vector<PVRIptvChannel>::iterator myChannel;
  for (myChannel = m_channels.begin(); myChannel < m_channels.end(); myChannel++)
  {
    if (myChannel->iUniqueId != (int) channel.iUniqueId)
    {
      continue;
    }

    if (!m_bEGPLoaded || iStart > m_iLastStart || iEnd > m_iLastEnd) 
    {
      if (LoadEPG(iStart, iEnd))
      {
        m_iLastStart = iStart;
        m_iLastEnd = iEnd;
      }
    }

    PVRIptvEpgChannel *epg;
    if ((epg = FindEpgForChannel(*myChannel)) == NULL || epg->epg.size() == 0)
    {
      return PVR_ERROR_NO_ERROR;
    }

    int iShift = m_bTSOverride ? m_iEPGTimeShift : myChannel->iTvgShift + m_iEPGTimeShift;

    vector<PVRIptvEpgEntry>::iterator myTag;
    for (myTag = epg->epg.begin(); myTag < epg->epg.end(); myTag++)
    {
      if ((myTag->endTime + iShift) < iStart) 
        continue;

      EPG_TAG tag;
      memset(&tag, 0, sizeof(EPG_TAG));

      tag.iUniqueBroadcastId  = myTag->iBroadcastId;
      tag.strTitle            = myTag->strTitle.c_str();
      tag.iChannelNumber      = myTag->iChannelId;
      tag.startTime           = myTag->startTime + iShift;
      tag.endTime             = myTag->endTime + iShift;
      tag.strPlotOutline      = myTag->strPlotOutline.c_str();
      tag.strPlot             = myTag->strPlot.c_str();
      tag.strIconPath         = myTag->strIconPath.c_str();
      tag.iGenreType          = EPG_GENRE_USE_STRING;        //myTag.iGenreType;
      tag.iGenreSubType       = 0;                           //myTag.iGenreSubType;
      tag.strGenreDescription = myTag->strGenreString.c_str();

      PVR->TransferEpgEntry(handle, &tag);

      if ((myTag->startTime + iShift) > iEnd)
        break;
    }

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

int PVRIptvData::GetFileContents(CStdString& url, std::string &strContent)
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

int PVRIptvData::ParseDateTime(CStdString strDate, bool iDateFormat)
{
  struct tm timeinfo;
  memset(&timeinfo, 0, sizeof(tm));

  if (iDateFormat)
  {
    sscanf(strDate, "%04d%02d%02d%02d%02d%02d", &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday, &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);
  }
  else
  {
    sscanf(strDate, "%02d.%02d.%04d%02d:%02d:%02d", &timeinfo.tm_mday, &timeinfo.tm_mon, &timeinfo.tm_year, &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);
  }

  timeinfo.tm_mon  -= 1;
  timeinfo.tm_year -= 1900;
  timeinfo.tm_isdst = -1;

  return mktime(&timeinfo);
}

PVRIptvChannel * PVRIptvData::FindChannel(const std::string &strId, const std::string &strName)
{
  CStdString strTvgName = strName;
  strTvgName.Replace(' ', '_');

  vector<PVRIptvChannel>::iterator it;
  for(it = m_channels.begin(); it < m_channels.end(); it++)
  {
    if (it->strTvgId == strId)
    {
      return &*it;
    }
    if (strTvgName == "") 
    {
      continue;
    }
    if (it->strTvgName == strTvgName)
    {
      return &*it;
    }
    if (it->strChannelName == strName)
    {
      return &*it;
    }
  }

  return NULL;
}

PVRIptvChannelGroup * PVRIptvData::FindGroup(const std::string &strName)
{
  vector<PVRIptvChannelGroup>::iterator it;
  for(it = m_groups.begin(); it < m_groups.end(); it++)
  {
    if (it->strGroupName == strName)
    {
      return &*it;
    }
  }

  return NULL;
}

PVRIptvEpgChannel * PVRIptvData::FindEpg(const std::string &strId)
{
  vector<PVRIptvEpgChannel>::iterator it;
  for(it = m_epg.begin(); it < m_epg.end(); it++)
  {
    if (it->strId == strId)
    {
      return &*it;
    }
  }

  return NULL;
}

PVRIptvEpgChannel * PVRIptvData::FindEpgForChannel(PVRIptvChannel &channel)
{
  vector<PVRIptvEpgChannel>::iterator it;
  for(it = m_epg.begin(); it < m_epg.end(); it++)
  {
    if (it->strId == channel.strTvgId)
    {
      return &*it;
    }
    CStdString strName = it->strName;
    strName.Replace(' ', '_');
    if (strName == channel.strTvgName
      || it->strName == channel.strTvgName)
    {
      return &*it;
    }
    if (it->strName == channel.strChannelName)
    {
      return &*it;
    }
  }

  return NULL;
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
  CStdString strCachedPath = GetUserFilePath(strCachedName);
  CStdString strFilePath = filePath;

  // check cached file is exists
  if (bUseCache && XBMC->FileExists(strCachedPath, false)) 
  {
    struct __stat64 statCached;
    struct __stat64 statOrig;

    XBMC->StatFile(strCachedPath, &statCached);
    XBMC->StatFile(strFilePath, &statOrig);

    bNeedReload = statCached.st_mtime < statOrig.st_mtime || statOrig.st_mtime == 0;
  } 
  else 
  {
    bNeedReload = true;
  }

  if (bNeedReload) 
  {
    GetFileContents(strFilePath, strContents);

    // write to cache
    if (bUseCache && strContents.length() > 0) 
    {
      void* fileHandle = XBMC->OpenFileForWrite(strCachedPath, true);
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
  if (m_strLogoPath.IsEmpty())
    return;

  vector<PVRIptvChannel>::iterator channel;
  for(channel = m_channels.begin(); channel < m_channels.end(); channel++)
  {
    channel->strLogoPath = PathCombine(m_strLogoPath, channel->strTvgLogo);
    //channel->strLogoPath.append(CHANNEL_LOGO_EXTENSION);
  }
}

void PVRIptvData::ReaplyChannelsLogos(const char * strNewPath)
{
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
  if (strNewPath != m_strXMLTVUrl)
  {
    m_strXMLTVUrl = strNewPath;
    m_bEGPLoaded = false;
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

CStdString PVRIptvData::ReadMarkerValue(std::string &strLine, const char* strMarkerName)
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
  while (c = *strString++)
    iId = ((iId << 5) + iId) + c; /* iId * 33 + c */

  return abs(iId);
}