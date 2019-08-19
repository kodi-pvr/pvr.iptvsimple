#include "Epg.h"

#include "Settings.h"
#include "../client.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"
#include "utilities/XMLUtils.h"

#include "p8-platform/util/StringUtils.h"
#include "rapidxml/rapidxml.hpp"

#include <chrono>
#include <regex>
#include <thread>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;
using namespace rapidxml;

Epg::Epg(Channels& channels)
      : m_channels(channels)
{
  m_xmltvLocation = Settings::GetInstance().GetEpgLocation();
  m_epgTimeShift = Settings::GetInstance().GetEpgTimeshiftSecs();
  m_tsOverride = Settings::GetInstance().GetTsOverride();
  m_lastStart = 0;
  m_lastEnd = 0;
}

void Epg::Clear()
{
  m_channelEpgs.clear();
  m_genres.clear();
}

bool Epg::LoadEPG(time_t start, time_t end)
{
  if (m_xmltvLocation.empty())
  {
    Logger::Log(LEVEL_NOTICE, "EPG file path is not configured. EPG not loaded.");
    return false;
  }

  std::string data;

  if (GetXMLTVFileWithRetries(data))
  {
    char* buffer = FillBufferFromXMLTVData(data);

    if (!buffer)
      return false;

    xml_document<> xmlDoc;
    try
    {
      xmlDoc.parse<0>(buffer);
    }
    catch (parse_error p)
    {
      Logger::Log(LEVEL_ERROR, "Unable parse EPG XML: %s", p.what());
      return false;
    }

    xml_node<>* rootElement = xmlDoc.first_node("tv");
    if (!rootElement)
    {
      Logger::Log(LEVEL_ERROR, "Invalid EPG XML: no <tv> tag found");
      return false;
    }

    if (!LoadChannelEpgs(rootElement))
      return false;

    LoadEpgEntries(rootElement, start, end);

    xmlDoc.clear();
  }
  else
  {
    return false;
  }

  LoadGenres();

  Logger::Log(LEVEL_NOTICE, "EPG Loaded.");

  if (Settings::GetInstance().GetEpgLogosMode() != EpgLogosMode::IGNORE_XMLTV)
    ApplyChannelsLogosFromEPG();

  return true;
}

bool Epg::GetXMLTVFileWithRetries(std::string& data)
{
  int bytesRead = 0;
  int count = 0;

  while (count < 3) // max 3 tries
  {
    if ((bytesRead = FileUtils::GetCachedFileContents(TVG_FILE_NAME, m_xmltvLocation, data, Settings::GetInstance().UseEPGCache())) != 0)
      break;

    Logger::Log(LEVEL_ERROR, "Unable to load EPG file '%s':  file is missing or empty. :%dth try.", m_xmltvLocation.c_str(), ++count);

    if (count < 3)
      std::this_thread::sleep_for(std::chrono::microseconds(2 * 1000 * 1000)); // sleep 2 sec before next try.
  }

  if (bytesRead == 0)
  {
    Logger::Log(LEVEL_ERROR, "Unable to load EPG file '%s':  file is missing or empty. After %d tries.", m_xmltvLocation.c_str(), count);
    return false;
  }

  return true;
}

char* Epg::FillBufferFromXMLTVData(std::string& data)
{
  std::string decompressed;
  char* buffer = nullptr;

  // gzip packed
  if (data[0] == '\x1F' && data[1] == '\x8B' && data[2] == '\x08')
  {
    if (!FileUtils::GzipInflate(data, decompressed))
    {
      Logger::Log(LEVEL_ERROR, "Invalid EPG file '%s': unable to decompress file.", m_xmltvLocation.c_str());
      return nullptr;
    }
    buffer = &(decompressed[0]);
  }
  else
    buffer = &(data[0]);

  XmltvFileFormat fileFormat = GetXMLTVFileFormat(buffer);

  if (fileFormat == XmltvFileFormat::INVALID)
  {
    Logger::Log(LEVEL_ERROR, "Invalid EPG file '%s': unable to parse file.", m_xmltvLocation.c_str());
    return nullptr;
  }

  if (fileFormat == XmltvFileFormat::TAR_ARCHIVE)
    buffer += 0x200; // RECORDSIZE = 512

  return buffer;
}

