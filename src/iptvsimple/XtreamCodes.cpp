/*
 *  Copyright (C) 2026 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "XtreamCodes.h"

#include "utilities/FileUtils.h"
#include "utilities/Logger.h"
#include "utilities/WebUtils.h"

#include <algorithm>
#include <map>

#include <kodi/tools/StringUtils.h>
#include <nlohmann/json.hpp>

using namespace kodi::tools;
using namespace iptvsimple;
using namespace iptvsimple::utilities;
using json = nlohmann::json;

namespace
{

std::string GetBaseUrl(const std::shared_ptr<InstanceSettings>& settings)
{
  std::string baseUrl = settings->GetXtreamServer();
  StringUtils::Trim(baseUrl);
  StringUtils::TrimRight(baseUrl, "/");
  if (baseUrl.find("://") == std::string::npos)
    baseUrl = HTTP_PREFIX + baseUrl;
  return baseUrl;
}

std::string GetCredentialsQuery(const std::shared_ptr<InstanceSettings>& settings)
{
  return "username=" + WebUtils::UrlEncode(settings->GetXtreamUsername()) +
         "&password=" + WebUtils::UrlEncode(settings->GetXtreamPassword());
}

const json& GetField(const json& object, const std::string& key)
{
  static const json null;
  if (object.is_object())
  {
    auto it = object.find(key);
    if (it != object.end())
      return *it;
  }
  return null;
}

// Servers are inconsistent about sending numbers as JSON numbers or strings
std::string GetString(const json& object, const std::string& key)
{
  const json& value = GetField(object, key);
  if (value.is_string())
    return value.get<std::string>();
  if (value.is_number_integer())
    return std::to_string(value.get<long long>());
  return {};
}

// A double quote would end the M3U attribute or hide the channel name from the parser
std::string GetM3UString(const json& object, const std::string& key)
{
  std::string value = GetString(object, key);
  std::replace(value.begin(), value.end(), '"', '\'');
  return value;
}

bool GetJson(const std::string& url, json& result)
{
  std::string content;
  if (FileUtils::GetFileContents(url, content) == 0)
    return false;

  result = json::parse(content, nullptr, false);
  return !result.is_discarded();
}

} // unnamed namespace

std::string XtreamCodes::GetApiUrl(const std::shared_ptr<InstanceSettings>& settings)
{
  return GetBaseUrl(settings) + "/player_api.php?" + GetCredentialsQuery(settings);
}

bool XtreamCodes::GetPlaylist(const std::shared_ptr<InstanceSettings>& settings, std::string& playlist)
{
  const std::string baseUrl = GetBaseUrl(settings);
  const std::string apiUrl = GetApiUrl(settings);

  json account;
  if (!GetJson(apiUrl, account))
  {
    Logger::Log(LEVEL_ERROR, "%s - Unable to contact Xtream Codes server '%s'", __FUNCTION__, baseUrl.c_str());
    return false;
  }

  const json& userInfo = GetField(account, "user_info");
  if (GetString(userInfo, "auth") != "1")
  {
    Logger::Log(LEVEL_ERROR, "%s - Xtream Codes login rejected by '%s'", __FUNCTION__, baseUrl.c_str());
    return false;
  }

  const std::string status = GetString(userInfo, "status");
  if (status != "Active")
  {
    Logger::Log(LEVEL_ERROR, "%s - Xtream Codes account status is '%s'", __FUNCTION__, status.c_str());
    return false;
  }

  std::map<std::string, std::string> categoryNames;
  json categories;
  if (GetJson(apiUrl + "&action=get_live_categories", categories) && categories.is_array())
  {
    for (const auto& category : categories)
      categoryNames[GetString(category, "category_id")] = GetM3UString(category, "category_name");
  }

  json streams;
  if (!GetJson(apiUrl + "&action=get_live_streams", streams) || !streams.is_array())
  {
    Logger::Log(LEVEL_ERROR, "%s - Unable to load live streams from '%s'", __FUNCTION__, baseUrl.c_str());
    return false;
  }

  // Only these two URL forms are recognised when generating Xtream Codes catchup sources
  const bool useHls = settings->GetXtreamStreamFormat() == XtreamStreamFormat::HLS;
  const std::string streamUrlPrefix = baseUrl + (useHls ? "/live/" : "/") +
                                      settings->GetXtreamUsername() + "/" +
                                      settings->GetXtreamPassword() + "/";
  const std::string streamUrlSuffix = useHls ? ".m3u8" : "";

  playlist = "#EXTM3U x-tvg-url=\"" + baseUrl + "/xmltv.php?" + GetCredentialsQuery(settings) + "\"\n";

  for (const auto& stream : streams)
  {
    const std::string streamId = GetString(stream, "stream_id");
    if (streamId.empty())
      continue;

    playlist += "#EXTINF:-1 tvg-id=\"" + GetM3UString(stream, "epg_channel_id") +
                "\" tvg-logo=\"" + GetM3UString(stream, "stream_icon") +
                "\" group-title=\"" + categoryNames[GetString(stream, "category_id")] + "\"";
    if (GetString(stream, "stream_type") == "radio_streams")
      playlist += " radio=\"true\"";
    if (GetString(stream, "tv_archive") == "1")
      playlist += " catchup=\"xc\" catchup-days=\"" + GetString(stream, "tv_archive_duration") + "\"";
    playlist += "," + GetM3UString(stream, "name") + "\n";
    playlist += streamUrlPrefix + streamId + streamUrlSuffix + "\n";
  }

  return true;
}
