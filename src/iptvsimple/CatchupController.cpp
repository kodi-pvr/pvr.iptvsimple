/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "CatchupController.h"

#include "Channels.h"
#include "Epg.h"
#include "Settings.h"
#include "data/Channel.h"
#include "utilities/Logger.h"
#include "utilities/TimeUtils.h"
#include "utilities/WebUtils.h"

#include <iomanip>
#include <regex>

#include <kodi/tools/StringUtils.h>

using namespace kodi::tools;
using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

CatchupController::CatchupController(Epg& epg, std::mutex* mutex)
  : m_epg(epg), m_mutex(mutex) {}

void CatchupController::ProcessChannelForPlayback(const Channel& channel, std::map<std::string, std::string>& catchupProperties)
{
  StreamType streamType = StreamTypeLookup(channel);

  // Anything from here is live!
  m_playbackIsVideo = false; // TODO: possible time jitter on UI as this will effect get stream times

  if (!m_fromEpgTag || m_controlsLiveStream)
  {
    Logger::Log(LEVEL_DEBUG, "%s - XXX1 - Not from EPG or controls live stream", __FUNCTION__);
    EpgEntry* liveEpgEntry = GetLiveEPGEntry(channel);
    Logger::Log(LEVEL_DEBUG, "%s - XXX2 - Found live EPG entry: %d", __FUNCTION__, liveEpgEntry != nullptr);
    if (m_controlsLiveStream && liveEpgEntry && !Settings::GetInstance().CatchupOnlyOnFinishedProgrammes())
    {
      Logger::Log(LEVEL_DEBUG, "%s - XXX3 - Starting controls live stream with live entry and not only finished programmes", __FUNCTION__);
      // Live timeshifting support with EPG entry
      UpdateProgrammeFrom(*liveEpgEntry, channel.GetTvgShift());
      m_catchupStartTime = liveEpgEntry->GetStartTime();
      m_catchupEndTime = liveEpgEntry->GetEndTime();
      Logger::Log(LEVEL_DEBUG, "%s - XXX4 - CatchupStartTimes: %lld, CatchupEndTime: %lld", __FUNCTION__, static_cast<long long>(m_catchupStartTime), static_cast<long long>(m_catchupEndTime));
    }
    else if (m_controlsLiveStream || !channel.IsCatchupSupported() ||
             (!m_controlsLiveStream && channel.IsCatchupSupported()))
    {
      Logger::Log(LEVEL_DEBUG, "%s - XXX3b - controls live stream or not catchup supported or (not controls live and catchup supported)", __FUNCTION__);
      ClearProgramme();
      m_programmeCatchupId.clear();
      m_catchupStartTime = 0;
      m_catchupEndTime = 0;
      Logger::Log(LEVEL_DEBUG, "%s - XXX4b - CatchupStartTimes: %lld, CatchupEndTime: %lld", __FUNCTION__, static_cast<long long>(m_catchupStartTime), static_cast<long long>(m_catchupEndTime));
    }
    m_fromEpgTag = false;
    Logger::Log(LEVEL_DEBUG, "%s - XXX5 - Ending Not from EPG or controls live stream", __FUNCTION__);
  }

  if (m_controlsLiveStream)
  {
    if (m_resetCatchupState)
    {
      Logger::Log(LEVEL_DEBUG, "%s - XXX6 - Starting reset catchup stream", __FUNCTION__);
      m_resetCatchupState = false;
      m_programmeCatchupId.clear();
      if (channel.IsCatchupSupported())
      {
        Logger::Log(LEVEL_DEBUG, "%s - XXX7 - Catchup supported", __FUNCTION__);
        m_timeshiftBufferOffset = Settings::GetInstance().GetCatchupDaysInSeconds(); //offset from now to start of catchup window
        m_timeshiftBufferStartTime = std::time(nullptr) - Settings::GetInstance().GetCatchupDaysInSeconds(); // now - the window size
      }
      else
      {
        Logger::Log(LEVEL_DEBUG, "%s - XXX7 - Catchup not supported", __FUNCTION__);
        m_timeshiftBufferOffset = 0;
        m_timeshiftBufferStartTime = 0;
      }
    }
    else
    {
      Logger::Log(LEVEL_DEBUG, "%s - XXX6b - Starting get current EPG entry", __FUNCTION__);
      EpgEntry* currentEpgEntry = GetEPGEntry(channel, m_timeshiftBufferStartTime + m_timeshiftBufferOffset);
      if (currentEpgEntry)
      {
        Logger::Log(LEVEL_DEBUG, "%s - XXX7b - Found current EPG entry: %d, timestamp: %lld", __FUNCTION__, currentEpgEntry != nullptr, m_timeshiftBufferStartTime + m_timeshiftBufferOffset);
        UpdateProgrammeFrom(*currentEpgEntry, channel.GetTvgShift());
      }
    }

    m_catchupStartTime = m_timeshiftBufferStartTime;

    // TODO: Need a method of updating an inputstream if already running such as web call to stream etc.
    // this will avoid inputstream restarts which are expensive, may be better placed in client.cpp
    // this also mean knowing when a stream has stopped
    SetCatchupInputStreamProperties(true, channel, catchupProperties, streamType);
  }
}

