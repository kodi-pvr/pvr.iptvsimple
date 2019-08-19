#include "PlaylistLoader.h"

#include "Settings.h"
#include "../client.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"

#include "p8-platform/util/StringUtils.h"

#include <map>
#include <regex>
#include <sstream>
#include <vector>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

PlaylistLoader::PlaylistLoader(Channels& channels, ChannelGroups& channelGroups)
      : m_channels(channels), m_channelGroups(channelGroups)
{
  m_m3uLocation = Settings::GetInstance().GetM3ULocation();
}

bool PlaylistLoader::LoadPlayList(void)
{
  if (m_m3uLocation.empty())
  {
    Logger::Log(LEVEL_NOTICE, "Playlist file path is not configured. Channels not loaded.");
    return false;
  }

  std::string playlistContent;
  if (!FileUtils::GetCachedFileContents(M3U_FILE_NAME, m_m3uLocation, playlistContent, Settings::GetInstance().UseM3UCache()))
  {
    Logger::Log(LEVEL_ERROR, "Unable to load playlist file '%s':  file is missing or empty.", m_m3uLocation.c_str());
    return false;
  }

  std::stringstream stream(playlistContent);

  /* load channels */
  bool isFirstLine = true;
  bool isRealTime = true;
  int epgTimeShift = 0;
  std::vector<int> currentChannelGroupIdList;

  Channel tmpChannel;

  std::string line;
  while (std::getline(stream, line))
  {
    line = StringUtils::TrimRight(line, " \t\r\n");
    line = StringUtils::TrimLeft(line, " \t");

    Logger::Log(LEVEL_DEBUG, "Read line: '%s'", line.c_str());

    if (line.empty())
      continue;

    if (isFirstLine)
    {
      isFirstLine = false;

      if (StringUtils::Left(line, 3) == "\xEF\xBB\xBF")
        line.erase(0, 3);

      if (StringUtils::StartsWith(line, M3U_START_MARKER)) //#EXTM3U
      {
        double tvgShiftDecimal = atof(ReadMarkerValue(line, TVG_INFO_SHIFT_MARKER).c_str());
        epgTimeShift = static_cast<int>(tvgShiftDecimal * 3600.0);
        continue;
      }
      else
      {
        Logger::Log(LEVEL_ERROR, "URL '%s' missing %s descriptor on line 1, attempting to parse it anyway.",
                  m_m3uLocation.c_str(), M3U_START_MARKER.c_str());
      }
    }

    if (StringUtils::StartsWith(line, M3U_INFO_MARKER)) //#EXTINF
    {
      tmpChannel.SetChannelNumber(m_channels.GetCurrentChannelNumber());
      currentChannelGroupIdList.clear();

      const std::string groupNamesListString = ParseIntoChannel(line, tmpChannel, currentChannelGroupIdList, epgTimeShift);

      if (!groupNamesListString.empty())
      {
        ParseAndAddChannelGroups(groupNamesListString, currentChannelGroupIdList, tmpChannel.IsRadio());
      }
    }
    else if (StringUtils::StartsWith(line, KODIPROP_MARKER)) //#KODIPROP:
    {
      ParseSinglePropertyIntoChannel(line, tmpChannel, KODIPROP_MARKER);
    }
    else if (StringUtils::StartsWith(line, EXTVLCOPT_MARKER)) //#EXTVLCOPT:
    {
      ParseSinglePropertyIntoChannel(line, tmpChannel, EXTVLCOPT_MARKER);
    }
    else if (StringUtils::StartsWith(line, PLAYLIST_TYPE_MARKER)) //#EXT-X-PLAYLIST-TYPE:
    {
      if (ReadMarkerValue(line, PLAYLIST_TYPE_MARKER) == "VOD")
        isRealTime = false;
    }
    else if (line[0] != '#')
    {
      Logger::Log(LEVEL_DEBUG, "Found URL: '%s' (current channel name: '%s')", line.c_str(), tmpChannel.GetChannelName().c_str());

      if (isRealTime)
        tmpChannel.AddProperty(PVR_STREAM_PROPERTY_ISREALTIMESTREAM, "true");

      Channel channel(tmpChannel);
      channel.SetStreamURL(line);

      m_channels.AddChannel(channel, currentChannelGroupIdList, m_channelGroups);

      tmpChannel.Reset();
      isRealTime = true;
    }
  }

  stream.clear();

  if (m_channels.GetChannelsAmount() == 0)
  {
    Logger::Log(LEVEL_ERROR, "Unable to load channels from file '%s':  file is corrupted.", m_m3uLocation.c_str());
    return false;
  }

  m_channels.ApplyChannelLogos();

  Logger::Log(LEVEL_NOTICE, "Loaded %d channels.", m_channels.GetChannelsAmount());
  return true;
}