const XmltvFileFormat Epg::GetXMLTVFileFormat(const char* buffer)
{
  // xml should starts with '<?xml'
  if (buffer[0] != '\x3C' || buffer[1] != '\x3F' || buffer[2] != '\x78' ||
      buffer[3] != '\x6D' || buffer[4] != '\x6C')
  {
    // check for BOM
    if (buffer[0] != '\xEF' || buffer[1] != '\xBB' || buffer[2] != '\xBF')
    {
      // check for tar archive
      if (strcmp(buffer + 0x101, "ustar") || strcmp(buffer + 0x101, "GNUtar"))
        return XmltvFileFormat::TAR_ARCHIVE;
      else
        return XmltvFileFormat::INVALID;
    }
  }

  return XmltvFileFormat::NORMAL;
}

bool Epg::LoadChannelEpgs(xml_node<>* rootElement)
{
  m_channelEpgs.clear();

  xml_node<>* channelNode = nullptr;
  for (channelNode = rootElement->first_node("channel"); channelNode; channelNode = channelNode->next_sibling("channel"))
  {
    ChannelEpg channelEpg;

    if (channelEpg.UpdateFrom(channelNode, m_channels))
      m_channelEpgs.push_back(channelEpg);
  }

  if (m_channelEpgs.size() == 0)
  {
    Logger::Log(LEVEL_ERROR, "EPG channels not found.");
    return false;
  }

  return true;
}

void Epg::LoadEpgEntries(xml_node<>* rootElement, int start, int end)
{
  int minShiftTime = m_epgTimeShift;
  int maxShiftTime = m_epgTimeShift;
  if (!m_tsOverride)
  {
    minShiftTime = SECONDS_IN_DAY;
    maxShiftTime = -SECONDS_IN_DAY;

    for (const auto& channel : m_channels.GetChannelsList())
    {
      if (channel.GetTvgShift() + m_epgTimeShift < minShiftTime)
        minShiftTime = channel.GetTvgShift() + m_epgTimeShift;
      if (channel.GetTvgShift() + m_epgTimeShift > maxShiftTime)
        maxShiftTime = channel.GetTvgShift() + m_epgTimeShift;
    }
  }

  xml_node<>* channelNode = nullptr;
  ChannelEpg* channelEpg = nullptr;
  int broadcastId = 0;

  for (channelNode = rootElement->first_node("programme"); channelNode; channelNode = channelNode->next_sibling("programme"))
  {
    std::string id;
    if (!GetAttributeValue(channelNode, "channel", id))
      continue;

    if (!channelEpg || StringUtils::CompareNoCase(channelEpg->GetId(), id) != 0)
    {
      if (!(channelEpg = FindEpgForChannel(id)))
        continue;
    }

    EpgEntry entry;
    if (entry.UpdateFrom(channelNode, channelEpg, id, broadcastId + 1, start, end, minShiftTime, maxShiftTime))
    {
      broadcastId++;

      channelEpg->GetEpgEntries().push_back(entry);
    }
  }
}


void Epg::ReloadEPG(const char* newPath)
{
  //P8PLATFORM::CLockObject lock(m_mutex);
  //TODO Lock should happen in calling class
  if (newPath != m_xmltvLocation)
  {
    m_xmltvLocation = newPath;
    Clear();

    if (LoadEPG(m_lastStart, m_lastEnd))
    {
      for (const auto& myChannel : m_channels.GetChannelsList())
      {
        PVR->TriggerEpgUpdate(myChannel.GetUniqueId());
      }
    }
  }
}