void CatchupController::ProcessEPGTagForTimeshiftedPlayback(const kodi::addon::PVREPGTag& epgTag, const Channel& channel, std::map<std::string, std::string>& catchupProperties)
{
  m_programmeCatchupId.clear();
  EpgEntry* epgEntry = GetEPGEntry(channel, epgTag.GetStartTime());
  if (epgEntry)
    m_programmeCatchupId = epgEntry->GetCatchupId();

  StreamType streamType = StreamTypeLookup(channel, true);

  if (m_controlsLiveStream)
  {
    UpdateProgrammeFrom(epgTag, channel.GetTvgShift());
    m_catchupStartTime = epgTag.GetStartTime();
    m_catchupEndTime = epgTag.GetEndTime();

    time_t timeNow = time(0);
    time_t programmeOffset = timeNow - m_catchupStartTime;
    time_t timeshiftBufferDuration = std::max(programmeOffset, Settings::GetInstance().GetCatchupDaysInSeconds());
    m_timeshiftBufferStartTime = timeNow - timeshiftBufferDuration;
    m_catchupStartTime = m_timeshiftBufferStartTime;
    m_catchupEndTime = timeNow;
    m_timeshiftBufferOffset = timeshiftBufferDuration - programmeOffset;

    m_resetCatchupState = false;

    // TODO: Need a method of updating an inputstream if already running such as web call to stream etc.
    // this will avoid inputstream restarts which are expensive, may be better placed in client.cpp
    // this also mean knowing when a stream has stopped
    SetCatchupInputStreamProperties(true, channel, catchupProperties, streamType);
  }
  else
  {
    UpdateProgrammeFrom(epgTag, channel.GetTvgShift());
    m_catchupStartTime = epgTag.GetStartTime();
    m_catchupEndTime = epgTag.GetEndTime();

    m_timeshiftBufferStartTime = 0;
    m_timeshiftBufferOffset = 0;

    m_fromEpgTag = true;
  }
}

void CatchupController::ProcessEPGTagForVideoPlayback(const kodi::addon::PVREPGTag& epgTag, const Channel& channel, std::map<std::string, std::string>& catchupProperties)
{
  m_programmeCatchupId.clear();
  EpgEntry* epgEntry = GetEPGEntry(channel, epgTag.GetStartTime());
  if (epgEntry)
    m_programmeCatchupId = epgEntry->GetCatchupId();

  StreamType streamType = StreamTypeLookup(channel, true);

  if (m_controlsLiveStream)
  {
    if (m_resetCatchupState)
    {
      UpdateProgrammeFrom(epgTag, channel.GetTvgShift());
      m_catchupStartTime = epgTag.GetStartTime();
      m_catchupEndTime = epgTag.GetEndTime();

      const time_t beginBuffer = Settings::GetInstance().GetCatchupWatchEpgBeginBufferSecs();
      const time_t endBuffer = Settings::GetInstance().GetCatchupWatchEpgEndBufferSecs();
      m_timeshiftBufferStartTime = m_catchupStartTime - beginBuffer;
      m_catchupStartTime = m_timeshiftBufferStartTime;
      m_catchupEndTime += endBuffer;
      m_timeshiftBufferOffset = beginBuffer;

      m_resetCatchupState = false;
    }

    // TODO: Need a method of updating an inputstream if already running such as web call to stream etc.
    // this will avoid inputstream restarts which are expensive, may be better placed in client.cpp
    // this also mean knowing when a stream has stopped
    SetCatchupInputStreamProperties(false, channel, catchupProperties, streamType);
  }
  else
  {
    UpdateProgrammeFrom(epgTag, channel.GetTvgShift());
    m_catchupStartTime = epgTag.GetStartTime();
    m_catchupEndTime = epgTag.GetEndTime();

    m_timeshiftBufferStartTime = 0;
    m_timeshiftBufferOffset = 0;
    m_catchupStartTime = m_catchupStartTime - Settings::GetInstance().GetCatchupWatchEpgBeginBufferSecs();
    m_catchupEndTime += Settings::GetInstance().GetCatchupWatchEpgEndBufferSecs();
  }

  if (m_catchupStartTime > 0)
    m_playbackIsVideo = true;
}

