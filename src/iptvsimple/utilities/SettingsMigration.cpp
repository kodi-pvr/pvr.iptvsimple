/*
 *  Copyright (C) 2005-2022 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "SettingsMigration.h"

#include "kodi/General.h"

#include <algorithm>
#include <utility>
#include <vector>

using namespace iptvsimple;
using namespace iptvsimple::utilities;

namespace
{
// <setting name, default value> maps
const std::vector<std::pair<const char*, const char*>> stringMap = {{"m3uPath", ""},
                                                                    {"m3uUrl", ""},
                                                                    {"defaultProviderName", ""},
                                                                    {"providerMappingFile", "special://userdata/addon_data/pvr.iptvsimple/providers/providerMappings.xml"},
                                                                    {"onetvgroup", ""},
                                                                    {"twotvgroup", ""},
                                                                    {"threetvgroup", ""},
                                                                    {"fourtvgroup", ""},
                                                                    {"fivetvgroup", ""},
                                                                    {"customtvgroupsfile", "special://userdata/addon_data/pvr.iptvsimple/channelGroups/customTVGroups-example.xml"},
                                                                    {"oneradiogroup", ""},
                                                                    {"tworadiogroup", ""},
                                                                    {"threeradiogroup", ""},
                                                                    {"fourradiogroup", ""},
                                                                    {"fiveradiogroup", ""},
                                                                    {"customradiogroupsfile", "special://userdata/addon_data/pvr.iptvsimple/channelGroups/customRadioGroups-example.xml"},
                                                                    {"epgPath", ""},
                                                                    {"epgUrl", ""},
                                                                    {"genresPath", "special://userdata/addon_data/pvr.iptvsimple/genres/genreTextMappings/genres.xml"},
                                                                    {"genresUrl", ""},
                                                                    {"logoPath", ""},
                                                                    {"logoBaseUrl", ""},
                                                                    {"catchupQueryFormat", ""},
                                                                    {"udpxyHost", ""},
                                                                    {"defaultUserAgent", ""},
                                                                    {"defaultInputstream", ""},
                                                                    {"defaultMimeType", ""}};

const std::vector<std::pair<const char*, int>> intMap = {{"m3uPathType", 1},
                                                         {"startNum", 1},
                                                         {"m3uRefreshMode", 0},
                                                         {"m3uRefreshIntervalMins", 60},
                                                         {"m3uRefreshHour", 4},
                                                         {"tvgroupmode", 0},
                                                         {"numtvgroups", 1},
                                                         {"radiogroupmode", 0},
                                                         {"numradiogroups", 1},
                                                         {"epgPathType", 1},
                                                         {"genresPathType", 0},
                                                         {"logoPathType", 1},
                                                         {"logoFromEpg", 1},
                                                         {"catchupDays", 5},
                                                         {"allChannelsCatchupMode", 0},
                                                         {"catchupOverrideMode", 0},
                                                         {"catchupWatchEpgBeginBufferMins", 5},
                                                         {"catchupWatchEpgEndBufferMins", 15},
                                                         {"udpxyPort", 4022}};

const std::vector<std::pair<const char*, float>> floatMap = {{"epgTimeShift", 0.0f},
                                                             {"catchupCorrection", 0.0f}};

const std::vector<std::pair<const char*, bool>> boolMap = {{"m3uCache", true},
                                                           {"numberByOrder", false},
                                                           {"enableProviderMappings", false},
                                                           {"tvChannelGroupsOnly", false},
                                                           {"radioChannelGroupsOnly", false},
                                                           {"epgCache", true},
                                                           {"epgTSOverride", false},
                                                           {"epgIgnoreCaseForChannelIds", true},
                                                           {"useEpgGenreText", false},
                                                           {"useLogosLocalPathOnly", false},
                                                           {"mediaEnabled", true},
                                                           {"mediaGroupByTitle", true},
                                                           {"mediaGroupBySeason", true},
                                                           {"mediaTitleSeasonEpisode", false},
                                                           {"mediaVODAsRecordings", true},
                                                           {"timeshiftEnabled", false},
                                                           {"timeshiftEnabledAll", false},
                                                           {"timeshiftEnabledHttp", false},
                                                           {"timeshiftEnabledUdp", false},
                                                           {"timeshiftEnabledCustom", false},
                                                           {"catchupEnabled", false},
                                                           {"catchupPlayEpgAsLive", false},
                                                           {"catchupOnlyOnFinishedProgrammes", false},
                                                           {"transformMulticastStreamUrls", false},
                                                           {"useFFmpegReconnect", true},
                                                           {"useInputstreamAdaptiveforHls", false}};

} // unnamed namespace

bool SettingsMigration::MigrateSettings(kodi::addon::IAddonInstance& target)
{
  std::string stringValue;
  bool boolValue{false};
  int intValue{0};

  if (target.CheckInstanceSettingString("kodi_addon_instance_name", stringValue) &&
      !stringValue.empty())
  {
    // Instance already has valid instance settings
    return false;
  }

  // Read pre-multi-instance settings from settings.xml, transfer to instance settings
  SettingsMigration mig(target);

  for (const auto& setting : stringMap)
    mig.MigrateStringSetting(setting.first, setting.second);

  for (const auto& setting : intMap)
    mig.MigrateIntSetting(setting.first, setting.second);

  for (const auto& setting : floatMap)
    mig.MigrateFloatSetting(setting.first, setting.second);

  for (const auto& setting : boolMap)
    mig.MigrateBoolSetting(setting.first, setting.second);

  if (mig.Changed())
  {
    // Set a title for the new instance settings
    std::string title = "Migrated Add-on Config";
    target.SetInstanceSettingString("kodi_addon_instance_name", title);

    return true;
  }
  return false;
}

bool SettingsMigration::IsMigrationSetting(const std::string& key)
{
  return std::any_of(stringMap.cbegin(), stringMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; }) ||
         std::any_of(intMap.cbegin(), intMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; }) ||
         std::any_of(floatMap.cbegin(), floatMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; }) ||
         std::any_of(boolMap.cbegin(), boolMap.cend(),
                     [&key](const auto& entry) { return entry.first == key; });
}

void SettingsMigration::MigrateStringSetting(const char* key, const std::string& defaultValue)
{
  std::string value;
  if (kodi::addon::CheckSettingString(key, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingString(key, value);
    m_changed = true;
  }
}

void SettingsMigration::MigrateIntSetting(const char* key, int defaultValue)
{
  int value;
  if (kodi::addon::CheckSettingInt(key, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingInt(key, value);
    m_changed = true;
  }
}

void SettingsMigration::MigrateFloatSetting(const char* key, float defaultValue)
{
  float value;
  if (kodi::addon::CheckSettingFloat(key, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingFloat(key, value);
    m_changed = true;
  }
}

void SettingsMigration::MigrateBoolSetting(const char* key, bool defaultValue)
{
  bool value;
  if (kodi::addon::CheckSettingBoolean(key, value) && value != defaultValue)
  {
    m_target.SetInstanceSettingBoolean(key, value);
    m_changed = true;
  }
}