std::string PlaylistLoader::ParseIntoChannel(const std::string& line, Channel& channel, std::vector<int>& groupIdList, int epgTimeShift)
{
  // parse line
  size_t colonIndex = line.find(':');
  size_t commaIndex = line.rfind(',');
  if (colonIndex != std::string::npos && commaIndex != std::string::npos && commaIndex > colonIndex)
  {
    // parse name
    std::string channelName = line.substr(commaIndex + 1);
    channelName = StringUtils::Trim(channelName);
    channel.SetChannelName(XBMC->UnknownToUTF8(channelName.c_str()));

    // parse info line containng the attributes for a channel
    const std::string infoLine = line.substr(colonIndex + 1, commaIndex - colonIndex - 1);

    std::string strTvgId      = ReadMarkerValue(infoLine, TVG_INFO_ID_MARKER);
    std::string strTvgName    = ReadMarkerValue(infoLine, TVG_INFO_NAME_MARKER);
    std::string strTvgLogo    = ReadMarkerValue(infoLine, TVG_INFO_LOGO_MARKER);
    std::string strChnlNo     = ReadMarkerValue(infoLine, TVG_INFO_CHNO_MARKER);
    std::string strRadio      = ReadMarkerValue(infoLine, RADIO_MARKER);
    std::string strTvgShift   = ReadMarkerValue(infoLine, TVG_INFO_SHIFT_MARKER);

    if (strTvgId.empty())
    {
      char buff[255];
      sprintf(buff, "%d", atoi(infoLine.c_str()));
      strTvgId.append(buff);
    }

    if (strTvgLogo.empty())
      strTvgLogo = channelName;

    if (!strChnlNo.empty())
      channel.SetChannelNumber(atoi(strChnlNo.c_str()));

    double tvgShiftDecimal = atof(strTvgShift.c_str());

    bool isRadio = !StringUtils::CompareNoCase(strRadio, "true");
    channel.SetTvgId(strTvgId);
    channel.SetTvgName(XBMC->UnknownToUTF8(strTvgName.c_str()));
    channel.SetTvgLogo(XBMC->UnknownToUTF8(strTvgLogo.c_str()));
    channel.SetTvgShift(static_cast<int>(tvgShiftDecimal * 3600.0));
    channel.SetRadio(isRadio);

    if (strTvgShift.empty())
      channel.SetTvgShift(epgTimeShift);

    return ReadMarkerValue(infoLine, GROUP_NAME_MARKER);
  }

  return "";
}

void PlaylistLoader::ParseAndAddChannelGroups(const std::string& groupNamesListString, std::vector<int>& groupIdList, bool isRadio)
{
  //groupNamesListString may have a single or multiple group names seapareted by ';'

  std::stringstream streamGroups(groupNamesListString);
  std::string groupName;

  while (std::getline(streamGroups, groupName, ';'))
  {
    groupName = XBMC->UnknownToUTF8(groupName.c_str());

    ChannelGroup group;
    group.SetGroupName(groupName);
    group.SetRadio(isRadio);

    int uniqueGroupId = m_channelGroups.AddChannelGroup(group);
    groupIdList.push_back(uniqueGroupId);
  }
}

void PlaylistLoader::ParseSinglePropertyIntoChannel(const std::string& line, Channel& channel, const std::string& markerName)
{
  const std::string value = ReadMarkerValue(line, markerName);
  auto pos = value.find('=');
  if (pos != std::string::npos)
  {
    const std::string prop = value.substr(0, pos);
    const std::string propValue = value.substr(pos + 1);
    channel.AddProperty(prop, propValue);

    Logger::Log(LEVEL_DEBUG, "%s - Found %s property: '%s' value: '%s'", __FUNCTION__, markerName.c_str(), prop.c_str(), propValue.c_str());
  }
}

void PlaylistLoader::ReloadPlayList(const char* newPath)
{
  //P8PLATFORM::CLockObject lock(m_mutex);
  //TODO Lock should happe in calling class
  if (newPath != m_m3uLocation)
  {
    m_m3uLocation = newPath;
    m_channels.Clear();
    m_channelGroups.Clear();

    if (LoadPlayList())
    {
      PVR->TriggerChannelUpdate();
      PVR->TriggerChannelGroupsUpdate();
    }
  }
}

std::string PlaylistLoader::ReadMarkerValue(const std::string& line, const std::string& markerName)
{
  size_t markerStart = line.find(markerName);
  if (markerStart != std::string::npos)
  {
    const std::string marker = markerName;
    markerStart += marker.length();
    if (markerStart < line.length())
    {
      char find = ' ';
      if (line[markerStart] == '"')
      {
        find = '"';
        markerStart++;
      }
      size_t markerEnd = line.find(find, markerStart);
      if (markerEnd == std::string::npos)
      {
        markerEnd = line.length();
      }
      return line.substr(markerStart, markerEnd - markerStart);
    }
  }

  return std::string("");
}