void CatchupController::SetCatchupInputStreamProperties(bool playbackAsLive, const Channel& channel, std::map<std::string, std::string>& catchupProperties, const StreamType& streamType)
{
  catchupProperties.insert({PVR_STREAM_PROPERTY_EPGPLAYBACKASLIVE, playbackAsLive ? "true" : "false"});

  catchupProperties.insert({"inputstream.ffmpegdirect.is_realtime_stream",
  	StringUtils::EqualsNoCase(channel.GetProperty(PVR_STREAM_PROPERTY_ISREALTIMESTREAM), "true") ? "true" : "false"});
  catchupProperties.insert({"inputstream.ffmpegdirect.stream_mode", "catchup"});

  catchupProperties.insert({"inputstream.ffmpegdirect.default_url", channel.GetStreamURL()});
  catchupProperties.insert({"inputstream.ffmpegdirect.playback_as_live", playbackAsLive ? "true" : "false"});
  catchupProperties.insert({"inputstream.ffmpegdirect.catchup_url_format_string", GetCatchupUrlFormatString(channel)});
  catchupProperties.insert({"inputstream.ffmpegdirect.catchup_buffer_start_time", std::to_string(m_catchupStartTime)});
  catchupProperties.insert({"inputstream.ffmpegdirect.catchup_buffer_end_time", std::to_string(m_catchupEndTime)});
  catchupProperties.insert({"inputstream.ffmpegdirect.catchup_buffer_offset", std::to_string(m_timeshiftBufferOffset)});
  catchupProperties.insert({"inputstream.ffmpegdirect.timezone_shift", std::to_string(m_epg.GetEPGTimezoneShiftSecs(channel) + channel.GetCatchupCorrectionSecs())});
  if (!m_programmeCatchupId.empty())
    catchupProperties.insert({"inputstream.ffmpegdirect.programme_catchup_id", m_programmeCatchupId});
  catchupProperties.insert({"inputstream.ffmpegdirect.catchup_terminates", channel.CatchupSourceTerminates() ? "true" : "false"});
  catchupProperties.insert({"inputstream.ffmpegdirect.catchup_granularity", std::to_string(channel.GetCatchupGranularitySeconds())});

  // TODO: Should also send programme start and duration potentially
  // When doing this don't forget to add Settings::GetInstance().GetCatchupWatchEpgBeginBufferSecs() + Settings::GetInstance().GetCatchupWatchEpgEndBufferSecs();
  // if in video playback mode from epg, i.e. if (!Settings::GetInstance().CatchupPlayEpgAsLive() && m_playbackIsVideo)s

  Logger::Log(LEVEL_DEBUG, "default_url - %s", WebUtils::RedactUrl(channel.GetStreamURL()).c_str());
  Logger::Log(LEVEL_DEBUG, "playback_as_live - %s", playbackAsLive ? "true" : "false");
  Logger::Log(LEVEL_DEBUG, "catchup_url_format_string - %s", WebUtils::RedactUrl(GetCatchupUrlFormatString(channel)).c_str());
  Logger::Log(LEVEL_DEBUG, "catchup_buffer_start_time - %s", std::to_string(m_catchupStartTime).c_str());
  Logger::Log(LEVEL_DEBUG, "catchup_buffer_end_time - %s", std::to_string(m_catchupEndTime).c_str());
  Logger::Log(LEVEL_DEBUG, "catchup_buffer_offset - %s", std::to_string(m_timeshiftBufferOffset).c_str());
  Logger::Log(LEVEL_DEBUG, "timezone_shift - %s", std::to_string(m_epg.GetEPGTimezoneShiftSecs(channel) + channel.GetCatchupCorrectionSecs()).c_str());
  Logger::Log(LEVEL_DEBUG, "programme_catchup_id - '%s'", m_programmeCatchupId.c_str());
  Logger::Log(LEVEL_DEBUG, "catchup_terminates - %s", channel.CatchupSourceTerminates() ? "true" : "false");
  Logger::Log(LEVEL_DEBUG, "catchup_granularity - %s", std::to_string(channel.GetCatchupGranularitySeconds()).c_str());
  Logger::Log(LEVEL_DEBUG, "mimetype - '%s'", channel.HasMimeType() ? channel.GetProperty("mimetype").c_str() : StreamUtils::GetMimeType(streamType).c_str());
}

