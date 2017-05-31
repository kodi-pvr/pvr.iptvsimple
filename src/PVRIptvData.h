#pragma once
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

#include <vector>
#include <string>
#include "p8-platform/util/StdString.h"
#include "client.h"
#include "p8-platform/threads/threads.h"

struct PVRIptvEpgEntry
{
  int         iBroadcastId;
  int         iChannelId;
  int         iGenreType;
  int         iGenreSubType;
  time_t      startTime;
  time_t      endTime;
  std::string strTitle;
  std::string strPlotOutline;
  std::string strPlot;
  std::string strIconPath;
  std::string strGenreString;
};

struct PVRIptvEpgChannel
{
  std::string                  strId;
  std::string                  strName;
  std::string                  strIcon;
  std::vector<PVRIptvEpgEntry> epg;
};

struct PVRIptvChannel
{
  bool        bRadio;
  int         iUniqueId;
  int         iChannelNumber;
  int         iEncryptionSystem;
  int         iTvgShift;
  std::string strChannelName;
  std::string strLogoPath;
  std::string strStreamURL;
  std::string strTvgId;
  std::string strTvgName;
  std::string strTvgLogo;
};

struct PVRIptvChannelGroup
{
  bool              bRadio;
  int               iGroupId;
  std::string       strGroupName;
  std::vector<int>  members;
};

struct PVRIptvEpgGenre
{
  int               iGenreType;
  int               iGenreSubType;
  std::string       strGenre;
};

class PVRIptvData : public P8PLATFORM::CThread
{
public:
  PVRIptvData(void);
  virtual ~PVRIptvData(void);

  virtual int       GetChannelsAmount(void);
  virtual PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  virtual bool      GetChannel(const PVR_CHANNEL &channel, PVRIptvChannel &myChannel);
  virtual int       GetChannelGroupsAmount(void);
  virtual bool      GetChannelByName(const std::string strChannelName, PVRIptvChannel &myChannel);
  virtual PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  virtual PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);
  virtual PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
  virtual PVR_ERROR GetEPGTagForChannel(EPG_TAG &tag, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
  virtual void      ReaplyChannelsLogos(const char * strNewPath);
  virtual void      ReloadPlayList(const char * strNewPath);
  virtual void      ReloadEPG(const char * strNewPath);

protected:
  virtual bool                 LoadPlayList(void);
  virtual bool                 LoadEPG(time_t iStart, time_t iEnd);
  virtual bool                 LoadGenres(void);
  virtual int                  GetFileContents(std::string& url, std::string &strContent);
  virtual PVRIptvChannel*      FindChannel(const std::string &strId, const std::string &strName);
  virtual PVRIptvChannelGroup* FindGroup(const std::string &strName);
  virtual PVRIptvEpgChannel*   FindEpg(const std::string &strId);
  virtual PVRIptvEpgChannel*   FindEpgForChannel(PVRIptvChannel &channel);
  virtual bool                 FindEpgGenre(const std::string& strGenre, int& iType, int& iSubType);
  virtual int                  ParseDateTime(std::string& strDate, bool iDateFormat = true);
  virtual bool                 GzipInflate( const std::string &compressedBytes, std::string &uncompressedBytes);
  virtual int                  GetCachedFileContents(const std::string &strCachedName, const std::string &strFilePath, 
                                                     std::string &strContent, const bool bUseCache = false);
  virtual void                 ApplyChannelsLogos();
  virtual void                 ApplyChannelsLogosFromEPG();
  virtual std::string          ReadMarkerValue(std::string &strLine, const char * strMarkerName);
  virtual int                  GetChannelId(const char * strChannelName, const char * strStreamUrl);

protected:
  virtual void *Process(void);

private:
  bool                              m_bTSOverride;
  int                               m_iEPGTimeShift;
  int                               m_iLastStart;
  int                               m_iLastEnd;
  std::string                       m_strXMLTVUrl;
  std::string                       m_strM3uUrl;
  std::string                       m_strLogoPath;
  std::vector<PVRIptvChannelGroup>  m_groups;
  std::vector<PVRIptvChannel>       m_channels;
  std::vector<PVRIptvEpgChannel>    m_epg;
  std::vector<PVRIptvEpgGenre>      m_genres;
};