PVR_ERROR Epg::GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t start, time_t end)
{
  for (const auto& myChannel : m_channels.GetChannelsList())
  {
    if (myChannel.GetUniqueId() != iChannelUid)
      continue;

    if (start > m_lastStart || end > m_lastEnd)
    {
      // reload EPG for new time interval only
      LoadEPG(start, end);
      {
        // doesn't matter is epg loaded or not we shouldn't try to load it for same interval
        m_lastStart = static_cast<int>(start);
        m_lastEnd = static_cast<int>(end);
      }
    }

    ChannelEpg* channelEpg = FindEpgForChannel(myChannel);
    if (!channelEpg || channelEpg->GetEpgEntries().size() == 0)
      return PVR_ERROR_NO_ERROR;

    int shift = m_tsOverride ? m_epgTimeShift : myChannel.GetTvgShift() + m_epgTimeShift;

    for (auto& epgEntry : channelEpg->GetEpgEntries())
    {
      if ((epgEntry.GetEndTime() + shift) < start)
        continue;

      EPG_TAG tag;
      memset(&tag, 0, sizeof(EPG_TAG));

      epgEntry.UpdateTo(tag, iChannelUid, shift, m_genres);

      PVR->TransferEpgEntry(handle, &tag);

      if ((epgEntry.GetStartTime() + shift) > end)
        break;
    }

    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

ChannelEpg* Epg::FindEpgForChannel(const std::string& id)
{
  for (auto& myChannelEpg : m_channelEpgs)
  {
    if (StringUtils::CompareNoCase(myChannelEpg.GetId(), id) == 0)
      return &myChannelEpg;
  }

  return nullptr;
}

ChannelEpg* Epg::FindEpgForChannel(const Channel& channel)
{
  for (auto& myChannelEpg : m_channelEpgs)
  {
    if (myChannelEpg.GetId() == channel.GetTvgId())
      return &myChannelEpg;

    const std::string name = std::regex_replace(myChannelEpg.GetName(), std::regex(" "), "_");
    if (name == channel.GetTvgName() || myChannelEpg.GetName() == channel.GetTvgName())
      return &myChannelEpg;

    if (myChannelEpg.GetName() == channel.GetChannelName())
      return &myChannelEpg;
  }

  return nullptr;
}

void Epg::ApplyChannelsLogosFromEPG()
{
  bool updated = false;

  for (const auto& channel : m_channels.GetChannelsList())
  {
    const ChannelEpg* channelEpg = FindEpgForChannel(channel);
    if (!channelEpg || channelEpg->GetIcon().empty())
      continue;

    // 1 - prefer logo from playlist
    if (!channel.GetLogoPath().empty() && Settings::GetInstance().GetEpgLogosMode() == EpgLogosMode::PREFER_M3U)
      continue;

    // 2 - prefer logo from epg
    if (!channelEpg->GetIcon().empty() && Settings::GetInstance().GetEpgLogosMode() == EpgLogosMode::PREFER_XMLTV)
    {
      m_channels.GetChannel(channel.GetUniqueId())->SetLogoPath(channelEpg->GetIcon());
      updated = true;
    }
  }

  if (updated)
    PVR->TriggerChannelUpdate();
}

bool Epg::LoadGenres()
{
  std::string data;

  // try to load genres from userdata folder
  std::string filePath = FileUtils::GetUserFilePath(GENRES_MAP_FILENAME);
  if (!XBMC->FileExists(filePath.c_str(), false))
  {
    // try to load file from addom folder
    filePath = FileUtils::GetClientFilePath(GENRES_MAP_FILENAME);
    if (!XBMC->FileExists(filePath.c_str(), false))
      return false;
  }

  FileUtils::GetFileContents(filePath, data);

  if (data.empty())
    return false;

  m_genres.clear();

  char* buffer = &(data[0]);
  xml_document<> xmlDoc;
  try
  {
    xmlDoc.parse<0>(buffer);
  }
  catch (parse_error p)
  {
    return false;
  }

  xml_node<>* pRootElement = xmlDoc.first_node("genres");
  if (!pRootElement)
    return false;

  for (xml_node<>* pGenreNode = pRootElement->first_node("genre"); pGenreNode; pGenreNode = pGenreNode->next_sibling("genre"))
  {
    EpgGenre genre;

    if (genre.UpdateFrom(pGenreNode))
      m_genres.push_back(genre);
  }

  xmlDoc.clear();
  return true;
}