StreamType CatchupController::StreamTypeLookup(const Channel& channel, bool fromEpg /* false */)
{
  StreamType streamType = m_streamManager.StreamTypeLookup(channel, GetStreamTestUrl(channel, fromEpg), GetStreamKey(channel, fromEpg));

  m_controlsLiveStream = StreamUtils::GetEffectiveInputStreamName(streamType, channel) == "inputstream.ffmpegdirect" && channel.CatchupSupportsTimeshifting();

  return streamType;
}

void CatchupController::UpdateProgrammeFrom(const kodi::addon::PVREPGTag& epgTag, int tvgShift)
{
  m_programmeStartTime = epgTag.GetStartTime();
  m_programmeEndTime = epgTag.GetEndTime();
  m_programmeTitle = epgTag.GetTitle();
  m_programmeUniqueChannelId = epgTag.GetUniqueChannelId();
  m_programmeChannelTvgShift = tvgShift;
}

void CatchupController::UpdateProgrammeFrom(const data::EpgEntry& epgEntry, int tvgShift)
{
  m_programmeStartTime = epgEntry.GetStartTime();
  m_programmeEndTime = epgEntry.GetEndTime();
  m_programmeTitle = epgEntry.GetTitle();
  m_programmeUniqueChannelId = epgEntry.GetChannelId();
  m_programmeChannelTvgShift = tvgShift;
}

void CatchupController::ClearProgramme()
{
  m_programmeStartTime = 0;
  m_programmeEndTime = 0;
  m_programmeTitle.clear();
  m_programmeUniqueChannelId = 0;
  m_programmeChannelTvgShift = 0;
}

namespace
{
void FormatUnits(const std::string& name, time_t tTime, std::string &urlFormatString)
{
  const std::regex timeSecondsRegex(".*(\\{" + name + ":(\\d+)\\}).*");
  std::cmatch mr;
  if (std::regex_match(urlFormatString.c_str(), mr, timeSecondsRegex) && mr.length() >= 3)
  {
    std::string timeSecondsExp = mr[1].first;
    std::string second = mr[1].second;
    if (second.length() > 0)
      timeSecondsExp = timeSecondsExp.erase(timeSecondsExp.find(second));
    std::string dividerStr = mr[2].first;
    second = mr[2].second;
    if (second.length() > 0)
      dividerStr = dividerStr.erase(dividerStr.find(second));

    const time_t divider = stoi(dividerStr);
    if (divider != 0)
    {
      time_t units = tTime / divider;
      if (units < 0)
        units = 0;
      urlFormatString.replace(urlFormatString.find(timeSecondsExp), timeSecondsExp.length(), std::to_string(units));
    }
  }
}

void FormatTime(const char ch, const struct tm *pTime, std::string &urlFormatString)
{
  std::string str = {'{', ch, '}'};
  size_t pos = urlFormatString.find(str);
  while (pos != std::string::npos)
  {
    std::ostringstream os;
    os << std::put_time(pTime, StringUtils::Format("%%%c", ch).c_str());
    std::string timeString = os.str();

    if (timeString.size() > 0)
      urlFormatString.replace(pos, str.size(), timeString);

    pos = urlFormatString.find(str);
  }
}

void FormatTime(const std::string name, const struct tm *pTime, std::string &urlFormatString, bool hasVarPrefix)
{
  std::string qualifier = hasVarPrefix ? "$" : "";
  qualifier += "{" + name + ":";
  size_t found = urlFormatString.find(qualifier);
  if (found != std::string::npos)
  {
    size_t foundStart = found + qualifier.size();
    size_t foundEnd = urlFormatString.find("}", foundStart + 1);
    if (foundEnd != std::string::npos)
    {
      std::string formatString = urlFormatString.substr(foundStart, foundEnd - foundStart);
      const std::regex timeSpecifiers("([YmdHMS])");
      formatString = std::regex_replace(formatString, timeSpecifiers, R"(%$&)");

      std::ostringstream os;
      os << std::put_time(pTime, formatString.c_str());
      std::string timeString = os.str();

      if (timeString.size() > 0)
        urlFormatString.replace(found, foundEnd - found + 1, timeString);
    }
  }
}

void FormatUtc(const std::string& str, time_t tTime, std::string &urlFormatString)
{
  auto pos = urlFormatString.find(str);
  if (pos != std::string::npos)
  {
    std::string utcTimeAsString = StringUtils::Format("%lu", tTime);
    urlFormatString.replace(pos, str.size(), utcTimeAsString);
  }
}

std::string FormatDateTime(time_t timeStart, time_t duration, const std::string &urlFormatString)
{
  std::string formattedUrl = urlFormatString;

  const time_t timeEnd = timeStart + duration;
  const time_t timeNow = std::time(0);

  std::tm dateTimeStart = SafeLocaltime(timeStart);
  std::tm dateTimeEnd = SafeLocaltime(timeEnd);
  std::tm dateTimeNow = SafeLocaltime(timeNow);

  FormatTime('Y', &dateTimeStart, formattedUrl);
  FormatTime('m', &dateTimeStart, formattedUrl);
  FormatTime('d', &dateTimeStart, formattedUrl);
  FormatTime('H', &dateTimeStart, formattedUrl);
  FormatTime('M', &dateTimeStart, formattedUrl);
  FormatTime('S', &dateTimeStart, formattedUrl);
  FormatUtc("{utc}", timeStart, formattedUrl);
  FormatUtc("${start}", timeStart, formattedUrl);
  FormatUtc("{utcend}", timeStart + duration, formattedUrl);
  FormatUtc("${end}", timeStart + duration, formattedUrl);
  FormatUtc("{lutc}", timeNow, formattedUrl);
  FormatUtc("${now}", timeNow, formattedUrl);
  FormatUtc("${timestamp}", timeNow, formattedUrl);
  FormatUtc("{duration}", duration, formattedUrl);
  FormatUnits("duration", duration, formattedUrl);
  FormatUtc("${offset}", timeNow - timeStart, formattedUrl);
  FormatUnits("offset", timeNow - timeStart, formattedUrl);

  FormatTime("utc", &dateTimeStart, formattedUrl, false);
  FormatTime("start", &dateTimeStart, formattedUrl, true);

  FormatTime("utcend", &dateTimeEnd, formattedUrl, false);
  FormatTime("end", &dateTimeEnd, formattedUrl, true);

  FormatTime("lutc", &dateTimeNow, formattedUrl, false);
  FormatTime("now", &dateTimeNow, formattedUrl, true);
  FormatTime("timestamp", &dateTimeNow, formattedUrl, true);

  Logger::Log(LEVEL_DEBUG, "%s - \"%s\"", __FUNCTION__, WebUtils::RedactUrl(formattedUrl).c_str());

  return formattedUrl;
}

std::string FormatDateTimeNowOnly(const std::string &urlFormatString)
{
  std::string formattedUrl = urlFormatString;
  const time_t timeNow = std::time(0);
  std::tm dateTimeNow = SafeLocaltime(timeNow);

  FormatUtc("{lutc}", timeNow, formattedUrl);
  FormatUtc("${now}", timeNow, formattedUrl);
  FormatUtc("${timestamp}", timeNow, formattedUrl);
  FormatTime("lutc", &dateTimeNow, formattedUrl, false);
  FormatTime("now", &dateTimeNow, formattedUrl, true);
  FormatTime("timestamp", &dateTimeNow, formattedUrl, true);

  Logger::Log(LEVEL_DEBUG, "%s - \"%s\"", __FUNCTION__, WebUtils::RedactUrl(formattedUrl).c_str());

  return formattedUrl;
}

std::string AppendQueryStringAndPreserveOptions(const std::string &url, const std::string &postfixQueryString)
{
  std::string urlFormatString;
  if (!postfixQueryString.empty())
  {
    // preserve any kodi protocol options after "|"
    size_t found = url.find_first_of('|');
    if (found != std::string::npos)
      urlFormatString = url.substr(0, found) + postfixQueryString + url.substr(found, url.length());
    else
      urlFormatString = url + postfixQueryString;
  }
  else
  {
    urlFormatString = url;
  }

  return urlFormatString;
}

std::string BuildEpgTagUrl(time_t startTime, time_t duration, const Channel& channel, long long timeOffset, const std::string& programmeCatchupId, int timezoneShiftSecs)
{
  std::string startTimeUrl;
  time_t timeNow = std::time(nullptr);
  time_t offset = startTime + timeOffset;

  if ((startTime > 0 && offset < (timeNow - 5)) || (channel.IgnoreCatchupDays() && !programmeCatchupId.empty()))
    startTimeUrl = FormatDateTime(offset - timezoneShiftSecs, duration, channel.GetCatchupSource());
  else
    startTimeUrl = FormatDateTimeNowOnly(channel.GetStreamURL());

  static const std::regex CATCHUP_ID_REGEX("\\{catchup-id\\}");
  if (!programmeCatchupId.empty())
    startTimeUrl = std::regex_replace(startTimeUrl, CATCHUP_ID_REGEX, programmeCatchupId);

  Logger::Log(LEVEL_DEBUG, "%s - %s", __FUNCTION__, WebUtils::RedactUrl(startTimeUrl).c_str());

  return startTimeUrl;
}

} // unnamed namespace

std::string CatchupController::GetCatchupUrlFormatString(const Channel& channel) const
{
  if (m_catchupStartTime > 0)
    return channel.GetCatchupSource();

  return "";
}

std::string CatchupController::GetCatchupUrl(const Channel& channel) const
{
  if (m_catchupStartTime > 0)
  {
    time_t duration = 60 * 60; // default one hour

    // use the programme duration if it's valid
    if (m_programmeStartTime > 0 && m_programmeStartTime < m_programmeEndTime)
    {
      duration = static_cast<time_t>(m_programmeEndTime - m_programmeStartTime);

      if (!Settings::GetInstance().CatchupPlayEpgAsLive() && m_playbackIsVideo)
        duration += Settings::GetInstance().GetCatchupWatchEpgBeginBufferSecs() + Settings::GetInstance().GetCatchupWatchEpgEndBufferSecs();

      time_t timeNow = time(0);
      // cap duration to timeNow
      if (m_programmeStartTime + duration > timeNow)
        duration = timeNow - m_programmeStartTime;
    }

    return BuildEpgTagUrl(m_catchupStartTime, duration, channel, m_timeshiftBufferOffset, m_programmeCatchupId, m_epg.GetEPGTimezoneShiftSecs(channel) + channel.GetCatchupCorrectionSecs());
  }

  return "";
}

std::string CatchupController::ProcessStreamUrl(const std::string& streamUrl) const
{
  //We only process  current time timestamps specifiers in this case
  return FormatDateTimeNowOnly(streamUrl);
}

std::string CatchupController::GetStreamTestUrl(const Channel& channel, bool fromEpg) const
{
 if (m_catchupStartTime > 0 || fromEpg)
    // Test URL from 2 hours ago for 1 hour duration.
    return BuildEpgTagUrl(std::time(nullptr) - (2 * 60 * 60), 60 * 60, channel, 0, m_programmeCatchupId, m_epg.GetEPGTimezoneShiftSecs(channel) + channel.GetCatchupCorrectionSecs());
  else
    return ProcessStreamUrl(channel.GetStreamURL());
}

std::string CatchupController::GetStreamKey(const Channel& channel, bool fromEpg) const
{
  // The streamKey is simply the channelId + StreamUrl or the catchup source
  // Either can be used to uniquely identify the StreamType/MimeType pairing
  if ((m_catchupStartTime > 0 || fromEpg) && m_timeshiftBufferOffset < (std::time(nullptr) - 5))
    std::to_string(channel.GetUniqueId()) + "-" + channel.GetCatchupSource();

  return std::to_string(channel.GetUniqueId()) + "-" + channel.GetStreamURL();
}

EpgEntry* CatchupController::GetLiveEPGEntry(const Channel& myChannel)
{
  std::lock_guard<std::mutex> lock(*m_mutex);

  return m_epg.GetLiveEPGEntry(myChannel);
}

EpgEntry* CatchupController::GetEPGEntry(const Channel& myChannel, time_t lookupTime)
{
  std::lock_guard<std::mutex> lock(*m_mutex);

  return m_epg.GetEPGEntry(myChannel, lookupTime);
}
